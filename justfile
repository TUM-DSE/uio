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

# this creates a symlink `result` to the build result
build_qcow2:
    #!/bin/bash
    mkdir -p ./tmp
    if [ ! -f ./tmp/nixos.qcow2 ]; then
        nix build .#nginx-image
        cp ./result/nixos.qcow2 ./tmp/nixos.qcow2
        chmod 644 ./tmp/nixos.qcow2
        qemu-img create -f raw ./tmp/disk.img 10G
    fi

# this depends on the result of build_qcow2
# for fs experiment:
# in the vm
# ```
# mkfs.ext4 /dev/vda
# mkdir -p /disk
# mount /dev/vda /mount
# cd /mount
# cp /mnt/fs_measure_static . // copy from the file from 9pfs
# mkdir fs_linux
# ./fs_measure_static
# ```
run_vm:
    sudo taskset -c 0,1 \
    qemu-system-x86_64 \
        -enable-kvm \
        -cpu host \
        -m 1024 \
        -drive file=/scratch/masa/ushell/tmp/nixos.qcow2 \
        -nographic \
        -netdev bridge,id=en0,br=virbr0 \
        -device virtio-net-pci,netdev=en0 \
        -fsdev local,id=myid,path=/scratch/masa/ushell/apps/unikraft_9p_measure/fs_linux,security_model=none \
        -device virtio-9p-pci,fsdev=myid,mount_tag=fs0,disable-modern=on,disable-legacy=off \
        -drive file=/scratch/masa/ushell/tmp/disk.img,if=none,id=drive-virtio-disk0,format=raw \
        -device virtio-blk-pci,scsi=off,drive=drive-virtio-disk0,id=virtio-disk0

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
