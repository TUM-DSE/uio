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
					  EBPF_RETURN_TYPE_INTEGER, 0, NULL);

	// bpf_map_get
	ebpf_argument_type_t args_bpf_map_get[] = {
	    EBPF_ARGUMENT_TYPE_CONST_SIZE_OR_ZERO,
	    EBPF_ARGUMENT_TYPE_CONST_SIZE_OR_ZERO,
	};
	helper_function_list_emplace_back(
	    g_bpf_helper_functions, 1, "bpf_map_get", bpf_map_get,
	    EBPF_RETURN_TYPE_INTEGER,
	    sizeof(args_bpf_map_get) / sizeof(ebpf_argument_type_t),
	    args_bpf_map_get);

	// bpf_map_put
	ebpf_argument_type_t args_bpf_map_put[] = {
	    EBPF_ARGUMENT_TYPE_CONST_SIZE_OR_ZERO,
	    EBPF_ARGUMENT_TYPE_CONST_SIZE_OR_ZERO,
	    EBPF_ARGUMENT_TYPE_CONST_SIZE_OR_ZERO,
	};
	helper_function_list_emplace_back(
	    g_bpf_helper_functions, 2, "bpf_map_put", bpf_map_put,
	    EBPF_RETURN_TYPE_INTEGER,
	    sizeof(args_bpf_map_put) / sizeof(ebpf_argument_type_t),
	    args_bpf_map_put);

	// bpf_map_del
	ebpf_argument_type_t args_bpf_map_del[] = {
	    EBPF_ARGUMENT_TYPE_CONST_SIZE_OR_ZERO,
	    EBPF_ARGUMENT_TYPE_CONST_SIZE_OR_ZERO,
	};
	helper_function_list_emplace_back(
	    g_bpf_helper_functions, 3, "bpf_map_del", bpf_map_del,
	    EBPF_RETURN_TYPE_INTEGER,
	    sizeof(args_bpf_map_del) / sizeof(ebpf_argument_type_t),
	    args_bpf_map_del);

	// bpf_get_addr
	ebpf_argument_type_t args_bpf_get_addr[] = {
	    EBPF_ARGUMENT_TYPE_PTR_TO_CTX,
	};
	helper_function_list_emplace_back(
	    g_bpf_helper_functions, 4, "bpf_get_addr", bpf_get_addr,
	    EBPF_RETURN_TYPE_INTEGER,
	    sizeof(args_bpf_get_addr) / sizeof(ebpf_argument_type_t),
	    args_bpf_get_addr);

	// bpf_probe_read
	ebpf_argument_type_t args_bpf_probe_read[] = {
	    EBPF_ARGUMENT_TYPE_PTR_TO_READABLE_MEM,
	    EBPF_ARGUMENT_TYPE_CONST_SIZE,
	};
	helper_function_list_emplace_back(
	    g_bpf_helper_functions, 5, "bpf_probe_read", bpf_probe_read,
	    EBPF_RETURN_TYPE_INTEGER,
	    sizeof(args_bpf_probe_read) / sizeof(ebpf_argument_type_t),
	    args_bpf_probe_read);

	// bpf_time_get_ns
	helper_function_list_emplace_back(g_bpf_helper_functions, 3,
					  "bpf_time_get_ns", bpf_time_get_ns,
					  EBPF_RETURN_TYPE_INTEGER, 0, NULL);

	// bpf_unwind
	ebpf_argument_type_t args_bpf_unwind[] = {
	    EBPF_ARGUMENT_TYPE_CONST_SIZE_OR_ZERO,
	};
	helper_function_list_emplace_back(
	    g_bpf_helper_functions, 6, "bpf_unwind", bpf_unwind,
	    EBPF_RETURN_TYPE_INTEGER_OR_NO_RETURN_IF_SUCCEED,
	    sizeof(args_bpf_unwind) / sizeof(ebpf_argument_type_t),
	    args_bpf_unwind);

	// bpf_puts
	ebpf_argument_type_t args_bpf_puts[] = {
	    EBPF_ARGUMENT_TYPE_PTR_TO_CTX,
	};
	helper_function_list_emplace_back(g_bpf_helper_functions, 7, "bpf_puts",
					  bpf_puts, EBPF_RETURN_TYPE_INTEGER,
					  sizeof(args_bpf_puts)
					      / sizeof(ebpf_argument_type_t),
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
