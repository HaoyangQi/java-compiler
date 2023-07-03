#include "hash.h"

/**
 * Hash function (djb2)
 * http://www.cse.yorku.ca/~oz/hash.html
 */
hash_bits hash_djb2(byte* str)
{
    hash_bits hash = 5381;
    int c;

    while (c = *str++)
    {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }

    return hash;
}

/**
 * Hash function (sdbm)
 * http://www.cse.yorku.ca/~oz/hash.html
 */
hash_bits hash_sdbm(byte* str)
{
    hash_bits hash = 0;
    int c;

    while (c = *str++)
    {
        hash = c + (hash << 6) + (hash << 16) - hash;
    }

    return hash;
}

hash_bits hash_mod(byte* str, hash_bits mod)
{
    return MOD_OF_HASH(hash_djb2(str), mod);
}
