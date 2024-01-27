#pragma once
#ifndef __COMPILER_TYPES_H__
#define __COMPILER_TYPES_H__

#if defined(_MSC_VER)
#if defined(_M_X64)
#define COMPILER_64
#else
#define COMPILER_32
#endif
#elif defined(__clang__) || defined(__INTEL_COMPILER) || defined(__GNUC__)
#if defined(__x86_64)
#define COMPILER_64
#else
#define COMPILER_32
#endif
#else
#define COMPILER_32
#endif

#ifndef __cplusplus
#include <stdbool.h>
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdint.h>

#define ASSERT_ALLOCATION(ptr) assert((ptr) != NULL)
#define ARRAY_SIZE(arr) (sizeof (arr) / sizeof ((arr)[0]))

typedef unsigned char byte;
typedef unsigned char bbit_flag;
typedef unsigned int ibit_flag;
typedef unsigned long int lbit_flag;

void* malloc_assert(size_t sz);
void* realloc_assert(void* p, size_t sz);
char* strmcpy_assert(const char* source);

#endif
