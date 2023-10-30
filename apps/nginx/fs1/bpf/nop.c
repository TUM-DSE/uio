#include "../../../bpf_prog/bpf_helpers.h"


__attribute__((section("tracer"), used))
int bpf_tracer(bpf_tracer_ctx_descriptor_t *ctx_descr)
{
	return 0;
}

// bpf_attach ngx_http_process_request_line /ushell/bpf/nop.o