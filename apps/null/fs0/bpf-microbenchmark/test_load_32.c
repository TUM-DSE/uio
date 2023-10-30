#include "../../../bpf_prog/bpf_helpers.h"

// bpf_exec bpf-microbenchmark2/test_load_32.o test_load_32

__attribute__((section("executable"), used))
__u64 test_load_32(uk_bpf_type_executable_t* context) {
        asm volatile("r2 = 0":::"r1","r2","");

        for(long long int count = 0; count < 1e6; count++) {
                asm volatile("r2 = *(u32 *)(r1 + 0)");
        }
        return 0;
}