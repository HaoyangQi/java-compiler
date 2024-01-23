#pragma once
#ifndef __COMPILER_ERROR_H__
#define __COMPILER_ERROR_H__

#include "types.h"

/**
 * Error Definition
 *
 * 0000 0000
 * ____
 * |    ____
 * |    |
 * |    Scope (4 bits)
 * Type (4 bits)
 *
 * both parts are described as integer index value,
 * numbered from 0 to 15 (0xF) consecutively
*/
typedef unsigned char error_definiton;

/**
 * Definition Mask
*/
#define ERR_DEF_MASK_LEVEL ((error_definiton)(0xF0))
#define ERR_DEF_MASK_SCOPE ((error_definiton)(0x0F))

/**
 * Error Type
 *
 * index value
*/
#define JEL_UNDEFINED 0
#define JEL_INFORMATION ((error_definiton)(0x10))
#define JEL_WARNING ((error_definiton)(0x20))
#define JEL_ERROR ((error_definiton)(0x30))

/**
 * Error Scope
 *
 * index value
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
#define JES_INTERNAL ((error_definiton)(0x01))
#define JES_RUNTIME ((error_definiton)(0x02))
#define JES_LEXICAL ((error_definiton)(0x03))
#define JES_SYNTAX ((error_definiton)(0x04))
#define JES_CONTEXT ((error_definiton)(0x05))
#define JES_OPTIMIZATION ((error_definiton)(0x06))
#define JES_LINKER ((error_definiton)(0x07))
#define JES_BUILD ((error_definiton)(0x08))

/**
 * Definition helpers
*/
#define DEFINE_ERROR(l, s) ((l) | (s))
#define JEL_TO_INDEX(l) (((error_definiton)(l)) >> 4)
#define DEFINE_RESERVED_ERROR 0
#define DEFINE_SYNTAX_ERROR ((error_definiton)(0x34))

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
    JAVA_E_FILE_NO_PATH,
    JAVA_E_FILE_OPEN_FAILED,
    JAVA_E_FILE_SIZE_NOT_MATCH,
    JAVA_E_TRAILING_CONTENT,
    JAVA_E_PKG_DECL_NO_NAME,
    JAVA_E_PKG_DECL_NO_SEMICOLON,
    JAVA_E_IMPORT_NO_NAME,
    JAVA_E_IMPORT_NO_SEMICOLON,
    JAVA_E_IMPORT_AMBIGUOUS,
    JAVA_E_IMPORT_DUPLICATE,

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
    /* number of info */
    size_t num_info;
    /* number of warnings */
    size_t num_warn;
    /* number of errors */
    size_t num_err;
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
size_t error_count(java_error* error, error_definiton error_level);

#endif
