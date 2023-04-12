{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";

    nixos-generators = {
      url = "github:nix-community/nixos-generators";
      inputs.nixpkgs.follows = "nixpkgs";
    };

    self-stable = {
      url = "git+ssh://git@github.com/mmisono/unikraft-development?ref=atc23&submodules=1";
      #url = "path:/scratch/okelmann/unikraft-development"; # use this for local development
      #url = "path:./"; # this wont include submodules. Nix bug?
      flake = false; # we dont want the flake but just the sources
    };
  };

  outputs = { self, nixpkgs, flake-utils, nixos-generators, self-stable }:
    (flake-utils.lib.eachSystem [ "x86_64-linux" ] (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
        pythonEnv = pkgs.python39.withPackages (ps: with ps; [
          pyflakes
          black # autoformat python code with `black misc/tests`
          pytest
          pandas
          psutil
          fire
          jedi
          tqdm # progress bars

          # for graphs.py
          natsort 
          matplotlib
          seaborn
        ]);
        buildDeps = [
          pkgs.yapf
          pkgs.mypy
          pkgs.ncurses
          pkgs.pkg-config
          pkgs.qemu
          pkgs.qemu_kvm
          pkgs.bear
          pkgs.gcc
          pkgs.libclang.python
          pkgs.clang-tools
          pkgs.redis
          pkgs.bison
          pkgs.flex
          pkgs.just
          pkgs.socat
          pkgs.curl
          pkgs.wget
          pkgs.bridge-utils
          pkgs.nettools
          pkgs.nixpkgs-fmt
          pkgs.glibc
          pkgs.sqlite
          pkgs.time # i dont think this is actually used, but `time` is checked for
          # pkgs.glibc.static
          pkgs.unzip # needed to make apps/nginx
          pkgs.cpio
          pkgs.libelf
        ];
      in
      {
        devShell = pkgs.gcc9Stdenv.mkDerivation {
          name = "devShell";
          buildInputs = buildDeps ++ [
            pythonEnv

            # needed for app/nginx benchmark
            pkgs.wrk 
            pkgs.nginx
            # gcc (is already in mkShell)
            pkgs.tokei
          ];
        };
        # app x shell x fs
        packages = builtins.listToAttrs ( pkgs.lib.flatten (
          pkgs.lib.forEach [ "nginx" "redis" "sqlite_benchmark" "sqlite3_backup" ] (app:
          pkgs.lib.forEach [ "noshell" "ushell" "ushellmpk" ] (shell:
          #pkgs.lib.forEach [ "initrd" "9p" ] (bootfs:
          pkgs.lib.forEach [ "initrd" ] (bootfs:
            pkgs.lib.nameValuePair "uk-${app}-${shell}-${bootfs}" (
              pkgs.callPackage ./misc/nix/uk-app.nix {
                inherit pkgs self-stable buildDeps;
                inherit app;
                config = "config.eval.${shell}.${bootfs}";
              }
            )
          )))
        )) //
        # (app x shell) with lto # TODO this should also include mpk
        builtins.listToAttrs ( pkgs.lib.flatten (
          pkgs.lib.forEach [ "nginx" "redis" "sqlite_benchmark" "sqlite3_backup" ] (app:
          pkgs.lib.forEach [ "noshell" "ushell" "ushellmpk" ] (shell:
          pkgs.lib.forEach [ "initrd" ] (bootfs:
            pkgs.lib.nameValuePair "uk-${app}-${shell}-${bootfs}-lto" (
              pkgs.callPackage ./misc/nix/uk-app.nix { 
                inherit pkgs self-stable buildDeps;
                inherit app;
                config = "config.eval.${shell}.${bootfs}.lto";
              }
            )
          )))
        )) //
        # (app x shell x memstat)
        builtins.listToAttrs ( pkgs.lib.flatten (
          pkgs.lib.forEach [ "count" ] (app:
          pkgs.lib.forEach [ "noshell" "ushell" "ushellmpk" ] (shell:
            pkgs.lib.nameValuePair "uk-${app}-${shell}-memstat" (
              pkgs.callPackage ./misc/nix/uk-app.nix {
                inherit pkgs self-stable buildDeps;
                inherit app;
                config = "config.eval.${shell}.memstat";
              }
            )
          ))
        )) //
        # app x shell x fs x memstat
        builtins.listToAttrs ( pkgs.lib.flatten (
          pkgs.lib.forEach [ "nginx" "sqlite3_backup" "sqlite_benchmark" ] (app:
          pkgs.lib.forEach [ "noshell" "ushell" "ushellmpk" ] (shell:
          pkgs.lib.forEach [ "initrd" ] (bootfs:
            pkgs.lib.nameValuePair "uk-${app}-${shell}-memstat-${bootfs}" (
              pkgs.callPackage ./misc/nix/uk-app.nix {
                inherit pkgs self-stable buildDeps;
                inherit app;
                config = "config.eval.${shell}.${bootfs}.memstat";
              }
            )
          )))
        )) //
        # some manual packages
        {
          uk-count-ushell = pkgs.callPackage ./misc/nix/uk-app.nix {
            inherit pkgs self-stable buildDeps;
            app = "count";
            config = "config.eval.ushell";
          };
          uk-count-ushellmpk = pkgs.callPackage ./misc/nix/uk-app.nix {
            inherit pkgs self-stable buildDeps;
            app = "count";
            config = "config.eval.ushellmpk";
          };
          uk-count-noshell = pkgs.callPackage ./misc/nix/uk-app.nix {
            inherit pkgs self-stable buildDeps;
            app = "count";
            config = "config.eval.noshell";
          };
          uk-count-ushell-lto = pkgs.callPackage ./misc/nix/uk-app.nix {
            inherit pkgs self-stable buildDeps;
            app = "count";
            config = "config.eval.ushell.lto";
          };
          uk-count-ushellmpk-lto = pkgs.callPackage ./misc/nix/uk-app.nix {
            inherit pkgs self-stable buildDeps;
            app = "count";
            config = "config.eval.ushellmpk.lto";
          };
          uk-count-noshell-lto = pkgs.callPackage ./misc/nix/uk-app.nix {
            inherit pkgs self-stable buildDeps;
            app = "count";
            config = "config.eval.noshell.lto";
          };
          nginx-image = pkgs.callPackage ./misc/nix/nginx-image.nix {
            inherit pkgs nixos-generators;
          };
        };
      }));
}
