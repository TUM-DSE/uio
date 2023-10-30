% bash run.sh
% redis-cli -h 172.44.0.2 -p 6379
172.44.0.2:6379> PING
PONG

### Entry point
- main.c in libs/redis creates a thread and then calls main() in https://github.com/redis/redis/blob/unstable/src/server.c

### BPF exmaple
- Programs are in [../bpf_progs](../bpf_progs)

- Tracing with BPF
```
> bpf_attach processCommand /ushell/bpf/count.o
Load /ushell/bpf/count.o
Program was attached.

# in the different terminal window
$ just benchmark

# in the ushell
> bpf_exec /ushell/bpf/get_count.o get_count processCommand
BPF program returned: 100002
```

