// bpf_exec bpf-microbenchmark/test_add.o test_add

__attribute__((section("test"), used))
int test_mul(void* context) {
        asm volatile("r2 = 1000000":::"r1","r2","");

        for(long long int count = 0; count < 1e6; count++) {
                asm volatile("r2 *= 1");
        }

        return 0;
}