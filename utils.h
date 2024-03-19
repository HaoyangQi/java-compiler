#pragma once
#ifndef __COMPILER_UTIL_H__
#define __COMPILER_UTIL_H__

#include "types.h"

bool is_prime(unsigned int n);
unsigned int find_next_prime(unsigned int n);
uint32_t find_next_pow2_32(uint32_t v);
uint64_t find_next_pow2_64(uint64_t v);
size_t find_next_pow2_size(size_t v);

#endif
