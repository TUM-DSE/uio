UK_ROOT ?= $(PWD)/../../unikraft
UK_LIBS ?= $(PWD)/../../libs
LIBS := $(UK_LIBS)/newlib:$(UK_LIBS)/ubpf:$(UK_LIBS)/ubpf_tracer
SYMFILE = build/ubpf_tracer_kvm-x86_64.sym

all:
	$(MAKE) -C $(UK_ROOT) A=$(PWD) L=$(LIBS)
	cp $(SYMFILE) fs0/debug.sym

$(MAKECMDGOALS):
	$(MAKE) -C $(UK_ROOT) A=$(PWD) L=$(LIBS) $(MAKECMDGOALS)
