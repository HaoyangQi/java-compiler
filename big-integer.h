/**
 * Big Integer Arithmetic Library
 *
 * all output parameters in APIs will be allocated internally
 * (except in-place update)
*/

#pragma once
#ifndef __COMPILER_LIB_BIG_INTEGER_H__
#define __COMPILER_LIB_BIG_INTEGER_H__

#include "types.h"

#define BIG_INT_SIZE_SHORT (2)
#define BIG_INT_SIZE_INT (4)
#define BIG_INT_SIZE_LONG (8)

typedef enum
{
    BI_RAW_GROW_RIGHT,
    BI_RAW_GROW_LEFT,
} bi_grow;

typedef enum
{
    BI_CMP_GREATER,
    BI_CMP_LESS,
    BI_CMP_EQUAL,
} bi_compare;

/**
 * Big integer
 *
 * raw contains string of the integer
*/
typedef struct
{
    char* raw;
} big_integer;

void init_big_integer(big_integer* bn, unsigned int digit);
void release_big_integer(big_integer* bn);

void bi_dadd(big_integer* dest, const unsigned int digit);
bool bi_dsub(big_integer* dest, const unsigned int digit);
void bi_dmul(big_integer* dest, const unsigned int digit);
void bi_ddiv(const big_integer* num, const unsigned int denom, big_integer* q, big_integer* r);

void bi_mul2(big_integer* dest);
void bi_mul10(big_integer* dest);
void bi_div2(const big_integer* d, big_integer* q, big_integer* r);
void bi_div10(const big_integer* d, big_integer* q, big_integer* r);

void bi_mulpow10(big_integer* dest, size_t e);
void bi_divpow10(const big_integer* d, size_t e, big_integer* q, big_integer* r);

void bi_append_digit(big_integer* dest, const char digit, bi_grow grow);
bi_compare bi_cmp(const big_integer* b1, const big_integer* b2);

void bi_add(big_integer* b1, const big_integer* b2);
bool bi_sub(big_integer* b1, const big_integer* b2);
void bi_div(const big_integer* num, const big_integer* denom, big_integer* q, big_integer* r);

int64_t bi_count_bits(const big_integer* d, bool* success);
uint64_t bi_truncate(const big_integer* d, size_t bits);

#endif
