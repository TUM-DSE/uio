{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    (flake-utils.lib.eachSystem [ "x86_64-linux" ] (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};

        fcache = pkgs.python39.pkgs.buildPythonPackage rec {
          pname = "fcache";
          version = "0.4.7";

          src = pkgs.python39.pkgs.fetchPypi {
            inherit pname version;
            sha256 = "sha256-p9FKcum12/IyGEpwyvn/OYIFg7ekK8ZEJM19lIfGPYY=";
          };

          preCheck = ''
            export HOME=$TEMPDIR;
          '';

          propagatedBuildInputs = [
            pkgs.python39.pkgs.appdirs
          ];
        };

        atpbar = pkgs.python39.pkgs.buildPythonPackage rec {
          pname = "atpbar";
          version = "1.0.0";

          src = pkgs.python39.pkgs.fetchPypi {
            inherit pname version;
            sha256 = "sha256-1vx5fN25YIjQAcxGYEULCDWHoaH265n58Ckj0tCYfaM=";
          };

          preCheck = ''
            export HOME=$TEMPDIR;
          '';

          checkInputs = [
            pkgs.python39.pkgs.ipywidgets
          ];
        };

        memorizePy = pkgs.python39.pkgs.buildPythonPackage rec {
          pname = "memorize.py";
          version = "1.2";

          src = pkgs.python39.pkgs.fetchPypi {
            inherit pname version;
            sha256 = "sha256-Dj7JtKKwlVJU5IDo+o+7an1NiPS2eKL3CX+WvERG0D4=";
          };

          preCheck = ''
            export HOME=$TEMPDIR;
          '';
        };

        htmllistparse = pkgs.python39.pkgs.buildPythonPackage rec {
          pname = "htmllistparse";
          version = "0.6.0";

          src = pkgs.python39.pkgs.fetchPypi {
            inherit pname version;
            sha256 = "sha256-msJ0oWWgK/UDZW30tI4a6e7A30ByOl+Zj5Aj0988jjI=";
          };

          preCheck = ''
            export HOME=$TEMPDIR;
          '';

          propagatedBuildInputs = [
            pkgs.python39.pkgs.fusepy
            pkgs.python39.pkgs.beautifulsoup4
            pkgs.python39.pkgs.requests
          ];
        };

        kraft = pkgs.python39.pkgs.buildPythonPackage rec {
          pname = "kraft";
          version = "0.1.0";

          src = pkgs.fetchFromGitHub {
            owner = "unikraft";
            repo = "kraft";
            rev = "6d046f4dd5ecd2075f927ee01df0fa4e842ca60e";
            sha256 = "sha256-FIlxhhy1q6R1K0Tl+oxlRUiPzmFl0r9UYCrfMFcoUdA=";
          };

          buildInputs = [
            pkgs.python39.pkgs.setuptools-scm
          ];

          propagatedBuildInputs = [
            pkgs.python39.pkgs.PyGithub
            pkgs.python39.pkgs.dpath
            pkgs.python39.pkgs.jsonschema_3
            pkgs.python39.pkgs.pyyaml
            pkgs.python39.pkgs.validators
            pkgs.python39.pkgs.cached-property
            pkgs.python39.pkgs.semver
            pkgs.python39.pkgs.colorama
            pkgs.python39.pkgs.tqdm
            pkgs.python39.pkgs.click
            pkgs.python39.pkgs.click-log
            pkgs.python39.pkgs.cookiecutter
            pkgs.python39.pkgs.feedparser
            pkgs.python39.pkgs.inquirer
            pkgs.python39.pkgs.python-dotenv
            pkgs.python39.pkgs.kconfiglib
            pkgs.python39.pkgs.ruamel-yaml
            pkgs.python39.pkgs.GitPython
            fcache
            atpbar
            memorizePy
            htmllistparse
          ];

          postPatch = ''
            sed -i "s/PyYAML >= 5.3.1, < 6/PyYAML/" requirements.txt
          '';

          SETUPTOOLS_SCM_PRETEND_VERSION = version;

          doCheck = false;

          meta = {
            homepage = "https://github.com/unikraft/kraft";
            description = "Define, configure, build, and run Unikraft unikernel applications.";
          };
        };

        pythonEnv = pkgs.python39.withPackages (ps: with ps; [
          kraft
        ]);
      in
      {
        devShell = pkgs.mkShell {
          buildInputs = [
            pythonEnv
          ];
        };
      }));
}
