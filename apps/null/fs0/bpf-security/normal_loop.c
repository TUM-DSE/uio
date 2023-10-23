#include "bpf_helpers.h"

// bpf_exec bpf-security/normal_loop.o normal_loop

#define ARRAY_LENGTH 40

__attribute__((section("executable"), used))
int normal_loop(uk_bpf_type_executable_t* context) {
    
    int index;
    int cumul = 0;
    __u8 array[ARRAY_LENGTH] = {0};

    for (index = 0; index < sizeof(array); index++) {
        if ((context->data + index) >= context->data_end)
            break;

        array[index] = 1;
    }

    for (index = 0; index < sizeof(array); index++) {
        cumul += array[index];
    }

    return cumul;
}
