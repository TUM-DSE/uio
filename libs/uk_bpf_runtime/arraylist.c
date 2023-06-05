#include "arraylist.h"

#include <stdlib.h>
#include <string.h>

struct ArrayListWithLabels *
list_init(uint64_t capacity, void (*destruct_entry)(struct LabeledEntry *))
{
	struct ArrayListWithLabels *list =
	    malloc(sizeof(struct ArrayListWithLabels));
	list->m_Length = 0;
	list->m_Capacity = capacity;
	list->m_List = malloc(sizeof(struct LabeledEntry) * capacity);
	list->destruct_entry = destruct_entry;
	return list;
}

void list_resize(struct ArrayListWithLabels *list, uint64_t new_capacity)
{
	if (list->m_Capacity == new_capacity)
		return;

	if (list->m_Length > new_capacity) {
		for (uint64_t i = new_capacity; i < list->m_Length; ++i) {
			list->destruct_entry(&list->m_List[i]);
		}
		list->m_Length = new_capacity;
	}

	if (new_capacity > 0) {
		list->m_List = realloc(list->m_List, sizeof(struct LabeledEntry)
							 * new_capacity);
	}

	list->m_Capacity = new_capacity;
}

void list_add_elem(struct ArrayListWithLabels *list, const char *label,
		   void *value)
{
	while (list->m_Length >= list->m_Capacity) {
		uint64_t new_capacity = list->m_Capacity * 2;
		if (new_capacity == 0)
			new_capacity = 1;
		list_resize(list, new_capacity);
	}

	list->m_List[list->m_Length].m_Label = malloc(strlen(label));
	strcpy(list->m_List[list->m_Length].m_Label, label);
	list->m_List[list->m_Length].m_Value = value;

	list->m_Length += 1;
}

void list_apply_function(struct ArrayListWithLabels *list,
			 void (*f)(struct LabeledEntry *))
{
	for (uint64_t i = 0; i < list->m_Length; ++i) {
		f(&(list->m_List[i]));
	}
}

void list_remove_elem(struct ArrayListWithLabels *list, const char *label)
{
	uint64_t cnt_removed = 0;
	uint64_t *remove_elems = malloc(list->m_Length * sizeof(uint64_t));
	for (uint64_t i = 0; i < list->m_Length; ++i) {
		if (!strcmp(list->m_List[i].m_Label, label)) {
			list->destruct_entry(&list->m_List[i]);
			cnt_removed++;
		}
		remove_elems[i] = cnt_removed;
	}
	for (uint64_t i = list->m_Length; i > 0; --i) {
		uint64_t current = i - 1;
		if (remove_elems[current] > 0) {
			list->m_List[current - remove_elems[current]].m_Label =
			    list->m_List[current].m_Label;
			list->m_List[current - remove_elems[current]].m_Value =
			    list->m_List[current].m_Value;
		}
	}
}

void list_print(struct ArrayListWithLabels *list,
		void (*print_entry)(struct LabeledEntry *))
{
	list_apply_function(list, print_entry);
}

void list_destroy(struct ArrayListWithLabels *list)
{
	list_resize(list, 0);
	free(list->m_List);
	list->m_List = NULL;
}
