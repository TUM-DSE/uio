#include "bpf_helpers.h"

int bpf_prog(void *arg)
{
	struct UbpfTracerCtx *ctx = arg;
	ctx->buf[0] = 'h';
	ctx->buf[1] = 'e';
	ctx->buf[2] = 'l';
	ctx->buf[3] = 'l';
	ctx->buf[4] = 'o';
	ctx->buf[5] = '!';
	ctx->buf[5] = '\n';
	ctx->buf[6] = '\0';
	bpf_puts(&ctx->buf);

	return 0;
}
