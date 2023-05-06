#include "bpf_helpers.h"

int bpf_prog(void *arg)
{
	if (!arg) {
		return -1;
	}
	__u64 addr = bpf_get_ret_addr((const char *)arg);
	if (addr == 0) {
		return -2;
	}
	__u64 count = bpf_map_get(addr, COUNT_KEY);
	if (count == UINT64_MAX) {
		count = 0;
	}

	return count;
}
