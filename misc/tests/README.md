# Evaluation

This set of scripts runs the evaluation of this project (`measure_*.py`) and generates the plots from the results (`graph.py`)

TODO add https://github.com/Mic92/vmsh/blob/main/tests/reproduce.py so that user only has to issue a single command to do everything

## Requirements

- A x86_86 CPU with native hardware virtualisation. Our test configuration expects at least 8 CPU cores.
- Python3 to run the scripts (tested with version 3.9).
- The [Nix](https://nixos.org/download.html) package manager to provide reproducible builts and handle software dependencies.
- Sudo
- Tmux


## Reproducability

- For reproducability, check the warnings printed before the benchmark starts to align your setup with ours regarding CPU frequency, hyperthreading, and CPU isolation.
- We run our experiments on a Linux 6.1.21 kernel (NixOS 22.11.20230417.a52af07).
- Our results are based on unikraft-development commit TODO and unikraft commit TODO (tarball also uploaded to zendo.org? TODO)
- Exact hardware specs: TODO


## Measurements

First, go to the root folder of this project and enter our development shell to issue the measurement commands:

```
nix develop
```

Set up the system:

```bash
# set up a bridge for qemu networking
pushd apps/nginx
just setup_bridge
popd

# you can choose to build all the unikraft kernels in advance
python3.9 ./misc/tests/nix.py
```

Now we run the actual tests:

```bash
# console responsiveness
rm -r ./misc/tests/measurements/console-stats.json
sudo python3.9 ./misc/tests/measure_console.py
python3.9 ./misc/tests/graph.py ./misc/tests/measurements/console-latest.tsv
ls ./misc/tests/measurements/console.pdf

# application performance
rm -r ./misc/tests/measurements/app-stats.json
sudo python3.9 ./misc/tests/measure_apps.py
python3.9 ./misc/tests/graph.py ./misc/tests/measurements/app-latest.tsv
ls ./misc/tests/measurements/app.pdf

# memory footprint
rm -r ./misc/tests/measurements/memory-stats.json
sudo python3.9 ./misc/tests/measure_memory.py
python3.9 ./misc/tests/graph.py ./misc/tests/measurements/memory-latest.tsv
ls ./misc/tests/measurements/memory.pdf

# image size
rm -r ./misc/tests/measurements/image-stats.json
sudo python3.9 ./misc/tests/measure_image.py
python3.9 ./misc/tests/graph.py ./misc/tests/measurements/image-latest.tsv
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

## Development

By default we use a pinned version of the measured unikraft kernels for reproducability. 
Measure other kernel configs/versions by updating the `self-stable.url` in `flake.nix` to point i.e. to your local git checkout. Every time the unikraft sources change, run `nix flake lock --update-input self-stable` to tell the nix builder about the new sources.

Measure_${title}.py files have a quick flag: Set to false runs a longer running version of the benchmark with less debug output which is meant for final evaluation.

## Note
- Use `python3.9`, not `python3` to avoid potential version problems
- `python3.9 -m IPython` runs an interactve ipython shell
