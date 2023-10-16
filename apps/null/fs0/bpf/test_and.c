// bpf_exec bpf/test_and.o test_and

__attribute__((section("test"), used))
int test_and(void* context) {
        asm volatile("r2 = 1");

        for(long long int count = 0; count < 1e6; count++) {
                asm volatile("r2 &= 1");
        }

        return 0;
}