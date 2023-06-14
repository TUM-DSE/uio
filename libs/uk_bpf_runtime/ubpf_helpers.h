#ifndef UBPF_HELPERS_H
#define UBPF_HELPERS_H

#include "helper_function_list.h"
#include "prog_type_list.h"

HelperFunctionList *init_builtin_bpf_helpers();
void print_helper_specs(void (*print_fn)(const char *));
void print_prog_type_infos(void (*print_fn)(const char *));

#endif /* UBPF_HELPERS_H */
