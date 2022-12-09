{pkgs, nixos-generators, ...}:
nixos-generators.nixosGenerate {
  inherit pkgs;
  modules = [ 
    ({
      system.stateVersion = "22.11";
      networking.firewall.enable = false;
      users.users.root.password = "password";
      services.getty.autologinUser = pkgs.lib.mkDefault "root";

      fileSystems."/mnt" = {
        device = "fs0";
        fsType = "9p";
        options = [ "trans=virtio" "nofail" "msize=104857600" ];
      };

      services.nginx = {
        enable = true;
        virtualHosts."localhost" = {
          default = true;
          root = "/mnt/nginx/html";
        };
        appendHttpConfig = ''
          #caching
          open_file_cache max=200000 inactive=20s;
          open_file_cache_valid 30s;
          open_file_cache_min_uses 2;
          open_file_cache_errors on;
       
          access_log off;
       
          keepalive_requests 1000000000;
        '';
        eventsConfig = ''
          worker_connections  40;
        '';
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
