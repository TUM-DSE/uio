#!/usr/bin/env python3

import json
import os
import re
import socket
import subprocess
import time
from contextlib import contextmanager
from dataclasses import dataclass
from pathlib import Path
from queue import Queue
from shlex import quote
from tempfile import TemporaryDirectory
from typing import Any, Dict, Iterator, List, Text, Optional, Union

from root import TEST_ROOT, PROJECT_ROOT
from procs import run, pprint_cmd, ChildFd
from pylib import unwrap, unsafe_cast


@dataclass
class VmImage:
    kernel: Path
    ## kernel headers & buildsystem
    kerneldir: Path
    squashfs: Path
    initial_ramdisk: Path
    kernel_params: List[str]


@dataclass
class UkVmSpec:
    flake_name: str
    kernel: Path
    app_cmdline: str
    netbridge: bool
    ushell_devices: bool
    ushelldir: Path
    symfile: Path
    initrd: Optional[Path]
    rootfs_9p: Optional[Path]
    fs1_9p: Optional[Path]


@dataclass
class NixosVmSpec:
    qcow: Path
    netbridge: bool
    mnt_9p: Optional[Path]


class QmpSession:
    def __init__(self, sock: socket.socket) -> None:
        self.sock = sock
        self.pending_events: Queue[Dict[str, Any]] = Queue()
        self.reader = sock.makefile("r")
        self.writer = sock.makefile("w")
        hello = self._result()
        assert "QMP" in hello, f"Unexpected result: {hello}"
        self.send("qmp_capabilities")

    def _readmsg(self) -> Dict[str, Any]:
        line = self.reader.readline()
        return json.loads(line)

    def _raise_unexpected_msg(self, msg: Dict[str, Any]) -> None:
        m = json.dumps(msg, sort_keys=True, indent=4)
        raise RuntimeError(f"Got unexpected qmp response: {m}")

    def _result(self) -> Dict[str, Any]:
        while True:
            # QMP is in the handshake
            res = self._readmsg()
            if "return" in res or "QMP" in res:
                return res
            elif "event" in res:
                self.pending_events.put(res)
                continue
            else:
                self._raise_unexpected_msg(res)

    def events(self) -> Iterator[Dict[str, Any]]:
        while not self.pending_events.empty():
            yield self.pending_events.get()

        res = self._readmsg()

        if "event" not in res:
            self._raise_unexpected_msg(res)
        yield res

    def send(self, cmd: str, args: Dict[str, str] = {}) -> Dict[str, str]:
        data: Dict[str, Any] = dict(execute=cmd)
        if args != {}:
            data["arguments"] = args

        json.dump(data, self.writer)
        self.writer.write("\n")
        self.writer.flush()
        return self._result()

    def query_vcpu_threads(self) -> List[int]:
        return [unsafe_cast(thread[unsafe_cast('thread-id')]) for thread in self.send("query-cpus-fast")['return']]
            


def is_port_open(ip: str, port: int, wait_response: bool = False) -> bool:
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        s.connect((ip, int(port)))
        if wait_response:
            s.recv(1)
        s.shutdown(2)
        return True
    except Exception:
        return False


@contextmanager
def connect_qmp(path: Path) -> Iterator[QmpSession]:
    sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    sock.connect(str(path))

    try:
        yield QmpSession(sock)
    finally:
        sock.close()


def parse_regs(qemu_output: str) -> Dict[str, int]:
    regs = {}
    for match in re.finditer(r"(\S+)\s*=\s*([0-9a-f ]+)", qemu_output):
        name = match.group(1)
        content = match.group(2).replace(" ", "")
        regs[name.lower()] = int(content, 16)
    return regs


def get_ssh_port(session: QmpSession) -> int:
    usernet_info = session.send(
        "human-monitor-command", args={"command-line": "info usernet"}
    )
    ssh_port = None
    for l in usernet_info["return"].splitlines():
        fields = l.split()
        if "TCP[HOST_FORWARD]" in fields and "22" in fields:
            ssh_port = int(l.split()[3])
    assert ssh_port is not None
    return ssh_port


def ssh_cmd(port: int) -> List[str]:
    key_path = PROJECT_ROOT / "misc/nix/ssh_key"
    key_path.chmod(0o400)
    return [
        "ssh",
        "-i",
        str(key_path),
        "-p",
        str(port),
        "-oBatchMode=yes",
        "-oIdentitiesOnly=yes", # avoid ssh agend
        "-oStrictHostKeyChecking=no", # avoid host key check
        "-oConnectTimeout=5", # fail early
        "-oUserKnownHostsFile=/dev/null", # dont try to add host as known
        "root@172.44.0.2",
    ]


class QemuVm:
    def __init__(
        self,
        qmp_session: QmpSession,
        tmux_session: str,
        pid: int,
        ushell_socket: Optional[Path],
    ) -> None:
        self.qmp_session = qmp_session
        self.tmux_session = tmux_session
        self.pid = pid
        self.ssh_port = 22  # get_ssh_port(qmp_session)
        self.ushell_socket = ushell_socket

    def events(self) -> Iterator[Dict[str, Any]]:
        return self.qmp_session.events()

    def set_vcpumap(self, cpumap: List[int]):
        threads = self.qmp_session.query_vcpu_threads()
        for i in range(0, min(len(cpumap), len(threads))):
            # pin threads[i] to cpu cpumap[i]
            run(["taskset", "-pc", str(cpumap[i]), str(threads[i])])

    def wait_for_ssh(self) -> None:
        """
        Block until ssh port is accessible
        """
        print(f"wait for ssh on {self.ssh_port}")
        while True:
            if (
                self.ssh_cmd(
                    ["echo", "ok"],
                    check=False,
                    stderr=subprocess.DEVNULL,
                    verbose=False,
                ).returncode
                == 0
            ):
                break
            time.sleep(0.1)

    def wait_for_ping(self, host: str) -> None:
        """
        Block until icmp is responding
        @host: example: 172.44.0.2
        """
        timeout = 0.5
        max_ = int(1 / timeout * 60)
        for i in range(0, max_):
            response = run(
                ["ping", "-c", "1", "-W", f"{timeout}", f"{host}"], check=False
            )
            if response.returncode == 0:
                return  # its up
        raise Exception(f"VM is still not responding after {max_}sec")

    def wait_for_death(self) -> None:
        print("waiting for qemu to stop...")
        while True:
            try:
                print(".")
                os.kill(self.pid, 0)
            except ProcessLookupError:
                print("...done")
                break
            else:
                time.sleep(1)

    def ssh_Popen(
        self,
        stdout: ChildFd = subprocess.PIPE,
        stderr: ChildFd = None,
        stdin: ChildFd = None,
    ) -> subprocess.Popen:
        """
        opens a background process with an interactive ssh session
        """
        cmd = ssh_cmd(self.ssh_port)
        pprint_cmd(cmd)
        return subprocess.Popen(cmd, stdin=stdin, stdout=stdout, stderr=stderr)

    def ssh_cmd(
        self,
        argv: List[str],
        extra_env: Dict[str, str] = {},
        check: bool = True,
        stdin: ChildFd = None,
        stdout: ChildFd = subprocess.PIPE,
        stderr: ChildFd = None,
        verbose: bool = True,
    ) -> "subprocess.CompletedProcess[Text]":
        """
        @return: CompletedProcess.stderr/stdout contains output of `cmd` which
        is run in the vm via ssh.
        """
        env_cmd = []
        if len(extra_env):
            env_cmd.append("env")
            env_cmd.append("-")
            for k, v in extra_env.items():
                env_cmd.append(f"{k}={v}")
        cmd = ssh_cmd(self.ssh_port) + ["--"] + env_cmd + [" ".join(map(quote, argv))]
        return run(
            cmd, stdin=stdin, stdout=stdout, stderr=stderr, check=check, verbose=verbose
        )

    def regs(self) -> Dict[str, int]:
        """
        Get cpu register:
        TODO: add support for multiple cpus
        """
        res = self.send(
            "human-monitor-command", args={"command-line": "info registers"}
        )
        return parse_regs(res["return"])

    def dump_physical_memory(self, addr: int, num_bytes: int) -> bytes:
        res = self.send(
            "human-monitor-command",
            args={"command-line": f"xp/{num_bytes}bx 0x{addr:x}"},
        )
        hexval = "".join(
            m.group(1) for m in re.finditer("0x([0-9a-f]{2})", res["return"])
        )
        return bytes.fromhex(hexval)

    def attach(self) -> None:
        """
        Attach to qemu session via tmux. This is useful for debugging
        """
        subprocess.run(["tmux", "-L", self.tmux_session, "attach"])

    def send(self, cmd: str, args: Dict[str, str] = {}) -> Dict[str, str]:
        """
        Send a Qmp command (https://wiki.qemu.org/Documentation/QMP)
        """
        return self.qmp_session.send(cmd, args)


def uk_qemu_command(
    spec: UkVmSpec,
    qmp_socket: Path,
    ushell_socket=Optional[Path],
    cpu_pinning: Optional[str] = None,
) -> List[str]:
    cmd = []

    # general args

    if cpu_pinning is not None:
        cmd += [
            "taskset",
            "-c",
            cpu_pinning,
        ]

    cmd += [
        "qemu-system-x86_64",
        "-enable-kvm",
        "-cpu",
        "host",
        "-m",
        "1024",
        "-kernel",
        f"{spec.kernel}",
        "-nographic",
        "-qmp",
        f"unix:{str(qmp_socket)},server,nowait",
    ]

    # kernel cmdline

    uk_cmdline = ""
    if spec.netbridge:
        uk_cmdline += "netdev.ipv4_addr=172.44.0.2 netdev.ipv4_gw_addr=172.44.0.1 netdev.ipv4_subnet_mask=255.255.255.0"
    cmdline = f"{uk_cmdline} -- {spec.app_cmdline}"
    cmd += ["-append", cmdline]

    # other devices

    if spec.netbridge:
        cmd += [
            "-netdev",
            "bridge,id=en0,br=virbr0",
            "-device",
            "virtio-net-pci,netdev=en0",
        ]

    if spec.ushell_devices:
        if ushell_socket is None:
            raise Exception(
                "ushell devices are activated but ushell socket is not defined"
            )
        cmd += [
            "-chardev",
            f"socket,path={ushell_socket},server=on,wait=off,id=char0",
            "-device",
            "virtio-serial",
            "-device",
            "virtconsole,chardev=char0,id=ushell,nr=0",
        ]

    if spec.initrd is not None:
        cmd += ["-initrd", f"{spec.initrd}"]

    if spec.rootfs_9p is not None:
        cmd += [
            "-fsdev",
            f"local,id=myid,path={spec.rootfs_9p},security_model=none",
            "-device",
            "virtio-9p-pci,fsdev=myid,mount_tag=fs0,disable-modern=on,disable-legacy=off",
        ]

    # if spec.fs1_9p is not None:
    if spec.ushelldir is not None:
        cmd += [
            "-fsdev",
            f"local,id=myid2,path={spec.ushelldir},security_model=none",
            "-device",
            "virtio-9p-pci,fsdev=myid2,mount_tag=fs1,disable-modern=on,disable-legacy=off",
        ]

    return cmd


def nixos_qemu_command(
    spec: NixosVmSpec,
    qmp_socket: Path,
    cpu_pinning: Optional[str] = None,
) -> List[str]:
    cmd = []

    # general args
    if cpu_pinning is not None:
        cmd += [
            "taskset",
            "-c",
            cpu_pinning,
        ]

    cmd += [
        "qemu-system-x86_64",
        "-enable-kvm",
        "-cpu",
        "host",
        "-m",
        "1024",
        "-device",
        "virtio-serial",
        "-drive",
        f"file={spec.qcow}",
        "-nographic",
        "-qmp",
        f"unix:{str(qmp_socket)},server,nowait",
    ]

    if spec.netbridge:
        cmd += [
            "-netdev",
            "bridge,id=en0,br=virbr0",
            "-device",
            "virtio-net-pci,netdev=en0",
        ]

    if spec.mnt_9p is not None:
        cmd += [
            "-fsdev",
            f"local,id=myid,path={spec.mnt_9p},security_model=none",
            "-device",
            "virtio-9p-pci,fsdev=myid,mount_tag=fs0,disable-modern=on,disable-legacy=off",
        ]

    return cmd


@contextmanager
def spawn_qemu(
    image: Union[UkVmSpec, NixosVmSpec],
    extra_args: List[str] = [],
    extra_args_pre: List[str] = [],
    log: Optional[Path] = None,
    cpu_pinning: Optional[str] = None,
    vcpu_pinning: List[int] = [],
) -> Iterator[QemuVm]:
    with TemporaryDirectory() as tempdir:
        qmp_socket = Path(tempdir).joinpath("qmp.sock")
        cmd = extra_args_pre.copy()
        ushell_socket = None
        if isinstance(image, UkVmSpec):
            ushell_socket = (
                Path(tempdir).joinpath("ushell.sock") if image.ushell_devices else None
            )
            # build qemu command for unikraft VM
            cmd += uk_qemu_command(
                image, qmp_socket, ushell_socket, cpu_pinning=cpu_pinning
            )
        elif isinstance(image, NixosVmSpec):
            # build qemu command for nixos VMs
            cmd += nixos_qemu_command(image, qmp_socket)
        else:
            raise Exception("unreachable")
        cmd += extra_args

        tmux_session = f"pytest-{os.getpid()}"
        tmux = [
            "tmux",
            "-L",
            tmux_session,
            "new-session",
            "-s",
            "qemu",
            "-d",  # QEMU_DEBUG: for debugging early qemu crashes, comment this and switch the following two lines:
            " ".join(map(quote, cmd))
            + (f" |& tee {str(log)}" if log is not None else ""),
            # " ".join(map(quote, cmd)) + " |& tee /tmp/foo; echo 'qemu ended'; sleep 999999",
        ]
        print("$ " + " ".join(map(quote, tmux)))
        subprocess.run(tmux, check=True)
        try:
            proc = subprocess.run(
                [
                    "tmux",
                    "-L",
                    tmux_session,
                    "list-panes",
                    "-a",
                    "-F",
                    "#{pane_pid}",
                ],
                stdout=subprocess.PIPE,
                check=True,
            )
            qemu_pid = int(proc.stdout)
            while not qmp_socket.exists():
                try:
                    os.kill(qemu_pid, 0)
                    time.sleep(0.1)
                except ProcessLookupError:
                    raise Exception(
                        "Qemu vm was terminated quickly. Check QEMU_DEBUG above."
                    )
            with connect_qmp(qmp_socket) as session:
                vm = QemuVm(session, tmux_session, qemu_pid, ushell_socket)
                vm.set_vcpumap(vcpu_pinning)
                yield vm
        finally:
            subprocess.run(["tmux", "-L", tmux_session, "kill-server"])
            while True:
                try:
                    os.kill(qemu_pid, 0)
                except ProcessLookupError:
                    break
                else:
                    print("waiting for qemu to stop")
                    time.sleep(1)
