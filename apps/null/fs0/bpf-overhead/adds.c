#include "../../../bpf_prog/bpf_helpers.h"

// bpf_exec bpf-overhead/adds.o adds

__attribute__((section("executable"), used))
void adds(uk_bpf_type_executable_t* context) {
    
    asm volatile("r0 = 0");
    
    #pragma clang loop unroll(full)
    for(int index = 0; index < 1e3; index++) {
        asm volatile("r0 += 1");
    }
}
