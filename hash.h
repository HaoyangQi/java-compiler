#pragma once
#ifndef __COMPILER_HASH_H__
#define __COMPILER_HASH_H__

#include "types.h"

uint32_t hash_djb2(const void* stream, size_t len);
uint32_t hash_sdbm(const void* str, size_t len);
uint32_t hash_jenkins(const void* stream, size_t len);
uint32_t hash_murmur32(const void* stream, uint32_t len, uint32_t seed);
uint64_t hash_murmur64(const void* stream, uint64_t len, uint64_t seed);

#ifdef COMPILER_32
typedef uint32_t hash;
typedef uint32_t bytes_length;
#else
typedef uint64_t hash;
typedef uint64_t bytes_length;
#endif

hash shash(const char* str);
hash bhash(const void* bytes, bytes_length len);

#endif
