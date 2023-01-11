## SQLite
- Compile and run

```
make menuconfig
just run
> CREATE TABLE foo(bar INT);
> INSERT INTO foo (bar) VALUES (1);
```

### use case example: SQLite backup
```
just attach
> load ushell/symbol.txt
> run ushell/call_backup
<CTRL-C> # quit ushell

# check contains
sqlite3 fs0/dump
> select * from foo;
```

