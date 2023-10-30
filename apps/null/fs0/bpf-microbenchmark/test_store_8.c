#include "../../../bpf_prog/bpf_helpers.h"

// bpf_exec bpf-microbenchmark2/test_store_8.o test_store_8

__attribute__((section("executable"), used))
__u64 test_store_8(uk_bpf_type_executable_t* context) {
        asm volatile("r2 = 0":::"r1","r2","");

        for(long long int count = 0; count < 1e6; count++) {
                asm volatile("*(u8 *)(r1 + 0) = r2");
        }

        return 0;
}