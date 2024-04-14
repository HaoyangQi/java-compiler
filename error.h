#pragma once
#ifndef __COMPILER_ERROR_H__
#define __COMPILER_ERROR_H__

#include "types.h"

/**
 * Error Type Descriptor
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
typedef uint8_t error_type;

/**
 * Definition Mask
*/
#define ERR_DEF_MASK_LEVEL ((error_type)(0xF0))
#define ERR_DEF_MASK_SCOPE ((error_type)(0x0F))

/**
 * Error Type
 *
 * index value
*/
#define JEL_UNDEFINED 0
#define JEL_INFORMATION ((error_type)(0x10))
#define JEL_WARNING ((error_type)(0x20))
#define JEL_ERROR ((error_type)(0x30))

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
#define JES_INTERNAL ((error_type)(0x01))
#define JES_RUNTIME ((error_type)(0x02))
#define JES_LEXICAL ((error_type)(0x03))
#define JES_SYNTAX ((error_type)(0x04))
#define JES_CONTEXT ((error_type)(0x05))
#define JES_OPTIMIZATION ((error_type)(0x06))
#define JES_LINKER ((error_type)(0x07))
#define JES_BUILD ((error_type)(0x08))

/**
 * Definition helpers
*/
#define DEFINE_ERROR(l, s) ((l) | (s))
#define JEL_TO_INDEX(l) (((error_type)(l)) >> 4)
#define DEFINE_RESERVED_ERROR 0
#define DEFINE_SYNTAX_ERROR ((error_type)(0x34))
#define DEFINE_CONTEXT_ERROR ((error_type)(0x35))

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
    JAVA_E_NO_DIGIT,
    JAVA_E_ILLEGAL_CHARACTER,
    JAVA_E_CHARACTER_LITERAL_ENCLOSE,
    JAVA_E_STRING_LITERAL_ENCLOSE,
    JAVA_E_MULTILINE_COMMENT_ENCLOSE,
    JAVA_E_AMBIGUIOUS_ERROR_ENTRY,
    JAVA_E_PASER_SWAP_FAILED,
    JAVA_E_PASER_DELETE_FAILED,
    JAVA_E_AMBIGUITY_DIVERGE,
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
    JAVA_E_TOP_LEVEL,
    JAVA_E_CLASS_NO_NAME,
    JAVA_E_CLASS_NO_BODY,
    JAVA_E_CLASS_NAME_DUPLICATE,
    JAVA_E_CLASS_BODY_ENCLOSE,
    java_E_CLASS_ENTENDS_NO_NAME,
    java_E_CLASS_IMPLEMENTS_NO_NAME,
    JAVA_E_INTERFACE_NO_NAME,
    JAVA_E_INTERFACE_NO_BODY,
    java_E_INTERFACE_ENTENDS_NO_NAME,
    JAVA_E_INTERFACE_BODY_ENCLOSE,
    JAVA_E_MEMBER_VAR_DUPLICATE,
    JAVA_E_MEMBER_VAR_DIM_AMBIGUOUS,
    JAVA_E_MEMBER_VAR_DIM_DUPLICATE,
    JAVA_E_MEMBER_VAR_NO_SEMICOLON,
    JAVA_E_MEMBER_METHOD_NO_SEMICOLON,
    JAVA_E_MEMBER_NO_TYPE,
    JAVA_E_MEMBER_NO_NAME,
    JAVA_E_MEMBER_AMBIGUOUS,
    JAVA_E_STATIC_INITIALIZER_NO_BODY,
    JAVA_E_BLOCK_ENCLOSE,
    JAVA_E_STATEMENT_UNRECOGNIZED,
    JAVA_E_STATEMENT_SWITCH,
    JAVA_E_STATEMENT_DO,
    JAVA_E_STATEMENT_BREAK,
    JAVA_E_STATEMENT_CONTINUE,
    JAVA_E_STATEMENT_RETURN,
    JAVA_E_STATEMENT_SYNCHRONIZED,
    JAVA_E_STATEMENT_THROW,
    JAVA_E_STATEMENT_TRY,
    JAVA_E_STATEMENT_IF,
    JAVA_E_STATEMENT_ELSE,
    JAVA_E_STATEMENT_WHILE,
    JAVA_E_STATEMENT_FOR,
    JAVA_E_STATEMENT_FOR_INITIALIZER_NO_SEMICOLON,
    JAVA_E_STATEMENT_FOR_CONDITION_NO_SEMICOLON,
    JAVA_E_STATEMENT_LABEL,
    JAVA_E_STATEMENT_CATCH,
    JAVA_E_STATEMENT_FINALLY,
    JAVA_E_STATEMENT_SWITCH_LABEL,
    JAVA_E_CONSTRUCTOR,
    JAVA_E_CONSTRUCTOR_INVOKE,
    JAVA_E_METHOD_DECLARATION,
    JAVA_E_METHOD_INVOKE,
    JAVA_E_FORMAL_PARAMETER,
    JAVA_E_THROWS_NO_TYPE,
    JAVA_E_NO_ARGUMENT,
    JAVA_E_ARRAY_INITIALIZER,
    JAVA_E_ARRAY_CREATION,
    JAVA_E_ARRAY_ACCESS,
    JAVA_E_OBJECT_CREATION,
    JAVA_E_INSTANCE_CREATION,
    JAVA_E_EXPRESSION_NO_OPERAND,
    JAVA_E_EXPRESSION_TOO_MANY_OPERAND,
    JAVA_E_EXPRESSION_INCOMPLETE_TERNARY,
    JAVA_E_EXPRESSION_UNHANDLED_BLOCK_CONTEXT,
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

    JAVA_E_MAX,
} java_error_id;

/**
 * Error Definitions
 *
 * ONLYSTATIC: this instance contains static data
 * shared across all compile tasks
 *
 * use java_error_logger for task-specific data
*/
typedef struct
{
    // type of each error ID
    error_type descriptor;
    // (parser) context name
    char* context;
    // message text of each error ID
    char* message;
} java_error_definition;

/**
 * Error Summary
*/
typedef struct
{
    // number of info
    size_t num_info;
    // number of warnings
    size_t num_warn;
    // number of errors
    size_t num_err;
} java_error_summary;

/**
 * Error Entry Type
 *
 * ERROR_ENTRY_AMBIGUITY does not contain
 * actual error info; instead, it manages an array
 * of error_stack for every pathway of ambguity
*/
typedef enum
{
    ERROR_ENTRY_NORMAL,
    ERROR_ENTRY_AMBIGUITY,
} java_error_entry_type;

/**
 * Error Entry
 *
 * Bi-directional linked list
*/
typedef struct _java_error_entry
{
    java_error_entry_type type;
    java_error_id id;
    line begin;
    line end;
    char* msg;

    struct _java_error_entry* prev;
    struct _java_error_entry* next;

    struct
    {
        struct _java_error_stack* arr;
        size_t len;
        size_t size;
    } ambiguity;
} java_error_entry;

/**
 * Error Stack
*/
typedef struct _java_error_stack
{
    // if this stack belong to an ERROR_ENTRY_AMBIGUITY, this field will be set
    struct _java_error_stack* amb_parent;
    // summary, excluding ambiguity entries
    java_error_summary summary;
    // number of ambiguity entry
    size_t num_ambiguity;

    java_error_entry* first;
    java_error_entry* last;
} java_error_stack;

/**
 * Error Data Instance
 *
 * Definition is indexed by java_error_id
 * summary will stop counting at first entry with divergence
*/
typedef struct
{
    // error definitions
    java_error_definition* def;

    // main error stack
    java_error_stack main_stream;
    // current error stream
    java_error_stack* current_stream;
} java_error_logger;

void init_error_logger(java_error_logger* logger);
void release_error_logger(java_error_logger* logger);
void clear_error_logger(java_error_logger* logger);

bool error_logger_log_ignore(java_error_logger* logger, java_error_id id);
void error_logger_vslog(java_error_logger* logger, line* begin, line* end, java_error_id id, va_list* arguments);
void error_logger_log(java_error_logger* logger, line* begin, line* end, java_error_id id, ...);
void error_logger_ambiguity_begin(java_error_logger* logger);
void error_logger_ambiguity_end(java_error_logger* logger);
bool error_logger_ambiguity_resolve(java_error_logger* logger, java_error_entry* entry, size_t idx);
size_t error_logger_count_main_ambiguity(java_error_logger* logger);
size_t error_logger_count_current_ambiguity(java_error_logger* logger);
java_error_entry* error_logger_get_main_top(const java_error_logger* logger);
java_error_entry* error_logger_get_current_top(const java_error_logger* logger);
void error_logger_count_main_summary(const java_error_logger* logger, java_error_summary* out);
void error_logger_count_current_summary(const java_error_logger* logger, java_error_summary* out);
bool error_logger_if_main_stack_no_error(const java_error_logger* logger);
bool error_logger_if_current_stack_no_error(const java_error_logger* logger);
char* error_logger_get_context_string(const java_error_logger* logger, java_error_id id);

#endif
