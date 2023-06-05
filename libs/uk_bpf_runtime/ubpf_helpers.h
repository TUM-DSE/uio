#ifndef UBPF_HELPERS_H
#define UBPF_HELPERS_H

#include "helper_function_list.h"

HelperFunctionList *init_builtin_bpf_helpers();
void additional_helpers_list_add(const char *label, void *function_ptr);
void additional_helpers_list_del(const char *label);
void print_helper_specs(void (*print_fn)(const char *));

#endif /* UBPF_HELPERS_H */
