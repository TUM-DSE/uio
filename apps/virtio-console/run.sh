sudo qemu-system-x86_64 \
    -enable-kvm \
    -chardev socket,path=/tmp/port0,server=on,wait=off,id=char0 \
    -device virtio-serial \
    -device virtconsole,chardev=char0,id=ushell,nr=0 \
    -kernel build/virtioconsole_kvm-x86_64 -nographic
