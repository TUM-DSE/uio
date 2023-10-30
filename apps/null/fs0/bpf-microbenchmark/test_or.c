// bpf_exec bpf-microbenchmark/test_or.o test_or

__attribute__((section("test"), used))
int test_or(void* context) {
        asm volatile("r2 = 1":::"r1","r2","");

        for(long long int count = 0; count < 1e6; count++) {
                asm volatile("r2 |= 0");
        }

        return 0;
}