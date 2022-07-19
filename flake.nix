{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    (flake-utils.lib.eachSystem [ "x86_64-linux" ] (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
      in
      {
        devShell = pkgs.mkShell {
          buildInputs = [
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
            pkgs.curl
            pkgs.wget
            pkgs.bridge-utils
            pkgs.nettools
            pkgs.nixpkgs-fmt
          ];
        };
      }));
}
