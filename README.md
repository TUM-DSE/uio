# uio

## Quick start

(Tested on [adelaide](https://github.com/TUM-DSE/doctor-cluster-config/blob/master/docs/hosts/adelaide.md))

```shell
git clone --recurse-submodules https://github.com/TUM-DSE/ushell
cd unikraft-development
direnv allow # (or nix develop)
```

(in one terminal window)
```shell
cd apps/count
cp config.eval.ushellmpk.bpf .config
make olddefconfig
make
just run
```

> Since .config contains the absolute path, we need to regenerate .config by running make olddefconfig

(in another terminal window)
```shell
cd apps/count
just attach
```

## Measurements / Evaluation

see [./misc/tests/README.md](./misc/tests/README.md)

## Docs
- [./misc/docs](./misc/docs)


