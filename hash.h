#pragma once
#ifndef __COMPILER_HASH_H__
#define __COMPILER_HASH_H__

// nothing fancy here, just calculate the modulo
#define MOD_OF_HASH(h, m) ((hash_bits)(h) % (m))

typedef unsigned long hash_bits;

typedef hash_bits(*hash_func)(unsigned char* str);

hash_bits hash(unsigned char* str);
hash_bits hash_mod(unsigned char* str, hash_bits mod);

#endif
