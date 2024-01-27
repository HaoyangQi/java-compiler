#include "types.h"

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

    char* s = (char*)malloc_assert(sizeof(char) * (strlen(source) + 1));
    strcpy(s, source);
    return s;
}
