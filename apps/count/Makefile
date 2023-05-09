UK_ROOT ?= $(PWD)/../../unikraft
UK_LIBS ?= $(PWD)/../../libs
LIBS := $(UK_LIBS)/newlib
LIBS := $(LIBS):$(UK_LIBS)/ubpf:$(UK_LIBS)/ubpf_tracer
KVM_BINARY  := build/count_kvm-x86_64
SYMFILE     := $(KVM_BINARY).sym
DBGFILE     := $(KVM_BINARY).dbg
USHELLDIR   := fs0
PROG_SRC    := $(shell find $(USHELLDIR)/ -maxdepth 1 -type f -regex ".*\.c")
PROG_OBJ    := $(patsubst $(USHELLDIR)/%.c, $(USHELLDIR)/%.o, $(PROG_SRC))

$(USHELLDIR)/%.o: $(USHELLDIR)/%.c
	gcc -I../common/include -DHAS_MPK -fPIC -c -o $@ $<

all: $(PROG_OBJ)
	@$(MAKE) -C $(UK_ROOT) A=$(PWD) L=$(LIBS)
	if [[ -f $(SYMFILE) ]]; then cp $(SYMFILE) $(USHELLDIR)/debug.sym; fi
	if [[ -f $(DBGFILE) ]]; then nm $(DBGFILE) | cut -d ' ' -f1,3 > ./$(USHELLDIR)/symbol.txt; fi
	if [[ ! -d build/$(USHELLDIR) ]]; then cp -r $(USHELLDIR) build/$(USHELLDIR); fi

$(MAKECMDGOALS):
	@$(MAKE) -C $(UK_ROOT) A=$(PWD) L=$(LIBS) $(MAKECMDGOALS)

.PHONY: prog

