UK_ROOT ?= $(PWD)/../../unikraft
UK_LIBS ?= $(PWD)/../../libs
LIBS := $(UK_LIBS)/pthread-embedded:$(UK_LIBS)/newlib:$(UK_LIBS)/nginx:$(UK_LIBS)/lwip
LIBS := $(LIBS):$(UK_LIBS)/ubpf:$(UK_LIBS)/ubpf_tracer

all:
	mkdir -p ./build
	pushd fs0 && find . | cpio --format=newc -o > ../build/fs0.cpio && popd
	@$(MAKE) -C $(UK_ROOT) A=$(PWD) L=$(LIBS)

$(MAKECMDGOALS):
	@$(MAKE) -C $(UK_ROOT) A=$(PWD) L=$(LIBS) $(MAKECMDGOALS)
