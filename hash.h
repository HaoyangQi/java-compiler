#pragma once
#ifndef __COMPILER_HASH_H__
#define __COMPILER_HASH_H__

#include "types.h"

uint32_t hash_djb2(void* stream, size_t len);
uint32_t hash_sdbm(void* str, size_t len);
uint32_t hash_jenkins(void* stream, size_t len);
uint32_t hash_murmur32(void* stream, uint32_t len, uint32_t seed);
uint64_t hash_murmur64(void* stream, uint64_t len, uint64_t seed);

#ifdef COMPILER_32
typedef uint32_t hash;
typedef uint32_t bytes_length;
#else
typedef uint64_t hash;
typedef uint64_t bytes_length;
#endif

hash shash(char* str);
hash bhash(void* bytes, bytes_length len);

#endif
