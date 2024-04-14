#include "types.h"

/**
 * copy line info data
*/
void line_copy(line* dest, line* src)
{
    ASSERT_ALLOCATION(dest);
    ASSERT_ALLOCATION(src);

    dest->ln = src->ln;
    dest->col = src->col;
}

/**
 * a verbose wrapper of allocation
*/
void* malloc_assert(size_t sz)
{
    void* d = malloc(sz);
    ASSERT_ALLOCATION(d);

    return d;
}

/**
 * a verbose wrapper of allocation
*/
void* realloc_assert(void* p, size_t sz)
{
    void* d = realloc(p, sz);
    ASSERT_ALLOCATION(d);

    return d;
}

/**
 * allocate a string buffer and copy source to it
*/
char* strmcpy_assert(const char* source)
{
    if (!source || strlen(source) <= 0)
    {
        return NULL;
    }

    size_t len = strlen(source);
    char* s = (char*)malloc_assert(sizeof(char) * (len + 1));

    strcpy(s, source);
    s[len] = '\0';

    return s;
}
