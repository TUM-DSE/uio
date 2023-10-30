#include "../../../bpf_prog/bpf_helpers.h"

// bpf_exec bpf-security/div_by_zero2.o div_by_zero2

__attribute__((section("executable"), used))
void div_by_zero2(uk_bpf_type_executable_t* context) {

    asm volatile("r0 = 0x42");
    asm volatile("r1 = 0");
    asm volatile("r0 /= r1");

    return;
}
