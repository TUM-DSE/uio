UK_ROOT ?= $(PWD)/../../unikraft
UK_LIBS ?= $(PWD)/../../libs
LIBS := $(UK_LIBS)/newlib
LIBS := $(LIBS):$(UK_LIBS)/ubpf:$(UK_LIBS)/ubpf_tracer
KVM_BINARY  := build/mpktest_kvm-x86_64
DBGFILE     := $(KVM_BINARY).dbg
USHELLDIR   := fs0

all:
	@$(MAKE) -C $(UK_ROOT) A=$(PWD) L=$(LIBS)
	if [[ -f $(DBGFILE) ]]; then nm $(DBGFILE) | cut -d ' ' -f1,3 > ./$(USHELLDIR)/symbol.txt; fi

$(MAKECMDGOALS):
	@$(MAKE) -C $(UK_ROOT) A=$(PWD) L=$(LIBS) $(MAKECMDGOALS)
