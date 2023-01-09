import confmeasure
from confmeasure import NOW
from root import MEASURE_RESULTS
from procs import run
from qemu import QemuVm

import os
import json
from typing import List, Any, Iterator, Dict, DefaultDict, Optional
from collections import defaultdict
from contextlib import contextmanager
import subprocess
import pandas as pd
from pathlib import Path
import time
import psutil


HOST_SSD = os.environ.get("HOST_SSD", "/dev/nvme0n1")
HOST_DIR = "/mnt/nvme"
# XXX use Path everywhere
HOST_DIR_PATH = Path(HOST_DIR)
GUEST_JAVDEV = "/dev/vdc"
GUEST_QEMUBLK = "/dev/vdb"
GUEST_QEMU9P = "/9p"
GUEST_JAVDEV_MOUNT = "/javdev"
GUEST_JAVDEV_PATH = Path(GUEST_JAVDEV_MOUNT)
GUEST_QEMUBLK_MOUNT = "/blk"
GUEST_QEMUBLK_PATH = Path(GUEST_QEMUBLK_MOUNT)


@contextmanager
def testbench_console(
    helpers: confmeasure.Helpers,
) -> Iterator[Any]:
    with helpers.spawn_qemu(helpers.uk_count()) as vm:
        yield vm


def blkdiscard() -> Any:
    run(["sudo", "chown", os.getlogin(), HOST_SSD])
    proc = run(
        ["sudo", "blkdiscard", "-f", HOST_SSD], check=False, stderr=subprocess.PIPE
    )
    while "Device or resource busy" in proc.stderr:
        print("blkdiscard: waiting for target not to be busy")
        time.sleep(1)
        proc = run(
            ["sudo", "blkdiscard", "-f", HOST_SSD], check=False, stderr=subprocess.PIPE
        )
    proc.check_returncode()


@contextmanager
def fresh_fs_ssd(image: Optional[Path] = None, filesize: int = 10) -> Iterator[Any]:
    while (
        "target is busy"
        in run(["sudo", "umount", HOST_SSD], check=False, stderr=subprocess.PIPE).stderr
    ):
        print("umount: waiting for target not to be busy")
        time.sleep(1)
    blkdiscard()
    if image:
        run(
            [
                "sudo",
                "dd",
                "status=progress",
                "bs=128M",
                "iflag=direct",
                "oflag=direct",
                "conv=fdatasync",
                f"if={image}",
                f"of={HOST_SSD}",
            ]
        )
        run(["sudo", "resize2fs", "-f", HOST_SSD])
    else:
        run(["sudo", "mkfs.ext4", HOST_SSD])
    if not HOST_DIR_PATH.exists():
        run(["sudo", "mkdir", "-p", HOST_DIR])
    run(["sudo", "mount", HOST_SSD, HOST_DIR])
    run(["sudo", "chown", os.getlogin(), HOST_DIR])
    run(["sudo", "chown", os.getlogin(), HOST_SSD])
    try:
        run(["touch", f"{HOST_DIR}/file"], check=True)
        # writing 1TB takes 10min. Fallocate a few seconds.
        # run(["fallocate", "-l", f"{filesize}G", f"{HOST_DIR}/file"], check=True)
        yield
    finally:
        run(["sudo", "chown", "0", HOST_SSD])
        if Path(HOST_DIR).is_mount():
            run(["sudo", "umount", HOST_DIR], check=False)


def check_ssd() -> None:
    if "HOST_SSD" in os.environ:
        return
    print(subprocess.check_output(["lsblk"]).decode())
    input_ = "y"
    input_ = input(f"Delete {HOST_SSD} to use for benchmark? [Y/n] ")
    if input_ != "Y" and input_ != "y" and input_ != "":
        print("Aborting.")
        exit(1)

    if HOST_SSD in open("/proc/mounts", "r").read():
        print("Please unmount the device first. ")
        exit(1)


def check_intel_turbo() -> None:
    path = "/sys/devices/system/cpu/intel_pstate/no_turbo"
    if os.path.exists(path):
        with open(path) as f:
            if f.readline() != "1\n":
                print(
                    """Please run: echo 1 | sudo tee /sys/devices/system/cpu/intel_pstate/no_turbo"""
                )
                exit(1)

def is_hyperthreading_enabled():
    cpuinfo = dict(map(str.strip, line.split(':'))
                   for line in open('/proc/cpuinfo')
                   if ':' in line)

    return cpuinfo['siblings'] != cpuinfo['cpu cores']

def check_root():
    import getpass
    if getpass.getuser() != 'root':
        print("Warn: Depending on your system configuration, qemu has to be started as root. In that case you have to start this script as root.")

def check_cpu_isolation():
    with open("/sys/devices/system/cpu/isolated") as f:
        lines = f.readlines()
        if len(f.readlines()) == 0:
            print("Warn: seems like you dont isolate cpu cores for the benchmark: add isolcpus=4-7 to your kernel parameters.")

def check_hyperthreading() -> None:
    if is_hyperthreading_enabled():
        print("Warn: Please disable hyperthreading in you BIOS.")
        # exit(1)

MEMORY_HOG = bytearray(0)


def check_memory() -> None:
    global MEMORY_HOG
    avail = psutil.virtual_memory().available
    GB = 1024 * 1024 * 1024
    needed = 12 * GB
    if avail > needed:
        print(
            f"""
Your system has {avail/GB:.1f} GB of memory available. To prevent excessive use of page caches, we are now allocating {(avail - needed)/GB:.1f} GB of memory which will leave you with {needed/GB:.1f} GB of available memory for the test.
        """
        )
        MEMORY_HOG = bytearray(avail - needed)


# look at those caches getting warm
def export_lineplot(name: str, data: Dict[str, List[float]]) -> None:
    frame = pd.DataFrame(data)
    path = f"{MEASURE_RESULTS}/{name}-{NOW}.tsv"
    print(path)
    frame.to_csv(path, index=False, sep="\t")
    frame.to_csv(f"{MEASURE_RESULTS}/{name}-latest.tsv", index=False, sep="\t")


def export_barplot(name: str, data: Dict[str, List[float]]) -> None:
    frame = pd.DataFrame(data)
    frame = frame.describe()
    path = f"{MEASURE_RESULTS}/{name}-{NOW}.tsv"
    print(path)
    frame.to_csv(path, sep="\t")
    frame.to_csv(f"{MEASURE_RESULTS}/{name}-latest.tsv", index=True, sep="\t")


def export_fio(name: str, data: Dict[str, List[float]]) -> None:
    os.makedirs(MEASURE_RESULTS, exist_ok=True)
    df = pd.DataFrame(data)
    print(df.describe())
    path = f"{MEASURE_RESULTS}/{name}-{NOW}.tsv"
    print(path)
    df.to_csv(path, index=True, sep="\t")
    df.to_csv(f"{MEASURE_RESULTS}/{name}-latest.tsv", index=True, sep="\t")


def read_stats(path: Path) -> DefaultDict[str, List]:
    stats: DefaultDict[str, List] = defaultdict(list)
    if not os.path.exists(path):
        return stats
    with open(path) as f:
        p = json.load(f)
        stats.update(p)
        return stats


def write_stats(path: Path, stats: Dict[str, List]) -> None:
    path.parent.mkdir(exist_ok=True, parents=True)
    with open(path, "w") as f:
        json.dump(
            stats,
            f,
            indent=4,
            sort_keys=True,
        )
