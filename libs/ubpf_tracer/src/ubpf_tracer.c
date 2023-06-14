#include "ubpf_tracer.h"

#include <arraylist.h>
#include <hash_chains.h>
#include <ubpf_helpers.h>
#include <ubpf_runtime.h>

#include <ubpf.h>
#include <ubpf_config.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const uint8_t nopl[] = {0x0f, 0x1f, 0x44, 0x00, 0x00};

void bpf_notify(void *function_id) {
  struct THmapValueResult *hmap_entry =
      hmap_get(get_tracer()->function_names, (uint64_t)function_id);
  if (hmap_entry->m_Result == HMAP_SUCCESS) {
    printf(YAY("notify: %s\n"), (char *)hmap_entry->m_Value);
  } else {
    printf(ERR("notify: Unknown function at %p\n"), function_id);
  }
}

uint64_t bpf_get_ret_addr(const char *function_name) {
  if (function_name == NULL) {
    return 0;
  }
  struct UbpfTracer *tracer = get_tracer();
  uint64_t fun_addr = get_function_address(tracer, function_name);
  if (fun_addr == 0)
    return 0;
  uint64_t nop_addr = get_nop_address(tracer, fun_addr);
  if (nop_addr == 0)
    return 0;
  uint64_t result = nop_addr + CALL_INSTRUCTION_SIZE;
  return result;
}

void destruct_cell(struct THashCell *elem) {
  free(elem->m_Value);
  elem->m_Value = NULL;
}

void destruct_entry(struct LabeledEntry *entry) {
  free(entry->m_Label);
  free(entry->m_Value);
}

void *nop_map_init() {
  uint64_t *value = calloc(1, sizeof(uint64_t));
  return (void *)value;
}

void *function_names_init() {
  char *value = calloc(50, sizeof(char));
  return (void *)value;
}

void vm_map_destruct_cell(struct THashCell *elem) {
  list_destroy(elem->m_Value);
  destruct_cell(elem);
}

void vm_map_destruct_entry(struct LabeledEntry *entry) {
  ubpf_destroy(entry->m_Value);
  destruct_entry(entry);
}

void *init_arraylist() { return (void *)list_init(10, &vm_map_destruct_entry); }

struct UbpfTracer *init_tracer() {
  struct UbpfTracer *tracer = malloc(sizeof(struct UbpfTracer));
  int map_result;
  tracer->vm_map =
      hmap_init(101, vm_map_destruct_cell, init_arraylist, &map_result);
  tracer->nop_map = hmap_init(101, destruct_cell, nop_map_init, &map_result);
  tracer->function_names =
      hmap_init(101, destruct_cell, function_names_init, &map_result);

  // register local helpers
  // TODO
  //tracer_helpers_add(tracer, "bpf_notify", bpf_notify);
  //tracer_helpers_add(tracer, "bpf_get_ret_addr", bpf_get_ret_addr);

  load_debug_symbols(tracer);

  return tracer;
}

struct UbpfTracer *get_tracer() {
  static struct UbpfTracer *tracer = NULL;
  if (tracer == NULL)
    tracer = init_tracer();
  return tracer;
}

void load_debug_symbols(struct UbpfTracer *tracer) {
  char *sym_list[] = { "/symbol.txt", "/ushell/symbol.txt", "/debug.sym", "/ushell/debug.sym" };
  FILE *file_debug_sym = NULL;
  uint32_t symbols_size = 1;
  int format_nm = 0;

  for (size_t i = 0; i < sizeof(sym_list) / sizeof(sym_list[0]); i++) {
    file_debug_sym = fopen(sym_list[i], "r");
    if (file_debug_sym != NULL) {
      if (i >= 2) {
        format_nm = 1;
      }
      break;
    }
  }
  if (file_debug_sym == NULL) {
    // FIXME
    return;
  }

  tracer->symbols_cnt = 0;
  tracer->symbols = malloc(sizeof(struct DebugInfo) * symbols_size);
  while (!feof(file_debug_sym)) {
    if (symbols_size <= tracer->symbols_cnt) {
      symbols_size *= 2;
      tracer->symbols =
          realloc(tracer->symbols, sizeof(struct DebugInfo) * symbols_size);
    }
    uint64_t *sym_addr = &tracer->symbols[tracer->symbols_cnt].address;
    char *sym_type = tracer->symbols[tracer->symbols_cnt].type;
    char *sym_id = tracer->symbols[tracer->symbols_cnt].identifier;
    if (format_nm) {
      if (fscanf(file_debug_sym, "%lx %s %s\n", sym_addr, sym_type, sym_id) !=
              3 ||
          feof(file_debug_sym))
        break;
    } else {
      if (fscanf(file_debug_sym, "%lx %s\n", sym_addr, sym_id) !=
              2 ||
          feof(file_debug_sym))
        break;
    }

    tracer->symbols_cnt++;
  }
}

int bpf_attach_internal(struct UbpfTracer *tracer, const char *function_name,
                        const char *bpf_filename, void (*print_fn)(char *str)) {
  if (function_name == NULL || bpf_filename == NULL)
    return 1;
  wrap_print_fn(128, YAY("Load %s\n"), bpf_filename);

  uint64_t nop_addr = find_nop_address(tracer, function_name, print_fn);
  if (nop_addr == 0) {
    print_fn(ERR("Can't insert BPF program (no nop).\n"));
    return 2;
  }

  size_t code_len;
  void *bpf_program = readfile(bpf_filename, 1024 * 1024, &code_len);
  if (bpf_program == NULL) {
    print_fn(ERR("Can't insert BPF program (file doesn't exist).\n"));
    return 3;
  }
  // TODO: verify bpf_program here

  struct ubpf_vm *vm = init_vm(NULL);
  char *errmsg;
  ubpf_load(vm, bpf_program, code_len, &errmsg);

  struct THmapValueResult *hmap_entry = hmap_get_or_create(
      tracer->vm_map, (uint64_t)nop_addr + CALL_INSTRUCTION_SIZE);
  if (hmap_entry->m_Result == HMAP_SUCCESS) {
    struct ArrayListWithLabels *list = hmap_entry->m_Value;
    bool nop_already_replaced = list->m_Length > 0;

    char *label = malloc(strlen(bpf_filename));
    strcpy(label, bpf_filename);
    list_add_elem(list, label, vm);

    if (!nop_already_replaced) {
      extern void _run_bpf_program();
      // _run_bpf_program is defined in ubpf_tracer_trampoline.S
      uint64_t run_bpf_address = (uint64_t)_run_bpf_program;
      uint8_t call_function[CALL_INSTRUCTION_SIZE];
      call_function[0] = CALL_OPCODE;
      uint32_t offset =
          (uint32_t)(run_bpf_address - nop_addr - sizeof(call_function));
      memcpy(&(call_function[1]), &offset, sizeof(offset));
      memcpy((void *)nop_addr, call_function, sizeof(call_function));
    }
    print_fn(YAY("Program was attached.\n"));
  } else {
    print_fn(ERR("Can't access vm_map.\n"));
  }
  return 0;
}

uint64_t get_function_address(struct UbpfTracer *tracer,
                              const char *function_name) {
  for (uint32_t i = 0; i < tracer->symbols_cnt; ++i) {
    if (strcmp(function_name, tracer->symbols[i].identifier) == 0) {
      return tracer->symbols[i].address;
    }
  }
  return 0;
}

uint64_t get_nop_address(struct UbpfTracer *tracer, uint64_t function_address) {
  struct THmapValueResult *map_entry =
      hmap_get(tracer->nop_map, function_address);
  if (map_entry->m_Result == HMAP_SUCCESS) {
    return *(uint64_t *)map_entry->m_Value;
  }
  return 0;
}

uint64_t find_nop_address(struct UbpfTracer *tracer, const char *function_name,
                          void (*print_fn)(char *str)) {

  uint64_t addr = get_function_address(tracer, function_name);
  // don't search more than 100 instructions
  uint64_t addr_max = addr + 100;
  if (addr == 0) {
    print_fn(ERR("Function not found.\n"));
    return 0;
  }

  // check if we already don't have the nop address saved
  uint64_t saved_nopl_addr = get_nop_address(tracer, addr);
  if (saved_nopl_addr != 0) {
    return saved_nopl_addr;
  }

  uint8_t nopl_idx = 0;
  uint8_t *nopl_addr = NULL;
  bool found_nopl = false;
  for (uint8_t *i = (uint8_t *)addr; i < (uint8_t *)addr_max; ++i) {
    if (*i == nopl[nopl_idx]) {
      if (nopl_idx == 0) {
        nopl_addr = i;
      }
      if (nopl_idx < sizeof(nopl) - 1) {
        nopl_idx++;
      } else {
        found_nopl = true;
        break;
      }
    } else {
      nopl_idx = 0;
    }
  }
  if (!found_nopl) {
    print_fn(ERR("Nopl not found in function.\n"));
    return 0;
  }

  // insert into nop map
  uint64_t *nopl_addr_copy = calloc(1, sizeof(uint64_t));
  *nopl_addr_copy = (uint64_t)nopl_addr;
  hmap_put(tracer->nop_map, (uint64_t)addr, nopl_addr_copy);

  // insert function name into function_names map
  char *function_name_copy = calloc(strlen(function_name) + 1, sizeof(char));
  strcpy(function_name_copy, function_name);
  hmap_put(tracer->function_names, (uint64_t)nopl_addr + CALL_INSTRUCTION_SIZE,
           function_name_copy);

  return (uint64_t)nopl_addr;
}

uint64_t ubpf_tracer_save_rax;
uint64_t ubpf_tracer_ret_addr;
void run_bpf_program() {
  // uint64_t ret_addr = (uint64_t)__builtin_return_address(0);

  // find vm in the vm_map
  struct THmapValueResult *hmap_entry =
      hmap_get(get_tracer()->vm_map, ubpf_tracer_ret_addr);
  if (hmap_entry->m_Result == HMAP_SUCCESS) {
    struct ArrayListWithLabels *list = hmap_entry->m_Value;
    for (uint64_t i = 0; i < list->m_Length; ++i) {
      size_t ctx_size = sizeof(struct UbpfTracerCtx);
      struct UbpfTracerCtx ctx = {};
      ctx.traced_function_address = ubpf_tracer_ret_addr;

      struct LabeledEntry list_item = list->m_List[i];
      struct ubpf_vm *vm = list_item.m_Value;

      uint64_t ret;
      if (ubpf_exec(vm, &ctx, ctx_size, &ret) < 0)
        ret = UINT64_MAX;
    }
  }
  free(hmap_entry);
}

void prog_list_print(const char *function_name,
                     const struct ArrayListWithLabels *list,
                     void (*print_fn)(char *str)) {
  size_t buf_size = 100 * (list->m_Length + 1);
  char *buf = calloc(buf_size, sizeof(char));
  int len = 0;
  len += snprintf(buf, buf_size - len, "%s:\n", function_name);
  for (size_t j = 0; j < list->m_Length; ++j) {
    len += snprintf(buf + len, buf_size - len, "  - %s\n",
                    list->m_List[j].m_Label);
  }
  print_fn(buf);
  free(buf);
}

int bpf_list_internal(struct UbpfTracer *tracer, const char *function_name,
                      void (*print_fn)(char *str)) {
  if (function_name != NULL) {
    uint64_t fun_addr = get_function_address(tracer, function_name);
    if (fun_addr == 0) {
      print_fn(ERR("Function not found.\n"));
      return 1;
    }

    uint64_t nop_addr = get_nop_address(tracer, fun_addr);
    if (nop_addr == 0) {
      print_fn(ERR("Function not traced.\n"));
      return 1;
    }

    struct THmapValueResult *attached_programs =
        hmap_get(tracer->vm_map, nop_addr + CALL_INSTRUCTION_SIZE);
    if (attached_programs->m_Result != HMAP_SUCCESS) {
      wrap_print_fn(100, ERR("No programs attached to %s\n"), function_name);
      return 1;
    }

    prog_list_print(function_name, attached_programs->m_Value, print_fn);
  } else {
    // list all
    size_t map_size = tracer->vm_map->m_Size;
    for (size_t i = 0; i < map_size; ++i) {
      struct THashCell *current = tracer->vm_map->m_Map[i];
      while (current != NULL) {
        struct THmapValueResult *fun_name =
            hmap_get(tracer->function_names, current->m_Key);
        if (fun_name->m_Result != HMAP_SUCCESS) {
          wrap_print_fn(100, ERR("Can't find function name for address %p\n"),
                        (void *)current->m_Key);
          continue;
        }

        prog_list_print(fun_name->m_Value, current->m_Value, print_fn);
        current = current->m_Next;
      }
    }
  }
  return 0;
}

int bpf_detach_internal(struct UbpfTracer *tracer, const char *function_name,
                        const char *bpf_filename, void (*print_fn)(char *str)) {
  uint64_t fun_addr = get_function_address(tracer, function_name);
  if (fun_addr == 0) {
    print_fn(ERR("Function not found\n"));
    return 1;
  }

  uint64_t nop_addr = get_nop_address(tracer, fun_addr);
  if (nop_addr == 0) {
    print_fn(ERR("Function not traced\n"));
    return 1;
  }

  // replace call with nop again
  memcpy((void *)nop_addr, nopl, sizeof(nopl));

  // remove entry from nop map
  hmap_del(tracer->nop_map, fun_addr);

  // remove entry from VM map
  hmap_del(tracer->vm_map, nop_addr + CALL_INSTRUCTION_SIZE);

  // remove entry from function names
  hmap_del(tracer->function_names, nop_addr + CALL_INSTRUCTION_SIZE);
  return 0;
}

int bpf_attach(const char *function_name, const char *bpf_filename,
               void (*print_fn)(char *str)) {
  return bpf_attach_internal(get_tracer(), function_name, bpf_filename,
                             print_fn);
}

int bpf_list(const char *function_name, void (*print_fn)(char *str)) {
  return bpf_list_internal(get_tracer(), function_name, print_fn);
}

int bpf_detach(const char *function_name, const char *bpf_filename,
               void (*print_fn)(char *str)) {
  return bpf_detach_internal(get_tracer(), function_name, bpf_filename,
                             print_fn);
}

