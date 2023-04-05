from procs import run
from root import PROJECT_ROOT


def main():
    proc = run([
        "tokei", f"{PROJECT_ROOT}/unikraft/lib/ushell",
        f"{PROJECT_ROOT}/unikraft/lib/ukpku", "-f", "-e", "**/test", "-e",
        "*.json"
    ])
    print(proc.stdout)


if __name__ == "__main__":
    main()
