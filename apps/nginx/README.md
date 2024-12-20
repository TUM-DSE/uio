## Nginx
- Compile and run

```
make menuconfig
make
just setup_bridge
just run
```

- Check if nginx works
```
just wget
```

### use case example: interactive debug shell
- Check logs
```
just attach
> ls nginx/logs/
access.log
error.log
nginx.pid
> cat nginx/logs/error.log
```

### use case example: configuration reload
- Send SIGHUP through ushell
```
just attach
> kill 1
> cat nginx/logs/error.log
[...]
2023/02/04 05:59:04 [notice] 1#0: signal 1 (SIGHUP) received from 1, reconfiguring
```

### BPF exmaple
- Programs are in [../bpf_progs](../bpf_progs)
- Execute BPF programs
```
just attach
> load /ushell/symbol.txt
Load 5136 symbols

# exec noop program
> bpf_exec /ushell/bpf/noop.bin
BPF program returned: 0
```

- Safe memory read with BPF
```
> bpf_exec /ushell/bpf/read_symbol.bin connection_counter
BPF program returned: 32
```

- Tracing with BPF
```
> bpf_attach ngx_http_process_request_line /ushell/bpf/count.bin
Load /ushell/bpf/count.bin
Program was attached.

# in the different terminal window
$ wrk -t 3 -d10s -c 30 http://172.44.0.2/index.html

# in the ushell
> bpf_exec /ushell/bpf/get_count.bin ngx_http_process_request_line
BPF program returned: 1621476
```

- Check BPF robustness
```
# simple loop => rejected
> bpf_exec /ushell/bpf/loop.bin
Failed to load code: infinite loop at PC 0

# oob memory access => stop execution
> bpf_exec /ushell/bpf/oob2.bin
BPF program execution failed.

# loop => stop execution due to instruction limits
> bpf_exec /ushell/bpf/loop2.bin
BPF program execution failed.
```
