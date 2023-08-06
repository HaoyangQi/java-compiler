#pragma once
#ifndef __COMPILER_LANG_SPEC_H__
#define __COMPILER_LANG_SPEC_H__

#include "types.h"

/**
 * Java language token character set
 *
 * make sure *firstchar have no intersection, otherwise
 * tokenizer logic needs to be redesigned
*/

#define isidfirstchar(c) (isalpha(c) || (c) == '_' || (c) == '$')
#define isidchar(c) (isalnum(c) || (c) == '_' || (c) == '$')
#define isbindigit(c) ((c) == '0' || (c) == '1')

#define ishspacechar(c) ((c) == ' ' || (c) == '\t' || (c) == '\f')
#define isvspacechar(c) ((c) == '\r' || (c) == '\n')
#define isspacechar(c) (ishspacechar(c) || isvspacechar(c))

#define ishexindicator(c) ((c) == 'x' || (c) == 'X')
#define isbinaryindicator(c) ((c) == 'b' || (c) == 'B')
#define isexpindicator(c) ((c) == 'e' || (c) == 'E')
#define isexpsign(c) ((c) == '+' || (c) == '-')
#define isfractionindicator(c) ((c) == '.')

#define islongsuffix(c) ((c) == 'l' || (c) == 'L')
#define isfloatsuffix(c) ((c) == 'f' || (c) == 'F')
#define isdoublesuffix(c) ((c) == 'd' || (c) == 'D')
#define isfpsuffix(c) (isfloatsuffix(c) || isdoublesuffix(c))

/**
 * Java reserved word disable flag
 *
 * bit mask, marks word as not reserved when any one or more is set
 *
 * TODO: so far we have none
 *
 * name uses D_ prefix
*/

/**
 * Java reserved word deprecation flag
 *
 * bit mask, marks word as deprecated when any one or more is set
 * once deprecated, a word is still reserved but it will trigger a warning
 * and automatically ignored
 *
 * name uses DP_ flag
*/

#define DP_C_FAMILY 0x01 /* C-family reserved words, not meaningful in Java */

/**
 * Java reserved word id
 *
 * modifiers stay on top because they are also bit flags
 * so the number of modifiers are limited by bit length
 * of data type chosen, and consideration of other IDs
*/
typedef enum
{
    RWID_PUBLIC = 0,
    RWID_PRIVATE,
    RWID_PROTECTED,
    RWID_FINAL,
    RWID_STATIC,
    RWID_ABSTRACT,
    RWID_TRANSIENT,
    RWID_SYNCHRONIZED,
    RWID_VOLATILE,

    RWID_DEFAULT,
    RWID_IF,
    RWID_THROW,
    RWID_BOOLEAN,
    RWID_DO,
    RWID_IMPLEMENTS,
    RWID_THROWS,
    RWID_BREAK,
    RWID_DOUBLE,
    RWID_IMPORT,
    RWID_BYTE,
    RWID_ELSE,
    RWID_INSTANCEOF,
    RWID_RETURN,
    RWID_TRY,
    RWID_CASE,
    RWID_EXTENDS,
    RWID_INT,
    RWID_SHORT,
    RWID_VOID,
    RWID_CATCH,
    RWID_INTERFACE,
    RWID_CHAR,
    RWID_FINALLY,
    RWID_LONG,
    RWID_SUPER,
    RWID_WHILE,
    RWID_CLASS,
    RWID_FLOAT,
    RWID_NATIVE,
    RWID_SWITCH,
    RWID_FOR,
    RWID_NEW,
    RWID_CONTINUE,
    RWID_PACKAGE,
    RWID_THIS,

    RWID_CONST, // not valid in language, but reserved
    RWID_GOTO, // not valid in language, but reserved

    RWID_TRUE,
    RWID_FALSE,
    RWID_NULL,

    /* MAX is always at the end represents the bound */
    RWID_MAX,
} rwid;

/**
 * operators
*/
typedef enum
{
    JT_OP_ASN, /* = */
    JT_OP_GT, /* > */
    JT_OP_LT, /* < */
    JT_OP_NEG, /* ! */
    JT_OP_CPM, /* ~ */
    JT_OP_AWR, /* -> */
    JT_OP_EQ, /* == */
    JT_OP_LE, /* <= */
    JT_OP_GE, /* >= */
    JT_OP_NE, /* != */
    JT_OP_LAND, /* && */
    JT_OP_LOR, /* || */
    JT_OP_INC, /* ++ */
    JT_OP_DEC, /* -- */
    JT_OP_ADD, /* + */
    JT_OP_SUB, /* - */
    JT_OP_MUL, /* * */
    JT_OP_DIV, /* / */
    JT_OP_AND, /* & */
    JT_OP_OR, /* | */
    JT_OP_XOR, /* ^ */
    JT_OP_MOD, /* % */
    JT_OP_LS, /* << */
    JT_OP_RS, /* >> */
    JT_OP_ZFRS, /* >>> */
    JT_OP_ADDASN, /* += */
    JT_OP_SUBASN, /* -= */
    JT_OP_MULASN, /* *= */
    JT_OP_DIVASN, /* /= */
    JT_OP_ANDASN, /* &= */
    JT_OP_ORASN, /* |= */
    JT_OP_XORASN, /* ^= */
    JT_OP_MODASN, /* %= */
    JT_OP_LSASN, /* <<= */
    JT_OP_RSASN, /* >>= */
    JT_OP_ZFRSASN, /* >>>= */

    /* not an op */
    JT_OP_NONE,
} java_operator_type;

/**
 * separators
 *
 * should notice that QST(?) and CL(:)
 * combined are operator, but we keep them
 * here because individually they serve other purposes
 * (QST will serve other purposes in future Java version)
*/
typedef enum
{
    JT_SP_PL, /* ( */
    JT_SP_PR, /* ) */
    JT_SP_BL, /* { */
    JT_SP_BR, /* } */
    JT_SP_SL, /* [ */
    JT_SP_SR, /* ] */
    JT_SP_SC, /* ; */
    JT_SP_CM, /* , */
    JT_SP_AT, /* @ */
    JT_SP_QST, /* ? */
    JT_SP_CL, /* : */
    JT_SP_CC, /* :: */
    JT_SP_DOT, /* . */
    JT_SP_DDD, /* ... */

    /* not a sp */
    JT_SP_NONE,
} java_separator_type;

typedef enum
{
    JT_LI_NUM,
    JT_LI_CHAR,
    JT_LI_STR,

    /* not a sp */
    JT_LI_NONE,
} java_literal_type;

/**
 * Java reserved word
 *
 * file buffer reader will read file content into memory
 * in binary format
*/
typedef struct _java_reserved_word
{
    /* word content */
    char* content;
    /* word id */
    rwid id;
    /* disable flag */
    bbit_flag disable;
    /* deprecation flag */
    bbit_flag deprecate;
} java_reserved_word;

extern java_reserved_word java_reserved_words[];
extern const unsigned int num_java_reserved_words;

#endif
