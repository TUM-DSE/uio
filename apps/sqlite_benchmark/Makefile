UK_ROOT ?= $(PWD)/../../unikraft
UK_LIBS ?= $(PWD)/../../libs
LIBS := $(UK_LIBS)/pthread-embedded:$(UK_LIBS)/newlib:$(UK_LIBS)/lwip:$(UK_LIBS)/sqlite
LIBS := $(LIBS):$(UK_LIBS)/ubpf:$(UK_LIBS)/ubpf_tracer
KVM_BINARY  := build/sqlite_benchmark_kvm-x86_64
SYMFILE     := $(KVM_BINARY).sym
DBGFILE     := $(KVM_BINARY).dbg
USHELLDIR   := fs1

all:
	mkdir -p ./build
	pushd fs0 && find . | cpio --format=newc -o > ../build/fs0.cpio && popd
	@$(MAKE) -C $(UK_ROOT) A=$(PWD) L=$(LIBS)
	if [[ -f $(SYMFILE) ]]; then cp $(SYMFILE) fs1/debug.sym; fi
	if [[ -f $(DBGFILE) ]]; then nm $(DBGFILE) | cut -d ' ' -f1,3 > ./$(USHELLDIR)/symbol.txt; fi
	if [[ ! -d build/$(USHELLDIR) ]]; then cp -r $(USHELLDIR) build/$(USHELLDIR); fi

$(MAKECMDGOALS):
	@$(MAKE) -C $(UK_ROOT) A=$(PWD) L=$(LIBS) $(MAKECMDGOALS)
