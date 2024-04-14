#pragma once
#ifndef __COMPILER_NUMBER_H__
#define __COMPILER_NUMBER_H__

#include "lexer.h"
#include "types.h"

/**
 * number truncation result flag (bit mask)
 *
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

/**
 * binary data information
 *
 * 1 major data: number AND stream
 *
 * s2b uses "stream" for character and string literals,
 * and uses "number" for number literals
 *
 * "stream" also contain aux info: byte count and a flag
 * indicates whether the stream contains wide character
*/
typedef struct
{
    uint64_t number;

    char* stream;
    size_t len;
    bool wide_char;
} binary_data;

void init_binary_data(binary_data* data);
number_truncation_status s2b(const char* content, java_number_type type, binary_data* data);

#endif
