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
