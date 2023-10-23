// bpf_exec bpf/test_div.o test_div

__attribute__((section("test"), used))
int test_div(void* context) {
        asm volatile("r2 = 1000000");

        for(long long int count = 0; count < 1e6; count++) {
                asm volatile("r2 /= 1");
        }

        return 0;
}