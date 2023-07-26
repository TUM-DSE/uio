Compile:
clang -c -O3 -target bpf  helper_test.c -o helper_test.o

Dump:
llvm-objdump -d helper_test.o | sed -n '/^0/,$p'

Run:
bpf_exec ../platform/tests/helper_test.o
bpf_exec ../platform/tests/helper_test2.o