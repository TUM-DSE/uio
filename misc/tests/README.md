# Evaluation

This set of scripts runs the evaluation of this project (`measure_*.py`) and generates the plots from the results (`graphs.py`)

## Requirements

- A x86_86 CPU with native hardware virtualisation and MPK.
- Our test configuration expects at least 8 CPU cores.
- Python3 to run the scripts (tested with version 3.9).
- The [Nix](https://nixos.org/download.html) package manager to provide reproducible builds and handle software dependencies.
- Sudo
- Tmux

## Reproducability

- [./measuerments/eurosys24](./measurements/eurosys24) contains the submitted version of our measurements.
- Operating Systems
    - Linux 6.2.12, NixOS, 22.11 (Raccoon)
- Hardware
    - CPU: Intel(R) Xeon(R) Gold 5317 CPU @ 3.00GHz 12 cores
    - Memory: Samsung DDR4 64 3200 MT/s x 4 (256 GB)
- Our results are based on ushell eurosys24 branch
- For reproducability, check the warnings printed before the benchmark starts to align your setup with ours regarding CPU frequency, hyperthreading, and CPU isolation.

## Preparation

First, go to the root folder of this project and enter our development shell to issue the measurement commands:

```
cd <path-to-repo>/
nix develop # this starts a development shell
```

Set up the system:

```bash
cd <path-to-repo>/misc/tests
just setup_bridge
```

Optionally, you can pre-build all unikraft applications defined in flake.nix.
However, this will build all applications, not only the ones used in the measurements, and take some time.
```bash
# This will take about 30-60 minutes on the evaluation machine
cd <path-to-repo>/misc/tests
python3.9 ./nix.py
```

## Measurements

The following commands run the measurements and generate the plots.

```bash
cd <path-to-repo>/misc/tests
bash ./run_all.sh
```

### Individual measurements

- Console responsiveness (~3min when applications are built)
```bash
rm -r ./misc/tests/measurements/console-stats.json
sudo python3.9 ./misc/tests/measure_console.py
python3.9 ./misc/tests/graphs.py ./misc/tests/measurements/console-latest.tsv
ls ./misc/tests/measurements/console.pdf
```

- Application performance (~3hr when applications are built)
```bash
rm -r ./misc/tests/measurements/app-stats.json
sudo python3.9 ./misc/tests/measure_apps.py
python3.9 ./misc/tests/graphs.py ./misc/tests/measurements/app-latest.tsv
ls ./misc/tests/measurements/app.pdf
```

- Memory footprint (~3min when applications are built)
```bash
rm -r ./misc/tests/measurements/memory-stats.json
sudo python3.9 ./misc/tests/measure_memory.py
python3.9 ./misc/tests/graphs.py ./misc/tests/measurements/memory-latest.tsv
ls ./misc/tests/measurements/memory.pdf
```

- Image size (~1min when applications are built)
```bash
rm -r ./misc/tests/measurements/image-stats.json
sudo python3.9 ./misc/tests/measure_image.py
python3.9 ./misc/tests/graphs.py ./misc/tests/measurements/image-latest.tsv
ls ./misc/tests/measurements/images.pdf
```

Each test follows some steps:

If results exists for a test in `*-stats.json`, the test is skipped. To re-run them, delete the file or entries.

Run the tests in `measure_*.py`.
Depending on your system configuration, QEMU has to be started as root.
In that case you have to start the python script as root.
The measure script will run different experiments, multiple times each.
It builds the respective unikraft kernel (like `uk_nginx()` in [nix.py](./nix.py)), boots a VM with it (`spawn_qemu()` in [qemu.py](./qemy.py)) and runs the benchmark afterwards.
The QEMU VM is started in a tmux session whose socket is logged to stdout and can be connected to via `tmux -L ...`.
The test results are stored in [tests](./.)/measurements as `{name}-{date}-{time}.tsv` files.

TSV files are handed to `graph.py` which plots and writes them into PDFs (such as `./images.pdf`).

By default test scripts run experiments multiple times to get a more stable result.
If you want to do quick experiments, set `QUICK=True` in the respective `measure_*.py` file.

## Measurement List

- Figure 3, console responsiveness: `measure_console.py`
- Figure 4, load times: `measure_app.py` (`ushell_run()`)
- Figure 5, application performance: `measure_app.py` (`sqlite_shell`, `redis_shell`, `nginx_ushell`)
- Figure 6, image sizes: `measure_image.py`

## Robustness (MPK)
See [apps/mpktest](../../apps/mpktest).

## Use-cases

### 1. Interactive debugigng shell
See [apps/nginx](../../apps/nginx).

### 2. Online re-configuration
See [apps/nginx](../../apps/nginx).

### 3. SQLite backup
See [apps/sqlite3_backup](../../apps/sqlite3_backup).

### 4. Performance monitoring with performance counters
See [apps/count](../../apps/count) (`perf` program).

### 5. BPF
See [apps/nginx](../../apps/nginx). Also see [apps/bpf_prog](../../apps/bpf_prog) for the actual BPF programs.

## Development

By default we use a pinned version of the measured unikraft kernels for reproducability.
Measure other kernel configs/versions by updating the `self-stable.url` in `flake.nix` to point i.e. to your local git checkout. Every time the unikraft sources change, run `nix flake lock --update-input self-stable` to tell the nix builder about the new sources.

Measure_${title}.py files have a quick flag: Set to false runs a longer running version of the benchmark with less debug output which is meant for final evaluation.

## Note
- Use `python3.9`, not `python3` to avoid potential version problems
- `python3.9 -m IPython` runs an interactve ipython shell
