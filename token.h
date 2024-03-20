#pragma once
#ifndef __COMPILER_TOKEN_H__
#define __COMPILER_TOKEN_H__

#include "types.h"
#include "langspec.h"
#include "file.h"
#include "symtbl.h"

/**
 * Token Classification
 *
 * token JT_EOF will keep recurring once reaching end of file
*/
typedef enum
{
    JT_IDENTIFIER,
    JT_RESERVED_WORD,
    JT_LITERAL,
    JT_OPERATOR,
    JT_SEPARATOR,
    JT_COMMENT,
    JT_EOF,
    JT_ILLEGAL,
} java_token_class;

/**
 * Java number literal type
*/
typedef enum
{
    /* decimal number (default) */
    JT_NUM_DEC = 0,
    /* headecimal number */
    JT_NUM_HEX,
    /* octal number */
    JT_NUM_OCT,
    /* binary number */
    JT_NUM_BIN,
    /* floating-point number (double) */
    JT_NUM_FP_DOUBLE,
    /* floating-point number (float) */
    JT_NUM_FP_FLOAT,
    /* not a number */
    JT_NUM_MAX,
} java_number_type;

/**
 * number bit size info
*/
typedef enum
{
    /* default bit size */
    JT_NUM_BIT_LENGTH_NORMAL = 32,
    /* long bit size */
    JT_NUM_BIT_LENGTH_LONG = 64,
} java_number_bit_length;

/**
 * number literal aux info
*/
typedef struct
{
    java_number_type type;
    java_number_bit_length bits;
} java_number_info;

/**
 * Language token model
*/
typedef struct _java_token
{
    /* start and end position of token */
    byte* from;
    byte* to;

    /* main class */
    java_token_class class;
    /* lexeme type */
    java_lexeme_type type;

    // aux info

    /* keyword info */
    java_reserved_word* keyword;
    /* number info */
    java_number_info number;
} java_token;

void init_token(java_token* token);
void get_next_token(java_token* token, file_buffer* buffer, hash_table* rw);
void delete_token(java_token* token);

#endif
