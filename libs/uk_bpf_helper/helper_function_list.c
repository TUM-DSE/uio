#include "helper_function_list.h"

#include <stdlib.h>

static HelperFunctionEntry *helper_function_entry_constructor(
        const UK_UBPF_INDEX_t index, const uint64_t prog_type_id,
        const char *function_name, const void *function_addr, const uk_ebpf_return_type_t ret_type,
        const UK_EBPF_HELPER_ARG_TYPE_NUM_t arg_type_count,
        const uk_ebpf_argument_type_t arg_types[]) {
    HelperFunctionEntry *entry =
            malloc(sizeof(HelperFunctionEntry)
                   + arg_type_count * sizeof(uk_ebpf_argument_type_t));
    if (entry == NULL) {
        return NULL;
    }

    entry->m_index = index;
    entry->m_prog_type_id = prog_type_id;

    entry->m_function_addr = function_addr;

    entry->m_function_signature.m_function_name =
            malloc(strlen(function_name) + 1);
    strncpy(entry->m_function_signature.m_function_name, function_name,
            strlen(function_name) + 1);
    entry->m_function_signature.m_return_type = ret_type;
    entry->m_function_signature.m_num_args = arg_type_count;

    if (arg_types != NULL) {
        for (int i = 0; i < arg_type_count; ++i) {
            entry->m_function_signature.m_arg_types[i] = arg_types[i];
        }
    }

    return entry;
}

static void helper_function_entry_destructor(HelperFunctionEntry *entry) {
    free(entry->m_function_signature.m_function_name);
    free(entry);
}

HelperFunctionList *helper_function_list_init() {
    HelperFunctionList *instance = malloc(sizeof(HelperFunctionList));

    if (instance == NULL) {
        return NULL;
    }

    instance->m_length = 0;
    instance->m_head = NULL;
    instance->m_tail = NULL;

    return instance;
}

HelperFunctionEntry *helper_function_list_emplace_back(
        HelperFunctionList *self, UK_UBPF_INDEX_t index, const uint64_t prog_type_id,
        const char *functionName, const void *functionAddr, const uk_ebpf_return_type_t retType,
        const UK_EBPF_HELPER_ARG_TYPE_NUM_t arg_type_count,
        const uk_ebpf_argument_type_t argTypes[]) {

    HelperFunctionEntry *entry = helper_function_entry_constructor(
            index, prog_type_id, functionName, functionAddr, retType, arg_type_count, argTypes);

    if (entry == NULL) {
        return NULL;
    }

    entry->m_next = NULL;

    if (self->m_length == 0) {
        self->m_head = entry;
    } else {
        self->m_tail->m_next = entry;
    }

    self->m_tail = entry;

    self->m_length++;

    return entry;
}

void helper_function_list_destroy(HelperFunctionList *self) {
    for (HelperFunctionEntry *entry = self->m_head; entry != NULL;) {
        HelperFunctionEntry *next = entry->m_next;
        helper_function_entry_destructor(entry);
        entry = next;
    }

    free(self);
}