#include "config.h"

int bpf_prog(void *arg)
{
	uint64_t key1 = 0;
	uint64_t key2 = 0;
	uint64_t value = bpf_map_get(key1, key2);

	return value;
}
