#include "../../../bpf_prog/bpf_helpers.h"


__attribute__((section("tracer"), used))
int bpf_tracer(bpf_tracer_ctx_descriptor_t *ctx_descr)
{
	if(ctx_descr->data_end - ctx_descr->data != sizeof(struct UbpfTracerCtx)) {
		return -1;
	}

	__u64 count = bpf_map_get(ctx_descr->ctx.traced_function_address, COUNT_KEY);
	if (count == UINT64_MAX) {
		count = 0;
	}

	count++;

	bpf_map_put(ctx_descr->ctx.traced_function_address, COUNT_KEY, count);

	return 0;
}

// bpf_attach ngx_http_process_request_line /ushell/bpf/count.o
