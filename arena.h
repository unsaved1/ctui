#ifndef ARENA_H_SENTRY
#define ARENA_H_SENTRY

#include <stddef.h>
#include <stdint.h>

#ifndef ARENA_REGION_DEFAULT_CAPACITY
#define ARENA_REGION_DEFAULT_CAPACITY (8*1024)
#endif // ARENA_REGION_DEFAULT_CAPACITY

#ifndef ARENA_DA_LOAD_FACTOR
#define ARENA_DA_LOAD_FACTOR 0.7
#endif // ARENA_DA_LOAD_FACTOR

#define arena_return_defer(value) do { result = (value); goto defer; } while(0)

typedef struct Region Region;

struct Region {
    Region *next;
    size_t count;
    size_t capacity;
    uintptr_t data[];
};

typedef struct Arena {
    Region *begin, *end;
} Arena;

typedef struct Arena_Mark {
    Region *region;
    size_t count; 
} Arena_Mark;

Region *new_region(size_t capacity);
void free_region(Region *r);

#ifndef ARENA_DA_INIT_CAP
#define ARENA_DA_INIT_CAP 256
#endif // ARENA_DA_INIT_CAP


void *arena_alloc(Arena *a, size_t size_bytes);
void *arena_realloc(Arena *a, void *oldptr, size_t oldsz, size_t newsz);
void *arena_recalloc(Arena *a, void *oldptr, size_t oldsz, size_t newsz);

void arena_reset(Arena *a);

int arena_strlen(const char *str);
void arena_memcpy(void *dest, const void *src);
void arena_memcpy_n(void *dest, const void *src, size_t n);
char *arena_strdup(Arena *a, const char *str);
char *arena_strdup_n(Arena *a, const char *str, size_t n);

Arena_Mark arena_snapshot(Arena *a);
void arena_rewind(Arena *a, Arena_Mark m);

#define arena_da_append(a, da, item)                                \
    do {                                                            \
        float load = (float)(da)->count / (da)->capacity;           \
        if (load >= ARENA_DA_LOAD_FACTOR || (da)->capacity == 0) {  \
            size_t new_capacity;                                    \
            if ((da)->capacity == 0)                                \
                new_capacity = ARENA_DA_INIT_CAP;                   \
            else                                                    \
                new_capacity = (da)->capacity*2;                    \
            (da)->items = arena_realloc(                            \
                (a),                                                \
                (da)->items,                                        \
                (da)->count * sizeof(*(da)->items),                 \
                new_capacity                                        \
            );                                                      \
            (da)->capacity = new_capacity;                          \
        }                                                           \
        (da)->items[(da)->count] = (item);                          \
        (da)->count++;                                              \
    } while (0)

#endif
