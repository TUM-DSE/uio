#include "bpf_helpers.h"

__attribute__((section("executable"), used))
int oob(uk_bpf_type_executable_t* context) {
        __u64 flag_address = bpf_get_addr(context->data);

        return *((int*)flag_address);
}


// bpf_exec bpf-security/oob.o oob the_flag