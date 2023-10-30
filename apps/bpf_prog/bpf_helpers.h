#ifndef BPF_HELPERS_H
#define BPF_HELPERS_H

#include <asm/types.h>

#define bpf_map_get ((__u64(*)(__u64 key1, __u64 key2))1)
#define bpf_map_put ((__u64(*)(__u64 key1, __u64 key2, __u64 value))2)
#define bpf_map_del ((__u64(*)(__u64 key1, __u64 key2))3)
#define bpf_get_addr_hack(function_name, dont_care) ((__u64(*)(const char*, __u64))4)(function_name, dont_care)
#define bpf_get_addr(function_name) bpf_get_addr_hack(function_name, 1)

#define bpf_probe_read ((__u64(*)(__u64 addr, __u64 size))5)
#define bpf_time_get_ns ((__u64(*)())6)
#define bpf_unwind ((__u64(*)(__u64 i))7)
#define bpf_puts ((__u64(*)(char *buf))8)

#define bpf_notify ((void (*)(__u64 function_address))30)
#define bpf_get_ret_addr ((__u64(*)(const char *function_name))31)

#define UINT64_MAX 0xffffffffffffffffULL

#define COUNT_KEY 0

struct UbpfTracerCtx {
	__u64 traced_function_address;
	char buf[120];
};

typedef struct bpf_tracer_ctx_descriptor {
    void* data;
    void* data_end;
    __u64 data_meta;

    struct UbpfTracerCtx ctx;
} bpf_tracer_ctx_descriptor_t;

#define MAX_EXECUTABLE_CTX_SIZE 1024

typedef struct uk_bpf_type_executable {
    char* data;
    char* data_end;
    __u64 data_meta;

    char storage[];
} uk_bpf_type_executable_t;

#endif /* BPF_HELPERS_H */