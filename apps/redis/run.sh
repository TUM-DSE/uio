sudo qemu-system-x86_64 \
    -enable-kvm \
    -cpu host \
    -m 500M \
    -fsdev local,id=myid,path=$(pwd)/fs0,security_model=none \
    -device virtio-9p-pci,fsdev=myid,mount_tag=fs0,disable-modern=on,disable-legacy=off \
    -netdev bridge,id=en0,br=virbr0 \
    -device virtio-net-pci,netdev=en0 \
    -append "netdev.ipv4_addr=172.44.0.2 netdev.ipv4_gw_addr=172.44.0.1 netdev.ipv4_subnet_mask=255.255.255.0 -- /redis.conf" \
    -kernel build/redis_kvm-x86_64 \
    -nographic

# NOTE: /redis.conf is given to the program as an augment
