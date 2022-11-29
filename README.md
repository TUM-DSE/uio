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

