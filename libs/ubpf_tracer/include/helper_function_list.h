#ifndef HELPER_FUNCTION_LIST_H
#define HELPER_FUNCTION_LIST_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "ebpf_helper_model.h"

typedef uint8_t UK_EBPF_HELPER_ARG_TYPE_NUM_t;

#define UK_EBPF_HELPER_FUNCTION_MAX_NAME_LENGTH 256

typedef unsigned int UK_UBPF_INDEX_t;

typedef struct HelperFunctionSignature {
	char *m_function_name;
	ebpf_return_type_t m_return_type;
	UK_EBPF_HELPER_ARG_TYPE_NUM_t m_num_args;
	ebpf_argument_type_t m_arg_types[];
} HelperFunctionSignature;

typedef struct HelperFunctionEntry HelperFunctionEntry;
typedef struct HelperFunctionEntry {
	HelperFunctionEntry *m_next;

	UK_UBPF_INDEX_t m_index;
	const void *m_function_addr;
	HelperFunctionSignature m_function_signature;
} HelperFunctionEntry;

/**
 * An linked list of helper function information.
 */
typedef struct HelperFunctionList {
	size_t m_length;
	HelperFunctionEntry *m_head;
	HelperFunctionEntry *m_tail;

	HelperFunctionEntry *(*m_entry_constructor)(
	    UK_UBPF_INDEX_t index, const char *function_name,
	    const void *function_addr, const ebpf_return_type_t ret_type,
	    const UK_EBPF_HELPER_ARG_TYPE_NUM_t argTypeCount,
	    const ebpf_argument_type_t argTypes[]);

	void (*m_entry_destructor)(struct HelperFunctionEntry *entry);
} HelperFunctionList;

HelperFunctionList *helper_function_list_init(
    HelperFunctionEntry *(*entry_constructor)(
	UK_UBPF_INDEX_t index, const char *function_name,
	const void *function_addr, const ebpf_return_type_t ret_type,
	const UK_EBPF_HELPER_ARG_TYPE_NUM_t arg_type_count,
	const ebpf_argument_type_t arg_types[]),
    void (*destruct_entry)(struct HelperFunctionEntry *));

bool helper_function_list_emplace_back(
    HelperFunctionList *self, UK_UBPF_INDEX_t index, const char *function_name,
    const void *function_addr, ebpf_return_type_t ret_type,
    UK_EBPF_HELPER_ARG_TYPE_NUM_t arg_type_count,
    const ebpf_argument_type_t arg_types[]);

void helper_function_list_remove_elem(HelperFunctionList *self,
				      const char *function_name);

void helper_function_list_apply_function(
    HelperFunctionList *self, void (*apply)(struct HelperFunctionEntry *entry));

void helper_function_list_print(
    HelperFunctionList *self,
    void (*printer)(struct HelperFunctionEntry *entry));

void helper_function_list_destroy(HelperFunctionList *self);

#endif /* HELPER_FUNCTION_LIST_H */
