#include "hash.h"

/**
 * Hash function (djb2)
 * http://www.cse.yorku.ca/~oz/hash.html
 *
 * Here comes the magic number of 33
 */
hash_bits hash(unsigned char* str)
{
    hash_bits hash = 5381;
    int c;

    while (c = *str++)
    {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }

    return hash;
}

hash_bits hash_mod(unsigned char* str, hash_bits mod)
{
    return MOD_OF_HASH(hash(str), mod);
}
