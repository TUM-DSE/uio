// bpf_exec bpf/test_neg.o test_neg

__attribute__((section("test"), used))
int test_neg(void* context) {
        asm volatile("r2 = 1");

        for(long long int count = 0; count < 1e6; count++) {
                asm volatile("r2 = -r2");
        }

        return 0;
}