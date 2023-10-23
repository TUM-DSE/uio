// bpf_exec bpf/test_for_loop.o test_for_loop

__attribute__((section("test"), used))
int test_for_loop(void* context) {
        asm volatile("r2 = 0");

        for(long long int count = 0; count < 1e6; count++) {
                asm volatile("");
        }

        return 0;
}
