#include "bpf_helpers.h"

__attribute__((section("executable"), used))
int infinity_loop(uk_bpf_type_executable_t* context) {

    asm volatile("r2 = 0x42");

    for(unsigned int index = 0; index <= 0xffffffff; index++) {
        asm volatile("r2 /= 1");
    }

    return 0;
}


// bpf_exec bpf-security/infinity_loop.o infinity_loop