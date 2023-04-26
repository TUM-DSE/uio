
#if !defined(__x86_64__)
// hack to include stdint.h with bpf target
#define __x86_64__
// NOTE: we can use <asm/types.h> and __u64 instead of stdint
#endif

#include <stdint.h>
#define bpf_map_get ((uint64_t(*)(uint64_t, uint64_t))0)
#define bpf_map_put ((void (*)(uint64_t, uint64_t, uint64_t))1)
#define bpf_notify ((void (*)(uint64_t))2)

struct UbpfTracerCtx {
  uint64_t traced_function_address;
};

int bpf_prog(void *arg) {
  struct UbpfTracerCtx *ctx = arg;
  bpf_notify(ctx->traced_function_address);

  return 0;
}
