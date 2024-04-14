#pragma once
#ifndef __COMPILER_LEXER_H__
#define __COMPILER_LEXER_H__

#include "types.h"
#include "langspec.h"
#include "file.h"
#include "symtbl.h"
#include "error.h"

#define lexer_error_missing_token(parser, token, id, token_name) \
    lexer_error(lexer, token, id, token_name, error_logger_get_context_string(lexer->logger, id))

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

    // line info

    line ln_begin;
    line ln_end;

    // aux info

    /* keyword info */
    java_reserved_word* keyword;
    /* number info */
    java_number_info number;
} java_token;

/**
 * Lexer Context
*/
typedef struct
{
    // copy mark
    bool is_copy;
    // file buffer
    file_buffer* buffer;
    // reserved words lookup table
    hash_table* rw;
    // error logger
    java_error_logger* logger;

    /**
     * TODO: future work: additional aid for resolving lexical ambiguity
     * if not JLT_UNDEFINED or JLT_MAX, it will check ambiguity as an
     * complementary of longest-matching algorithm run by lexer
     *
     * e.g. >>> but expect=JLT_SYM_ANGLE_BRACKET_CLOSE(>), then
     * lexer_next_token will get a ">" instead of ">>>" by default
     *
     * this is an transient flag, every lexer_next_token will set it
     * back to JLT_MAX, successful or not
     *
     * whe this is setm an error will be issued if next one cannot be
     * extracted as "expected"
    */
    java_lexeme_type expect;

    // current line info
    line ln_cur;
    // last character location in previous line
    line ln_prev;
} java_lexer;

void init_token(java_token* token);
void release_token(java_token* token);
void delete_token(java_token* token);

void init_lexer(java_lexer* lexer, file_buffer* buffer, hash_table* rw, java_error_logger* logger);
java_lexer* copy_lexer(const java_lexer* lexer);
void release_lexer(java_lexer* lexer);

void lexer_error(java_lexer* lexer, java_token* token, java_error_id id, ...);
void lexer_expect(java_lexer* lexer, java_lexeme_type token_type);
void lexer_next_token(java_lexer* lexer, java_token* token);

#endif
