from procs import run
from root import PROJECT_ROOT


def main():
    print("ushell")
    proc = run([
        "tokei", f"{PROJECT_ROOT}/unikraft/lib/ushell",
        f"{PROJECT_ROOT}/unikraft/lib/ukpku",
        f"{PROJECT_ROOT}/libs/ubpf_tracer/src",
        f"{PROJECT_ROOT}/libs/ubpf_tracer/include",
        "-f", "-e", "**/test", "-e", "*.json"
    ])
    print(proc.stdout)

    print("ubpf")
    proc = run([
        "tokei",
        f"{PROJECT_ROOT}/repo/ubpf/vm/ubpf_vm.c",
        f"{PROJECT_ROOT}/repo/ubpf/vm/ubpf_loader.c",
        f"{PROJECT_ROOT}/repo/ubpf/vm/ubpf_int.h",
        f"{PROJECT_ROOT}/repo/ubpf/vm/ebpf.h",
        "-f",
    ])
    print(proc.stdout)


if __name__ == "__main__":
    main()
