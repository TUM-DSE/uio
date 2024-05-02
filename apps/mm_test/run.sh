sudo qemu-system-x86_64 \
    -cpu host \
    -smp 1 \
    -m 1G \
    -enable-kvm \
    -kernel build/mm_test_kvm-x86_64 -nographic
