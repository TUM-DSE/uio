#include "ubpf_helpers.h"

#include <ubpf.h>
#include <stdio.h>

// #define UBPF_DEBUG
#ifdef UBPF_DEBUG
#define debug(msg, ...)                                                        \
	do {                                                                   \
		printf("[Debug] %s:%d %s(): ", __FILE__, __LINE__, __func__);  \
		printf(msg "\n", ##__VA_ARGS__);                               \
	} while (0)
#else
#define debug(fmt, ...)                                                        \
	do {                                                                   \
	} while (0)
#endif

struct THashMap *g_bpf_map = NULL;

HelperFunctionList *g_bpf_helper_functions = NULL;
HelperFunctionList *g_additional_helpers = NULL;

void destruct_cell_l2(struct THashCell *cell)
{
	free(cell->m_Value);
}
void *create_cell_l2()
{
	uint64_t *cell = calloc(1, sizeof(uint64_t));
	return cell;
}

void destruct_cell_l1(struct THashCell *cell)
{
	hmap_destroy(cell->m_Value);
	free(cell->m_Value);
}

void *create_cell_l1()
{
	int err = 0;
	return hmap_init(101, &destruct_cell_l2, &create_cell_l2, &err);
}

struct THashMap *init_bpf_map()
{
	int err = 0;
	return hmap_init(101, &destruct_cell_l1, &create_cell_l1, &err);
}

// implementation of the built in helper functions
uint64_t bpf_map_noop()
{
	return 0;
}

#define BPF_MAP_VALUE_UNDEFINED UINT64_MAX // -1

uint64_t bpf_map_get(uint64_t key1, uint64_t key2)
{
	if (g_bpf_map == NULL) {
		g_bpf_map = init_bpf_map();
	}

	uint64_t value = BPF_MAP_VALUE_UNDEFINED;

	struct THmapValueResult *hmap_entry_l1 = hmap_get(g_bpf_map, key1);
	if (hmap_entry_l1->m_Result == HMAP_SUCCESS) {
		struct THmapValueResult *hmap_entry_l2 =
		    hmap_get(hmap_entry_l1->m_Value, key2);
		if (hmap_entry_l2->m_Result == HMAP_SUCCESS) {
			value = *(uint64_t *)hmap_entry_l2->m_Value;
			debug("(GET) bpf_map[%lu][%lu] = %lu\n", key1, key2,
			      value);
		}

		free(hmap_entry_l2);
	}

	free(hmap_entry_l1);

	debug("(GET) bpf_map[%lu][%lu] = X\n", key1, key2);
	return value;
}

uint64_t bpf_map_put(uint64_t key1, uint64_t key2, uint64_t value)
{
	debug("(PUT) bpf_map[%lu][%lu] = %lu\n", key1, key2, value);
	if (g_bpf_map == NULL) {
		g_bpf_map = init_bpf_map();
	}

	uint64_t originalValue = bpf_map_get(key1, key2);

	struct THmapValueResult *hmap_entry_l1 =
	    hmap_get_or_create(g_bpf_map, key1);
	if (hmap_entry_l1->m_Result == HMAP_SUCCESS) {

		uint64_t *value_copy = calloc(1, sizeof(uint64_t));
		*value_copy = value;
		struct THmapValueResult *hmap_entry_l2 =
		    hmap_put(hmap_entry_l1->m_Value, key2, value_copy);
		if (hmap_entry_l2->m_Result != HMAP_SUCCESS) {
			free(value_copy);
		}

		free(hmap_entry_l2);
	}

	free(hmap_entry_l1);

	return originalValue;
}

uint64_t bpf_map_del(uint64_t key1, uint64_t key2)
{
	debug("(DEL) bpf_map[%lu][%lu]", key1, key2);
	if (g_bpf_map == NULL) {
		return BPF_MAP_VALUE_UNDEFINED;
	}

	uint64_t originalValue = bpf_map_get(key1, key2);

	struct THmapValueResult *hmap_entry_l1 = hmap_get(g_bpf_map, key1);
	if (hmap_entry_l1->m_Result != HMAP_NOTFOUND) {
		struct THashMap *map_l2 = hmap_entry_l1->m_Value;

		if (hmap_entry_l1->m_Result == HMAP_SUCCESS) {
			hmap_del(map_l2, key2);
			if (map_l2->m_Elems == 0) {
				hmap_del(g_bpf_map, key1);
			}
		}
	}

	free(hmap_entry_l1);

	return originalValue;
}

void bpf_map_dump(void (*print_fn)(char *str))
{
	if (g_bpf_map == NULL) {
		return;
	}
}

uint64_t bpf_unwind(uint64_t i)
{
	return i;
}

HelperFunctionEntry *helper_function_entry_constructor(
    const UK_UBPF_INDEX_t index, const char *function_name,
    const void *function_addr, const ebpf_return_type_t ret_type,
    const UK_EBPF_HELPER_ARG_TYPE_NUM_t arg_type_count,
    const ebpf_argument_type_t arg_types[])
{
	HelperFunctionEntry *entry =
	    malloc(sizeof(HelperFunctionEntry)
		   + arg_type_count * sizeof(ebpf_argument_type_t));
	if (entry == NULL) {
		return NULL;
	}

	entry->m_index = index;
	entry->m_function_addr = function_addr;

	entry->m_function_signature.m_function_name =
	    malloc(strlen(function_name) + 1);
	strncpy(entry->m_function_signature.m_function_name, function_name,
		strlen(function_name) + 1);
	entry->m_function_signature.m_return_type = ret_type;
	entry->m_function_signature.m_num_args = arg_type_count;
	for (int i = 0; i < arg_type_count; ++i) {
		entry->m_function_signature.m_arg_types[i] = arg_types[i];
	}

	return entry;
}

void helper_function_entry_destructor(HelperFunctionEntry *entry)
{
	free(entry->m_function_signature.m_function_name);
	free(entry);
}

/**
 * This function initializes the builtin helper function list if it is not
 * ready.
 * @return The helper function list.
 */
HelperFunctionList *get_instance_builtin_bpf_helpers()
{
	if (g_bpf_helper_functions) {
		return g_bpf_helper_functions;
	}

	g_bpf_helper_functions =
	    helper_function_list_init(helper_function_entry_constructor,
				      helper_function_entry_destructor);
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
	    EBPF_RETURN_TYPE_INTEGER, 2, args_bpf_map_get);

	// bpf_map_put
	ebpf_argument_type_t args_bpf_map_put[] = {
	    EBPF_ARGUMENT_TYPE_CONST_SIZE_OR_ZERO,
	    EBPF_ARGUMENT_TYPE_CONST_SIZE_OR_ZERO,
	    EBPF_ARGUMENT_TYPE_CONST_SIZE_OR_ZERO,
	};
	helper_function_list_emplace_back(
	    g_bpf_helper_functions, 2, "bpf_map_put", bpf_map_put,
	    EBPF_RETURN_TYPE_INTEGER, 3, args_bpf_map_put);

	/*
	 * REGISTER_HELPER(bpf_map_noop);
      REGISTER_HELPER(bpf_map_get);
      REGISTER_HELPER(bpf_map_put);
      REGISTER_HELPER(bpf_map_del);
      REGISTER_HELPER(bpf_get_addr);
      REGISTER_HELPER(bpf_probe_read);
      REGISTER_HELPER(bpf_time_get_ns);
      REGISTER_HELPER(bpf_puts);
	 register_helper(function_index, "bpf_unwind", bpf_unwind);
      ubpf_set_unwind_function_index(vm, function_index);
	 */

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

struct ubpf_vm *init_vm(FILE *logfile)
{
	struct ubpf_vm *vm = ubpf_create();
	if (logfile != NULL) {
		fprintf(logfile, "attached BPF helpers:\n");
	}

	/*
	uint64_t function_index = 0;

	/* register generail helper functions */
	/*
#define REGISTER_HELPER(name)                                                  \
	register_helper(function_index, #name, name);                          \
	function_index++;

	REGISTER_HELPER(bpf_map_noop);
	REGISTER_HELPER(bpf_map_get);
	REGISTER_HELPER(bpf_map_put);
	REGISTER_HELPER(bpf_map_del);
	REGISTER_HELPER(bpf_get_addr);
	REGISTER_HELPER(bpf_probe_read);
	REGISTER_HELPER(bpf_time_get_ns);
	REGISTER_HELPER(bpf_puts);

	if (helper_list == NULL) {
		helper_list = additional_helpers;
	}

	if (helper_list != NULL) {
		for (uint64_t i = 0; i < helper_list->m_Length; ++i) {
			struct LabeledEntry elem = helper_list->m_List[i];
			register_helper(function_index, elem.m_Label,
					elem.m_Value);
			function_index++;
		}
	}

	register_helper(function_index, "bpf_unwind", bpf_unwind);
	ubpf_set_unwind_function_index(vm, function_index);*/
	return vm;
}

void *readfile(const char *path, size_t maxlen, size_t *len)
{
	FILE *file = fopen(path, "r");

	if (file == NULL) {
		fprintf(stderr, "Failed to open %s: %s\n", path,
			strerror(errno));
		return NULL;
	}

	char *data = calloc(maxlen, 1);
	size_t offset = 0;
	size_t rv;
	while ((rv = fread(data + offset, 1, maxlen - offset, file)) > 0) {
		offset += rv;
	}

	if (ferror(file)) {
		fprintf(stderr, "Failed to read %s: %s\n", path,
			strerror(errno));
		fclose(file);
		free(data);
		return NULL;
	}

	if (!feof(file)) {
		fprintf(stderr,
			"Failed to read %s because it is too large (max %u "
			"bytes)\n",
			path, (unsigned)maxlen);
		fclose(file);
		free(data);
		return NULL;
	}

	fclose(file);
	if (len) {
		*len = offset;
	}
	return (void *)data;
}

int bpf_exec(const char *filename, void *args, size_t args_size, int debug,
	     void (*print_fn)(char *str))
{
	FILE *logfile = NULL;
	if (debug != 0) {
		logfile = fopen("bpf_exec.log", "a");
	}

	if (logfile != NULL) {
		fprintf(logfile, "\n# bpf_exec %s", filename);
		if (args != NULL) {
			fprintf(logfile, " %s", (char *)args);
		}
		fprintf(logfile, "\n");
	}

	struct ubpf_vm *vm = init_vm(logfile);
	size_t code_len;
	void *code = readfile(filename, 1024 * 1024, &code_len);
	if (code == NULL) {
		fclose(logfile);
		return 1;
	}
	char *errmsg;
	int rv;
#if defined(UBPF_HAS_ELF_H)
	rv = ubpf_load_elf(vm, code, code_len, &errmsg);
#else
	rv = ubpf_load(vm, code, code_len, &errmsg);
#endif
	free(code);

	if (rv < 0) {
		size_t buf_size = 100 + strlen(errmsg);
		wrap_print_fn(buf_size, ERR("Failed to load code: %s\n"),
			      errmsg);
		if (logfile != NULL) {
			fprintf(logfile, "Failed to load code: %s\n", errmsg);
		}

		free(errmsg);
		ubpf_destroy(vm);
		if (logfile != NULL) {
			fclose(logfile);
		}
		return 1;
	}

	uint64_t ret;
	if (ubpf_exec(vm, args, args_size, &ret) < 0) {
		print_fn(ERR("BPF program execution failed.\n"));
		if (logfile != NULL) {
			fprintf(logfile, "BPF program execution failed.\n");
		}
	} else {
		wrap_print_fn(100, YAY("BPF program returned: %lu\n"), ret);
		if (logfile != NULL) {
			fprintf(logfile, "BPF program returned: %lu\n", ret);
		}
	}
	ubpf_destroy(vm);
	if (logfile != NULL) {
		fclose(logfile);
	}
	return 0;
}

uint64_t bpf_get_addr(const char *function_name)
{
	void *ushell_symbol_get(const char *symbol);
	uint64_t fun_addr = (uint64_t)ushell_symbol_get(function_name);
	return fun_addr;
}

uint64_t bpf_probe_read(uint64_t addr, uint64_t size)
{
	if (size != 1 && size != 4 && size != 8) {
		debug("bpf_probe_read: invalid size %lu\n", size);
		return 0;
	}

	/* check if addr is valid */
	struct uk_pagetable;
	int ukplat_pt_walk(struct uk_pagetable *, uint64_t, uint64_t *,
			   uint64_t *, uint64_t *);
	struct uk_pagetable *ukplat_pt_get_active(void);
	struct uk_pagetable *pt = ukplat_pt_get_active();
	int rc;
	uint64_t page_addr = addr & ~0xfffULL;
	uint64_t pte = 0;
	rc = ukplat_pt_walk(pt, page_addr, NULL, NULL, &pte);
	if (rc != 0 || (pte & 1) == 0) {
		// invalid address
		debug("bpf_probe_read: invalid addr %lu, %lu, %lu\n", addr,
		      page_addr, pte);
		return 0;
	}

	if (size == 1) {
		return *(uint8_t *)addr;
	} else if (size == 4) {
		return *(uint32_t *)addr;
	}

	return *(uint64_t *)addr;
}

uint64_t bpf_time_get_ns()
{
	uint64_t ukplat_monotonic_clock(void);
	return ukplat_monotonic_clock();
}

// TODO:
// - check size, null termination
// - support format string
void bpf_puts(char *buf)
{
	void ushell_puts(char *);
	ushell_puts(buf);
}

void print_helper_specs(void (*print_fn)(const char *))
{
	char* buffer[8];

	for (HelperFunctionEntry *entry = g_bpf_helper_functions->m_head;
	     entry != NULL; entry = entry->m_next) {
		print_fn(entry->m_function_signature.m_function_name);
		print_fn("->");
		itoa(buffer, entry->m_function_signature.m_return_type, 16);
		print_fn(buffer);
		print_fn(":");
		for (int index = 0;
		     index < entry->m_function_signature.m_num_args; index++) {
			itoa(buffer, entry->m_function_signature.m_arg_types[index]);
			print_fn(buffer);
			if (index
			    != entry->m_function_signature.m_num_args - 1) {
				print_fn(",");
			}
		}

		if (entry->m_next != NULL) {
			print_fn(";");
		}
	}
}
