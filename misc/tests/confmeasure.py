#!/usr/bin/env python3

import contextlib
import sys
from datetime import datetime
from pathlib import Path
from typing import List, Type, Optional, Union, Iterator
from contextlib import contextmanager

import pytest
from qemu import QemuVm, VmImage, NixosVmSpec, UkVmSpec, spawn_qemu
import nix
from root import TEST_ROOT

sys.path.append(str(TEST_ROOT.parent))


NOW = datetime.now().strftime("%Y%m%d-%H%M%S")

# passed to numactl, starts with 0
CORES_QEMU = "4"
CORES_VCPU1 = "5"
CORES_BENCHMARK = "6,7"


class Helpers:
    @staticmethod
    def root() -> Path:
        return TEST_ROOT

    @staticmethod
    def notos_image() -> VmImage:
        return nix.notos_image(nix=".#measurement-image")

    @staticmethod
    def busybox_image() -> "contextlib._GeneratorContextManager[Path]":
        return nix.busybox_image()

    @staticmethod
    def uk_sqlite3_backup(shell: str = "ushell", bootfs: str = "9p", bpf: str = "") -> UkVmSpec:
        return nix.uk_sqlite3_backup(shell, bootfs, bpf)

    @staticmethod
    def uk_sqlite(shell: str = "ushell", bootfs: str = "9p", bpf: str = "") -> UkVmSpec:
        return nix.uk_sqlite(shell, bootfs, bpf)

    @staticmethod
    def uk_redis(shell: str = "ushell", bootfs: str = "9p", bpf: str = "") -> UkVmSpec:
        return nix.uk_redis(shell, bootfs, bpf)

    @staticmethod
    def uk_nginx(shell: str = "ushell", bootfs: str = "9p", bpf: str = "") -> UkVmSpec:
        return nix.uk_nginx(shell, bootfs, bpf)

    @staticmethod
    def uk_count(shell: str = "ushell", bpf: str = "") -> UkVmSpec:
        return nix.uk_count(shell, bpf)

    @staticmethod
    @contextmanager
    def nixos_nginx() -> Iterator[NixosVmSpec]:
        with nix.nixos_nginx() as a:
            yield a

    @staticmethod
    def spawn_qemu(
        image: Union[UkVmSpec, NixosVmSpec],
        log: Optional[Path] = None,
        extra_args: List[str] = [],
        extra_args_pre: List[str] = [],
        cgroup: bool = False,
    ) -> "contextlib._GeneratorContextManager[QemuVm]":
        return spawn_qemu(
            image, extra_args, extra_args_pre, log=log, cpu_pinning=CORES_QEMU, vcpu_pinning=[int(CORES_VCPU1)], cgroup=cgroup
        )


@pytest.fixture
def helpers() -> Type[Helpers]:
    return Helpers
