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

