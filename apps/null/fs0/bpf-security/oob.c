#include "../../../bpf_prog/bpf_helpers.h"

__attribute__((section("executable"), used))
int oob(uk_bpf_type_executable_t* context) {
        if (context->data_end - context->data < 2) {
                return -1;
        }

        __u64 flag_address = bpf_get_addr(context->data);

        return *((int*)flag_address);
}


// bpf_exec bpf-security/oob.o oob the_flag
// bpf_exec bpf-security/oob.o oob Abracadabra