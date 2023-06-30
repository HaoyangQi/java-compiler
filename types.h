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

#define ASSERT_ALLOCATION(ptr) assert((ptr) != NULL)

typedef unsigned char byte;

#endif
