#include "config.h"

int bpf_prog(void *arg)
{
	uint64_t key1 = 0;
	uint64_t key2 = 0;
	uint64_t value = 0x1234;
	bpf_map_put(key1, key2, value);

	return 0;
}
