#ifndef BPF_HELPERS_H
#define BPF_HELPERS_H

#include <stdint.h>

#define bpf_map_get ((uint64_t(*)(uint64_t, uint64_t))0)
#define bpf_map_put ((void (*)(uint64_t, uint64_t, uint64_t))1)
#define bpf_strtoll ((long long (*)(const char *st, int base))2)
#define bpf_strtoull ((unsigned long long (*)(const char *st, int base))3)

#define bpf_notify ((void (*)(uint64_t))4)

#endif /* BPF_HELPERS_H */
