UK_ROOT ?= $(PWD)/../../unikraft
UK_LIBS ?= $(PWD)/../../libs
LIBS := $(UK_LIBS)/newlib:$(UK_LIBS)/ubpf:$(UK_LIBS)/ubpf_tracer
KVM_BINARY  := build/ubpf_tracer_kvm-x86_64
SYMFILE     := $(KVM_BINARY).sym
BPF_SRC     := $(shell find bpf_prog/ -maxdepth 1 -type f -regex ".*\.c")
BPF_OBJ     := $(patsubst %.c,build/%.o,$(BPF_SRC))
BPF_BIN     := $(patsubst %.c,build/%.bin,$(BPF_SRC))
BPF_FS0_BIN := $(patsubst bpf_prog/%.c,fs0/%.bin,$(BPF_SRC))
SOCKET_PATH := /tmp/port0


all: $(BPF_FS0_BIN)
	$(MAKE) -C $(UK_ROOT) A=$(PWD) L=$(LIBS)
	@cp $(SYMFILE) fs0/debug.sym

$(MAKECMDGOALS):
	$(MAKE) -C $(UK_ROOT) A=$(PWD) L=$(LIBS) $(MAKECMDGOALS)

build/bpf_prog:
	@mkdir -p $@

build/bpf_prog/%.o: bpf_prog/%.c build/bpf_prog
	@clang -c -target bpf -o $@ $<

build/bpf_prog/%.bin: build/bpf_prog/%.o
	@objcopy -I elf64-little -O binary $< $@

fs0/%.bin: build/bpf_prog/%.bin
	@cp $< $@

run: all
	@sudo qemu-system-x86_64 \
	  -cpu host \
	  -enable-kvm \
	  -m 500M \
	  -chardev socket,path=$(SOCKET_PATH),server=on,wait=off,id=char0 \
	  -device virtio-serial \
	  -device virtconsole,chardev=char0,id=ushell,nr=0 \
	  -fsdev local,id=myid,path="$(PWD)/fs0",security_model=none \
	  -device virtio-9p-pci,fsdev=myid,mount_tag=fs0,disable-modern=on,disable-legacy=off \
	  -kernel $(KVM_BINARY) \
	  -nographic

attach:
	@sudo socat $(SOCKET_PATH) -

.PHONY: all run
