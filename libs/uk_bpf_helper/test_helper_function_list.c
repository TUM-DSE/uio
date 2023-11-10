#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "helper_function_list.h"


void test_create_new_list() {
	HelperFunctionList *list = helper_function_list_init();
	assert(list);
	assert(list->m_length == 0);
	assert(list->m_head == NULL);
	assert(list->m_tail == NULL);
	helper_function_list_destroy(list);
}

void test_push_back_work() {
	HelperFunctionList *list = helper_function_list_init();
	uk_ebpf_argument_type_t dummy_arg_types[] = {4, 5, 6};
	assert(helper_function_list_emplace_back(list, 1, 10, "test", (void *) 42, 7,
					      sizeof(dummy_arg_types) / sizeof(uk_ebpf_argument_type_t), dummy_arg_types));

	assert(list->m_length == 1);
	assert(list->m_head != NULL && list->m_head == list->m_tail);
	assert(list->m_head->m_index == 1);
    assert(list->m_head->m_prog_type_id == 10);
	assert(list->m_head->m_function_addr == (void *) 42);
	assert(strcmp(list->m_head->m_function_signature.m_function_name, "test") == 0);
	assert(list->m_head->m_function_signature.m_return_type == 7);
	assert(list->m_head->m_function_signature.m_num_args == 3);
	assert(list->m_head->m_function_signature.m_arg_types[0] == 4);
	assert(list->m_head->m_function_signature.m_arg_types[1] == 5);
	assert(list->m_head->m_function_signature.m_arg_types[2] == 6);

	helper_function_list_destroy(list);
}


HelperFunctionList *helper_generate_dummy_data() {
	HelperFunctionList *list = helper_function_list_init();
    uk_ebpf_argument_type_t dummy_arg_types[] = {4, 5, 6};

	assert(helper_function_list_emplace_back(list, 11, 101,
                                             "test0", (void *) 42, 0, sizeof(dummy_arg_types), dummy_arg_types));
	assert(helper_function_list_emplace_back(list, 12, 102,
                                             "test1", (void *) 42, 1, sizeof(dummy_arg_types), dummy_arg_types));
	assert(helper_function_list_emplace_back(list, 13, 103,
                                             "test2", (void *) 42, 2, sizeof(dummy_arg_types), dummy_arg_types));
	assert(helper_function_list_emplace_back(list, 14, 104,
                                             "test3", (void *) 42, 3, sizeof(dummy_arg_types), dummy_arg_types));
	assert(helper_function_list_emplace_back(list, 15, 105,
                                             "test4", (void *) 42, 4, sizeof(dummy_arg_types), dummy_arg_types));
	assert(helper_function_list_emplace_back(list, 16, 106,
                                             "test5", (void *) 42, 5, sizeof(dummy_arg_types), dummy_arg_types));
	assert(helper_function_list_emplace_back(list, 17, 107,
                                             "test6", (void *) 42, 6, sizeof(dummy_arg_types), dummy_arg_types));

	HelperFunctionEntry *elem = list->m_head;
	assert(strcmp(elem->m_function_signature.m_function_name, "test0") == 0);

	elem = elem->m_next;
	assert(strcmp(elem->m_function_signature.m_function_name, "test1") == 0);

	elem = elem->m_next;
	assert(strcmp(elem->m_function_signature.m_function_name, "test2") == 0);

	elem = elem->m_next;
	assert(strcmp(elem->m_function_signature.m_function_name, "test3") == 0);

	elem = elem->m_next;
	assert(strcmp(elem->m_function_signature.m_function_name, "test4") == 0);

	elem = elem->m_next;
	assert(strcmp(elem->m_function_signature.m_function_name, "test5") == 0);

	elem = elem->m_next;
	assert(strcmp(elem->m_function_signature.m_function_name, "test6") == 0);

	assert(list->m_head->m_index == 11);
	assert(list->m_tail->m_index == 17);

	int index = 0;
	for (HelperFunctionEntry *entry = list->m_head; entry != NULL; entry = entry->m_next) {
		assert(entry->m_index == 11 + index);
        assert(entry->m_prog_type_id == 101 + index);
		index++;
	}

	assert(list->m_length == 7);
	assert(index == list->m_length);

	return list;
}

void test_independent_lists_work() {
	HelperFunctionList *list1 = helper_generate_dummy_data();
	HelperFunctionList *list2 = helper_generate_dummy_data();

	assert(list1->m_length == 7);
	assert(list2->m_length == 7);

	list1->m_head->m_function_signature.m_function_name[0] = 'a';
	assert(strcmp(list1->m_head->m_function_signature.m_function_name, "aest0") == 0);
	assert(strcmp(list2->m_head->m_function_signature.m_function_name, "test0") == 0);

	helper_function_list_destroy(list1);
	helper_function_list_destroy(list2);
}

int main() {
	test_create_new_list();
	test_push_back_work();

	test_independent_lists_work();

	exit(EXIT_SUCCESS);
}
