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

- Tracing with BPF
```
> bpf_attach ngx_http_process_request_line /ushell/bpf/count.o
Load /ushell/bpf/count.o
Program was attached.

# in the different terminal window
$ wrk -t 3 -d10s -c 30 http://172.44.0.2/index.html

# in the ushell
> bpf_exec /ushell/bpf/get_count.o get_count ngx_http_process_request_line
BPF program returned: 1621476
```