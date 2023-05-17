from root import MEASURE_RESULTS
import confmeasure
import measure_helpers as util
from pylib import unwrap, unsafe_cast
from procs import run
import root

from typing import List, Any, Optional, Callable, Iterator, Tuple
import time
import socket as s
import select
import re
from tqdm import tqdm
from tempfile import TemporaryDirectory
from pathlib import Path
import numpy as np
import threading
from contextlib import contextmanager
import shutil


# overwrite the number of samples to take to a minimum
QUICK = False
# QUICK = True


DURATION = "1m"
if QUICK:
    DURATION = "10s"


SIZE = 10
WARMUP = 0
if QUICK:
    WARMUP = 0
    SIZE = 2


# REDIS_REPS = 100000
REDIS_REPS = 10000000
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

def readconsole(ushell: s.socket) -> str:
    fd = ushell.fileno()
    timeout=0.1
    (r, _, _) = select.select([fd], [], [], timeout)
    buf = ""
    while fd in r:
        out = ushell.recv(1).decode()
        buf += out
        (r, _, _) = select.select([fd], [], [], timeout)
        if len(out) == 0:
            break
    return buf

def expect(ushell: s.socket, timeout: int, until: Optional[str] = None) -> bool:
    """
    @return true if terminated because of until
    """
    if QUICK:
        print("begin readall until", until if until is not None else "<none>")

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
        else:
            print(f"ok: {buf}") # newline
    return ret


def assertline(ushell: s.socket, value: str) -> None:
    assert expect(ushell, 2, f"{value}")  # not sure why and when how many \r's occur.


def sendall(ushell: s.socket, content: str) -> None:
    if QUICK:
        print(f"[writeall] {content.strip()} ({len(content)} bytes)")
    c = ushell.send(str.encode(content))
    if QUICK:
        print(f"[writeall] sent {c} bytes")
    if c != len(content):
        raise Exception("TODO implement writeall")


def run_bin(ushell: s.socket, prompt: str, run: str = "hello", load: Optional[str] = None) -> float:
    """
    send newline and expect prompt back. Time to reply is returned.
    """
    if QUICK:
        print("measuring run")
    sw = time.monotonic()

    if load is not None:
        sendall(ushell, f"load {load}")
        assertline(ushell, prompt)

    sendall(ushell, f"run {run}")
    assertline(ushell, prompt)

    sw = time.monotonic() - sw

    time.sleep(0.5)
    return sw

def load_bin(ushell: s.socket, prompt: str, run: str = "hello"):
    sw = time.monotonic()
    sendall(ushell, f"prog-load {run}")
    assertline(ushell, prompt)
    sw = time.monotonic() - sw
    time.sleep(0.5)
    return sw

def load_sym(ushell: s.socket, prompt: str, symfile: str = "symbol.txt"):
    sw = time.monotonic()
    sendall(ushell, f"load {symfile}")
    assertline(ushell, prompt)
    sw = time.monotonic() - sw
    time.sleep(0.5)
    return sw

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

def lshuman_using_ushell(ushell: s.socket, alive) -> None:
    sendall(ushell, "\n")
    assert expect(ushell, 2, "> ")
    time.sleep(1)
    while alive.acquire(False):
        sendall(ushell, "ls\n")
        assert expect(ushell, 2, "> ")
        alive.release()
        time.sleep(1)

def wait_output(ushell: s.socket, c = ".") -> None:
    while True:
        r = readconsole(ushell)
        if len(r) > 0:
            if QUICK: print(f"[console recv] {r}")
            return
        print(c, end="", flush=True)
        time.sleep(1)

def perf_using_ushell(ushell: s.socket, alive, prepare, num=1000) -> None:
    sendall(ushell, f"load /ushell/symbol.txt\n")
    # loading symbol may take time
    wait_output(ushell, ".")
    sendall(ushell, f"run /ushell/perf.o {num}\n")
    # wait until at least we get one result
    wait_output(ushell, "*")
    with prepare:
        prepare.notifyAll()
    while alive.acquire(False):
        alive.release()
        r = readconsole(ushell)
        if QUICK and len(r) > 0: print(f"[console recv] {r}")
        time.sleep(1)

def attach_bpf_ushell(ushell: s.socket, alive, prepare, function_name,
                      prog="/ushell/bpf/count.bin") -> None:
    sendall(ushell, f"load /ushell/symbol.txt\n")
    # loading symbol may take time
    wait_output(ushell, ".")
    sendall(ushell, f"bpf_attach {function_name} {prog}\n")
    wait_output(ushell, "*")
    with prepare:
        prepare.notifyAll()
    while alive.acquire(False):
        alive.release()
        r = readconsole(ushell)
        if QUICK and len(r) > 0: print(f"[console recv] {r}")
        time.sleep(1)
    if QUICK:
        # for debug
        sendall(ushell, f"bpf_exec /ushell/bpf/get_count.bin {function_name}\n")
        r = readconsole(ushell)
        if len(r) > 0:
            print(f"[console recv] {r}")
        else:
            print(f"[console recv] BUG: No output")

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
    bpf: str = "",
    force: bool = False,
) -> None:
    """
    per sample: 4s
    """
    name = f"redis_{shell}"
    if len(bpf) > 0: name += f"_{bpf}"
    name += f"_{bootfs}_{human}"
    name_set = f"{name}-set"
    name_get = f"{name}-get"
    if not force and (name_set in stats.keys() and name_get in stats.keys()):
        print(f"skip {name}")
        return

    def experiment() -> Tuple[float, float]:
        ushell = s.socket(s.AF_UNIX)

        # with util.testbench_console(helpers) as vm:
        with helpers.spawn_qemu(helpers.uk_redis(shell=shell, bootfs=bootfs,
                                                 bpf=bpf)) as vm:
            vm.wait_for_ping("172.44.0.2")
            if vm.ushell_socket is not None:
                ushell.connect(bytes(vm.ushell_socket))

            # ensure readiness of system
            # time.sleep(1) # guest network stack is up, but also wait for application to start
            time.sleep(
                2
            )  # for the count app we dont really have a way to check if it is online

            human_ = None
            if human == "lshuman":
                # start human ushell usage
                alive = threading.Semaphore()
                human_ = threading.Thread(
                    target=lambda: lshuman_using_ushell(ushell, alive),
                    name="Human ushell user",
                )
                human_.start()
            elif human == "perf":
                # run perf
                alive = threading.Semaphore()
                prepare = threading.Condition()
                human_ = threading.Thread(
                    target=lambda: perf_using_ushell(ushell, alive, prepare),
                    name="Human ushell user",
                )
                human_.start()
                with prepare:
                    prepare.wait()
            elif human == "bpf":
                # attach bpf
                alive = threading.Semaphore()
                prepare = threading.Condition()
                function_name = "processCommand"
                human_ = threading.Thread(
                    target=lambda: attach_bpf_ushell(ushell, alive, prepare, function_name),
                    name="Human ushell user",
                )
                human_.start()
                with prepare:
                    prepare.wait()
            elif human == "nohuman":
                pass
            else:
                raise Exception(f"unknown human {human}")

            values = redis_bench("172.44.0.2", 6379)

            if human_ is not None:
                # terminate human
                alive.acquire(True)
                human_.join(timeout=5)
                if human_.is_alive():
                    raise Exception("human is still alive")

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


# sqlite benchmark
def sqlite_ushell(
    helpers: confmeasure.Helpers,
    stats: Any,
    shell: str = "ushell",
    bootfs: str = "9p",
    human: str = "nohuman",
    bpf: str = "",
    force: bool = False,
) -> None:
    """
    per sample:
    with 9p: 150s
    with initrd: 10s
    """
    name = f"sqlite_{shell}"
    if len(bpf) > 0: name += f"_{bpf}"
    name += f"_{bootfs}_{human}"
    if not force and name in stats.keys():
        print(f"skip {name}")
        return

    def experiment() -> float:
        ushell = s.socket(s.AF_UNIX)

        # with util.testbench_console(helpers) as vm:
        with TemporaryDirectory() as tempdir_:
            log = Path(tempdir_) / "qemu.log"
            with helpers.spawn_qemu(
                helpers.uk_sqlite(shell=shell, bootfs=bootfs, bpf=bpf), log=log
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


def nginx_ushell(
    helpers: confmeasure.Helpers,
    stats: Any,
    shell: str = "ushell",
    bootfs: str = "9p",
    human: str = "nohuman",
    bpf: str = "",
    force: bool = False,
) -> None:
    """
    per sample: 65s
    """
    name = f"nginx_{shell}"
    if len(bpf) > 0: name += f"_{bpf}"
    name += f"_{bootfs}_{human}"
    if not force and name in stats.keys():
        print(f"skip {name}")
        return
    print(f"run {name}")

    def experiment(human) -> float:
        ushell = s.socket(s.AF_UNIX)

        # with util.testbench_console(helpers) as vm:
        with helpers.spawn_qemu(helpers.uk_nginx(shell=shell, bootfs=bootfs,
                                                 bpf=bpf)) as vm:
            vm.wait_for_ping("172.44.0.2")
            if vm.ushell_socket is not None:
                ushell.connect(bytes(vm.ushell_socket))

            # ensure readiness of system
            # time.sleep(1) # guest network stack is up, but also wait for application to start
            time.sleep(
                2
            )  # for the count app we dont really have a way to check if it is online

            human_ = None
            if human == "lshuman":
                # start human ushell usage
                alive = threading.Semaphore()
                human_ = threading.Thread(
                    target=lambda: lshuman_using_ushell(ushell, alive),
                    name="Human ushell user",
                )
                human_.start()
            elif human == "perf":
                # run perf
                alive = threading.Semaphore()
                prepare = threading.Condition()
                human_ = threading.Thread(
                    target=lambda: perf_using_ushell(ushell, alive, prepare),
                    name="Human ushell user",
                )
                human_.start()
                with prepare:
                    prepare.wait()
            elif human == "bpf":
                # attach bpf
                alive = threading.Semaphore()
                prepare = threading.Condition()
                function_name = "ngx_http_process_request_line"
                human_ = threading.Thread(
                    target=lambda: attach_bpf_ushell(ushell, alive, prepare, function_name),
                    name="Human ushell user",
                )
                human_.start()
                with prepare:
                    prepare.wait()
            elif human == "nohuman":
                pass
            else:
                raise Exception(f"unknown human {human}")

            value = nginx_bench("172.44.0.2")

            if human_ is not None:
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
        run(["taskset", "-pc", confmeasure.CORES_QEMU, str(nginx.pid)])

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


def ushell_run(
    helpers: confmeasure.Helpers,
    stats: Any,
    shell: str = "ushell",
    bpf: str = "",
    force: bool = False,
) -> None:
    """
    per sample:
    """
    name_ = f"{shell}"
    if len(bpf) > 0:
        name_ += f"-{bpf}"
    name_load = f"{name_}_run"
    name_load_sym = f"{name_}_load_sym"
    name_load_cached = f"{name_}-run-cached"
    if not force and (name_load in stats.keys() and name_load_cached in stats.keys()):
        print(f"skip {name_}")
        return

    def experiment(loadable: str, exec_args: str = "") -> List[float]:
        ushell = s.socket(s.AF_UNIX)
        cmdline = f"{loadable} {exec_args}"

        # with util.testbench_console(helpers) as vm:
        with TemporaryDirectory() as tempdir_:
            log = Path(tempdir_) / "qemu.log"
            vm_spec = helpers.uk_count(shell=shell, bpf=bpf)
            with helpers.spawn_qemu(vm_spec, log=log) as vm:
                # if vm_spec.fs1_9p is not None:
                    # raise Exception("unwrap failed")
                bin_source = unwrap(vm_spec.ushelldir) / f"{loadable}.c"
                bin_include = unwrap(vm_spec.fs1_9p) / "../../common/include"
                bin_out = unwrap(vm_spec.ushelldir) / f"{loadable}"
                if vm_spec.ushelldir.name == "fs1":
                    bin_syms = "/ushell/symbol.txt"
                else:
                    bin_syms = "/symbol.txt"

                # building is now handled in the app's Makefile; we do not do it here
                # build the executable
                #run(["gcc", f"-I{bin_include}", "-DHAS_MPK", "-fPIC", "-c", "-o", str(bin_out), str(bin_source)])
                # extract symbols
                #run(["sh", "-c", f"nm {unwrap(vm_spec.kernel)}.dbg | cut -d ' ' -f1,3 > {bin_syms}"])

                # vm.wait_for_ping("172.44.0.2")
                ushell.connect(bytes(unsafe_cast(vm.ushell_socket)))

                # ensure readiness of system
                time.sleep(5) # guest network stack is up, but also wait for application to start
                # time.sleep(2) # for the count app we dont really have a way to check if it is online

                # load has huge variances (0.009-0.000,09s). Thus we exclude it from measurements.
                # sendall(ushell, f"load {bin_syms}\n")
                # assertline(ushell, "> ")
                # time.sleep(0.5)
                load_sym_time = load_sym(ushell, "> ", symfile=bin_syms)
                time.sleep(0.5)

                # value = run_bin(ushell, "> ", run=cmdline, load=None)
                # value_cached = run_bin(ushell, "> ", run=cmdline, load=None)
                load_time = load_bin(ushell, "> ", run=cmdline)
                time.sleep(0.5)
                load_time_cached = load_bin(ushell, "> ", run=cmdline)

                print(f"load_sym_time: {load_sym_time}")
                print(f"load_time: {load_time}")
                print(f"load_time (cached): {load_time_cached}")

                # return values
                # vm.wait_for_death()
            ushell.close()
            return [load_sym_time, load_time, load_time_cached]

    # samples = sample(lambda: experiment("set_count_func", exec_args = "3"))
    samples = sample(lambda: experiment("perf.o", exec_args = ""))

    load_sym_ = []
    load_ = []
    load_cached = []
    for load_sym_time, load_time, load_time_cached in samples:
        load_sym_ += [load_sym_time]
        load_ += [load_time]
        load_cached += [load_time_cached]

    stats[name_load_sym] = load_sym_
    stats[name_load] = load_
    stats[name_load_cached] = load_cached
    util.write_stats(STATS_PATH, stats)


def calculate_average_overhead(stats):
    def overhead(baseline, new) -> float:
        """
        new = baseline - overhead * baseline
        new = baseline (1 - overhead)
        =>
        overhead = (baseline - new) / baseline
        """
        return (np.mean(baseline) - np.mean(new)) / np.mean(baseline)
    ushell = [
        overhead(stats['nginx_noshell_initrd_nohuman'], 
                 stats['nginx_ushell_initrd_nohuman']),
        np.mean([
            overhead(stats['redis_noshell_initrd_nohuman-get'], 
                     stats['redis_ushell_initrd_nohuman-get']),
            overhead(stats['redis_noshell_initrd_nohuman-set'], 
                     stats['redis_ushell_initrd_nohuman-set'])
            ]),
        overhead(stats['sqlite_noshell_initrd_nohuman'], 
                 stats['sqlite_ushell_initrd_nohuman']),
        ]
    ushellmpk = [
        overhead(stats['nginx_noshell_initrd_nohuman'], 
                 stats['nginx_ushellmpk_initrd_nohuman']),
        np.mean([
            overhead(stats['redis_noshell_initrd_nohuman-get'], 
                     stats['redis_ushellmpk_initrd_nohuman-get']),
            overhead(stats['redis_noshell_initrd_nohuman-set'], 
                     stats['redis_ushellmpk_initrd_nohuman-set'])
            ]),
        overhead(stats['sqlite_noshell_initrd_nohuman'], 
                 stats['sqlite_ushellmpk_initrd_nohuman']),
        ]
    print("Ushell overhead:")
    print(ushell)
    print(f"mean: {np.mean(ushell)}")
    print("Ushellmpk overhead:")
    print(ushellmpk)
    print(f"mean: {np.mean(ushellmpk)}")
    stddev = np.mean([
        np.std(stats['nginx_noshell_initrd_nohuman']) / np.mean(stats['nginx_noshell_initrd_nohuman']),
        np.mean([
            np.std(stats['redis_noshell_initrd_nohuman-get']) / np.mean(stats['redis_noshell_initrd_nohuman-get']),
            np.std(stats['redis_noshell_initrd_nohuman-set']) / np.mean(stats['redis_noshell_initrd_nohuman-set']),
            ]),
        np.std(stats['sqlite_noshell_initrd_nohuman']) / np.mean(stats['sqlite_noshell_initrd_nohuman']),
        ])
    print(f"mean stddev of baselines to estimate confidence in overhead values: {stddev}")

    return {
        "ushell_overhead": [np.mean(ushell)],
        "ushellmpk_overhead": [np.mean(ushellmpk)],
        "baseline_unikraft_stddev": [stddev]
        }


def check_requirements():
    util.check_intel_turbo()
    util.check_hyperthreading()
    util.check_root()
    util.check_cpu_isolation()

def main_old() -> None:
    """
    not quick: ~42min
    """
    check_requirements()
    helpers = confmeasure.Helpers()

    stats = util.read_stats(STATS_PATH)

    def with_all_configs(f, dur):
        for shell in ["ushell", "noshell", "ushellmpk"]:
            for bootfs in ["initrd"]:
                print(f"\nmeasure performance for {f.__name__} ({shell}, {bootfs}) (~{dur}sec each)\n")
                f(helpers, stats, shell=shell, bootfs=bootfs)
                if shell == "ushellmpk":
                    for bpf in ["bpf", "bpf-nomcount"]:
                        print(f"\nmeasure performance for {f.__name__} ({shell}, {bootfs} {bpf}) (~{dur}sec each)\n")
                        f(helpers, stats, shell=shell, bootfs=bootfs, bpf=bpf)



    print("\nmeasure performance when running external ushell apps (~8sec each)\n")
    ushell_run(helpers, stats, shell = "ushell") # 10x 8s
    print("\nmeasure performance when running external ushellmpk apps (~8sec each)\n")
    ushell_run(helpers, stats, shell = "ushellmpk") # 10x 8s

    with_all_configs(sqlite_ushell, 8)  # 3x10x 8s
    with_all_configs(redis_ushell, 4)  # 3x10x 4s
    with_all_configs(nginx_ushell, 65)  # 3x10x 65s

    # 5x 65s
    # print("\nmeasure performance for nginx ushell with initrd and human interaction\n")
    # nginx_ushell(helpers, stats, shell="ushell", bootfs="initrd", human="lshuman")
    # 5x 65s
    # print("\nmeasure performance for nginx ushellmpk with initrd and human interaction\n")
    # nginx_ushell(helpers, stats, shell="ushellmpk", bootfs="initrd", human="lshuman")
    # 5x 65s
    # print("\nmeasure performance for nginx ushell with 9p and human interaction\n")
    # nginx_ushell(helpers, stats, shell="ushell", bootfs="9p", human="lshuman")

    # print("\nmeasure performance for nginx native\n")
    # nginx_native(helpers, stats)  # 5x 65s

    # print("\nmeasure performance for nginx qemu with 9p\n")
    # nginx_qemu_9p(helpers, stats)  # 5x 75s

    util.export_fio("app", stats)
    means = calculate_average_overhead(stats)
    util.export_fio("app-mean", means)

def main(all_: bool = False) -> None:
    """all_: run all measurements, not just the ones used in the paper
    """
    check_requirements()
    helpers = confmeasure.Helpers()
    stats = util.read_stats(STATS_PATH)

    if all_:
        print("Run all measurements")
    if QUICK:
        print("Do quick experiment")
    else:
        print("Do full experiment")

    ## program loading performance
    ushell_run(helpers, stats, shell = "ushell", bpf="bpf")
    ushell_run(helpers, stats, shell = "ushellmpk", bpf="bpf")

    ## app performance with no shell
    nginx_ushell(helpers, stats, shell="noshell", bootfs="initrd")
    sqlite_ushell(helpers, stats, shell="noshell", bootfs="initrd")
    redis_ushell(helpers, stats, shell="noshell", bootfs="initrd")

    if all_:
        nginx_ushell(helpers, stats, shell="noshell-mcount", bootfs="initrd")
        sqlite_ushell(helpers, stats, shell="noshell-mcount", bootfs="initrd")
        redis_ushell(helpers, stats, shell="noshell-mcount", bootfs="initrd")

    ## app performance with ushell (nompk, nobpf (mcount))
    nginx_ushell(helpers, stats, shell="ushell", bootfs="initrd")
    sqlite_ushell(helpers, stats, shell="ushell", bootfs="initrd")
    redis_ushell(helpers, stats, shell="ushell", bootfs="initrd")

    ## app performance with ushell (nompk, bpf)
    nginx_ushell(helpers, stats, shell="ushell", bpf="bpf", bootfs="initrd")
    sqlite_ushell(helpers, stats, shell="ushell", bpf="bpf", bootfs="initrd")
    redis_ushell(helpers, stats, shell="ushell", bpf="bpf", bootfs="initrd")

    if all_:
        nginx_ushell(helpers, stats, shell="ushell", bpf="bpf-nomcount", bootfs="initrd")
        sqlite_ushell(helpers, stats, shell="ushell", bpf="bpf-nomcount", bootfs="initrd")
        redis_ushell(helpers, stats, shell="ushell", bpf="bpf-nomcount", bootfs="initrd")

    ## app performance with ushell (mpk, nobpf)
    if all_:
        nginx_ushell(helpers, stats, shell="ushellmpk", bootfs="initrd")
        sqlite_ushell(helpers, stats, shell="ushellmpk", bootfs="initrd")
        redis_ushell(helpers, stats, shell="ushellmpk", bootfs="initrd")

    ## app performance with ushell (mpk+bpf)
    nginx_ushell(helpers, stats, shell="ushellmpk", bpf="bpf", bootfs="initrd")
    sqlite_ushell(helpers, stats, shell="ushellmpk", bpf="bpf", bootfs="initrd")
    redis_ushell(helpers, stats, shell="ushellmpk", bpf="bpf", bootfs="initrd")

    if all_:
        nginx_ushell(helpers, stats, shell="ushellmpk", bpf="bpf-nomcount", bootfs="initrd")
        sqlite_ushell(helpers, stats, shell="ushellmpk", bpf="bpf-nomcount", bootfs="initrd")
        redis_ushell(helpers, stats, shell="ushellmpk", bpf="bpf-nomcount", bootfs="initrd")

    ## app performance with ushell progs
    ## ls
    nginx_ushell(helpers, stats, shell="ushell", bootfs="initrd", bpf="bpf", human="lshuman")
    nginx_ushell(helpers, stats, shell="ushellmpk", bootfs="initrd", bpf="bpf", human="lshuman")
    redis_ushell(helpers, stats, shell="ushell", bootfs="initrd", bpf="bpf", human="lshuman")
    redis_ushell(helpers, stats, shell="ushellmpk", bootfs="initrd", bpf="bpf", human="lshuman")
    #sqlite_ushell(helpers, stats, shell="ushellmpk", bootfs="initrd", bpf="bpf", human="lshuman")

    ## perf
    nginx_ushell(helpers, stats, shell="ushell", bootfs="initrd", bpf="bpf", human="perf")
    nginx_ushell(helpers, stats, shell="ushellmpk", bootfs="initrd", bpf="bpf", human="perf")
    redis_ushell(helpers, stats, shell="ushell", bootfs="initrd", bpf="bpf", human="perf")
    redis_ushell(helpers, stats, shell="ushellmpk", bootfs="initrd", bpf="bpf", human="perf")

    ## app performance with BPF tracing
    nginx_ushell(helpers, stats, shell="ushell", bootfs="initrd", bpf="bpf", human="bpf")
    nginx_ushell(helpers, stats, shell="ushellmpk", bootfs="initrd", bpf="bpf", human="bpf")
    redis_ushell(helpers, stats, shell="ushell", bootfs="initrd", bpf="bpf", human="bpf")
    redis_ushell(helpers, stats, shell="ushellmpk", bootfs="initrd", bpf="bpf", human="bpf")

    # Export results
    util.export_fio("app", stats)

if __name__ == "__main__":
    from argparse import ArgumentParser
    argparser = ArgumentParser()
    argparser.add_argument("--all", action="store_true",
                           help="run all measurements, not just the ones used in the paper")
    args = argparser.parse_args()
    main(args.all)
