#pragma once
#ifndef __COMPILER_NUMBER_H__
#define __COMPILER_NUMBER_H__

#include "token.h"
#include "types.h"

/**
 * number truncation result flag (bit mask)
 *
 * interger overflow flags includes each other,
 * so the value is carefully selected
 *
 * e.g. if a number overflows u8, then it also overflows int8
*/
typedef enum
{
    NTS_SUCCESS = 0,
    NTS_OVERFLOW_U8 = 0x0001,
    NTS_OVERFLOW_U16 = 0x0002,
    NTS_OVERFLOW_U32 = 0x0004,
    NTS_OVERFLOW_U64 = 0x0008,
    NTS_OVERFLOW_FP32_EXP = 0x0010,
    NTS_OVERFLOW_FP32_MAN = 0x0020,
    NTS_OVERFLOW_FP64_EXP = 0x0040,
    NTS_OVERFLOW_FP64_MAN = 0x0080,
    NTS_OVERFLOW_INT8 = 0x0100,
    NTS_OVERFLOW_INT16 = 0x0200,
    NTS_OVERFLOW_INT32 = 0x0400,
    NTS_OVERFLOW_INT64 = 0x0800,
} number_truncation_status;

number_truncation_status s2b(const char* content, java_number_type type, uint64_t* data);

#endif
