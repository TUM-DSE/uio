from root import MEASURE_RESULTS
import confmeasure
import measure_helpers as util
from procs import run
import root

from typing import List, Any, Optional, Callable, Generator, Iterator
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
QUICK = True


DURATION = "1m"
if QUICK:
    DURATION = "10s"


SIZE = 5
WARMUP = 0
if QUICK:
    WARMUP = 0
    SIZE = 5


# QUICK: 20s else: 5min
def sample(
    f: Callable[[], Optional[float]], size: int = SIZE, warmup: int = WARMUP
) -> List[float]:
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


def nginx_bench(host_port, length: str = DURATION, connections: int = 30, threads: int = 14) -> Any:

    cmd = [ "taskset", "-c", confmeasure.CORES_BENCHMARK, 
           "wrk", "-t", str(threads),
           f"-d{length}", "-c", str(connections),
           f"http://{host_port}/index.html" ]
    result = run(cmd)
    print(result.stdout)
    line = re.findall("^Requests/sec:.*$", result.stdout, flags=re.MULTILINE)[0]
    value = float(line.split(" ")[-1]) # requests / second
    return value
    


def nginx_ushell(helpers: confmeasure.Helpers, stats: Any) -> None:
    name = "nginx_ushell"
    if name in stats.keys():
        print(f"skip {name}")
        return

    def experiment() -> float:
        ushell = s.socket(s.AF_UNIX)

        # with util.testbench_console(helpers) as vm:
        with helpers.spawn_qemu(helpers.uk_nginx()) as vm:
            vm.wait_for_ping("172.44.0.2")
            ushell.connect(bytes(vm.ushell_socket))

            # ensure readiness of system
            # time.sleep(1) # guest network stack is up, but also wait for application to start
            time.sleep(2) # for the count app we dont really have a way to check if it is online
            value = nginx_bench("172.44.0.2")

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

        # breakpoint()
        nginx.kill()
        # try:
            # os.killpg(os.getpgid(nginx.pid), signal.SIGTERM)
        # except ProcessLookupError:
            # pass

        nginx.wait(timeout=10)

        # import psutil
        # print("foobar", flush=True)
        # breakpoint()
        # parent = psutil.Process(nginx.pid)
        # for child in parent.children(recursive=True):  # or parent.children() for recursive=False
            # print(child.pid)
            # breakpoint()
            # child.terminate()
        # parent.terminate()

        while True:
            try:
                os.kill(nginx.pid, 0)
            except ProcessLookupError:
                break
            else:
                print(f"waiting for {cmd[0]} to stop")
                time.sleep(1)


def nginx_native(helpers: confmeasure.Helpers, stats: Any) -> None:
    name = "nginx_native"
    if name in stats.keys():
        print(f"skip {name}")
        return

    def experiment() -> float:
        with run_nginx_native():
            time.sleep(2)
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
    print("measure performance for nginx ushell")
    nginx_ushell(helpers, stats)
    print("measure performance for nginx native")
    nginx_native(helpers, stats)

    util.export_fio("console", stats)  # TODO rename


if __name__ == "__main__":
    main()
