{pkgs, nixos-generators, ...}:
nixos-generators.nixosGenerate {
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
}
