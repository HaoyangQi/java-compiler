#pragma once
#ifndef __COMPILER_TOKEN_H__
#define __COMPILER_TOKEN_H__

#include "types.h"
#include "langspec.h"
#include "file.h"
#include "symtbl.h"

/**
 * Top-level language lexeme type
 *  token JT_EOF will keep recurring once reaching end of file
*/
typedef enum
{
    JT_IDENTIFIER,
    JT_LITERAL,
    JT_OPERATOR,
    JT_SEPARATOR,
    JT_COMMENT,
    JT_EOF,
    JT_ILLEGAL,
} java_token_type;

/**
 * ID context classification
*/
typedef enum
{
    /* general ID */
    JT_ID_GENERIC,
    /* type name */
    JT_ID_TYPE,
    /* package name */
    JT_ID_PACKAGE,
    /* (part of) import name */
    JT_ID_IMPORT,
    /* class name */
    JT_ID_CLASS,
    /* interface name */
    JT_ID_INTERFACE,
    /* not an ID */
    JT_ID_NONE,
} java_identifier_type;

/**
 * Java number literal type
 *
*/
typedef enum
{
    /* decimal number (default) */
    JT_NUM_DEC,
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
    JT_NUM_NONE,
} java_number_type;

/**
 * number bit size info
*/
typedef enum
{
    /* default bit size */
    JT_NUM_BIT_LENGTH_NORMAL,
    /* long bit size */
    JT_NUM_BIT_LENGTH_LONG,
    /* not a number */
    JT_NUM_BIT_LENGTH_NONE,
} java_number_bit_length;

typedef enum
{
    /* single-line comment */
    JT_CM_SINGLE_LINE,
    /* multi-line comment */
    JT_CM_MULTI_LINE,
    /* not a cmment */
    JT_CM_NONE,
} java_comment_type;

typedef union
{
    /* secondary type: operator */
    java_operator_type op;
    /* secondary type: separator */
    java_separator_type sp;
    /* secondary type: identifier */
    java_identifier_type id;
    /* secondary type: literal */
    java_literal_type li;
    /* secondary type: comment */
    java_comment_type cm;
} java_token_subtype;

/**
 * Language token model
*/
typedef struct _java_token
{
    /* start and end position of token */
    byte* from;
    byte* to;

    /* main type */
    java_token_type type;
    /* secondary type */
    java_token_subtype subtype;

    // aux info

    /* keyword type id, RWID_MAX is set if not */
    java_reserved_word* keyword;
    /* number type id, JT_NUM_NONE is set if not */
    java_number_type number;
    /* number data size */
    java_number_bit_length number_bit_length;
} java_token;

void get_next_token(java_token* token, file_buffer* buffer, java_symbol_table* table);

#endif
