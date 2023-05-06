#include "bpf_helpers.h"

// example:
// > bpf_exec read_symbol.bin count

int bpf_prog(void *arg)
{
	__u64 addr = bpf_get_addr((const char*)arg);

	if (addr == 0) {
		char errmsg[] = "symbol not found\n";
		bpf_puts(&errmsg[0]);
		return -1;
	}

	__u64 val = bpf_probe_read(addr, 8);

	return val;
}
