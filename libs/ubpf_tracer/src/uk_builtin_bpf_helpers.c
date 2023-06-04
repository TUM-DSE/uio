#include "uk_builtin_bpf_helpers.h"

#include "hash_chains.h"

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

// private functions
static void destruct_cell_l2(struct THashCell *cell)
{
	free(cell->m_Value);
}
static void *create_cell_l2()
{
	uint64_t *cell = calloc(1, sizeof(uint64_t));
	return cell;
}

static void destruct_cell_l1(struct THashCell *cell)
{
	hmap_destroy(cell->m_Value);
	free(cell->m_Value);
}

static void *create_cell_l1()
{
	int err = 0;
	return hmap_init(101, &destruct_cell_l2, &create_cell_l2, &err);
}

static struct THashMap *init_bpf_map()
{
	int err = 0;
	return hmap_init(101, &destruct_cell_l1, &create_cell_l1, &err);
}
// end of private functions

// implementation of the builtin helper functions
uint64_t bpf_map_noop()
{
	return 0;
}

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

uint64_t bpf_unwind(uint64_t i)
{
	return i;
}

// TODO:
// - check size, null termination
// - support format string
void bpf_puts(char *buf)
{
	void ushell_puts(char *);
	ushell_puts(buf);
}