#include "ubpf_helpers.h"
#include "uk_builtin_bpf_helpers.h"

#include <uk_bpf_helper_utils.h>
#include <uk_program_types.h>

HelperFunctionList *g_bpf_helper_functions = NULL;
BpfProgTypeList *g_bpf_prog_types = NULL;

/**
 * This function initializes the builtin helper function list if it is not
 * ready.
 * @return The helper function list.
 */
HelperFunctionList *init_builtin_bpf_helpers() {
    if (g_bpf_helper_functions) {
        return g_bpf_helper_functions;
    }

    g_bpf_helper_functions = helper_function_list_init();
    g_bpf_prog_types = bpf_prog_type_list_init();

    // add program types
    uint64_t counter = 1;
    bpf_prog_type_list_emplace_back(g_bpf_prog_types, counter++, "tracer", false,
                                    sizeof(prog_with_argument_t),
                                    offsetof(prog_with_argument_t, data_ptr),
                                    offsetof(prog_with_argument_t, data_end_ptr),
                                    offsetof(prog_with_argument_t, ctx_metadata));

    // add builtin helper functions
    // bpf_map_noop
    helper_function_list_emplace_back(g_bpf_helper_functions, 0, UK_EBPF_PROG_TYPE_UNSPECIFIED,
                                      "bpf_map_noop", bpf_map_noop,
                                      UK_EBPF_RETURN_TYPE_INTEGER, 0, NULL);

    // bpf_map_get
    uk_ebpf_argument_type_t args_bpf_map_get[] = {
            EBPF_ARGUMENT_TYPE_ANYTHING,
            EBPF_ARGUMENT_TYPE_ANYTHING,
    };
    helper_function_list_emplace_back(
            g_bpf_helper_functions, 1, UK_EBPF_PROG_TYPE_UNSPECIFIED, "bpf_map_get", bpf_map_get,
            UK_EBPF_RETURN_TYPE_INTEGER,
            sizeof(args_bpf_map_get) / sizeof(uk_ebpf_argument_type_t),
            args_bpf_map_get);

    // bpf_map_put
    uk_ebpf_argument_type_t args_bpf_map_put[] = {
            EBPF_ARGUMENT_TYPE_ANYTHING,
            EBPF_ARGUMENT_TYPE_ANYTHING,
            EBPF_ARGUMENT_TYPE_ANYTHING,
    };
    helper_function_list_emplace_back(
            g_bpf_helper_functions, 2, UK_EBPF_PROG_TYPE_UNSPECIFIED, "bpf_map_put", bpf_map_put,
            UK_EBPF_RETURN_TYPE_INTEGER,
            sizeof(args_bpf_map_put) / sizeof(uk_ebpf_argument_type_t),
            args_bpf_map_put);

    // bpf_map_del
    uk_ebpf_argument_type_t args_bpf_map_del[] = {
            EBPF_ARGUMENT_TYPE_ANYTHING,
            EBPF_ARGUMENT_TYPE_ANYTHING,
    };
    helper_function_list_emplace_back(
            g_bpf_helper_functions, 3, UK_EBPF_PROG_TYPE_UNSPECIFIED, "bpf_map_del", bpf_map_del,
            UK_EBPF_RETURN_TYPE_INTEGER,
            sizeof(args_bpf_map_del) / sizeof(uk_ebpf_argument_type_t),
            args_bpf_map_del);

    // bpf_get_addr
    /*
    // TODO
    uk_ebpf_argument_type_t args_bpf_get_addr[] = {
            UK_EBPF_ARGUMENT_TYPE_PTR_TO_READABLE_MEM,
    };
    helper_function_list_emplace_back(
            g_bpf_helper_functions, 4, "bpf_get_addr", bpf_get_addr,
            UK_EBPF_RETURN_TYPE_INTEGER,
            sizeof(args_bpf_get_addr) / sizeof(uk_ebpf_argument_type_t),
            args_bpf_get_addr);
    */
    // bpf_probe_read
    /*
    // TODO
    uk_ebpf_argument_type_t args_bpf_probe_read[] = {
            EBPF_ARGUMENT_TYPE_PTR_TO_CTX,
            UK_EBPF_ARGUMENT_TYPE_CONST_SIZE,
    };
    helper_function_list_emplace_back(
            g_bpf_helper_functions, 5, "bpf_probe_read", bpf_probe_read,
            UK_EBPF_RETURN_TYPE_INTEGER,
            sizeof(args_bpf_probe_read) / sizeof(uk_ebpf_argument_type_t),
            args_bpf_probe_read);
    */
    // bpf_time_get_ns
    helper_function_list_emplace_back(g_bpf_helper_functions, 6,
                                      UK_EBPF_PROG_TYPE_UNSPECIFIED,
                                      "bpf_time_get_ns", bpf_time_get_ns,
                                      UK_EBPF_RETURN_TYPE_INTEGER, 0,
                                      NULL);

    // bpf_unwind
    uk_ebpf_argument_type_t args_bpf_unwind[] = {
            EBPF_ARGUMENT_TYPE_ANYTHING,
    };
    helper_function_list_emplace_back(
            g_bpf_helper_functions, 7, UK_EBPF_PROG_TYPE_UNSPECIFIED,
            "bpf_unwind", bpf_unwind,
            UK_EBPF_RETURN_TYPE_INTEGER_OR_NO_RETURN_IF_SUCCEED,
            sizeof(args_bpf_unwind) / sizeof(uk_ebpf_argument_type_t),
            args_bpf_unwind);

    // bpf_puts
    uk_ebpf_argument_type_t args_bpf_puts[] = {
            UK_EBPF_ARGUMENT_TYPE_PTR_TO_READABLE_MEM,
    };
    helper_function_list_emplace_back(g_bpf_helper_functions, 8,
                                      UK_EBPF_PROG_TYPE_UNSPECIFIED,
                                      "bpf_puts", bpf_puts,
                                      UK_EBPF_RETURN_TYPE_INTEGER,
                                      sizeof(args_bpf_puts)
                                      / sizeof(uk_ebpf_argument_type_t),
                                      args_bpf_puts);

    return g_bpf_helper_functions;
}

void print_helper_specs(void (*print_fn)(const char *)) {
    if (g_bpf_helper_functions) {
        marshall_bpf_helper_definitions(g_bpf_helper_functions, print_fn);
    }
}

void print_prog_type_infos(void (*print_fn)(const char *)) {
    if (g_bpf_prog_types) {
        marshall_bpf_prog_types(g_bpf_prog_types, print_fn);
    }
}