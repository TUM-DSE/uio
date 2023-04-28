#include "bpf_helpers.h"
#include "config.h"

int bpf_prog(void *arg)
{
	if (!arg) {
		return -1;
	}
	uint64_t addr = (uint64_t)bpf_strtoull(arg, 16);
	uint64_t count = bpf_map_get(addr, COUNT_KEY);
	if (count == UINT64_MAX) {
		count = 0;
	}

	return count;
}
