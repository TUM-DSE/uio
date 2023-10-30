#include "../../../bpf_prog/bpf_helpers.h"

__attribute__((section("executable"), used))
__u64 type_safty(uk_bpf_type_executable_t* context) {
    if (context->data_end - context->data < 2) {
        return -1;
    }

    asm volatile("r1 *= 1");
    __u64 pointer = bpf_get_addr(context->data);

    return bpf_probe_read(pointer, 8);
}

// bpf_exec bpf-security/type_safty.o type_safty the_flag