#include "bpf_helpers.h"

__attribute__((section("executable"), used))
char type_safty(uk_bpf_type_executable_t* context) {

    long long int pointer = (long long int)context->data;
    asm volatile("r2 *= 1");

    if (pointer >= context->data_end) {
        return -1;
    }

    return *((char*)pointer);
}

// bpf_exec bpf-security/type_safty.o type_safty