## count
- Compile and run

```
make menuconfig
make
just run
```

### uShell execution

- Prepare
``` just compile_progs
just gen_sym_txt
```

- Attach
```
just attach
> load symbol.txt
```

- List directories
```
> ls
```

- Run program (program is loaded if necessary)
```
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
    - change the counter value by directly writing a global variable
```
> run set_count 100
```

- set_count_func
    - change the counter value by calling function
```
> run set_count_func 100
```
- perf
    - print llc cache misses using performance counters
```
> run perf
```

