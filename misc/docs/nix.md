## Bulding Unikraft apps with Nix
- [./misc/nix/uk-app.nix](../nix/uk-app.nix) defines main build rules
    - We need to manually download libraries (see `postUnpack`) as nix build system runs in a sandbox and we need to download all necessary libraries before the build phase
- Define rules for bulding app in [flake.nix](../../flake.nix)
    - Excerpt from the flake.nix
    - This build [`./apps/count`](../../apps/count) with the [`./apps/count/config.eval.ushell`](../../apps/count/config.eval.ushell) config
```
packages = {
    [...]
    uk-count-ushell = pkgs.callPackage ./misc/nix/uk-app.nix {
      inherit pkgs self-stable buildDeps;
      app = "count";
      config = "config.eval.ushell";
    };
    [...]
}
```

## Examples
- Upadte the flake lock
    - `nix flake lock --update-input self-stable`
    - Nix uses this locked version to build the app
    - We need to run this command every time we update the source
    - Also see [./misc/tests/README.md](../tests/README.md) for reproducible builds and tests
- Build a Unikraft app
    - `nix build .#uk-count-ushell`
    - If build fails, try to run command `--keep-failed` and check the logs
- Build all
    - `python3.9 ./misc/tests/nix.py`

