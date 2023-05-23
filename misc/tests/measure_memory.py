from root import MEASURE_RESULTS
import confmeasure
import measure_helpers as util
import root

from typing import List, Any, Optional, Callable, Iterator, Tuple
import time
import re
from tqdm import tqdm
from tempfile import TemporaryDirectory
from pathlib import Path
from contextlib import contextmanager
from collections import defaultdict
import numpy as np

STATS_PATH = MEASURE_RESULTS.joinpath("memory-stats.json")

def get_memstats_cgroup(cgroup_name: str) -> Tuple[int, int, int]:
    path = Path(f"/sys/fs/cgroup/{cgroup_name}")
    if not path.exists():
        raise ValueError(f"cgroup {cgroup_name} does not exist")
    with open(str(path / "memory.current"), "r") as f:
        current = int(f.read())
    with open(str (path / "memory.peak"), "r") as f:
        peak = int(f.read())
    with open(str (path / "pids.current"), "r") as f:
        pids = int(f.read())
    return (current, peak, pids)

SIZE = 10
WARMUP = 0
def sample(f: Callable[[], Any], size: int = SIZE, warmup: int = WARMUP) -> List[Any]:
    ret = []
    for i in tqdm(range(0, warmup)):
        f()
    for i in tqdm(range(0, size)):
        f()

def measure_memory(helpers: confmeasure.Helpers,
                   stats: Any,
                   app: str,
                   shell: str,
                   bpf: str = "",
                   bootfs: str = "initrd",
                   force: bool = False) -> None:

    name = f"uk-{app}-{shell}"
    if len(bpf) > 0:
        name += f"-{bpf}"
    if app != "count":
        name += f"-{bootfs}"
    if not force and name in stats.keys():
        print(f"skip {name}")
        return
    print(f"measure {name} memory consumption")

    shellname = f"{shell}-memstat"
    if app == "count":
        vmspec = helpers.uk_count(shell=shellname, bpf=bpf)
    elif app == "nginx":
        vmspec = helpers.uk_nginx(shell=shellname, bootfs=bootfs, bpf=bpf)
    elif app == "redis":
        vmspec = helpers.uk_redis(shell=shellname, bootfs=bootfs, bpf=bpf)
    elif app == "sqlite3_backup":
        vmspec = helpers.uk_sqlite3_backup(shell=shellname, bootfs=bootfs, bpf=bpf)
    elif app == "sqlite_benchmark":
        vmspec = helpers.uk_sqlite(shell=shellname, bootfs=bootfs, bpf=bpf)
    else:
        raise ValueError(f"unknown app {app}")

    keys = [
        "last_alloc_size",
        "max_alloc_size",
        "min_alloc_size",
        "tot_nb_allocs",
        "tot_nb_frees",
        "cur_nb_allocs",
        "max_nb_allocs",
        "cur_mem_use",
        "max_mem_use",
        "nb_enomem",
    ]

    memstats = defaultdict(list)

    def experiment():
        with TemporaryDirectory() as tempdir_:
            log = Path(tempdir_) / "qemu.log"
            with helpers.spawn_qemu(vmspec, log=log, cgroup=True) as vm:
                time.sleep(2)

                cnt = 0
                while True:
                    with open(log, "r") as f:
                        msg = f.read()
                        if "max_mem_use:" in msg:
                            break
                    print(".")
                    cnt += 1
                    time.sleep(1)
                    if cnt > 20:
                        print(msg)
                        raise ValueError("timeout")

                # get the mem stats
                current, peak, pids = get_memstats_cgroup("ushell")
                memstats["total_host_mem_current"] += [current]
                memstats["total_host_mem_peak"] += [peak]

                if shell == "noshell":
                    memstats["total_host_mem_with_shell_current"] += [current]
                    memstats["total_host_mem_with_shell_peak"] += [peak]
                else:
                    # spawn a socat process
                    sh = vm.socat_Popen()
                    time.sleep(1)
                    # get the mem stats
                    current, peak, pids_ = get_memstats_cgroup("ushell")
                    assert pids_ == (pids + 1), f"{pids_} != {pids + 1}"
                    memstats["total_host_mem_with_shell_current"] += [current]
                    memstats["total_host_mem_with_shell_peak"] += [peak]

            with open(log, "r") as f:
                for l in f.readlines():
                    for k in keys:
                        if l.startswith(k):
                            memstats[k] += [int(l.split(":")[1])]

            return memstats

    sample(lambda: experiment())

    # choose median
    memstats_ = {}
    for k, v in memstats.items():
        print(f"{k}: {v}, med: {np.median(v)}, mean: {np.mean(v)} std: {np.std(v)}")
        memstats_[k] = np.median(v)

    stats[name] = memstats_
    util.write_stats(STATS_PATH, stats)


def main():
    util.check_intel_turbo()
    util.check_hyperthreading()
    util.check_root()
    util.check_cpu_isolation()
    helpers = confmeasure.Helpers()

    print(f"use stat file {STATS_PATH}")
    stats = util.read_stats(STATS_PATH)

    print("\nmeasure memory consumption")
    for app in ["count", "nginx", "redis", "sqlite_benchmark", "sqlite3_backup"]:

        # for shell in ["noshell", "ushell", "ushellmpk"]:
        #     measure_memory(helpers, stats, app, shell)

        measure_memory(helpers, stats, app, "noshell")
        measure_memory(helpers, stats, app, "ushellmpk", "bpf")

    util.export_fio("memory", stats)


if __name__ == "__main__":
    main()
