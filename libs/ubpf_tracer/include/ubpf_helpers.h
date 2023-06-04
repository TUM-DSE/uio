#ifndef UBPF_HELPERS_H
#define UBPF_HELPERS_H

#include "hash_chains.h"
#include "helper_function_list.h"

#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <ubpf.h>

#define ERR(st) "\033[0m\033[1;31m" st "\033[0m"
#define YAY(st) "\033[0m\033[1;32m" st "\033[0m"
#define wrap_print_fn(BUF_SIZE, ...)                                           \
  {                                                                            \
    char *buf = calloc(BUF_SIZE, sizeof(char));                                \
    snprintf(buf, BUF_SIZE, __VA_ARGS__);                                      \
    print_fn(buf);                                                             \
    free(buf);                                                                 \
  }

#define register_helper(idx, label, fun_ptr)                                   \
  {                                                                            \
    if (logfile != NULL) {                                                     \
      fprintf(logfile, " - [%lu]: %s\n", idx, label);                          \
    }                                                                          \
    ubpf_register(vm, idx, label, fun_ptr);                                    \
  }

// BPF helperes
uint64_t bpf_map_get(uint64_t key1, uint64_t key2);
void bpf_map_put(uint64_t key1, uint64_t key2, uint64_t value);
void bpf_map_del(uint64_t key1, uint64_t key2);
uint64_t bpf_get_addr(const char *function_name);
uint64_t bpf_probe_read(uint64_t addr, uint64_t size);
uint64_t bpf_time_get_ns();
void bpf_puts(char *buf);

HelperFunctionList *get_instance_builtin_bpf_helpers();
void additional_helpers_list_add(const char *label, void *function_ptr);
void additional_helpers_list_del(const char *label);

struct ubpf_vm *init_vm(FILE *logfile);

void *readfile(const char *path, size_t maxlen, size_t *len);

// shell commands
int bpf_exec(const char *filename, void *args, size_t args_size, int debug,
             void (*print_fn)(char *str));

#endif /* UBPF_HELPERS_H */
