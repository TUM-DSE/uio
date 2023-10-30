#include "../../../bpf_prog/bpf_helpers.h"

// bpf_exec bpf-overhead/hash.o hash

__attribute__((section("executable"), used))
__u64 hash(uk_bpf_type_executable_t* context) {
    
    __u64 sum = 0;

    for(int index = 0; index < 256; index++) {
        char* input = context->data + index;
        if(input >= context->data_end) {
            break;
        }

        char to_add = *input;

        if(to_add >= 'A' && to_add <= 'Z') {
            to_add += 'A' - 'a';
        } else if(to_add >= '0' && to_add <= '9') {
            to_add -= '0';
        }

        sum += to_add;
    }

    return sum;
}
