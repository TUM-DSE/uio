UK_ROOT ?= $(PWD)/../../unikraft
UK_LIBS ?= $(PWD)/../../libs
LIBS := $(UK_LIBS)/pthread-embedded:$(UK_LIBS)/newlib:$(UK_LIBS)/sqlite
LIBS := $(LIBS):$(UK_LIBS)/ubpf:$(UK_LIBS)/ubpf_tracer
KVM_BINARY  := build/sqlite3_backup_kvm-x86_64
SYMFILE     := $(KVM_BINARY).sym
DBGFILE     := $(KVM_BINARY).dbg
USHELLDIR   := fs1
PROG_SRC    := $(shell find $(USHELLDIR)/ -maxdepth 1 -type f -regex ".*\.c")
PROG_OBJ    := $(patsubst $(USHELLDIR)/%.c, $(USHELLDIR)/%.o, $(PROG_SRC))

$(USHELLDIR)/%.o: $(USHELLDIR)/%.c
	gcc -I../common/include -DHAS_MPK -fPIC -fno-stack-protector -c -o $@ $<

all: $(PROG_OBJ)
	mkdir -p ./build
	pushd fs0 && find . | cpio --format=newc -o > ../build/fs0.cpio && popd
	@$(MAKE) -C $(UK_ROOT) A=$(PWD) L=$(LIBS)
	if [[ -f $(SYMFILE) ]]; then cp $(SYMFILE) fs1/debug.sym; fi
	if [[ -f $(DBGFILE) ]]; then nm $(DBGFILE) | cut -d ' ' -f1,3 > ./$(USHELLDIR)/symbol.txt; fi
	if [[ ! -d build/$(USHELLDIR) ]]; then cp -r $(USHELLDIR) build/$(USHELLDIR); fi

$(MAKECMDGOALS):
	@$(MAKE) -C $(UK_ROOT) A=$(PWD) L=$(LIBS) $(MAKECMDGOALS)
