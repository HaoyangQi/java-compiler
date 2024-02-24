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
typedef unsigned char error_descriptor;

/**
 * Definition Mask
*/
#define ERR_DEF_MASK_LEVEL ((error_descriptor)(0xF0))
#define ERR_DEF_MASK_SCOPE ((error_descriptor)(0x0F))

/**
 * Error Type
 *
 * index value
*/
#define JEL_UNDEFINED 0
#define JEL_INFORMATION ((error_descriptor)(0x10))
#define JEL_WARNING ((error_descriptor)(0x20))
#define JEL_ERROR ((error_descriptor)(0x30))

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
#define JES_INTERNAL ((error_descriptor)(0x01))
#define JES_RUNTIME ((error_descriptor)(0x02))
#define JES_LEXICAL ((error_descriptor)(0x03))
#define JES_SYNTAX ((error_descriptor)(0x04))
#define JES_CONTEXT ((error_descriptor)(0x05))
#define JES_OPTIMIZATION ((error_descriptor)(0x06))
#define JES_LINKER ((error_descriptor)(0x07))
#define JES_BUILD ((error_descriptor)(0x08))

/**
 * Definition helpers
*/
#define DEFINE_ERROR(l, s) ((l) | (s))
#define JEL_TO_INDEX(l) (((error_descriptor)(l)) >> 4)
#define DEFINE_RESERVED_ERROR 0
#define DEFINE_SYNTAX_ERROR ((error_descriptor)(0x34))
#define DEFINE_CONTEXT_ERROR ((error_descriptor)(0x35))

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
    JAVA_E_CLASS_NO_NAME,
    JAVA_E_CLASS_NO_BODY,
    JAVA_E_CLASS_NAME_DUPLICATE,
    JAVA_E_CLASS_BODY_ENCLOSE,
    JAVA_E_MEMBER_VAR_DUPLICATE,
    JAVA_E_MEMBER_VAR_DIM_AMBIGUOUS,
    JAVA_E_MEMBER_VAR_DIM_DUPLICATE,
    JAVA_E_MEMBER_VAR_NO_SEMICOLON,
    JAVA_E_MEMBER_NO_TYPE,
    JAVA_E_MEMBER_NO_NAME,
    JAVA_E_MEMBER_AMBIGUOUS,
    JAVA_E_EXPRESSION_NO_OPERAND,
    JAVA_E_EXPRESSION_NO_OPERATOR,
    JAVA_E_EXPRESSION_NO_LVALUE,
    JAVA_E_EXPRESSION_LITERAL_LVALUE,
    JAVA_E_EXPRESSION_NO_SEMICOLON,
    JAVA_E_NUMBER_OVERFLOW_INT8,
    JAVA_E_NUMBER_OVERFLOW_INT16,
    JAVA_E_NUMBER_OVERFLOW_INT32,
    JAVA_E_NUMBER_OVERFLOW_INT64,
    JAVA_E_NUMBER_OVERFLOW_U16,
    JAVA_E_NUMBER_OVERFLOW_FP32_EXP,
    JAVA_E_NUMBER_OVERFLOW_FP64_EXP,
    JAVA_E_PART_EXPONENT_OVERFLOW,
    JAVA_E_PART_INTEGER_OVERFLOW,
    JAVA_E_LOCAL_VAR_DUPLICATE,
    JAVA_E_LOCAL_VAR_DIM_AMBIGUOUS,
    JAVA_E_LOCAL_VAR_DIM_DUPLICATE,
    JAVA_E_LOCAL_VAR_NO_SEMICOLON,
    JAVA_E_VAR_NO_DECLARATOR,
    JAVA_E_VAR_NO_ARR_ENCLOSE,
    JAVA_E_VAR_NO_INITIALIZER,
    JAVA_E_PARAM_DUPLICATE,
    JAVA_E_PARAM_DIM_AMBIGUOUS,
    JAVA_E_PARAM_DIM_DUPLICATE,
    JAVA_E_METHOD_DUPLICATE,
    JAVA_E_METHOD_DIM_AMBIGUOUS,
    JAVA_E_METHOD_DIM_DUPLICATE,
    JAVA_E_IF_LOCAL_VAR_DECL,
    JAVA_E_WHILE_LOCAL_VAR_DECL,
    JAVA_E_REF_UNDEFINED,
    JAVA_E_BREAK_UNBOUND,
    JAVA_E_CONTINUE_UNBOUND,
    JAVA_E_EXPRESSION_LIST_INCOMPLETE,
    JAVA_E_EXPRESSION_PARENTHESIS,
    JAVA_E_TYPE_NO_ARR_ENCLOSE,

    /**
     * Ambiguity Error Flags
     *
     * INTERNAL:
     * the following are not errors
     * they are placeholders for
     * error parser
     *
     * JAVA_E_AMBIGUITY_START
     * Err |
     * Err |---JAVA_E_AMBIGUITY_PATH_1
     * ... |
     * JAVA_E_AMBIGUITY_SEPARATOR
     * Err |
     * Err |---JAVA_E_AMBIGUITY_PATH_2
     * ... |
     * JAVA_E_AMBIGUITY_END
     *
     * If ambiguity is resolved, the ID
     * JAVA_E_AMBIGUITY_START will be changed
     * into JAVA_E_AMBIGUITY_PATH_1 or
     * JAVA_E_AMBIGUITY_PATH_2 accordingly
    */

    JAVA_E_AMBIGUITY_START,
    JAVA_E_AMBIGUITY_PATH_1,
    JAVA_E_AMBIGUITY_PATH_2,
    JAVA_E_AMBIGUITY_SEPARATOR,
    JAVA_E_AMBIGUITY_END,

    JAVA_E_MAX,
} java_error_id;

/**
 * Error Definitions
 *
 * ONLYSTATIC: this instance contains static data
 * shared across all compile tasks
 *
 * use java_error_stack for task-specific data
*/
typedef struct
{
    /* ID-to-descriptor mapping */
    error_descriptor* descriptor;
    /* ID-to-message mapping */
    char** message;
} java_error_definition;

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
    /* error definitions */
    java_error_definition* def;
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
} java_error_stack;

void init_error_definition(java_error_definition* err_def);
void release_error_definition(java_error_definition* err_def);

void init_error_stack(java_error_stack* error, java_error_definition* def);
void release_error_stack(java_error_stack* error);
void clear_error_stack(java_error_stack* error);

java_error_entry* error_stack_top(java_error_stack* error);
bool error_stack_empty(java_error_stack* error);
void error_stack_rewind(java_error_stack* error, java_error_entry* new_top);
bool error_stack_pop(java_error_stack* error);
void error_stack_push(java_error_stack* error, java_error_entry* item);
void error_stack_concat(java_error_stack* dest, java_error_stack* src);
void error_stack_entry_delete(java_error_stack* error, java_error_entry* entry);

void error_log(java_error_stack* error, java_error_id id, size_t ln, size_t col);
size_t error_count(java_error_stack* error, error_descriptor error_level);

#endif
