#include "bpf_helpers.h"

// bpf_exec bpf-security/null_exception.o null_exception Abracadabra

__attribute__((section("executable"), used))
int null_exception(uk_bpf_type_executable_t* context) {
        __u64 target_address = bpf_get_addr(context->data);

        return *((int*)target_address);
}
