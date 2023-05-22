# vim: set ft=make et :
#
# This justfile is running application build by nix
# e.g.,
# ```
# # build apps
# $ nix build .#uk-nginx-ushellmpk-bpf-initrd
# # ./result is a symlink to the build result
# $ just run_nginx
# ```

USHELL_SOCK := "/tmp/ushell.sock"

default:
    @just --choose

run_nginx:
    sudo qemu-system-x86_64 \
        -enable-kvm \
        -cpu host \
        -m 1024 \
        -kernel ./result/nginx_kvm-x86_64 \
        -nographic \
        -append 'netdev.ipv4_addr=172.44.0.2 netdev.ipv4_gw_addr=172.44.0.1 netdev.ipv4_subnet_mask=255.255.255.0 -- -c /nginx/conf/nginx.conf' \
        -netdev bridge,id=en0,br=virbr0 \
        -device virtio-net-pci,netdev=en0 \
        -chardev socket,path={{USHELL_SOCK}},server=on,wait=off,id=char0 \
        -device virtio-serial \
        -device virtconsole,chardev=char0,id=ushell,nr=0 \
        -initrd ./result/fs0.cpio \
        -fsdev local,id=myid2,path=./result/fs1,security_model=none \
        -device virtio-9p-pci,fsdev=myid2,mount_tag=fs1,disable-modern=on,disable-legacy=off

setup_bridge:
    #!/usr/bin/env bash
    ip a s virbr0 >/dev/null 2>&1
    if [ $? ]; then
        sudo brctl addbr virbr0
        sudo ip a a 172.44.0.1/24 dev virbr0
        sudo ip l set dev virbr0 up
    fi

DURATION_DEFAULT := '10s'

wrk duration=DURATION_DEFAULT:
    wrk -t 14 -d{{duration}} -c 30 http://172.44.0.2/index.html

attach:
    sudo socat {{USHELL_SOCK}} -
