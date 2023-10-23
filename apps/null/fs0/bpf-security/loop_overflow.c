#include "bpf_helpers.h"

// bpf_exec bpf-security/loop_overflow.o loop_overflow

__attribute__((section("executable"), used))
int loop_overflow(uk_bpf_type_executable_t* context) {
        for(int count = 0; count < 127; count++) {
            asm volatile("");
        }

        return 0;
}
