borrowed from https://github.com/cffs/app-sqlite-test

# SQLite Performance Experiment

This repository is used for a performance experiment of SQLite running on
[Unikraft](https://github.com/unikraft/unikraft).

This experiment compares four different configuration, each on its own branch
of this repository:

- Branch `newlib-native` uses the native port of SQLite to Unikraft, with the
  [newlib][] libc and [pthread-embedded][].
- Branch `musl-native` uses the native port of SQLite to Unikraft, with
  [musl][] libc.
- Branch `musl-compat` uses a static library of SQLite built with `musl-gcc`
  on a standard GNU\Linux box (we recommend Debian), which is then linked with
  Unikraft, also using [musl][].
- Branch `linux-native` does not use Unikraft at all. It just runs the same
  benchmark running SQLite built with `musl-gcc`, as a standard Linux process
  (no virtualisation).

All Unikraft experiments use the [TLSF][] memory allocator, while the Linux one
uses the default Linux allocator.

[newlib]: https://github.com/unikraft/lib-newlib
[musl]: https://github.com/unikraft/lib-musl
[pthread-embedded]: https://github.com/unikraft/lib-pthread-embedded
[TLSF]: https://github.com/unikraft/lib-tlsf

To use this application, you will need Unikraft and the relevant libraries. A
script https://github.com/unikraft/eurosys21-artifacts can do all the setup for
you, including cloning the different branches of this repository.

To reproduce this experiment by hand, you will also need original SQLite
include files in an `include` folder, and do a `make menuconfig` before
building with `make`.

For the Linux test, if `/tmp` is on tmpfs, i.e. a RAM disk, it will be used.
Otherwise the experiment will create a RAM disk. It is important to use a RAM
disk to be fair as the Unikraft experiments are using the RAM disk approach,
and the benchmark is I/O intensive. Note that on some Linux boxes, using `/tmp`
on tmpfs is actually faster than using a dedicated RAM disk mounted using tmpfs
as well (sync operations return immediately with `/tmp` while provoking context
switches with other RAM disks).
