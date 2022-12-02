from root import MEASURE_RESULTS
import confmeasure
import measure_helpers as util

from typing import List, Any, Optional, Callable
import time
import pty
import os
import sys
import termios
import signal
import socket as s
import select


# overwrite the number of samples to take to a minimum
QUICK = False


SIZE = 32
WARMUP = 0
if QUICK:
    WARMUP = 0
    SIZE = 2


# QUICK: 20s else: 5min
def sample(
    f: Callable[[], Optional[float]], size: int = SIZE, warmup: int = WARMUP
) -> List[float]:
    ret = []
    for i in range(0, warmup):
        f()
    for i in range(0, size):
        r = f()
        if r is None:
            return []
        ret += [r]
    return ret


STATS_PATH = MEASURE_RESULTS.joinpath("console-stats.json")


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


def vmsh_console(helpers: confmeasure.Helpers, stats: Any) -> None:
    name = "vmsh-console"
    if name in stats.keys():
        print(f"skip {name}")
        return

    ushell = s.socket(s.AF_UNIX)

    with util.testbench_console(helpers) as vm:
        # vm.wait_for_ping("172.44.0.2")
        ushell.connect(bytes(vm.ushell_socket))

        # ensure readiness of system
        # time.sleep(1) # guest network stack is up, but also wait for application to start
        time.sleep(2) # for the count app we dont really have a way to check if it is online
        
        sendall(ushell, "\n")
        while not expect(ushell, 10, "> "): pass

        samples = sample(lambda: echo_newline(ushell, "> "))
        print("samples:", samples)

    ushell.close()

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
    print("measure performance for vmsh console")
    vmsh_console(helpers, stats)

    # util.export_fio("console", stats)  # TODO rename


if __name__ == "__main__":
    main()
