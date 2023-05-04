#ifndef BPF_HELPERS_H
#define BPF_HELPERS_H

#include <asm/types.h>

#define bpf_map_get ((__u64(*)(__u64 key1, __u64 key2))1)
#define bpf_map_put ((void (*)(__u64 key1, __u64 key2, __u64 value))2)
#define bpf_map_del ((void (*)(__u64 key1, __u64 key2))3)
#define bpf_get_addr ((__u64(*)(const char *function_name))4)
#define bpf_probe_read ((__u64(*)(__u64 addr, __u64 size))5)
#define bpf_time_get_ns ((__u64(*)())6)
#define bpf_puts ((void (*)(char *buf))7)

#define bpf_notify ((void (*)(__u64 function_address))8)
#define bpf_get_ret_addr ((__u64(*)(const char *function_name))9)

#define UINT64_MAX 0xffffffffffffffffULL

#define COUNT_KEY 0

struct UbpfTracerCtx {
	__u64 traced_function_address;
	char buf[120];
};

#endif /* BPF_HELPERS_H */

