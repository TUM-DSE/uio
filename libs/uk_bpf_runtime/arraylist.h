#ifndef ARRAYLIST_H
#define ARRAYLIST_H

#include <stdbool.h>
#include <stdint.h>

struct LabeledEntry {
	char *m_Label;
	void *m_Value;
};

struct ArrayListWithLabels {
	uint64_t m_Length;
	uint64_t m_Capacity;
	struct LabeledEntry *m_List;
	void (*destruct_entry)(struct LabeledEntry *);
};

struct ArrayListWithLabels *
list_init(uint64_t capacity, void (*destruct_entry)(struct LabeledEntry *));

void list_resize(struct ArrayListWithLabels *list, uint64_t new_capacity);

void list_add_elem(struct ArrayListWithLabels *list, const char *label,
		   void *value);

void list_apply_function(struct ArrayListWithLabels *list,
			 void (*f)(struct LabeledEntry *));

void list_remove_elem(struct ArrayListWithLabels *list, const char *label);

void list_print(struct ArrayListWithLabels *list,
		void (*print_entry)(struct LabeledEntry *));

void list_destroy(struct ArrayListWithLabels *list);

#endif /* ARRAYLIST_H */
