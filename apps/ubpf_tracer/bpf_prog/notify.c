#include "config.h"

int bpf_prog(void *arg)
{
	struct UbpfTracerCtx *ctx = arg;
	bpf_notify(ctx->traced_function_address);

	return 0;
}
