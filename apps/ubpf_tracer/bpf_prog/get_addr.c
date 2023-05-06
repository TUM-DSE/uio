#include "bpf_helpers.h"
#include "config.h"

int bpf_prog(void *arg)
{
	if (!arg) {
		return -1;
	}
	uint64_t addr = bpf_get_ret_addr((const char *)arg);
	if (addr == 0) {
		return -1;
	}
	return addr;
}
