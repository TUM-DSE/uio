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

- Programs are cached. If you want to free them, then
```
> free hello
> free-all  // free all programs
```
