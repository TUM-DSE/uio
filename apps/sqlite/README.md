### File system settings
- 9pfs: vfscore: Configuration > 9pfs
- initrd: vfscore: Configuration > InitRD

### Run
```
% bash run_initrd.sh
sqlite> .read script.sql
sqlite> select * from .tab;
```

### Entry point
- shell.c in sqlite
