
from root import MEASURE_RESULTS
import confmeasure
import measure_helpers as util
import nix
from typing import Any

import os


STATS_PATH = MEASURE_RESULTS.joinpath("symsize-stats.json")


def sym_size(helpers: confmeasure.Helpers, stats: Any, vmspec: nix.UkVmSpec) -> None:
    name = vmspec.flake_name
    if name in stats.keys():
        print(f"skip {name}")
        return

    num_bytes = os.path.getsize(vmspec.symfile)

    stats[name] = [ num_bytes ]
    util.write_stats(STATS_PATH, stats)

def main() -> None:
    """
    not quick: takes a few seconds
    """
    util.check_intel_turbo()
    util.check_hyperthreading()
    util.check_root()
    util.check_cpu_isolation()
    helpers = confmeasure.Helpers()

    stats = util.read_stats(STATS_PATH)

    print("measure symbol file size of different unikraft images (takes a few seconds, if all images are already built)")

    # ushell (mpk + bpf, mcount)
    sym_size(helpers, stats, nix.uk_count("ushellmpk", "bpf"))
    sym_size(helpers, stats, nix.uk_nginx("ushellmpk", "initrd", "bpf"))
    sym_size(helpers, stats, nix.uk_redis("ushellmpk", "initrd", "bpf"))
    sym_size(helpers, stats, nix.uk_sqlite3_backup("ushellmpk", "initrd", "bpf"))

    util.export_fio("symsize", stats)

if __name__ == "__main__":
    main()
