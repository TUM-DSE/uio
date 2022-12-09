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
def sample(f: Callable[[], Any], size: int = SIZE, warmup: int = WARMUP) -> List[Any]:
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


def nginx_bench(
    host_port, length: str = DURATION, connections: int = 30, threads: int = 14
) -> float:

    cmd = [
        "taskset",
        "-c",
        confmeasure.CORES_BENCHMARK,
        "wrk",
        "-t",
        str(threads),
        f"-d{length}",
        "-c",
        str(connections),
        f"http://{host_port}/index.html",
    ]
    result = run(cmd)
    print(result.stdout)
    line = re.findall("^Requests/sec:.*$", result.stdout, flags=re.MULTILINE)[0]
    value = float(line.split(" ")[-1])  # requests / second
    return value


def redis_bench(
    host: str,
    port: int,
    reps: int = REDIS_REPS,
    concurrent_connections: int = 30,
    payload_size: int = 3,
    keepalive: int = 1,
    pipelining: int = 16,
) -> Tuple[float, float]:
    queries: str = "get,set"  # changing this probably changes output parsing
    cmd = [
        "taskset",
        "-c",
        confmeasure.CORES_BENCHMARK,
        "redis-benchmark",
        "--csv",
        "-q",
        "-n",
        f"{reps}",
        "-c",
        f"{concurrent_connections}",
        "-h",
        f"{host}",
        "-p",
        f"{port}",
        "-d",
        f"{payload_size}",
        "-k",
        f"{keepalive}",
        "-t",
        f"{queries}",
        "-P",
        f"{pipelining}",
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


def redis_ushell(
    helpers: confmeasure.Helpers,
    stats: Any,
    shell: str = "ushell",
    bootfs: str = "9p",
    human: str = "nohuman",
) -> None:
    """
    per sample: 4s
    """
    name = f"redis_{shell}_{bootfs}_{human}"
    name_set = f"{name}-set"
    name_get = f"{name}-get"
    if name_set in stats.keys() and name_get in stats.keys():
        print(f"skip {name}")
        return

    def experiment() -> Tuple[float, float]:
        ushell = s.socket(s.AF_UNIX)

        # with util.testbench_console(helpers) as vm:
        with helpers.spawn_qemu(helpers.uk_redis(shell=shell, bootfs=bootfs)) as vm:
            vm.wait_for_ping("172.44.0.2")
            ushell.connect(bytes(vm.ushell_socket))

            # ensure readiness of system
            # time.sleep(1) # guest network stack is up, but also wait for application to start
            time.sleep(
                2
            )  # for the count app we dont really have a way to check if it is online

            values = redis_bench("172.44.0.2", 6379)

            ushell.close()
            return values

    samples = sample(lambda: experiment())
    sets = []
    gets = []

    for (a, b) in samples:
        sets += [a]
        gets += [b]

    stats[name_set] = sets
    stats[name_get] = gets
    util.write_stats(STATS_PATH, stats)


def sqlite_ushell(
    helpers: confmeasure.Helpers,
    stats: Any,
    shell: str = "ushell",
    bootfs: str = "9p",
    human: str = "nohuman",
) -> None:
    """
    per sample:
    with 9p: 150s
    with initrd: 10s
    """
    name = f"sqlite_{shell}_{bootfs}_{human}"
    if name in stats.keys():
        print(f"skip {name}")
        return

    def experiment() -> float:
        ushell = s.socket(s.AF_UNIX)

        # with util.testbench_console(helpers) as vm:
        with TemporaryDirectory() as tempdir_:
            log = Path(tempdir_) / "qemu.log"
            with helpers.spawn_qemu(
                helpers.uk_sqlite(shell=shell, bootfs=bootfs), log=log
            ) as vm:
                # vm.wait_for_ping("172.44.0.2")
                # ushell.connect(bytes(vm.ushell_socket))

                # ensure readiness of system
                # time.sleep(1) # guest network stack is up, but also wait for application to start
                # time.sleep(2) # for the count app we dont really have a way to check if it is online

                # values = redis_bench("172.44.0.2", 6379)

                # return values
                vm.wait_for_death()
            ushell.close()
            with open(log, "r") as f:
                sec = float(f.readlines()[-1])
                print(f"sql operations took {sec}")
                return sec

    samples = sample(lambda: experiment())

    stats[name] = samples
    util.write_stats(STATS_PATH, stats)


import threading


def nginx_ushell(
    helpers: confmeasure.Helpers,
    stats: Any,
    shell: str = "ushell",
    bootfs: str = "9p",
    human: str = "nohuman",
) -> None:
    """
    per sample: 65s
    """
    name = f"nginx_{shell}_{bootfs}_{human}"
    if name in stats.keys():
        print(f"skip {name}")
        return

    def human_using_ushell(ushell: s.socket, alive) -> None:
        sendall(ushell, "\n")
        assert expect(ushell, 2, "> ")
        time.sleep(1)
        while alive.acquire(False):
            sendall(ushell, "ls\n")
            assert expect(ushell, 2, "> ")
            alive.release()
            time.sleep(1)

    def experiment(human) -> float:
        ushell = s.socket(s.AF_UNIX)

        # with util.testbench_console(helpers) as vm:
        with helpers.spawn_qemu(helpers.uk_nginx(shell=shell, bootfs=bootfs)) as vm:
            vm.wait_for_ping("172.44.0.2")
            ushell.connect(bytes(vm.ushell_socket))

            # ensure readiness of system
            # time.sleep(1) # guest network stack is up, but also wait for application to start
            time.sleep(
                2
            )  # for the count app we dont really have a way to check if it is online

            if human == "lshuman":
                # start human ushell usage
                alive = threading.Semaphore()
                human_ = threading.Thread(
                    target=lambda: human_using_ushell(ushell, alive),
                    name="Human ushell user",
                )
                human_.start()

            value = nginx_bench("172.44.0.2")

            if human == "lshuman":
                # terminate human
                alive.acquire(True)
                human_.join(timeout=5)
                if human_.is_alive():
                    raise Exception("human is still alive")

            ushell.close()
            return value

    samples = sample(lambda: experiment(human))

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
            str(tempdir / "html/index.html"),
        )

        print(f"\nnginx workdir {tempdir}", flush=True)
        cmd = [
            "nginx",
            "-c",
            str(root.PROJECT_ROOT / "apps/nginx/nginx.conf"),
            "-e",
            str(tempdir / "nginx.error"),
            "-p",
            str(tempdir),
        ]
        import subprocess

        nginx = subprocess.Popen(cmd)  # , preexec_fn=os.setsid)
        run(["taskset", "-p", confmeasure.CORES_QEMU, str(nginx.pid)])

        yield nginx

        nginx.kill()
        nginx.wait(timeout=10)


def nginx_qemu_9p(helpers: confmeasure.Helpers, stats: Any) -> None:
    """
    per sample: 75s
    """
    name = "nginx_qemu_9p"
    if name in stats.keys():
        print(f"skip {name}")
        return

    def experiment() -> float:
        with helpers.nixos_nginx() as nixos:
            with helpers.spawn_qemu(nixos) as vm:
                vm.wait_for_ping("172.44.0.2")

                # ensure readiness of system
                # time.sleep(1) # guest network stack is up, but also wait for application to start
                time.sleep(
                    2
                )  # for the count app we dont really have a way to check if it is online

                return nginx_bench("172.44.0.2")

    samples = sample(lambda: experiment())

    stats[name] = samples
    util.write_stats(STATS_PATH, stats)


def nginx_native(helpers: confmeasure.Helpers, stats: Any) -> None:
    """
    per sample: 65s
    """
    name = "nginx_native"
    if name in stats.keys():
        print(f"skip {name}")
        return

    def experiment() -> float:
        with run_nginx_native():
            time.sleep(2)  # await nginx readiness
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
    not quick: ~70 min
    """
    util.check_intel_turbo()
    helpers = confmeasure.Helpers()

    stats = util.read_stats(STATS_PATH)

    def with_all_configs(f):
        for shell in ["ushell", "noshell"]:
            for bootfs in ["initrd", "9p"]:
                print(f"\nmeasure performance for {f.__name__} ({shell}, {bootfs})\n")
                f(helpers, stats, shell=shell, bootfs=bootfs)

    with_all_configs(sqlite_ushell)  # 2x5x 150s + 2x5x 4s
    with_all_configs(redis_ushell)  # 4x5x 4s
    with_all_configs(nginx_ushell)  # 4x5x 65s
    # 2x5x 65s
    print("\nmeasure performance for nginx ushell with initrd and human interaction\n")
    nginx_ushell(helpers, stats, shell="ushell", bootfs="initrd", human="lshuman")
    print("\nmeasure performance for nginx ushell with 9p and human interaction\n")
    nginx_ushell(helpers, stats, shell="ushell", bootfs="9p", human="lshuman")

    print("\nmeasure performance for nginx native\n")
    nginx_native(helpers, stats)  # 5x 65s

    print("\nmeasure performance for nginx qemu with 9p\n")
    nginx_qemu_9p(helpers, stats)  # 5x 75s

    util.export_fio("app", stats)


if __name__ == "__main__":
    main()
