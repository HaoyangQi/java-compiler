#include "error.h"

/**
 * Common Error Messages
*/
static char* ERR_MSG_NO_SEMICOLON = "Expected ';'.";
static char* ERR_MSG_NO_RIGHT_BRACE = "Expected '}'.";

/**
 * Initialize Error Manager
*/
void init_error(java_error* error)
{
    error->definition = (error_definiton*)malloc_assert(sizeof(error_definiton) * JAVA_E_MAX);
    error->message = (char**)malloc_assert(sizeof(char*) * JAVA_E_MAX);
    error->data = NULL;
    error->top = NULL;
    error->num_info = 0;
    error->num_warn = 0;
    error->num_err = 0;

    /* Error Definitions */

    error->definition[JAVA_E_RESERVED] = DEFINE_RESERVED_ERROR;
    error->definition[JAVA_E_FILE_NO_PATH] = DEFINE_ERROR(JEL_ERROR, JES_RUNTIME);
    error->definition[JAVA_E_FILE_OPEN_FAILED] = DEFINE_ERROR(JEL_ERROR, JES_RUNTIME);
    error->definition[JAVA_E_FILE_SIZE_NOT_MATCH] = DEFINE_ERROR(JEL_ERROR, JES_RUNTIME);
    error->definition[JAVA_E_TRAILING_CONTENT] = DEFINE_ERROR(JEL_WARNING, JES_LEXICAL);
    error->definition[JAVA_E_PKG_DECL_NO_NAME] = DEFINE_SYNTAX_ERROR;
    error->definition[JAVA_E_PKG_DECL_NO_SEMICOLON] = DEFINE_SYNTAX_ERROR;
    error->definition[JAVA_E_IMPORT_NO_NAME] = DEFINE_SYNTAX_ERROR;
    error->definition[JAVA_E_IMPORT_NO_SEMICOLON] = DEFINE_SYNTAX_ERROR;
    error->definition[JAVA_E_IMPORT_AMBIGUOUS] = DEFINE_CONTEXT_ERROR;
    error->definition[JAVA_E_IMPORT_DUPLICATE] = DEFINE_ERROR(JEL_WARNING, JES_CONTEXT);
    error->definition[JAVA_E_CLASS_NO_NAME] = DEFINE_SYNTAX_ERROR;
    error->definition[JAVA_E_CLASS_NO_BODY] = DEFINE_SYNTAX_ERROR;
    error->definition[JAVA_E_CLASS_NAME_DUPLICATE] = DEFINE_CONTEXT_ERROR;
    error->definition[JAVA_E_CLASS_BODY_ENCLOSE] = DEFINE_CONTEXT_ERROR;
    error->definition[JAVA_E_MEMBER_VAR_DUPLICATE] = DEFINE_CONTEXT_ERROR;
    error->definition[JAVA_E_MEMBER_VAR_DIM_AMBIGUOUS] = DEFINE_CONTEXT_ERROR;
    error->definition[JAVA_E_MEMBER_VAR_DIM_DUPLICATE] = DEFINE_ERROR(JEL_WARNING, JES_CONTEXT);
    error->definition[JAVA_E_MEMBER_VAR_NO_SEMICOLON] = DEFINE_SYNTAX_ERROR;
    error->definition[JAVA_E_MEMBER_NO_TYPE] = DEFINE_SYNTAX_ERROR;
    error->definition[JAVA_E_MEMBER_NO_NAME] = DEFINE_SYNTAX_ERROR;
    error->definition[JAVA_E_MEMBER_AMBIGUOUS] = DEFINE_SYNTAX_ERROR;
    error->definition[JAVA_E_EXPRESSION_NO_OPERAND] = DEFINE_SYNTAX_ERROR;
    error->definition[JAVA_E_NUMBER_OVERFLOW] = DEFINE_CONTEXT_ERROR;

    /* Error Messages */

    error->message[JAVA_E_RESERVED] = "(Unregistered error)";
    error->message[JAVA_E_FILE_NO_PATH] = "File name is not valid.";
    error->message[JAVA_E_FILE_OPEN_FAILED] = "File '%s' failed to open.";
    error->message[JAVA_E_FILE_SIZE_NOT_MATCH] = "File '%s' has incorrect loading size comparing to what is reported from file system.";
    error->message[JAVA_E_TRAILING_CONTENT] = "Unrecognized trailing content.";
    error->message[JAVA_E_PKG_DECL_NO_NAME] = "Expected 'name' in package declaration.";
    error->message[JAVA_E_PKG_DECL_NO_SEMICOLON] = ERR_MSG_NO_SEMICOLON;
    error->message[JAVA_E_IMPORT_NO_NAME] = "Expected 'name' in import declaration.";
    error->message[JAVA_E_IMPORT_NO_SEMICOLON] = ERR_MSG_NO_SEMICOLON;
    error->message[JAVA_E_IMPORT_AMBIGUOUS] = "Ambiguous import type name, resolution of same type diverges.";
    error->message[JAVA_E_IMPORT_DUPLICATE] = "Duplicated import, discarded.";
    error->message[JAVA_E_CLASS_NO_NAME] = "Expected 'name' in class declaration.";
    error->message[JAVA_E_CLASS_NO_BODY] = "Expected class body in class declaration.";
    error->message[JAVA_E_CLASS_NAME_DUPLICATE] = "Duplicated class name.";
    error->message[JAVA_E_CLASS_BODY_ENCLOSE] = ERR_MSG_NO_RIGHT_BRACE;
    error->message[JAVA_E_MEMBER_VAR_DUPLICATE] = "Duplicated member variable.";
    error->message[JAVA_E_MEMBER_VAR_DIM_AMBIGUOUS] = "Dimension definition mismatched.";
    error->message[JAVA_E_MEMBER_VAR_DIM_DUPLICATE] = "Duplicated dimension definition.";
    error->message[JAVA_E_MEMBER_VAR_NO_SEMICOLON] = ERR_MSG_NO_SEMICOLON;
    error->message[JAVA_E_MEMBER_NO_TYPE] = "Expected 'type' in member declaration.";
    error->message[JAVA_E_MEMBER_NO_NAME] = "Expected 'name' in member declaration.";
    error->message[JAVA_E_MEMBER_AMBIGUOUS] = "Incomplete member declaration.";
    error->message[JAVA_E_EXPRESSION_NO_OPERAND] = "Invalid expression: expected operand.";
    error->message[JAVA_E_NUMBER_OVERFLOW] = "Invalid number: number overflows.";
}

/**
 * Release Error Manager
*/
void release_error(java_error* error)
{
    free(error->definition);
    free(error->message);
    clear_error(error);
}

/**
 * Clear Error Stack
*/
void clear_error(java_error* error)
{
    java_error_entry* cur = error->data;

    while (cur)
    {
        error->data = cur->next;
        free(cur);
        cur = error->data;
    }

    error->data = NULL;
    error->top = NULL;
}

/**
 * Get error stack state
*/
java_error_entry* error_stack_top(java_error* error)
{
    return error->top;
}

/**
 * Check is stack is empty
*/
bool error_stack_empty(java_error* error)
{
    return error->data == NULL;
}

/**
 * Stack rewind to a new top
*/
void error_stack_rewind(java_error* error, java_error_entry* new_top)
{
    while (error->top != new_top && error_stack_pop(error));
}

/**
 * Pop stack top element
*/
bool error_stack_pop(java_error* error)
{
    if (error->top == NULL)
    {
        return false;
    }

    java_error_entry* top = error->top;

    if (top->prev)
    {
        top->prev->next = NULL;
    }
    else
    {
        error->data = NULL;
    }

    error->top = top->prev;
    free(top);

    return true;
}

/**
 * Push an entry to error stack
*/
void error_stack_push(java_error* error, java_error_entry* item)
{
    if (!item)
    {
        return;
    }

    if (error->data)
    {
        error->top->next = item;
        item->prev = error->top;
    }
    else
    {
        error->data = item;
    }

    error->top = item;
}

/**
 * Error Entry Generator
*/
static java_error_entry* error_new_entry(java_error_id id, size_t ln, size_t col)
{
    java_error_entry* entry = (java_error_entry*)malloc_assert(sizeof(java_error_entry));

    entry->id = id;
    entry->ln = ln;
    entry->col = col;
    entry->prev = NULL;
    entry->next = NULL;

    return entry;
}

/**
 * Log an error
*/
void error_log(java_error* error, java_error_id id, size_t ln, size_t col)
{
    if (!error)
    {
        return;
    }

    error_stack_push(error, error_new_entry(id, ln, col));

    switch (error->definition[id] & ERR_DEF_MASK_LEVEL)
    {
        case JEL_INFORMATION:
            error->num_info++;
            break;
        case JEL_WARNING:
            error->num_warn++;
            break;
        case JEL_ERROR:
            error->num_err++;
            break;
        default:
            break;
    }
}

/**
 * count specific level
*/
size_t error_count(java_error* error, error_definiton error_level)
{
    switch (error_level)
    {
        case JEL_INFORMATION:
            return error->num_info;
        case JEL_WARNING:
            return error->num_warn;
        case JEL_ERROR:
            return error->num_err;
        default:
            return 0;
    }
}
