### Executing command
- Prepare
```
just compile_hello
just gen_sym_txt
```

- Run
```
just attach
> load symbol.txt
> run hello
```

- Load program without execution
```
> prog-load hello
```

- Programs are cached. If you want to free them, then
```
> free hello
> free-all  // free all programs
```

### Program examples
- set_count
    - change the counter value
```
> run set_count 100
```

- perf
    - print llc cache misses using performance counters
```
> run perf
```

