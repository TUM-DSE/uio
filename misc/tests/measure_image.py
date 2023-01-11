from root import MEASURE_RESULTS
import confmeasure
import measure_helpers as util
import nix
from typing import Any

import os


STATS_PATH = MEASURE_RESULTS.joinpath("image-stats.json")


def image_size(helpers: confmeasure.Helpers, stats: Any, vmspec: nix.UkVmSpec) -> None:
    name = vmspec.flake_name
    if name in stats.keys():
        print(f"skip {name}")
        return

    num_bytes = os.path.getsize(vmspec.kernel)


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

    print("measure size of different unikraft images (takes a few seconds, if all images are already built)")

    image_size(helpers, stats, nix.uk_count("noshell"))
    image_size(helpers, stats, nix.uk_count("ushell"))
    image_size(helpers, stats, nix.uk_nginx("noshell", "initrd"))
    image_size(helpers, stats, nix.uk_nginx("ushell", "initrd"))
    image_size(helpers, stats, nix.uk_redis("noshell", "initrd"))
    image_size(helpers, stats, nix.uk_redis("ushell", "initrd"))
    # image_size(helpers, stats, nix.uk_sqlite("noshell", "initrd"))
    # image_size(helpers, stats, nix.uk_sqlite("ushell", "initrd"))
    image_size(helpers, stats, nix.uk_sqlite3_backup("noshell", "initrd"))
    image_size(helpers, stats, nix.uk_sqlite3_backup("ushell", "initrd"))

    image_size(helpers, stats, nix.uk_count("noshell", lto=True))
    image_size(helpers, stats, nix.uk_count("ushell", lto=True))
    image_size(helpers, stats, nix.uk_nginx("noshell", "initrd", lto=True))
    image_size(helpers, stats, nix.uk_nginx("ushell", "initrd", lto=True))
    image_size(helpers, stats, nix.uk_redis("noshell", "initrd", lto=True))
    image_size(helpers, stats, nix.uk_redis("ushell", "initrd", lto=True))
    # image_size(helpers, stats, nix.uk_sqlite("noshell", "initrd", lto=True))
    # image_size(helpers, stats, nix.uk_sqlite("ushell", "initrd", lto=True))
    image_size(helpers, stats, nix.uk_sqlite3_backup("noshell", "initrd", lto=True))
    image_size(helpers, stats, nix.uk_sqlite3_backup("ushell", "initrd", lto=True))

    util.export_fio("image", stats)


if __name__ == "__main__":
    main()
