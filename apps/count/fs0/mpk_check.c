#include "unicall_wrapper.h"

#include <stdint.h>

extern void ushell_puts(char *);

char ok[] = "OK\n";
char no[] = "NO\n";

#define SHOULD_ENABLED                                                         \
	do {                                                                   \
		if (ushell_write_is_enabled()) {                               \
			ushell_puts(ok);                                       \
		} else {                                                       \
			ushell_puts(no);                                       \
		}                                                              \
	} while (0)

#define SHOULD_DISABLED                                                        \
	do {                                                                   \
		if (!ushell_write_is_enabled()) {                              \
			ushell_puts(ok);                                       \
		} else {                                                       \
			ushell_puts(no);                                       \
		}                                                              \
	} while (0)

__attribute__((always_inline)) static inline uint32_t rdpkru(void)
{
	uint32_t res;

	asm volatile("xor %%ecx, %%ecx;"
		     "rdpkru;"
		     "movl %%eax, %0"
		     : "=r"(res)::"rax", "rdx", "ecx");

	return res;
}

__attribute__((section(".text"))) int main(int argc, char *argv[])
{
	// at first, check if the write is enabled
	SHOULD_DISABLED;

	// then, enable it
	ushell_enable_write();
	// check again
	SHOULD_ENABLED;

	// nesting also should work
	ushell_enable_write();
	SHOULD_ENABLED;

	// then, disable it
	ushell_disable_write();
	// check again
	SHOULD_DISABLED;

	// perform some long computation to see if MPK works well with
	// interrupts
	volatile int i, j;
	int a, n = 30000;
	for (i = 0; i < n; i++) {
		for (j = 0; j < n; j++) {
			a += j;
		}
	}

	SHOULD_DISABLED;

	uint32_t pkru_val = rdpkru();

	return pkru_val;
}
