#include <stdio.h>
#include <unistd.h>

// cf. Intel SDM CHAPTER 19 PERFORMANCE MONITORING

#define IA32_PERF_EVENTSEL0 0x186
#define IA32_PERF_EVENTSEL1 0x187

#define IA32_PERF_PERFCTR0 0xc1
#define IA32_PERF_PERFCTR1 0xc2

#define IA32_PERF_GLOBAL_STATUS 0x38e
#define IA32_PERF_GLOBAL_CTRL 0x38f

#define IA32_FIXED_CTR_CTRL 0x38d
#define IA32_FIXED_CTR0 0x309 // Instructions Retired
#define IA32_FIXED_CTR1 0x30a // Unhalted CPU Cycles
#define IA32_FIXED_CTR2 0x30b // Reference CPU Cycles

static inline void cpuid(__u32 fn, __u32 subfn, __u32 *eax, __u32 *ebx,
			 __u32 *ecx, __u32 *edx)
{
	asm volatile("cpuid"
		     : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
		     : "a"(fn), "c"(subfn));
}

static inline void rdmsr(unsigned int msr, __u32 *lo, __u32 *hi)
{
	asm volatile("rdmsr" : "=a"(*lo), "=d"(*hi) : "c"(msr));
}

static inline __u64 rdmsrl(unsigned int msr)
{
	__u32 lo, hi;

	rdmsr(msr, &lo, &hi);
	return ((__u64)lo | (__u64)hi << 32);
}

static inline void wrmsr(unsigned int msr, __u32 lo, __u32 hi)
{
	asm volatile("wrmsr"
		     : /* no outputs */
		     : "c"(msr), "a"(lo), "d"(hi));
}

static inline void wrmsrl(unsigned int msr, __u64 val)
{
	wrmsr(msr, (__u32)(val & 0xffffffffULL), (__u32)(val >> 32));
}

// CPUID.0AH: EAX[7:0] > 0
int check_pmc()
{
	__u32 eax, ebx, ecx, edx;
	cpuid(0x0A, 0, &eax, &ebx, &ecx, &edx);
	int version = eax & 0xff;
#if 0
	printf("CPUID.0AH: EAX=%#08x\n", eax);
	printf("version = %d\n", version);
#endif
	return version > 0;
}

unsigned long get_counter_state()
{
	unsigned msr;
	msr = IA32_PERF_GLOBAL_CTRL;

	return rdmsrl(msr);
}

void enable_counter()
{
	__u64 val;
	unsigned msr;
	__u32 eax, ebx, ecx, edx;
	int num_counters, num_fixed_counters;

	cpuid(0x0A, 0, &eax, &ebx, &ecx, &edx);
	num_counters = (eax >> 8) & 0xff;
	num_fixed_counters = edx & 0xf;
#if 1
	printf("num of counters = %d\n", num_counters);
	printf("num of fixed counters = %d\n", num_fixed_counters);
#endif

	// enable all counters
	val = (((__u64)((1 << num_fixed_counters) - 1)) << 32)
	      | ((1 << num_counters) - 1);
	msr = IA32_PERF_GLOBAL_CTRL;
	wrmsrl(msr, val);
}

void set_eventsel(__u8 umask, __u8 event, int sel)
{
	__u64 val;
	unsigned msr;

	// enable = 1 (0x4)
	// os = 1, user=1 (0x3)
	val = 0x00430000 | ((__u64)umask << 8) | event;
	msr = IA32_PERF_EVENTSEL0 + sel;
	wrmsrl(msr, val);
}

void enable_instruction_retired(int sel)
{
	__u8 umask = 0x00;
	__u8 event = 0xc0;
	set_eventsel(umask, event, sel);
}

void enable_llc_misses(int sel)
{
	__u8 umask = 0x41;
	__u8 event = 0x2e;
	set_eventsel(umask, event, sel);
}

unsigned long rdpmc_ctr(int sel)
{
	unsigned msr;
	msr = IA32_PERF_PERFCTR0 + sel;
	return rdmsrl(msr);
}

int main()
{
	int i = 0;
	unsigned long before, after;

	if (!check_pmc()) {
		printf("pmc not available\n");
		return 0;
	}

	before = get_counter_state();
	printf("enable counter\n");
	enable_counter();
	after = get_counter_state();
	printf("counter state: %#lx => %#lx\n", before, after);

	enable_instruction_retired(0);
	enable_llc_misses(1);

	for (i = 0; i < 3; i++) {
		sleep(1);
		unsigned long c0 = rdpmc_ctr(0);
		unsigned long c1 = rdpmc_ctr(1);
		printf("%ld instructions, %ld cache-misses\n", c0, c1);
	}

	return 0;
}
