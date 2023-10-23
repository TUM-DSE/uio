#include "bpf_helpers.h"

// bpf_exec bpf-security/div_by_nonzero.o div_by_nonzero

__attribute__((section("executable"), used))
void div_by_nonzero(uk_bpf_type_executable_t* context) {

    asm volatile("r0 = 0x42");
    asm volatile("r1 = 0");
    asm volatile("r0 /= 1");

    return;
}
