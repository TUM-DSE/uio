#ifndef HASH_CHAINS_H
#define HASH_CHAINS_H

#include <stdint.h>

struct THashCell {
  uint64_t m_Key;
  void *m_Value;
  struct THashCell *m_Next;
};

struct THashMap {
  uint64_t m_Size;
  uint64_t m_Elems;
  struct THashCell **m_Map;
  void (*destruct_cell)(struct THashCell *);
  void *(*create_cell)();
};

enum THmapResultCode {
  HMAP_SUCCESS = 0,
  HMAP_BADARGUMENT,
  HMAP_ALLOCFAIL,
  HMAP_NOTFOUND,
};

struct THmapValueResult {
  void *m_Value;
  enum THmapResultCode m_Result;
};

struct THashMap *hmap_init(uint64_t size,
                           void (*destruct_cell)(struct THashCell *),
                           void *(*create_cell)(), int *errcode);

struct THashCell *hmap_new_cell(uint64_t key, void *value);

struct THmapValueResult *hmap_put(struct THashMap *hmap, uint64_t key,
                                  void *value);

struct THmapValueResult *hmap_get(struct THashMap *hmap, uint64_t key);

struct THmapValueResult *hmap_get_or_create(struct THashMap *hmap,
                                            uint64_t key);

struct THmapValueResult *hmap_del(struct THashMap *hmap, uint64_t key);

void hmap_destroy(struct THashMap *hmap);

#endif /* HASH_CHAINS_H */
