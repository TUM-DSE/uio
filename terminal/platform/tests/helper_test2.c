#include <stdint.h>

#define NULL 0

static uint64_t (*bpf_map_get)(uint64_t mapFd, uint64_t key2) = (void *) 1;

static uint64_t (*invalid_bpf_map_get)(uint64_t mapFd, uint64_t key2, uint64_t dummy) = (void *) 1;

static void (*bpf_puts)(char *str) = (void *) 7;

struct prog_with_argument {
    void *data;
    void *data_end;
    uint32_t data_meta;
};


__attribute__((section("tracer/test1")))
int bpf_invalid_access_context(struct prog_with_argument *ctx) {

    int *ptr = ctx->data;
    return *(int *) ptr;
}

__attribute__((section("tracer/test2"), used))
int bpf_valid_access_context(struct prog_with_argument *ctx) {
    void *data_end = ctx->data_end;
    void *data = ctx->data;

    const int offset = 4;

    if (data + offset + sizeof(int) > data_end) {
        return 0;
    }

    int *ptr = offset + data;
    return *(int *) ptr;
}
