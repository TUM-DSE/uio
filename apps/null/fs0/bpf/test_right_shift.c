// bpf_exec bpf/test_right_shift.o test_right_shift

__attribute__((section("test"), used))
int test_right_shift(void* context) {
        asm volatile("r2 = 0");

        for(long long int count = 0; count < 1e6; count++) {
                asm volatile("r2 >>= 1");
        }

        return 0;
}