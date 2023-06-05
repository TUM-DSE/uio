#include "ubpf_helpers.h"

#include <uk_bpf_helper_utils.h>
#include "uk_builtin_bpf_helpers.h"

HelperFunctionList *g_bpf_helper_functions = NULL;
HelperFunctionList *g_additional_helpers = NULL;

/**
 * This function initializes the builtin helper function list if it is not
 * ready.
 * @return The helper function list.
 */
HelperFunctionList *init_builtin_bpf_helpers()
{
	// TODO add mpk protection back
	if (g_bpf_helper_functions) {
		return g_bpf_helper_functions;
	}

	g_bpf_helper_functions = helper_function_list_init();

	// add builtin helper functions
	// bpf_map_noop
	helper_function_list_emplace_back(g_bpf_helper_functions, 0,
					  "bpf_map_noop", bpf_map_noop,
					  UK_EBPF_RETURN_TYPE_INTEGER, 0, NULL);

	// bpf_map_get
	uk_ebpf_argument_type_t args_bpf_map_get[] = {
	    UK_EBPF_ARGUMENT_TYPE_CONST_SIZE_OR_ZERO,
	    UK_EBPF_ARGUMENT_TYPE_CONST_SIZE_OR_ZERO,
	};
	helper_function_list_emplace_back(
	    g_bpf_helper_functions, 1, "bpf_map_get", bpf_map_get,
	    UK_EBPF_RETURN_TYPE_INTEGER,
	    sizeof(args_bpf_map_get) / sizeof(uk_ebpf_argument_type_t),
	    args_bpf_map_get);

	// bpf_map_put
	uk_ebpf_argument_type_t args_bpf_map_put[] = {
	    UK_EBPF_ARGUMENT_TYPE_CONST_SIZE_OR_ZERO,
	    UK_EBPF_ARGUMENT_TYPE_CONST_SIZE_OR_ZERO,
	    UK_EBPF_ARGUMENT_TYPE_CONST_SIZE_OR_ZERO,
	};
	helper_function_list_emplace_back(
	    g_bpf_helper_functions, 2, "bpf_map_put", bpf_map_put,
	    UK_EBPF_RETURN_TYPE_INTEGER,
	    sizeof(args_bpf_map_put) / sizeof(uk_ebpf_argument_type_t),
	    args_bpf_map_put);

	// bpf_map_del
	uk_ebpf_argument_type_t args_bpf_map_del[] = {
	    UK_EBPF_ARGUMENT_TYPE_CONST_SIZE_OR_ZERO,
	    UK_EBPF_ARGUMENT_TYPE_CONST_SIZE_OR_ZERO,
	};
	helper_function_list_emplace_back(
	    g_bpf_helper_functions, 3, "bpf_map_del", bpf_map_del,
	    UK_EBPF_RETURN_TYPE_INTEGER,
	    sizeof(args_bpf_map_del) / sizeof(uk_ebpf_argument_type_t),
	    args_bpf_map_del);

	// bpf_get_addr
	uk_ebpf_argument_type_t args_bpf_get_addr[] = {
	    UK_EBPF_ARGUMENT_TYPE_PTR_TO_CTX,
	};
	helper_function_list_emplace_back(
	    g_bpf_helper_functions, 4, "bpf_get_addr", bpf_get_addr,
	    UK_EBPF_RETURN_TYPE_INTEGER,
	    sizeof(args_bpf_get_addr) / sizeof(uk_ebpf_argument_type_t),
	    args_bpf_get_addr);

	// bpf_probe_read
	uk_ebpf_argument_type_t args_bpf_probe_read[] = {
	    UK_EBPF_ARGUMENT_TYPE_PTR_TO_READABLE_MEM,
	    UK_EBPF_ARGUMENT_TYPE_CONST_SIZE,
	};
	helper_function_list_emplace_back(
	    g_bpf_helper_functions, 5, "bpf_probe_read", bpf_probe_read,
	    UK_EBPF_RETURN_TYPE_INTEGER,
	    sizeof(args_bpf_probe_read) / sizeof(uk_ebpf_argument_type_t),
	    args_bpf_probe_read);

	// bpf_time_get_ns
	helper_function_list_emplace_back(g_bpf_helper_functions, 3,
					  "bpf_time_get_ns", bpf_time_get_ns,
					  UK_EBPF_RETURN_TYPE_INTEGER, 0, NULL);

	// bpf_unwind
	uk_ebpf_argument_type_t args_bpf_unwind[] = {
	    UK_EBPF_ARGUMENT_TYPE_CONST_SIZE_OR_ZERO,
	};
	helper_function_list_emplace_back(
	    g_bpf_helper_functions, 6, "bpf_unwind", bpf_unwind,
	    UK_EBPF_RETURN_TYPE_INTEGER_OR_NO_RETURN_IF_SUCCEED,
	    sizeof(args_bpf_unwind) / sizeof(uk_ebpf_argument_type_t),
	    args_bpf_unwind);

	// bpf_puts
	uk_ebpf_argument_type_t args_bpf_puts[] = {
	    UK_EBPF_ARGUMENT_TYPE_PTR_TO_CTX,
	};
	helper_function_list_emplace_back(g_bpf_helper_functions, 7, "bpf_puts",
					  bpf_puts, UK_EBPF_RETURN_TYPE_INTEGER,
					  sizeof(args_bpf_puts)
					      / sizeof(uk_ebpf_argument_type_t),
					  args_bpf_puts);

	return g_bpf_helper_functions;
}

void additional_helpers_list_add(const char *label, void *function_ptr)
{
	// TODO
	/*if (additional_helpers == NULL) {
		additional_helpers = init_helper_list();
	}

	list_add_elem(additional_helpers, label, function_ptr);*/
}

void additional_helpers_list_del(const char *label)
{
	// TODO
	/*
	if (additional_helpers != NULL) {
		list_remove_elem(additional_helpers, label);
	}
	 */
}

void print_helper_specs(void (*print_fn)(const char *))
{
	HelperFunctionList *list = init_builtin_bpf_helpers();
	marshall_bpf_helper_definitions(list, print_fn);
}
