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
typedef unsigned int bit_flag;
typedef unsigned long int long_bit_flag;

#endif
