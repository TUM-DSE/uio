from root import MEASURE_RESULTS
import confmeasure
import measure_helpers as util
from procs import run
import root

from typing import List, Any, Optional, Callable, Generator, Iterator, Tuple
import time
import pty
import os
import sys
import termios
import signal
import socket as s
import select
import re
from tqdm import tqdm
from tempfile import TemporaryDirectory
from pathlib import Path


# overwrite the number of samples to take to a minimum
QUICK = False


DURATION = "1m"
if QUICK:
    DURATION = "10s"


SIZE = 5
WARMUP = 0
if QUICK:
    WARMUP = 0
    SIZE = 2


REDIS_REPS = 100000
if QUICK:
    REDIS_REPS = 10000


# QUICK: 20s else: 5min
def sample(
    f: Callable[[], Any], size: int = SIZE, warmup: int = WARMUP
) -> List[Any]:
    ret = []
    for i in tqdm(range(0, warmup)):
        f()
    for i in tqdm(range(0, size)):
        r = f()
        if r is None:
            return []
        ret += [r]
    return ret


STATS_PATH = MEASURE_RESULTS.joinpath("app-stats.json")


def expect(ushell: s.socket, timeout: int, until: Optional[str] = None) -> bool:
    """
    @return true if terminated because of until
    """
    if QUICK:
        print("begin readall until", until)

    fd = ushell.fileno()

    buf = ""
    ret = False
    (r, _, _) = select.select([fd], [], [], timeout)
    if QUICK:
        print("[readall] ", end="")
    while fd in r:
        out = ushell.recv(1).decode()
        buf += out
        if until is not None and until in buf:
            ret = True
            break
        (r, _, _) = select.select([fd], [], [], timeout)
        if len(out) == 0:
            break
    if QUICK:
        if not ret:
            print(f"'{buf}' != '{until}'")
    return ret


def assertline(ushell: s.socket, value: str) -> None:
    assert expect(ushell, 2, f"{value}")  # not sure why and when how many \r's occur.


def sendall(ushell: s.socket, content: str) -> None:
    if QUICK:
        print("[writeall]", content.strip())
    c = ushell.send(str.encode(content))
    if c != len(content):
        raise Exception("TODO implement writeall")


def echo_newline(ushell: s.socket, prompt: str) -> float:
    """
    send newline and expect prompt back. Time to reply is returned.
    """
    if QUICK:
        print("measuring echo")
    sw = time.monotonic()
    sendall(ushell, "\n")

    assertline(ushell, prompt)
    sw = time.monotonic() - sw

    time.sleep(0.5)
    return sw


def nginx_bench(host_port, length: str = DURATION, connections: int = 30, threads: int = 14) -> float:

    cmd = [ "taskset", "-c", confmeasure.CORES_BENCHMARK, 
           "wrk", "-t", str(threads),
           f"-d{length}", "-c", str(connections),
           f"http://{host_port}/index.html" ]
    result = run(cmd)
    print(result.stdout)
    line = re.findall("^Requests/sec:.*$", result.stdout, flags=re.MULTILINE)[0]
    value = float(line.split(" ")[-1]) # requests / second
    return value

def redis_bench(host: str, port: int, reps: int = REDIS_REPS, concurrent_connections: int = 30, payload_size: int = 3, keepalive: int = 1, pipelining: int = 16) -> Tuple[float, float]:
    queries: str = "get,set" # changing this probably changes output parsing
    cmd = [ "taskset", "-c", confmeasure.CORES_BENCHMARK, 
           "redis-benchmark", "--csv", "-q",
           "-n", f"{reps}", 
           "-c", f"{concurrent_connections}",
           "-h", f"{host}",
           "-p", f"{port}",
           "-d", f"{payload_size}",
           "-k", f"{keepalive}",
           "-t", f"{queries}",
           "-P", f"{pipelining}"
        ]
    result = run(cmd)
    print(result.stdout)
    from io import StringIO
    csvdata = StringIO(result.stdout)
    import csv
    data = list(csv.reader(csvdata))
    set_ = float(data[1][1])
    get = float(data[2][1])
    # line = re.findall("^Requests/sec:.*$", result.stdout, flags=re.MULTILINE)[0]
    # value = float(line.split(" ")[-1]) # requests / second
    return (set_, get)


def redis_ushell(helpers: confmeasure.Helpers, stats: Any, with_human: bool = False) -> None:
    name = "redis"
    name_set = f"{name}-set"
    name_get = f"{name}-get"
    if name_set in stats.keys() and name_get in stats.keys():
        print(f"skip {name}")
        return

    def experiment() -> Tuple[float, float]:
        ushell = s.socket(s.AF_UNIX)

        # with util.testbench_console(helpers) as vm:
        with helpers.spawn_qemu(helpers.uk_redis()) as vm:
            vm.wait_for_ping("172.44.0.2")
            ushell.connect(bytes(vm.ushell_socket))

            # ensure readiness of system
            # time.sleep(1) # guest network stack is up, but also wait for application to start
            time.sleep(2) # for the count app we dont really have a way to check if it is online

            values = redis_bench("172.44.0.2", 6379)
            
            return values

        ushell.close()

    samples = sample(lambda: experiment())
    sets = []
    gets = []

    for (a, b) in samples:
        sets += [a]
        gets += [b]

    stats[name_set] = sets
    stats[name_get] = gets
    util.write_stats(STATS_PATH, stats)


import threading

def nginx_ushell(helpers: confmeasure.Helpers, stats: Any, with_human: bool = False) -> None:
    name = "nginx_ushell"
    if name in stats.keys():
        print(f"skip {name}")
        return

    def human_using_ushell(ushell: s.socket, alive) -> None:
        sendall(ushell, "\n")
        expect(ushell, 10, "> ")
        while alive.acquire(False):
            alive.release()
            time.sleep(1)
            sendall(ushell, "ls\n")
            expect(ushell, 10, "> ")

    def experiment() -> float:
        ushell = s.socket(s.AF_UNIX)

        # with util.testbench_console(helpers) as vm:
        with helpers.spawn_qemu(helpers.uk_nginx()) as vm:
            vm.wait_for_ping("172.44.0.2")
            ushell.connect(bytes(vm.ushell_socket))

            # ensure readiness of system
            # time.sleep(1) # guest network stack is up, but also wait for application to start
            time.sleep(2) # for the count app we dont really have a way to check if it is online

            if with_human:
                # start human ushell usage
                alive = threading.Semaphore()
                human = threading.Thread(target=lambda: human_using_ushell(ushell, alive), name="Human ushell user")
                human.start()

            value = nginx_bench("172.44.0.2")
            
            if with_human:
                # terminate human
                alive.acquire(True)
                human.join(timeout=5)
                if human.is_alive(): raise Exception("human is still alive")

            return value

        ushell.close()

    samples = sample(lambda: experiment())

    stats[name] = samples
    util.write_stats(STATS_PATH, stats)


from contextlib import contextmanager
import shutil
@contextmanager
def run_nginx_native() -> Iterator[Any]:
    with TemporaryDirectory() as tempdir_:
        tempdir = Path(tempdir_)
        (tempdir / "logs").mkdir(exist_ok=True)
        (tempdir / "html").mkdir(exist_ok=True)
        shutil.copy(
            str(root.PROJECT_ROOT / "apps/nginx/fs0/nginx/html/index.html"),
            str(tempdir / "html/index.html")
        )

        print(f"\nnginx workdir {tempdir}", flush=True)
        cmd = [
            "nginx", "-c", str(root.PROJECT_ROOT / "apps/nginx/nginx.conf"),
            "-e", str(tempdir / "nginx.error"), "-p", str(tempdir)
        ]
        import subprocess
        nginx = subprocess.Popen(cmd) #, preexec_fn=os.setsid)
        run(["taskset", "-p", confmeasure.CORES_BENCHMARK, str(nginx.pid)])

        yield nginx

        nginx.kill()
        nginx.wait(timeout=10)


def nginx_native(helpers: confmeasure.Helpers, stats: Any) -> None:
    name = "nginx_native"
    if name in stats.keys():
        print(f"skip {name}")
        return

    def experiment() -> float:
        with run_nginx_native():
            time.sleep(2) # await nginx readiness
            value = nginx_bench("localhost:1337")

            return value

    samples = sample(lambda: experiment())

    stats[name] = samples
    util.write_stats(STATS_PATH, stats)


def set_console_raw() -> None:
    fd = sys.stdin.fileno()
    new = termios.tcgetattr(fd)
    new[3] = new[3] & ~termios.ECHO
    termios.tcsetattr(fd, termios.TCSADRAIN, new)


def native(helpers: confmeasure.Helpers, stats: Any) -> None:
    name = "native"
    if name in stats.keys():
        print(f"skip {name}")
        return
    (pid, ptsfd) = pty.fork()

    if pid == 0:
        # normalize prompt by removing bash version number
        os.environ["PS1"] = "$ "
        set_console_raw()
        os.execlp("/bin/sh", "/bin/sh")

    assert expect(ptsfd, 2, "$")
    samples = sample(
        lambda: echo(
            ptsfd,
            "$",
        )
    )
    print("samples:", samples)
    os.kill(pid, signal.SIGKILL)
    os.waitpid(pid, 0)

    os.close(ptsfd)

    stats[name] = samples
    util.write_stats(STATS_PATH, stats)


def ssh(helpers: confmeasure.Helpers, stats: Any) -> None:
    name = "ssh"
    if name in stats.keys():
        print(f"skip {name}")
        return
    (ptmfd, ptsfd) = pty.openpty()
    (ptmfd_stub, ptsfd_stub) = pty.openpty()
    pts_stub = os.readlink(f"/proc/self/fd/{ptsfd_stub}")
    os.close(ptsfd_stub)
    with util.testbench_console(helpers, pts_stub, guest_cmd=["/bin/ls"]) as vm:
        # breakpoint()
        sh = vm.ssh_Popen(stdin=ptsfd, stdout=ptsfd, stderr=ptsfd)
        assert expect(ptmfd, 2, "~]$")
        samples = sample(lambda: echo(ptmfd, "~]$"))
        sh.kill()
        sh.wait()
        print("samples:", samples)

    os.close(ptmfd_stub)
    os.close(ptsfd)
    os.close(ptmfd)

    stats[name] = samples
    util.write_stats(STATS_PATH, stats)


def main() -> None:
    """
    not quick: 5 * fio_suite(5min) + 2 * sample(5min) = 35min
    """
    util.check_intel_turbo()
    helpers = confmeasure.Helpers()

    stats = util.read_stats(STATS_PATH)

    # print("measure performance for native")
    # native(helpers, stats)
    # print("measure performance for ssh")
    # ssh(helpers, stats)
    print("measure performance for redis ushell")
    redis_ushell(helpers, stats)
    print("measure performance for nginx ushell")
    nginx_ushell(helpers, stats, with_human=False)
    print("measure performance for nginx native")
    nginx_native(helpers, stats)

    util.export_fio("console", stats)  # TODO rename


if __name__ == "__main__":
    main()
