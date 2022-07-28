rm /tmp/port0
mkfifo /tmp/port0
sudo qemu-system-x86_64 \
    -enable-kvm \
    -chardev pipe,path=/tmp/port0,id=char0 \
    -device virtio-serial \
    -device virtconsole,chardev=char0,name=ushell \
    -kernel build/virtioconsole_kvm-x86_64 -nographic
