## Note about calling BPF helper functions in C
We can emit call instructions for calling BPF helper function like this.

```c
#include <asm/types.h>
#define bpf_map_get ((__u64(*)(__u64 key1, __u64 key2))0)
int bpf_prog (void *arg)
{
	__u64 value = bpf_map_get(0, 0);
	return 0;
}
```

However, clang generates correct bytecode only when we disable optimization (`-O0`).

```
$ clang -O0 -Wall -c -target bpf -o a.o a.c
$ llvm-objdump -D a.o
[...]

0000000000000000 <bpf_prog>:
       0:       b7 01 00 00 00 00 00 00 r1 = 0
       1:       b7 02 00 00 00 00 00 00 r2 = 0
       2:       85 00 00 00 01 00 00 00 call 1
       3:       95 00 00 00 00 00 00 00 exit
```

If we compile with optimization, the entire .text section is removed.

```
$ clang -O1 -Wall -c -target bpf -o a.o a.c
$ llvm-objdump -D a.o
// no .text section
```

Apparently, casting 0 into a function pointer is undefined behavior, and thus .text is removed. Casting other values into function pointers works fine.
Probably we should use other ways to call BPF helper functions such as inline assembly.
Currently, we just do not use helper function zero to avoid this problem.

## Debug
- Compile BPF into ELF
```
clang -Wall -c -target bpf -o a.o a.c
```

- Check BPF ELF object
```
llvm-objdump -D a.o
```

- Emit LLVM IR
```
clang -Wall -c -target bpf -emit-llvm -S a.c
cat a.ll
```

- Extract BPF bytecode from ELF
```
objcopy -I elf64-little -O binary a.o a.bin
```

- Disassemble BPF bytecode
```
ubpf/bin/ubpf-disassembler a.bin
```
