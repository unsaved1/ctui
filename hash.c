#include "hash.h"

typedef enum fnv1a_config {
    FNV1a_OFFSET_BASIS = 2166136261U,
    FNV1a_PRIME = 16777619U
} fnv1a_config;

uint32_t genhash(uint8_t *str)
{
    uint32_t hash = 5381;
    while (*str) {
        hash = ((hash << 5) + hash) + *str; 
        str++;
    }
    return hash;
}

uint32_t fnv1a(const char *key)
{
    uint32_t hash = FNV1a_OFFSET_BASIS;
    while (*key) {
        hash ^= (uint32_t)(*key);
        hash *= FNV1a_PRIME;
        key++;
    }
    return hash;
}

uint64_t make_key(uint32_t parent_id, uint32_t name_hash)
{
    return ((uint64_t)parent_id << 32) | name_hash; 
}

uint32_t hash_key(uint64_t key) 
{
    key ^= key >> 33;
    key *= 0xff51afd7ed558ccdULL;
    key ^= key >> 33;
    key *= 0xc4ceb9fe1a85ec53ULL;
    key ^= key >> 33;
    return (uint32_t)key;
}
