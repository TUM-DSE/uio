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

STATS_PATH = MEASURE_RESULTS.joinpath("memory-stats.json")


def measure_memory(helpers: confmeasure.Helpers, stats: Any,
                   shell: str) -> None:

    if f"{shell}" in stats.keys():
        print(f"skip {shell}")
        return
    print(f"measure {shell} memory consumption")

    memstats = {}

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

    with TemporaryDirectory() as tempdir_:
        log = Path(tempdir_) / "qemu.log"
        with helpers.spawn_qemu(helpers.uk_count(shell=f"{shell}-memstat"),
                                log=log) as vm:
            time.sleep(5)
            while True:
                with open(log, "r") as f:
                    if "max_mem_use:" in f.read():
                        break
                print(".")
                time.sleep(1)

        with open(log, "r") as f:
            for l in f.readlines():
                for k in keys:
                    if l.startswith(k):
                        memstats[k] = int(l.split(":")[1])

    stats[f"{shell}"] = memstats
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
    measure_memory(helpers, stats, "noshell")
    measure_memory(helpers, stats, "ushell")


if __name__ == "__main__":
    main()
