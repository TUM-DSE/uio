sudo qemu-system-x86_64 \
    --append "arg1 arg2 arg3" \
    -kernel build/argtest_kvm-x86_64 -nographic
