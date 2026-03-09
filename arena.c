#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "arena.h"

Region *new_region(size_t capacity) 
{
    size_t calc_size = sizeof(Region) + sizeof(uintptr_t) * capacity;
    Region *r = (Region *)malloc(calc_size); 
    r->next = NULL;
    r->capacity = capacity;
    r->count = 0;
    return r;
}

void free_region(Region *r)
{
    free(r);
}

void *arena_alloc(Arena *a, size_t size)
{
    size_t calc_size = (size + sizeof(uintptr_t) - 1)/sizeof(uintptr_t);
    if (a->end == NULL) {
        size_t capacity = ARENA_REGION_DEFAULT_CAPACITY;
        if (capacity < calc_size) capacity = calc_size;
        Region *r = new_region(capacity);
        a->end = r;
        a->begin = a->end;
    }

    while (a->end->count + calc_size > a->end->capacity && a->end->next != NULL)
        a->end = a->end->next;

    if (a->end->count + calc_size > a->end->capacity) {
        size_t capacity = ARENA_REGION_DEFAULT_CAPACITY;
        if (capacity < calc_size) capacity = calc_size;
        a->end->next = new_region(capacity);
        a->end = a->end->next;
    }

    void *res = &a->end->data[a->end->count];
    a->end->count += calc_size;

    return res;
}

void *arena_realloc(Arena *a, void *oldptr, size_t oldsz, size_t newsz) 
{
    if (newsz < oldsz) return oldptr;
    size_t i;
    char *newptr = arena_alloc(a, newsz);
    char *newptr_char = (char *)newptr;
    const char *oldptr_char = (char *)oldptr;
    for (i = 0; i < oldsz; i++)
        newptr_char[i] = oldptr_char[i];
    return newptr;
}

void *arena_recalloc(Arena *a, void *oldptr, size_t oldsz, size_t newsz) 
{
    if (newsz < oldsz) return oldptr;
    size_t i;
    void *newptr = arena_alloc(a, newsz);
    memset(newptr, 0, newsz);
    char *newptr_char = (char *)newptr;
    const char *oldptr_char = (char *)oldptr;
    for (i = 0; i < oldsz; i++)
        newptr_char[i] = oldptr_char[i];
    return newptr;
}

Arena_Mark arena_snapshot(Arena *a)
{
    Arena_Mark m;
    m.region = a->end;
    if (a->end == NULL) {
        m.count = 0;
    } else {
        m.count = a->end->count;
    }
    return m;
}

void arena_rewind(Arena *a, Arena_Mark m) 
{
    if (m.region == NULL) {
        arena_reset(a);
        return;
    }

    m.region->count = m.count;
    Region *tmp_r = m.region->next;
    while (tmp_r) {
        tmp_r->count = 0;
        tmp_r = tmp_r->next;
    }

    a->end = m.region;
}

void arena_reset(Arena *a)
{
    while (a->begin) {
        Region *tmp_r = a->begin;
        a->begin = a->begin->next;
        free(tmp_r);
    }
}

void arena_memcpy(void *dest, const void *src)
{
    const char *s = (char *)src;
    char *d = (char *)dest;
    while(*s) {
        *d = *s;
        d++;
        s++;
    } 
}

void arena_memcpy_n(void *dest, const void *src, size_t n)
{
    const char *s = (char *)src;
    char *d = (char *)dest;
    while(*s && n) {
        *d = *s;
        d++;
        s++;
        n--;
    } 
}

int arena_strlen(const char *str) 
{
    const char *tmp = str;
    while (*tmp) 
        tmp++;
    return tmp - str;
}

char *arena_strdup(Arena *a, const char *str)
{
    size_t n = arena_strlen(str); 
    char *d = arena_alloc(a, n + 1);
    arena_memcpy_n(d, str, n);
    d[n] = '\0';
    return d;
}

char *arena_strdup_n(Arena *a, const char *str, size_t n)
{
    char *d = arena_alloc(a, n + 1);
    arena_memcpy_n(d, str, n);
    d[n] = '\0';
    return d;
}


