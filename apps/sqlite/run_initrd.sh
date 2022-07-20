sudo qemu-system-x86_64 \
    -enable-kvm \
    -cpu host \
    -m 500M \
    -initrd archive.cpio \
    -kernel build/sqlite_kvm-x86_64 \
    -nographic
