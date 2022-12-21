# ushell

## develop:

(Tested on adelaide)

```shell
git clone --recurse-submodules https://github.com/mmisono/unikraft-development
cd unikraft-development
direnv allow # (or nix develop)
```

(in one terminal window)
```shell
cd apps/count
make menuconfig
make
just run
```

> Since .config contains the absolute path, we need to regenerate .config by running make menuconfig 

(in another terminal window)
```shell
cd apps/count
just attach
```

## measurements

```bash
# enter the development shell
nix develop

# set up a bridge for qemu networking
pushd apps/nginx
just setup_bridge
popd

# you can choose to build all the unikraft kernels in advance
python3.9 ./misc/test/nix.py

# run the actual tests

rm -r ./misc/tests/measurements/console-stats.json
sudo python3.9 ./misc/tests/measure_console.py # depending on your system configuration, qemu has to be started as root. In that case you have to start python as root. 
python3.9 ./misc/tests/graph.py misc/tests/measurements/console-latest.tsv
ls ./console.pdf

rm -r ./misc/tests/measurements/app-stats.json
sudo python3.9 ./misc/tests/measure_apps.py
python3.9 ./misc/tests/graph.py misc/tests/measurements/app-latest.tsv
ls ./redis.pdf ./sqlite.pdf ./nginx.pdf

rm -r ./misc/tests/measurements/image-stats.json
sudo python3.9 ./misc/tests/measure_image.py
python3.9 ./misc/tests/graph.py misc/tests/measurements/image-latest.tsv
ls ./images.pdf
```

### developing measurements

By default we use a pinned version of the measured unikraft kernels for reproducability. 
Measure other kernel configs/versions by updating the `self-stable.url` in `flake.nix` to point i.e. to your local git checkout. Every time the unikraft sources change, run `nix flake lock --update-input self-stable` to tell the nix builder about the new sources.

Measure_${title}.py files have a quick flag: Set to false runs a longer running version of the benchmark with less debug output which is meant for final evaluation.

Implementation Structure:

- nix.py: here we build our kernels
- qemu.py: here we start qemu in a tmux session
- measure_${title}.py: here we run all benchmarks that are not present yet in the .json file. On completion, it is written into the .tsv file.
- graph.py: takes a tsv file and creates plots from it depending on its filename
