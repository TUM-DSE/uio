sudo qemu-system-x86_64 \
    -enable-kvm \
    -cpu host \
    -fsdev local,id=myid,path=$(pwd)/fs0,security_model=none \
    -device virtio-9p-pci,fsdev=myid,mount_tag=fs0,disable-modern=on,disable-legacy=off \
    -kernel build/test9p_kvm-x86_64 \
    -nographic
