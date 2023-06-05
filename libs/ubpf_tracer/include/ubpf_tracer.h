#ifndef UBPF_TRACER_H
#define UBPF_TRACER_H
#include "arraylist.h"
#include "hash_chains.h"
#include "ubpf_helpers.h"
#include <ubpf_runtime.h>

#include <ubpf.h>
#include <ubpf_config.h>

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define CALL_OPCODE 0xe8
#define CALL_INSTRUCTION_SIZE 5

struct UbpfTracer {
  struct DebugInfo *symbols; // { function_name -> function_address }
  uint32_t symbols_cnt;
  struct THashMap *nop_map;        // { function_address -> nop_address }
  struct THashMap *vm_map;         // { ret_address -> List<(label, ubpf_vm)> }
  struct THashMap *function_names; // { ret_address -> function_name }
};

struct UbpfTracerCtx {
  uint64_t traced_function_address;
  char buf[120];
};

struct DebugInfo {
  uint64_t address;
  char type[20];
  char identifier[50];
};

struct UbpfTracer *init_tracer();
struct UbpfTracer *get_tracer();

void load_debug_symbols(struct UbpfTracer *tracer);
uint64_t get_function_address(struct UbpfTracer *tracer,
                              const char *function_name);
uint64_t get_nop_address(struct UbpfTracer *tracer, uint64_t function_address);
uint64_t find_nop_address(struct UbpfTracer *tracer, const char *function_name,
                          void (*print_fn)(char *str));
void tracer_helpers_add(struct UbpfTracer *tracer, const char *label,
                        void *function_ptr);
void tracer_helpers_del(struct UbpfTracer *tracer, const char *label);
void run_bpf_program();

void *readfile(const char *path, size_t maxlen, size_t *len);

// BPF helpers
void bpf_notify(void *function_id);
uint64_t bpf_get_ret_addr(const char *function_name);
uint64_t bpf_get_addr(const char *function_name);

// shell commands
int bpf_attach(const char *function_name, const char *bpf_filename,
               void (*print_fn)(char *str));
int bpf_list(const char *function_name, void (*print_fn)(char *str));
int bpf_detach(const char *function_name, const char *bpf_filename,
               void (*print_fn)(char *str));

#endif /* UBPF_TRACER_H */
