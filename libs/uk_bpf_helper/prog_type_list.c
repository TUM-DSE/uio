#include "prog_type_list.h"

#include <stdlib.h>

BpfProgTypeList *bpf_prog_type_list_init() {
    BpfProgTypeList *list = malloc(sizeof(BpfProgTypeList));
    if (list == NULL) {
        return NULL;
    }

    list->m_length = 0;
    list->m_head = NULL;
    list->m_tail = NULL;

    return list;
}


BpfProgType *bpf_prog_type_list_emplace_back(BpfProgTypeList *self, uint64_t prog_type_id,
                                             const char *prog_type_name, bool privileged,
                                             int ctx_descriptor_struct_size, int offset_to_data_ptr,
                                             int offset_to_data_end_ptr, int offset_to_ctx_metadata) {
    BpfProgType *entry = malloc(sizeof(BpfProgType) + strlen(prog_type_name) + 1);

    if (entry == NULL) {
        return NULL;
    }

    entry->m_next = NULL;

    strncpy(entry->m_prog_type_name, prog_type_name, strlen(prog_type_name) + 1);

    entry->prog_type_id = prog_type_id;
    entry->privileged = privileged;

    entry->ctx_descriptor_struct_size = ctx_descriptor_struct_size;
    entry->offset_to_data_ptr = offset_to_data_ptr;
    entry->offset_to_data_end_ptr = offset_to_data_end_ptr;
    entry->offset_to_ctx_metadata = offset_to_ctx_metadata;


    // push back the entry to self
    if (self->m_length == 0) {
        self->m_head = entry;
        self->m_tail = entry;
    } else {
        self->m_tail->m_next = entry;
        self->m_tail = entry;
    }

    self->m_length++;

    return entry;
}

void bpf_prog_type_list_destroy(BpfProgTypeList *self) {
    for (BpfProgType *entry = self->m_head; entry != NULL;) {
        BpfProgType *next = entry->m_next;

        free(entry);
        entry = next;
    }

    self->m_length = 0;
    self->m_head = NULL;
    self->m_tail = NULL;

    free(self);
}