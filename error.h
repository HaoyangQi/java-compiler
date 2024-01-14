#pragma once
#ifndef __COMPILER_ERROR_H__
#define __COMPILER_ERROR_H__

#include "types.h"

/**
 * Error Definition
 *
 * 0000 0000 0000 0000
 * ____
 * |    ______________
 * |    |
 * |    Scope (12 bits)
 * Type (4 bits)
*/
typedef unsigned short error_definiton;

/**
 * Definition Mask
*/
#define ERR_DEF_MASK_LEVEL ((error_definiton)(0xF000))
#define ERR_DEF_MASK_SCOPE ((error_definiton)(0x0FFF))

/**
 * Error Type
 *
 * Bit flag
*/
#define JEL_UNDEFINED 0
#define JEL_INFORMATION ((error_definiton)(0x1000))
#define JEL_WARNING ((error_definiton)(0x2000))
#define JEL_ERROR ((error_definiton)(0x4000))

/**
 * Error Scope
 *
 * Bit flag
 *
 * INTERNAL: error in compiler logic
 * RUNTIME: memory failure, I/O error, etc
 * LEXICAL: error from lexer
 * SYNTAX: error from parser
 * CONTEXT: error from IR
 * OPTIMIZATION: error from optimizer
 * LINKER: error from linker
 * BUILD: error from building process
*/
#define JES_UNDEFINED 0
#define JES_INTERNAL ((error_definiton)(0x0001))
#define JES_RUNTIME ((error_definiton)(0x0002))
#define JES_LEXICAL ((error_definiton)(0x0004))
#define JES_SYNTAX ((error_definiton)(0x0008))
#define JES_CONTEXT ((error_definiton)(0x0010))
#define JES_OPTIMIZATION ((error_definiton)(0x0020))
#define JES_LINKER ((error_definiton)(0x0040))
#define JES_BUILD ((error_definiton)(0x0080))

/**
 * Definition helpers
*/
#define DEFINE_ERROR(l, s) ((l) | (s))
#define DEFINE_RESERVED_ERROR 0
#define DEFINE_SYNTAX_ERROR ((error_definiton)(0x4008))

/**
 * Error Message ID
 *
 * definition = map[ID]
 *
 * ID = 0 is reserved
*/
typedef enum
{
    JAVA_E_RESERVED = 0,
    JAVA_E_TRAILING_CONTENT,
    JAVA_E_PKG_DECL_NO_NAME,
    JAVA_E_PKG_DECL_NO_SEMICOLON,

    JAVA_E_MAX,
} java_error_id;

/**
 * Error Entry
 *
 * Bi-directional linked list
*/
typedef struct _java_error_entry
{
    java_error_id id;
    size_t ln;
    size_t col;

    struct _java_error_entry* prev;
    struct _java_error_entry* next;
} java_error_entry;

/**
 * Error Data Instance
 *
 * Definition is indexed by java_error_id
*/
typedef struct
{
    /* ID-to-definition mapping */
    error_definiton* definition;
    /* ID-to-message mapping */
    char** message;
    /* error stack */
    java_error_entry* data;
    /* error stack top */
    java_error_entry* top;
} java_error;

void init_error(java_error* error);
void release_error(java_error* error);
void clear_error(java_error* error);

java_error_entry* error_stack_top(java_error* error);
bool error_stack_empty(java_error* error);
void error_stack_rewind(java_error* error, java_error_entry* new_top);
bool error_stack_pop(java_error* error);
void error_stack_push(java_error* error, java_error_entry* item);

void error_log(java_error* error, java_error_id id, size_t ln, size_t col);

#endif
