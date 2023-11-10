#ifndef USHELL_TERMINAL_PROG_TYPE_LIST_H
#define USHELL_TERMINAL_PROG_TYPE_LIST_H

#include "helper_function_list.h"

#include <stdint.h>
#include <string.h>

typedef struct BpfProgType BpfProgType;
typedef struct BpfProgType {
    BpfProgType *m_next;

    uint64_t prog_type_id;

    bool privileged;

    int ctx_descriptor_struct_size;
    int offset_to_data_ptr;
    int offset_to_data_end_ptr;
    int offset_to_ctx_metadata;

    char m_prog_type_name[];
} BpfProgType;

/**
 * An linked list of BPF prog_type information.
 */
typedef struct BpfProgTypeList {
    size_t m_length;
    BpfProgType *m_head;
    BpfProgType *m_tail;
} BpfProgTypeList;

BpfProgTypeList *bpf_prog_type_list_init();

BpfProgType *bpf_prog_type_list_emplace_back(BpfProgTypeList *self, uint64_t prog_type_id,
                                             const char *prog_type_name, bool privileged,
                                             int ctx_descriptor_struct_size, int offset_to_data_ptr,
                                             int offset_to_data_end_ptr, int offset_to_ctx_metadata);

void bpf_prog_type_list_destroy(BpfProgTypeList *self);

#endif //USHELL_TERMINAL_PROG_TYPE_LIST_H
