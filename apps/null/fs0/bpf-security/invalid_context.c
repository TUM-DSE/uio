#include "../../../bpf_prog/bpf_helpers.h"

__attribute__((section("executable"), used))
__u64 invalid_context(uk_bpf_type_executable_t* context) {

     if (context->data_end - context->data < 1) {
        return -1;
    }

    return *context->data;
}

// bpf_exec bpf-security/invalid_context.o invalid_context