/**
 * Hash Functions
 *
 * Hash functions produces 32/64-bit hash string
 *
 * A hash function takes in any data stream, read it byte-wise,
 * and calculate hash
*/

#include "hash.h"

/**
 * Hash function (djb2)
 * http://www.cse.yorku.ca/~oz/hash.html
 */
uint32_t hash_djb2(const void* stream, size_t len)
{
    const char* buf = (const char*)stream;
    uint32_t hash = 5381;

    for (int i = 0; i < len; i++)
    {
        hash = ((hash << 5) + hash) + buf[i]; /* hash * 33 + c */
    }

    return hash;
}

/**
 * Hash function (sdbm)
 * http://www.cse.yorku.ca/~oz/hash.html
 */
uint32_t hash_sdbm(const void* stream, size_t len)
{
    const char* buf = (const char*)stream;
    uint32_t hash = 0;

    for (int i = 0; i < len; i++)
    {
        hash = buf[i] + (hash << 6) + (hash << 16) - hash;
    }

    return hash;
}

/**
 * Jenkins OAAT (One-At-A-Time) Hash
 * https://en.wikipedia.org/wiki/Jenkins_hash_function
*/
uint32_t hash_jenkins(const void* stream, size_t len)
{
    const char* buf = (const char*)stream;
    uint32_t hash = 0;

    for (int i = 0; i < len; i++)
    {
        hash += buf[i];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }

    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);

    return hash;
}

/**
 * Murmur Hash 32-bit: MurmurHashUnaligned2
 *
 * Algorithm from GCC library:
 * https://github.com/gcc-mirror/gcc/blob/master/libstdc%2B%2B-v3/libsupc%2B%2B/hash_bytes.cc#L74
*/
uint32_t hash_murmur32(const void* stream, uint32_t len, uint32_t seed)
{
    const uint32_t m = 0x5bd1e995;
    uint32_t hash = seed ^ len;
    uint32_t k;
    const char* buf = (const char*)stream;

    // Mix 4 bytes at a time into the hash.
    while (len >= 4)
    {
        // unaligned load: differ result across endiannesses
        memcpy(&k, buf, sizeof(uint32_t));

        // scramble
        k *= m;
        k ^= k >> 24;
        k *= m;
        hash *= m;
        hash ^= k;

        // step
        buf += 4;
        len -= 4;
    }

    // Handle the last few bytes of the input array.
    switch (len)
    {
        case 3:
            k = (unsigned char)(buf[2]);
            hash ^= k << 16;
            // fall-through
        case 2:
            k = (unsigned char)(buf[1]);
            hash ^= k << 8;
            // fall-through
        case 1:
            k = (unsigned char)(buf[0]);
            hash ^= k;
            hash *= m;
    };

    // Do a few final mixes of the hash.
    hash ^= hash >> 13;
    hash *= m;
    hash ^= hash >> 15;

    return hash;
}

static uint64_t murmur64_shift_scramble(uint64_t v)
{
    return v ^ (v >> 47);
}

/**
 * Murmur Hash 64-bit: MurmurHashUnaligned2
 *
 * Algorithm from GCC library:
 * https://github.com/gcc-mirror/gcc/blob/master/libstdc%2B%2B-v3/libsupc%2B%2B/hash_bytes.cc#L138
*/
uint64_t hash_murmur64(const void* stream, uint64_t len, uint64_t seed)
{
    static const uint64_t mul = (((uint64_t)0xc6a4a793UL) << 32UL) + (uint64_t)0x5bd1e995UL;
    const char* const buf = (const char*)stream;

    // Remove the bytes not divisible by the sizeof(size_t).  This
    // allows the main loop to process the data as 64-bit integers.
    const uint64_t len_aligned = len & ~(uint64_t)0x7;
    const char* const end = buf + len_aligned;
    uint64_t hash = seed ^ (len * mul);
    uint64_t k, data;

    for (const char* p = buf; p != end; p += 8)
    {
        memcpy(&k, p, sizeof(uint64_t));

        data = murmur64_shift_scramble(k * mul) * mul;
        hash ^= data;
        hash *= mul;
    }

    int n = len & 0x7;

    if (n != 0)
    {
        // data = load_bytes(end, n);
        data = 0;
        while (n-- > 0)
        {
            data = (data << 8) + (unsigned char)(end[n]);
        }

        hash ^= data;
        hash *= mul;
    }

    hash = murmur64_shift_scramble(hash) * mul;
    hash = murmur64_shift_scramble(hash);

    return hash;
}

/**
 * Hash function interface
 *
 * By default we use the best function: murmur hash
*/

static unsigned long __hash_seed = 0xc70f6907UL;

hash shash(const char* str)
{
#ifdef COMPILER_32
    return hash_murmur32(str, strlen(str), (uint32_t)__hash_seed);
#else
    return hash_murmur64(str, strlen(str), (uint64_t)__hash_seed);
#endif
}

hash bhash(const void* bytes, bytes_length len)
{
#ifdef COMPILER_32
    return hash_murmur32(bytes, len, (uint32_t)__hash_seed);
#else
    return hash_murmur64(bytes, len, (uint64_t)__hash_seed);
#endif
}
