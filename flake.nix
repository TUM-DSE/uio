{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";

    nixos-generators = {
      url = "github:nix-community/nixos-generators";
      inputs.nixpkgs.follows = "nixpkgs";
    };

    self-stable = {
      #url = ":mmisono/unikraft-development?submodules=1";
      url = "git+https://github.com/mmisono/unikraft-development?ref=main&submodules=1";
      #url = "path:/scratch/okelmann/unikraft-development"; # for local development
      flake = false;
    };
  };

  outputs = { self, nixpkgs, flake-utils, nixos-generators, self-stable }:
    (flake-utils.lib.eachSystem [ "x86_64-linux" ] (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
        pythonEnv = pkgs.python39.withPackages (ps: with ps; [
          pyflakes
          fire
          jedi
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
        ];
      in
      {
        devShell = pkgs.mkShell {
          buildInputs = buildDeps ++ [
            pythonEnv

            # needed for app/nginx benchmark
            pkgs.wrk 
            pkgs.nginx
          ];
        };
        packages = {
          uk-nginx = pkgs.callPackage ./misc/nix/uk-nginx.nix { 
            inherit pkgs self-stable buildDeps;
          };
          nginx-image = pkgs.callPackage ./misc/nix/nginx-image.nix { 
            inherit pkgs nixos-generators; 
          };
        };
      }));
}
