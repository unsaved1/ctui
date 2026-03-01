#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "errno.h"
#include "str.h"

CTUI_String_View ctui_sv_from_parts(const char *data, size_t n)
{
    CTUI_String_View sv;
    sv.length = n;
    sv.data = data;
    return sv;
}

CTUI_String_View ctui_sv_trim_left(CTUI_String_View sv)
{
    size_t i = 0;
    while (i < sv.length && isspace(sv.data[i]))
        i++;
    return ctui_sv_from_parts(sv.data + i, sv.length - i);
}

CTUI_String_View nob_sv_trim_right(CTUI_String_View sv)
{
    size_t i = 0;
    while (i < sv.length && isspace(sv.data[sv.length - 1 - i]))
        i++;
    return ctui_sv_from_parts(sv.data, sv.length - i);
}

CTUI_String_View ctui_sv_chop_by_space(CTUI_String_View *sv)
{
    size_t i = 0;
    while (i < sv->length && !isspace(sv->data[i]))
        i++;

    CTUI_String_View result = ctui_sv_from_parts(sv->data, i);

    if (i < sv->length) {
        sv->length -= i + 1;
        sv->data  += i + 1;
    } else {
        sv->length -= i;
        sv->data  += i;
    }

    return result;
}

bool ctui_cmpstr(const char *str, const char *pat) 
{
    while (*str) {
        if (*str != *pat) 
            return false;
        str++;
        pat++;
    }
    return true;
}

bool ctui_read_file_to_buf(Arena *a, char *path, CTUI_String_Builder *sb) 
{
    bool result = true;

    FILE *f = fopen(path, "r");
    size_t new_length = 0;
    long long m = 0;
    if (f == NULL)                 arena_return_defer(false);
    if (fseek(f, 0, SEEK_END) < 0) arena_return_defer(false);
    m = ftell(f);
    if (m < 0)                     arena_return_defer(false);
    if (fseek(f, 0, SEEK_SET) < 0) arena_return_defer(false);

    new_length = sb->length + m;

    if (new_length > sb->capacity) {
        sb->items = arena_realloc(                                  
            a,                                        
            sb->items,                                
            sb->length * sizeof(*sb->items),         
            new_length                                
        );                                              
        sb->capacity = new_length;
    }

    fread(sb->items + sb->length, m, 1, f);

    if (ferror(f)) {            
        // TODO: Afaik, ferror does not set errno. So the error reporting in defer is not correct in this case.
        arena_return_defer(false);
    }
    
    sb->length= new_length;
    
defer:
    if (!result) fprintf(stderr, "Could not read file %s: %s", path, strerror(errno));
    if (f) fclose(f);
    return result;
}

