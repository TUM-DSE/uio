#include "../../../bpf_prog/bpf_helpers.h"

__attribute__((section("executable"), used))
int get_count(uk_bpf_type_executable_t *ctx)
{
	if(ctx->data_end == ctx->data) {
		return 0;
	}

	__u64 addr = bpf_get_ret_addr(ctx->data);
	if (addr == 0) {
		return -1; // function not found
	}
	
	__u64 count = bpf_map_get(addr, COUNT_KEY);
	if (count == UINT64_MAX) {
		count = 0;
	}

	return count;
}

// bpf_exec /ushell/bpf/get_count.o get_count ngx_http_process_request_line