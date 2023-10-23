#include "bpf_helpers.h"

// bpf_exec bpf-security/div_by_zero1.o div_by_zero1

__attribute__((section("executable"), used))
void div_by_zero1(uk_bpf_type_executable_t* context) {

    asm volatile("r0 = 0x42");
    asm volatile("r1 = 0");
    asm volatile("r0 /= 0");

    return;
}
