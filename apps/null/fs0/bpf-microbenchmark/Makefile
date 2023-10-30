SRC = $(wildcard *.c)               # list of source files
OBJS = $(patsubst %.c, %.o, $(SRC)) # list of object files
OBJS_NEED_MODIFY = test_mod.obj

all: $(OBJS) $(OBJS_NEED_MODIFY)

%.o: %.c
	clang -c -target bpf -Wall $< -o $@
	llvm-objdump -d $@ > $@.asm

test_mod.obj: test_mod.o
	sed -e 's/\x37\x02/\x97\x02/' test_mod.o > test_mod.obj
	rm test_mod.o
	llvm-objdump -d $@ > $@.asm

clean:
	rm *.o || true 
	rm *.obj || true
	rm *.asm || true