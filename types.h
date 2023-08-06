#pragma once
#ifndef __COMPILER_TYPES_H__
#define __COMPILER_TYPES_H__

#ifndef __cplusplus
#include <stdbool.h>
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#define ASSERT_ALLOCATION(ptr) assert((ptr) != NULL)
#define ARRAY_SIZE(arr) (sizeof (arr) / sizeof ((arr)[0]))

typedef unsigned char byte;
typedef unsigned char bbit_flag;
typedef unsigned int ibit_flag;
typedef unsigned long int lbit_flag;

void* malloc_assert(size_t sz);

#endif
