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
