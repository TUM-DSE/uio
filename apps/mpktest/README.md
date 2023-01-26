## mpktest
- Compile and run

```
make menuconfig
make
just run
```

### MPK test
- These commands should succeed
```
just attach
> load symbol.txt
> test_var_read
> test_var_write_wrapper
> test_call_wrapper
> run write_with_wrapper
> run call_with_wapper
```

- These commands should fail
```
just attach
> load symbol.txt
> test_var_write
> test_call
> run write
> run call
```

