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
 * Java Lexeme Type ID
 *
 * JLT_RWD_*: reserved words
 * JLT_SYM_*: all sequences made by non-letter symbols
 * JLT_LTR_*: literals
 * JLT_CMT_*: comments
 *
 * Modifier words stay on top for bit field data,
 * the number represents which bit (from LSB) it occupies
 * That means we can have at most 64 modifiers, if we
 * have more than that in the future, we need to redesign
 * this
*/
typedef enum
{
    // modifier words
    JLT_RWD_PUBLIC = 0,
    JLT_RWD_PRIVATE = 1,
    JLT_RWD_PROTECTED = 2,
    JLT_RWD_FINAL = 3,
    JLT_RWD_STATIC = 4,
    JLT_RWD_ABSTRACT = 5,
    JLT_RWD_TRANSIENT = 6,
    JLT_RWD_SYNCHRONIZED = 7,
    JLT_RWD_VOLATILE = 8,
    // type words
    JLT_RWD_BOOLEAN,
    JLT_RWD_DOUBLE,
    JLT_RWD_BYTE,
    JLT_RWD_INT,
    JLT_RWD_SHORT,
    JLT_RWD_VOID,
    JLT_RWD_CHAR,
    JLT_RWD_LONG,
    JLT_RWD_FLOAT,
    // keywords
    JLT_RWD_DEFAULT,
    JLT_RWD_IF,
    JLT_RWD_THROW,
    JLT_RWD_DO,
    JLT_RWD_IMPLEMENTS,
    JLT_RWD_THROWS,
    JLT_RWD_BREAK,
    JLT_RWD_IMPORT,
    JLT_RWD_ELSE,
    JLT_RWD_INSTANCEOF,
    JLT_RWD_RETURN,
    JLT_RWD_TRY,
    JLT_RWD_CASE,
    JLT_RWD_EXTENDS,
    JLT_RWD_CATCH,
    JLT_RWD_INTERFACE,
    JLT_RWD_FINALLY,
    JLT_RWD_SUPER,
    JLT_RWD_WHILE,
    JLT_RWD_CLASS,
    JLT_RWD_NATIVE,
    JLT_RWD_SWITCH,
    JLT_RWD_FOR,
    JLT_RWD_NEW,
    JLT_RWD_CONTINUE,
    JLT_RWD_PACKAGE,
    JLT_RWD_THIS,
    // not valid in language, but reserved
    JLT_RWD_CONST,
    JLT_RWD_GOTO,
    // literals but are words
    JLT_RWD_TRUE,
    JLT_RWD_FALSE,
    JLT_RWD_NULL,

    // symbol character set
    // this part uses English word to describe
    JLT_SYM_EQUAL, /* = */
    JLT_SYM_ANGLE_BRACKET_OPEN, /* < */
    JLT_SYM_ANGLE_BRACKET_CLOSE, /* > */
    JLT_SYM_EXCALMATION, /* ! */
    JLT_SYM_TILDE, /* ~ */
    JLT_SYM_PLUS, /* + */
    JLT_SYM_MINUS, /* - */
    JLT_SYM_ASTERISK, /* * */
    JLT_SYM_FORWARD_SLASH, /* / */
    JLT_SYM_AMPERSAND, /* & */
    JLT_SYM_PIPE, /* | */
    JLT_SYM_CARET, /* ^ */
    JLT_SYM_PERCENT, /* % */
    JLT_SYM_PARENTHESIS_OPEN, /* ( */
    JLT_SYM_PARENTHESIS_CLOSE, /* ) */
    JLT_SYM_BRACE_OPEN, /* { */
    JLT_SYM_BRACE_CLOSE, /* } */
    JLT_SYM_BRACKET_OPEN, /* [ */
    JLT_SYM_BRACKET_CLOSE, /* ] */
    JLT_SYM_SEMICOLON, /* ; */
    JLT_SYM_COMMA, /* , */
    JLT_SYM_AT, /* @ */
    JLT_SYM_QUESTION, /* ? */
    JLT_SYM_COLON, /* : */
    JLT_SYM_DOT, /* . */

    // symbol sequences specifically for Java
    JLT_SYM_METHOD_REFERENCE, /* :: */
    JLT_SYM_VARIADIC, /* ... */
    JLT_SYM_ARROW, /* -> */
    JLT_SYM_RELATIONAL_EQUAL, /* == */
    JLT_SYM_LESS_EQUAL, /* <= */
    JLT_SYM_GREATER_EQUAL, /* >= */
    JLT_SYM_NOT_EQUAL, /* != */
    JLT_SYM_LOGIC_AND, /* && */
    JLT_SYM_LOGIC_OR, /* || */
    JLT_SYM_INCREMENT, /* ++ */
    JLT_SYM_DECREMENT, /* -- */
    JLT_SYM_LEFT_SHIFT, /* << */
    JLT_SYM_RIGHT_SHIFT, /* >> */
    JLT_SYM_RIGHT_SHIFT_UNSIGNED, /* >>> */
    JLT_SYM_ADD_ASSIGNMENT, /* += */
    JLT_SYM_SUBTRACT_ASSIGNMENT, /* -= */
    JLT_SYM_MULTIPLY_ASSIGNMENT, /* *= */
    JLT_SYM_DIVIDE_ASSIGNMENT, /* /= */
    JLT_SYM_BIT_AND_ASSIGNMENT, /* &= */
    JLT_SYM_BIT_OR_ASSIGNMENT, /* |= */
    JLT_SYM_BIT_XOR_ASSIGNMENT, /* ^= */
    JLT_SYM_MODULO_ASSIGNMENT, /* %= */
    JLT_SYM_LEFT_SHIFT_ASSIGNMENT, /* <<= */
    JLT_SYM_RIGHT_SHIFT_ASSIGNMENT, /* >>= */
    JLT_SYM_RIGHT_SHIFT_UNSIGNED_ASSIGNMENT, /* >>>= */

    // literals
    JLT_LTR_NUMBER,
    JLT_LTR_CHARACTER,
    JLT_LTR_STRING,

    // comments
    JLT_CMT_SINGLE_LINE,
    JLT_CMT_MULTI_LINE,

    // Not valid
    JLT_MAX,
} java_lexeme_type;

/**
 * NOTE: do NOT write macros below for types with >= 2 bounds,
 * because it might be misused and impact performance
 *
 * write them as a function in parser-util.c
*/

#define JAVA_LEXEME_MODIFIER_WORD(type) ((type) <= JLT_RWD_VOLATILE)
#define JAVA_LEXEME_MODIFIER_OR_TYPE_WORD(type) ((type) <= JLT_RWD_FLOAT)

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
    java_lexeme_type id;
    /* disable flag */
    bbit_flag disable;
    /* deprecation flag */
    bbit_flag deprecate;
} java_reserved_word;

extern java_reserved_word java_reserved_words[];
extern const unsigned int num_java_reserved_words;

#endif
