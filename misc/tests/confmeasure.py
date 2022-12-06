#!/usr/bin/env python3

import contextlib
import sys
from datetime import datetime
from pathlib import Path
from typing import List, Type, Optional

import pytest
from qemu import QemuVm, VmImage, VmSpec, spawn_qemu
import nix
from root import TEST_ROOT

sys.path.append(str(TEST_ROOT.parent))


NOW = datetime.now().strftime("%Y%m%d-%H%M%S")

# passed to numactl, starts with 0
CORES_VMSH = "1-3"
CORES_QEMU = "4"
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
    def uk_nginx() -> VmSpec:
        return nix.uk_nginx()

    @staticmethod
    def uk_count() -> VmSpec:
        return nix.uk_count()

    # @staticmethod
    # def spawn_vmsh_command(
    # args: List[str], cargo_executable: str = "vmsh"
    # ) -> VmshPopen:
    # return spawn_vmsh_command(
    # args, cargo_executable, target="release", pin_cores=CORES_VMSH
    # )

    # @staticmethod
    # def run_vmsh_command(args: List[str], cargo_executable: str = "vmsh") -> VmshPopen:
    # proc = spawn_vmsh_command(
    # args, cargo_executable, target="release", pin_cores=CORES_VMSH
    # )
    # assert proc.wait() == 0
    # return proc

    @staticmethod
    def spawn_qemu(
        image: VmSpec,
        extra_args: List[str] = [],
        extra_args_pre: List[str] = [],
    ) -> "contextlib._GeneratorContextManager[QemuVm]":
        return spawn_qemu(image, extra_args, extra_args_pre, cpu_pinning=CORES_QEMU)



@pytest.fixture
def helpers() -> Type[Helpers]:
    return Helpers
