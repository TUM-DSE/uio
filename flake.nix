{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    (flake-utils.lib.eachSystem [ "x86_64-linux" ] (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
        pythonEnv = pkgs.python39.withPackages (ps: with ps; [
          pyflakes
          fire
          jedi
        ]);
      in
      {
        devShell = pkgs.mkShell {
          buildInputs = [
            pythonEnv
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
            # pkgs.glibc.static
            pkgs.unzip # needed to make apps/nginx
          ];
        };
      }));
}
