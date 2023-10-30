SRC = $(wildcard *.c)               # list of source files
OBJS = $(patsubst %.c, %.o, $(SRC)) # list of object files

all: $(OBJS)

%.o: %.c
	clang -c -target bpf -Wall -fno-stack-protector $< -o $@
	llvm-objdump -d $@ > $@.asm

clean:
	rm *.o || true 
	rm *.asm || true