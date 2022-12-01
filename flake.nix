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
          uk-nginx = pkgs.stdenv.mkDerivation {
            pname = "uk-nginx";
            version = "0.9.0-ushell";
            nativeBuildInputs = with pkgs; buildDeps ++ [
              which
            ];
            src = self-stable;
            postUnpack = let 
              wgets = with wgets; {
                libpthread-embedded = with libpthread-embedded; {
                  version = "44b41d760a433915d70a7be9809651b0a65e001d";
                  src = builtins.fetchurl {
                    url = "https://github.com/RWTH-OS/pthread-embedded/archive/${version}.zip";
                    sha256 = "1a6rbdndxik9wgcbv6lb5gw692pkvl5gf8vdhsa4b5q0kn2z26mp";
                  };
                  dst = "apps/nginx/build/libpthread-embedded/${version}.zip";
                };

                libnewlibc = with libnewlibc; {
                  version = "newlib-2.5.0.20170922";
                  src = builtins.fetchurl {
                    url = "http://sourceware.org/pub/newlib/${version}.tar.gz";
                    sha256 = "0xbj5vhkmn54yv7gzprqb9wk9lsj57ypn1fs6f1rmf2mj6xsrk0n";
                  };
                  dst = "apps/nginx/build/libnewlibc/${version}.tar.gz";
                };

                libnginx = with libnginx; {
                  version = "nginx-1.15.6";
                  src = builtins.fetchurl {
                    url = "http://nginx.org/download/${version}.tar.gz";
                    sha256 = "1ikchbnq1dv8wjnsf6jj24xkb36vcgigyps71my8r01m41ycdn53";
                  };
                  dst = "apps/nginx/build/libnginx/${version}.tar.gz";
                };

                liblwip = with liblwip; {
                  version = "UNIKRAFT-2_1_x";
                  src = builtins.fetchurl {
                    url = "https://github.com/unikraft/fork-lwip/archive/refs/heads/${version}.zip";
                    sha256 = "1bq38ikhnlqfyb24s7mixr68qnsm4dlvr96g4y9z8ih9lz45mw8w";
                  };
                  dst = "apps/nginx/build/liblwip/${version}.zip";
                };
              };
            in ''
              # srcsUnpack src_absolute destination_relative
              function srcsUnpack () {
                mkdir -p $(dirname $sourceRoot/$2)
                cp $1 $sourceRoot/$2
              }
            '' + pkgs.lib.strings.concatStrings
              (pkgs.lib.lists.forEach 
                (pkgs.lib.attrsets.attrValues wgets)
                (resource: "srcsUnpack ${resource.src} ${resource.dst}\n")
              );
            postPatch = ''
              patchShebangs /build/source/unikraft/support/scripts/uk_build_configure.sh
              patchShebangs /build/source/unikraft/support/scripts/configupdate
              patchShebangs /build/source/unikraft/support/scripts/sect-strip.py
            '';
            configurePhase = ''
              cd apps/nginx
              make oldconfig # ensure consistency and update absolute paths
            '';
            buildPhase = ''
              make -j
            '';
            installPhase = ''
              mkdir -p $out
              cp -r build/* $out
            '';

            # doesnt seem to break anything, but i'm pretty sure we cant profit from it
            dontFixup = true; 
          };
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
