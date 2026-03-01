#ifndef CTUI_STR_H_SENTRY
#define CTUI_STR_H_SENTRY

#include <stdlib.h>
#include <stdbool.h>
#include "arena.h"

typedef struct CTUI_String_Builder {
    size_t capacity;
    size_t length;
    char *items;
} CTUI_String_Builder;

typedef struct CTUI_String_View {
    size_t length;
    const char *data;
} CTUI_String_View;


CTUI_String_View ctui_sv_from_parts(const char *data, size_t count);
CTUI_String_View ctui_sv_trim_left(CTUI_String_View sv);
CTUI_String_View ctui_sv_chop_by_space(CTUI_String_View *sv);
bool ctui_cmpstr(const char *str, const char *pat); 

bool ctui_read_file_to_buf(Arena *a, char *path, CTUI_String_Builder *sb);

#endif
