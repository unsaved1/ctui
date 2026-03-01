#ifndef CTUI_HASH_H_SENTRY
#define CTUI_HASH_H_SENTRY

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "arena.h"
#include "str.h"

uint32_t genhash(uint8_t *str);
uint32_t fnv1a(const char *key);
uint64_t make_key(uint32_t parent_id, uint32_t name_hash);
uint32_t hash_key(uint64_t key);

#define DECLARE_HASHMAP(name, value_type)   \
typedef struct name##Item name##Item; \
struct name##Item {  \
    name##Item *next; \
    const char *key; \
    value_type value;\
}; \
typedef struct name {  \
    size_t capacity; \
    size_t length; \
    name##Item **items;\
} name; \
name##Item *name##_get(name *hm, uint64_t key, const char *string_key);  \
void name##_set(Arena *a, name *hm, uint64_t key, const char *string_key, value_type value);

#define arena_init_hm(a, hm, cap)                                       \
    do {                                                                \
        (hm) = arena_alloc((a), sizeof(*(hm)));                         \
        (hm)->capacity = (cap);                                         \
        (hm)->length = 0;                                               \
        (hm)->items = arena_alloc((a), sizeof(*(hm)->items) * (cap));    \
        memset((hm)->items, 0, sizeof(*((hm)->items)) * (cap));          \
    } while (0)

#define DEFINE_HASHMAP(name, value_type)    \
name##Item *name##_get(name *hm, uint64_t key, const char *string_key)  \
{ \
    uint32_t h = hash_key(key) % (hm)->capacity;  \
    name##Item *tmp = (name##Item *)(hm)->items[h];\
    if (!tmp) \
        return NULL;\
    while (tmp) {\
        if (ctui_cmpstr(tmp->key, (string_key)))\
            return tmp;\
        tmp = tmp->next;\
    }\
    return NULL;\
}\
void name##_set(Arena *a, name *hm, uint64_t key, const char *string_key, value_type value) \
{ \
    uint32_t h = hash_key(key) % (hm)->capacity;  \
    name##Item *newitem = (name##Item *)arena_alloc((a), sizeof(*newitem));\
    newitem->key = (string_key);\
    newitem->value = value;\
    newitem->next = NULL;\
    if ((hm)->items[h]) \
        newitem->next = (hm)->items[h];\
    (hm)->items[h] = newitem;\
    (hm)->length++;\
}

#endif
