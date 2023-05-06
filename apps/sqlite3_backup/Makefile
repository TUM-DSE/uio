UK_ROOT ?= $(PWD)/../../unikraft
UK_LIBS ?= $(PWD)/../../libs
LIBS := $(UK_LIBS)/pthread-embedded:$(UK_LIBS)/newlib:$(UK_LIBS)/sqlite
LIBS := $(LIBS):$(UK_LIBS)/ubpf:$(UK_LIBS)/ubpf_tracer
KVM_BINARY  := build/sqlite3_backup_kvm-x86_64
SYMFILE     := $(KVM_BINARY).sym

all:
	@$(MAKE) -C $(UK_ROOT) A=$(PWD) L=$(LIBS)
	if [[ -f $(SYMFILE) ]]; then cp $(SYMFILE) fs1/debug.sym; fi

$(MAKECMDGOALS):
	@$(MAKE) -C $(UK_ROOT) A=$(PWD) L=$(LIBS) $(MAKECMDGOALS)
