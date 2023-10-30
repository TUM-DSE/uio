#include "../../../bpf_prog/bpf_helpers.h"


__attribute__((section("executable"), used))
__u64 nop(uk_bpf_type_executable_t* context) {
	return 0;
}

// bpf_exec bpf-overhead/nop.o nop