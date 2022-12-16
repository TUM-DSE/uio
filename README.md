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

TODO: describe how to set up networking

```bash
# enter the development shell
nix develop

# you can choose to build all the things in advance
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
```

Measure other code other from what is locked by flake.lock:
Make `self-stable.url` in `flake.nix` point i.e. to your local git checkout and run `nix flake lock --update-input self-stable` every time the sources change.
