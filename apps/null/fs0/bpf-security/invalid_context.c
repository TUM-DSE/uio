#include "bpf_helpers.h"

__attribute__((section("no_context"), used))
long long int invalid_context(uk_bpf_type_executable_t* context) {

    long long int checksum = 0;

    char* first_char = context->data;
    if(first_char >= context->data_end) {
        return -1;
    }

    checksum += *first_char;

    return checksum;
}

// bpf_exec bpf-security/invalid_context.o invalid_context