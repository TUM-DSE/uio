{pkgs, self-stable, buildDeps, app, config ? ".config", ...}:
pkgs.stdenv.mkDerivation {
  pname = "uk-${app}";
  version = "0.9.0-${config}";
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
        dst = "apps/${app}/build/libpthread-embedded/${version}.zip";
      };

      libnewlibc = with libnewlibc; {
        version = "newlib-2.5.0.20170922";
        src = builtins.fetchurl {
          url = "http://sourceware.org/pub/newlib/${version}.tar.gz";
          sha256 = "0xbj5vhkmn54yv7gzprqb9wk9lsj57ypn1fs6f1rmf2mj6xsrk0n";
        };
        dst = "apps/${app}/build/libnewlibc/${version}.tar.gz";
      };

      liblwip = with liblwip; {
        version = "UNIKRAFT-2_1_x";
        src = builtins.fetchurl {
          url = "https://github.com/unikraft/fork-lwip/archive/refs/heads/${version}.zip";
          sha256 = "1bq38ikhnlqfyb24s7mixr68qnsm4dlvr96g4y9z8ih9lz45mw8w";
        };
        dst = "apps/${app}/build/liblwip/${version}.zip";
      };

      libnginx = with libnginx; {
        version = "nginx-1.15.6";
        src = builtins.fetchurl {
          url = "http://nginx.org/download/${version}.tar.gz";
          sha256 = "1ikchbnq1dv8wjnsf6jj24xkb36vcgigyps71my8r01m41ycdn53";
        };
        dst = "apps/${app}/build/libnginx/${version}.tar.gz";
      };

      libredis = with libredis; {
        version = "5.0.6";
        src = builtins.fetchurl {
          url = "https://github.com/antirez/redis/archive/${version}.zip";
          sha256 = "1njdkpzzhg28cavq5n7rv4p6my0x7g3d3lg9cnc66s42rk8xd9z5";
        };
        dst = "apps/${app}/build/libredis/${version}.zip";
      };
    };
  in ''
    # clean sourcetree, in case of impure development tree
    pushd $sourceRoot/apps/${app}
    make clean
    rm -r build
    popd

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
    cd apps/${app}
    [[ "${config}" == ".config" ]] || cp ${config} .config
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
}
