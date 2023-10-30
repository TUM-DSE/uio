// bpf_exec bpf-microbenchmark/test_mod.obj test_mod

__attribute__((section("test"), used))
int test_mod(void* context) {
        asm volatile("r2 = 1000000":::"r1","r2","");

        for(long long int count = 0; count < 1e6; count++) {
                asm volatile("r2 /= 1"); // LLVM do not support mod, we wll update it mannuelly with mod instruction
        }

        return 0;
}