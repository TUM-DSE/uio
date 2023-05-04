UK_ROOT ?= $(PWD)/../../unikraft
UK_LIBS ?= $(PWD)/../../libs
LIBS := $(UK_LIBS)/newlib
LIBS := $(LIBS):$(UK_LIBS)/ubpf:$(UK_LIBS)/ubpf_tracer
KVM_BINARY  := build/count_kvm-x86_64
SYMFILE     := $(KVM_BINARY).sym

all:
	@$(MAKE) -C $(UK_ROOT) A=$(PWD) L=$(LIBS)
	@cp $(SYMFILE) fs0/debug.sym

$(MAKECMDGOALS):
	@$(MAKE) -C $(UK_ROOT) A=$(PWD) L=$(LIBS) $(MAKECMDGOALS)
