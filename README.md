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
nix develop 
# delete results from previous runs
rm -r ./misc/tests/measurements 
# or to redo specific ones
rm -r ./misc/tests/measurements/*.json
sudo python3.9 ./misc/tests/measure_console.py
sudo python3.9 ./misc/tests/measure_apps.py
```

Measure other code other from what is locked by flake.lock:
Make `self-stable.url` in `flake.nix` point i.e. to your local git checkout and run `nix flake lock --update-input self-stable` every time the sources change.
