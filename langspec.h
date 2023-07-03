#pragma once
#ifndef __COMPILER_LANG_SPEC_H__
#define __COMPILER_LANG_SPEC_H__

#include "types.h"

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
typedef enum _rwid
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
} rwid;

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
    bit_flag disable;
    /* deprecation flag */
    bit_flag deprecate;
} java_reserved_word;

extern java_reserved_word java_reserved_words[];
extern const unsigned int num_java_reserved_words;

#endif
