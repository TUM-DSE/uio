#include <cstdlib>
#include <cassert>

extern "C" {
#include "prog_type_list.h"
}


void assert_create_empty_list() {
    BpfProgTypeList *list = bpf_prog_type_list_init();
    assert(list != nullptr);
    assert(list->m_length == 0);
    assert(list->m_head == nullptr);
    assert(list->m_tail == nullptr);
    bpf_prog_type_list_destroy(list);
}

void assert_create_list() {
    BpfProgTypeList *list = bpf_prog_type_list_init();
    assert(list != nullptr);
    assert(list->m_length == 0);
    assert(list->m_head == nullptr);
    assert(list->m_tail == nullptr);

    UK_UBPF_INDEX_t indexes[] = {1, 2};
    auto *entry = bpf_prog_type_list_emplace_back(list, "prog_type1", 2, indexes);

    assert(list->m_length == 1);
    assert(list->m_head == entry);
    assert(list->m_tail == entry);

    assert(entry->m_next == nullptr);
    assert(entry->m_length == 2);
    assert(strcmp(entry->m_prog_type_name, "prog_type1") == 0);
    assert(entry->m_allowed_helper_indexes[0] == 1);
    assert(entry->m_allowed_helper_indexes[1] == 2);

    bpf_prog_type_list_destroy(list);
}

void assert_create_multiple_list() {
    BpfProgTypeList *list = bpf_prog_type_list_init();
    assert(list != nullptr);
    assert(list->m_length == 0);
    assert(list->m_head == nullptr);
    assert(list->m_tail == nullptr);

    UK_UBPF_INDEX_t indexes[] = {1, 2};
    auto *entry = bpf_prog_type_list_emplace_back(list, "prog_type1/12/", 2, indexes);

    assert(list->m_length == 1);
    assert(list->m_head == entry);
    assert(list->m_tail == entry);

    assert(entry->m_next == nullptr);
    assert(entry->m_length == 2);
    assert(strcmp(entry->m_prog_type_name, "prog_type1/12/") == 0);
    assert(entry->m_allowed_helper_indexes[0] == 1);
    assert(entry->m_allowed_helper_indexes[1] == 2);

    UK_UBPF_INDEX_t indexes2[] = {3, 4};
    auto *entry2 = bpf_prog_type_list_emplace_back(list, "prog_type2/34/", 2, indexes2);

    assert(list->m_length == 2);
    assert(list->m_head == entry);
    assert(list->m_tail == entry2);

    assert(entry->m_next == entry2);
    assert(entry->m_length == 2);
    assert(strcmp(entry->m_prog_type_name, "prog_type1/12/") == 0);
    assert(entry->m_allowed_helper_indexes[0] == 1);
    assert(entry->m_allowed_helper_indexes[1] == 2);

    assert(entry2->m_next == nullptr);
    assert(entry2->m_length == 2);
    assert(strcmp(entry2->m_prog_type_name, "prog_type2/34/") == 0);
    assert(entry2->m_allowed_helper_indexes[0] == 3);
    assert(entry2->m_allowed_helper_indexes[1] == 4);


    bpf_prog_type_list_destroy(list);
}


int main() {
    assert_create_empty_list();
    assert_create_list();
    assert_create_multiple_list();

    exit(EXIT_SUCCESS);
}