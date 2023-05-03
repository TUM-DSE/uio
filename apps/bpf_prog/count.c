#include "bpf_helpers.h"

int bpf_prog(void *arg)
{
	struct UbpfTracerCtx *ctx = arg;
	__u64 count = bpf_map_get(ctx->traced_function_address, COUNT_KEY);
	if (count == UINT64_MAX) {
		count = 0;
	}
	count++;
	bpf_map_put(ctx->traced_function_address, COUNT_KEY, count);

	return 0;
}
