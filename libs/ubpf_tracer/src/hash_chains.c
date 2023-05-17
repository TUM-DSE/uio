#include "hash_chains.h"

struct THashMap *hmap_init(uint64_t size,
                           void (*destruct_cell)(struct THashCell *),
                           void *(*create_cell)(), int *errcode) {
  if (errcode == NULL) {
    return NULL;
  }
  if (size == 0) {
    *errcode = HMAP_BADARGUMENT;
    return NULL;
  }

  struct THashMap *hmap = (struct THashMap *)calloc(1, sizeof(struct THashMap));

  if (hmap == NULL) {
    *errcode = HMAP_ALLOCFAIL;
    return NULL;
  }

  hmap->m_Size = size;
  hmap->m_Elems = 0;
  hmap->m_Map = (struct THashCell **)calloc(size, sizeof(struct THashCell *));
  hmap->destruct_cell = destruct_cell;
  hmap->create_cell = create_cell;

  if (hmap->m_Map == NULL) {
    *errcode = HMAP_ALLOCFAIL;
    return NULL;
  }

  *errcode = HMAP_SUCCESS;

  return hmap;
}

struct THashCell *hmap_new_cell(uint64_t key, void *value) {
  struct THashCell *cell = (struct THashCell *)malloc(sizeof(struct THashCell));
  if (cell == NULL)
    return NULL;

  cell->m_Key = key;
  cell->m_Value = value;
  cell->m_Next = NULL;

  return cell;
}

struct THmapValueResult *hmap_put(struct THashMap *hmap, uint64_t key,
                                  void *value) {
  struct THmapValueResult *result = calloc(1, sizeof(struct THmapValueResult));
  if (result == NULL)
    return NULL;

  result->m_Value = NULL;

  if (hmap == NULL) {
    result->m_Result = HMAP_BADARGUMENT;
    return result;
  }

  uint64_t map_idx = key % hmap->m_Size;
  struct THashCell *current = hmap->m_Map[map_idx];
  struct THashCell *prev;
  // first cell in the chain?
  if (current == NULL) {
    struct THashCell *cell = hmap_new_cell(key, value);
    if (cell == NULL) {
      result->m_Result = HMAP_ALLOCFAIL;
      return result;
    }
    hmap->m_Map[map_idx] = cell;
    hmap->m_Elems++;

    result->m_Value = value;
    result->m_Result = HMAP_SUCCESS;
    return result;
  }

  // let's see if the key is already there
  while (current != NULL) {
    if (current->m_Key == key) {
      free(current->m_Value);
      current->m_Value = value;
      result->m_Value = value;
      result->m_Result = HMAP_SUCCESS;
      return result;
    }
    prev = current;
    current = current->m_Next;
  }

  // add to the tail of the chain
  struct THashCell *cell = hmap_new_cell(key, value);
  if (cell == NULL) {
    result->m_Result = HMAP_ALLOCFAIL;
    return result;
  }
  hmap->m_Elems++;
  prev->m_Next = cell;
  result->m_Result = HMAP_SUCCESS;
  return result;
}

struct THmapValueResult *hmap_del(struct THashMap *hmap, uint64_t key) {
  struct THmapValueResult *result = calloc(1, sizeof(struct THmapValueResult));
  if (result == NULL)
    return NULL;

  result->m_Value = NULL;

  if (hmap == NULL) {
    result->m_Result = HMAP_BADARGUMENT;
    return result;
  }

  uint64_t map_idx = key % hmap->m_Size;
  struct THashCell *current = hmap->m_Map[map_idx];
  struct THashCell *prev = NULL;
  if (current == NULL) {
    result->m_Result = HMAP_NOTFOUND;
    return result;
  }

  // let's see if the key is already there
  while (current != NULL) {
    if (current->m_Key == key) {
      // delete
      if (prev == NULL) {
        hmap->m_Map[map_idx] = current->m_Next;
      } else {
        prev->m_Next = current->m_Next;
      }
      hmap->destruct_cell(current);
      free(current);
      hmap->m_Elems--;
      result->m_Result = HMAP_SUCCESS;
      return result;
    }
    prev = current;
    current = current->m_Next;
  }

  result->m_Result = HMAP_NOTFOUND;
  return result;
}

struct THmapValueResult *hmap_get(struct THashMap *hmap, uint64_t key) {
  struct THmapValueResult *result = calloc(1, sizeof(struct THmapValueResult));
  if (result == NULL)
    return NULL;

  result->m_Value = NULL;

  if (hmap == NULL) {
    result->m_Result = HMAP_BADARGUMENT;
    return result;
  }

  uint64_t map_idx = key % hmap->m_Size;

  result->m_Result = HMAP_NOTFOUND;
  struct THashCell *current = hmap->m_Map[map_idx];
  while (current != NULL) {
    if (current->m_Key == key) {
      result->m_Value = current->m_Value;
      result->m_Result = HMAP_SUCCESS;
      return result;
    }
    current = current->m_Next;
  }

  result->m_Result = HMAP_NOTFOUND;
  return result;
}

struct THmapValueResult *hmap_get_or_create(struct THashMap *hmap,
                                            uint64_t key) {
  struct THmapValueResult *result = hmap_get(hmap, key);
  if (result->m_Result == HMAP_NOTFOUND) {
    free(result);
    return hmap_put(hmap, key, hmap->create_cell());
  }
  return result;
}

void hmap_destroy(struct THashMap *hmap) {
  if (hmap == NULL)
    return;
  struct THashCell *current, *next;
  for (uint64_t i = 0; i < hmap->m_Size; ++i) {
    current = hmap->m_Map[i];
    while (current != NULL) {
      next = current->m_Next;
      hmap->destruct_cell(current);
      free(current);
      current = next;
    }
  }
  free(hmap->m_Map);
  free(hmap);
}
