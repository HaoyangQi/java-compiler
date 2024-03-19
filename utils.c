#include "utils.h"

bool is_prime(unsigned int n)
{
    for (int i = 2; i <= n / 2; i++)
    {
        if (n % i == 0)
        {
            return false;
        }
    }

    return n <= 1 ? false : true;
}

unsigned int find_next_prime(unsigned int n)
{
    while (!is_prime(n))
    {
        n++;
    }

    return n;
}

uint32_t find_next_pow2_32(uint32_t v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;

    return v;
}

uint64_t find_next_pow2_64(uint64_t v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v |= v >> 32;
    v++;

    return v;
}

size_t find_next_pow2_size(size_t v)
{
#ifdef COMPILER_64
    return find_next_pow2_64(v);
#else
    return find_next_pow2_32(v);
#endif
}
