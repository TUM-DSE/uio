## Memory statistics of unikraft

### libukalloc
- `LIBUKALLOC_IFSTATS_GLOBAL`
    - We can get stat by calling `uk_alloc_stats_get()` or `uk_alloc_stats_get_global()`
- `LIBUKALLOC_IFSTATS_PERLIB`
    - If this option is enabled, then `uk_alloc_stats_get()` returns stats for
      the callee library
