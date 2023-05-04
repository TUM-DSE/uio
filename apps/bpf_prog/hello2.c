#include "bpf_helpers.h"

// This does not work for now
int bpf_prog(void *arg)
{
	char buf[] = "hello!\n";
	bpf_puts(&buf[0]);

	return 0;
}
