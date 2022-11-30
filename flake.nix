{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";

    nixos-generators = {
      url = "github:nix-community/nixos-generators";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs = { self, nixpkgs, flake-utils, nixos-generators }:
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
            pkgs.cpio

            # needed for app/nginx benchmark
            pkgs.wrk 
            pkgs.nginx

          ];
        };
        packages = {
          nginx-image = nixos-generators.nixosGenerate {
            inherit pkgs;
            modules = [ 
              ({
                system.stateVersion = "22.11";
                networking.firewall.enable = false;
                users.users.root.password = "password";
                services.getty.autologinUser = pkgs.lib.mkDefault "root";

                services.nginx = {
                  enable = true;
                };

                networking.useDHCP = false;
                networking.interfaces.ens4.useDHCP = false;
                networking.interfaces.ens4.ipv4.addresses = [ {
                  address = "172.44.0.2";
                  prefixLength = 24;
                } ];
                networking.defaultGateway = "172.44.0.1";
              })
            ];
            format = "qcow";
          };
        };
      }));
}
