#include <stdint.h>

#define NULL 0

static uint64_t (*bpf_map_get)(uint64_t mapFd, uint64_t key2) = (void *) 1;

static uint64_t (*invalid_bpf_map_get)(uint64_t mapFd, uint64_t key2, uint64_t dummy) = (void *) 1;

static void (*bpf_puts)(char *str) = (void *) 8;

__attribute__((section("test/no_context1")))
void bpf_call_map_helper_valid() {
    bpf_map_get(0, 42);
}

__attribute__((section("test/no_context2")))
void bpf_call_map_helper_invalid() {
    invalid_bpf_map_get(0, 42, 4242);
}