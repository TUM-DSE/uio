#include "bpf_helpers.h"

int bpf_prog(void *arg)
{
	// NOTE: we need to use stack memory
	char buf[] = "hello!\n";
	bpf_puts(&buf[0]);

	return 0;
}
