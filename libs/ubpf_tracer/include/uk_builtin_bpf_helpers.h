//
// Created by ken on 04.06.23.
//

#ifndef USHELL_TERMINAL_UK_BUILTIN_BPF_HELPERS_H
#define USHELL_TERMINAL_UK_BUILTIN_BPF_HELPERS_H

#include <stdint.h>

#define BPF_MAP_VALUE_UNDEFINED UINT64_MAX // -1

uint64_t bpf_map_noop();
uint64_t bpf_map_get(uint64_t key1, uint64_t key2);
uint64_t bpf_map_put(uint64_t key1, uint64_t key2, uint64_t value);
uint64_t bpf_map_del(uint64_t key1, uint64_t key2);
uint64_t bpf_get_addr(const char *function_name);
uint64_t bpf_probe_read(void* addr, uint64_t size);
uint64_t bpf_time_get_ns();

uint64_t bpf_unwind(uint64_t i);
uint64_t bpf_puts(const char *buf);

#endif // USHELL_TERMINAL_UK_BUILTIN_BPF_HELPERS_H
