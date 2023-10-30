// bpf_exec bpf-microbenchmark/test_sub.o test_sub

__attribute__((section("test"), used))
int test_sub(void* context) {
        asm volatile("r2 = 1000000":::"r1","r2","");

        for(long long int count = 0; count < 1e6; count++) {
                asm volatile("r2 -= 1");
        }

        return 0;
}