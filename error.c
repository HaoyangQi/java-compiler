#include "error.h"

/**
 * Common Error Messages
*/
static char* ERR_MSG_NO_SEMICOLON = "Expected ';'.";
static char* ERR_MSG_NO_RIGHT_BRACE = "Expected '}'.";
static char* ERR_MSG_DIM_DEF_DIVERGE = "Dimension definition mismatched.";
static char* ERR_MSG_DIM_DEF_DUPLICATE = "Duplicated dimension definition.";

/**
 * Initialize Error Definitions
*/
void init_error_definition(java_error_definition* err_def)
{
    err_def->descriptor = (error_descriptor*)malloc_assert(sizeof(error_descriptor) * JAVA_E_MAX);
    err_def->message = (char**)malloc_assert(sizeof(char*) * JAVA_E_MAX);

    /* Error Definitions */

    err_def->descriptor[JAVA_E_RESERVED] = DEFINE_RESERVED_ERROR;
    err_def->descriptor[JAVA_E_FILE_NO_PATH] = DEFINE_ERROR(JEL_ERROR, JES_RUNTIME);
    err_def->descriptor[JAVA_E_FILE_OPEN_FAILED] = DEFINE_ERROR(JEL_ERROR, JES_RUNTIME);
    err_def->descriptor[JAVA_E_FILE_SIZE_NOT_MATCH] = DEFINE_ERROR(JEL_ERROR, JES_RUNTIME);
    err_def->descriptor[JAVA_E_TRAILING_CONTENT] = DEFINE_ERROR(JEL_WARNING, JES_LEXICAL);
    err_def->descriptor[JAVA_E_PKG_DECL_NO_NAME] = DEFINE_SYNTAX_ERROR;
    err_def->descriptor[JAVA_E_PKG_DECL_NO_SEMICOLON] = DEFINE_SYNTAX_ERROR;
    err_def->descriptor[JAVA_E_IMPORT_NO_NAME] = DEFINE_SYNTAX_ERROR;
    err_def->descriptor[JAVA_E_IMPORT_NO_SEMICOLON] = DEFINE_SYNTAX_ERROR;
    err_def->descriptor[JAVA_E_IMPORT_AMBIGUOUS] = DEFINE_CONTEXT_ERROR;
    err_def->descriptor[JAVA_E_IMPORT_DUPLICATE] = DEFINE_ERROR(JEL_WARNING, JES_CONTEXT);
    err_def->descriptor[JAVA_E_CLASS_NO_NAME] = DEFINE_SYNTAX_ERROR;
    err_def->descriptor[JAVA_E_CLASS_NO_BODY] = DEFINE_SYNTAX_ERROR;
    err_def->descriptor[JAVA_E_CLASS_NAME_DUPLICATE] = DEFINE_CONTEXT_ERROR;
    err_def->descriptor[JAVA_E_CLASS_BODY_ENCLOSE] = DEFINE_CONTEXT_ERROR;
    err_def->descriptor[JAVA_E_MEMBER_VAR_DUPLICATE] = DEFINE_CONTEXT_ERROR;
    err_def->descriptor[JAVA_E_MEMBER_VAR_DIM_AMBIGUOUS] = DEFINE_CONTEXT_ERROR;
    err_def->descriptor[JAVA_E_MEMBER_VAR_DIM_DUPLICATE] = DEFINE_ERROR(JEL_WARNING, JES_CONTEXT);
    err_def->descriptor[JAVA_E_MEMBER_VAR_NO_SEMICOLON] = DEFINE_SYNTAX_ERROR;
    err_def->descriptor[JAVA_E_MEMBER_NO_TYPE] = DEFINE_SYNTAX_ERROR;
    err_def->descriptor[JAVA_E_MEMBER_NO_NAME] = DEFINE_SYNTAX_ERROR;
    err_def->descriptor[JAVA_E_MEMBER_AMBIGUOUS] = DEFINE_SYNTAX_ERROR;
    err_def->descriptor[JAVA_E_EXPRESSION_NO_OPERAND] = DEFINE_SYNTAX_ERROR;
    err_def->descriptor[JAVA_E_EXPRESSION_NO_OPERATOR] = DEFINE_SYNTAX_ERROR;
    err_def->descriptor[JAVA_E_EXPRESSION_NO_LVALUE] = DEFINE_SYNTAX_ERROR;
    err_def->descriptor[JAVA_E_EXPRESSION_LITERAL_LVALUE] = DEFINE_SYNTAX_ERROR;
    err_def->descriptor[JAVA_E_EXPRESSION_NO_SEMICOLON] = DEFINE_SYNTAX_ERROR;
    err_def->descriptor[JAVA_E_NUMBER_OVERFLOW_INT8] = DEFINE_ERROR(JEL_WARNING, JES_SYNTAX);
    err_def->descriptor[JAVA_E_NUMBER_OVERFLOW_INT16] = DEFINE_ERROR(JEL_WARNING, JES_SYNTAX);
    err_def->descriptor[JAVA_E_NUMBER_OVERFLOW_INT32] = DEFINE_ERROR(JEL_WARNING, JES_SYNTAX);
    err_def->descriptor[JAVA_E_NUMBER_OVERFLOW_INT64] = DEFINE_ERROR(JEL_WARNING, JES_SYNTAX);
    err_def->descriptor[JAVA_E_NUMBER_OVERFLOW_U16] = DEFINE_ERROR(JEL_WARNING, JES_SYNTAX);
    err_def->descriptor[JAVA_E_NUMBER_OVERFLOW_FP32_EXP] = DEFINE_ERROR(JEL_WARNING, JES_SYNTAX);
    err_def->descriptor[JAVA_E_NUMBER_OVERFLOW_FP64_EXP] = DEFINE_ERROR(JEL_WARNING, JES_SYNTAX);
    err_def->descriptor[JAVA_E_PART_EXPONENT_OVERFLOW] = DEFINE_CONTEXT_ERROR;
    err_def->descriptor[JAVA_E_PART_INTEGER_OVERFLOW] = DEFINE_CONTEXT_ERROR;
    err_def->descriptor[JAVA_E_LOCAL_VAR_DUPLICATE] = DEFINE_CONTEXT_ERROR;
    err_def->descriptor[JAVA_E_LOCAL_VAR_DIM_AMBIGUOUS] = DEFINE_CONTEXT_ERROR;
    err_def->descriptor[JAVA_E_LOCAL_VAR_DIM_DUPLICATE] = DEFINE_CONTEXT_ERROR;
    err_def->descriptor[JAVA_E_LOCAL_VAR_NO_SEMICOLON] = DEFINE_CONTEXT_ERROR;
    err_def->descriptor[JAVA_E_VAR_NO_DECLARATOR] = DEFINE_SYNTAX_ERROR;
    err_def->descriptor[JAVA_E_VAR_NO_ARR_ENCLOSE] = DEFINE_SYNTAX_ERROR;
    err_def->descriptor[JAVA_E_PARAM_DUPLICATE] = DEFINE_CONTEXT_ERROR;
    err_def->descriptor[JAVA_E_PARAM_DIM_AMBIGUOUS] = DEFINE_CONTEXT_ERROR;
    err_def->descriptor[JAVA_E_PARAM_DIM_DUPLICATE] = DEFINE_CONTEXT_ERROR;
    err_def->descriptor[JAVA_E_METHOD_DUPLICATE] = DEFINE_CONTEXT_ERROR;
    err_def->descriptor[JAVA_E_METHOD_DIM_AMBIGUOUS] = DEFINE_CONTEXT_ERROR;
    err_def->descriptor[JAVA_E_METHOD_DIM_DUPLICATE] = DEFINE_CONTEXT_ERROR;
    err_def->descriptor[JAVA_E_IF_LOCAL_VAR_DECL] = DEFINE_SYNTAX_ERROR;
    err_def->descriptor[JAVA_E_WHILE_LOCAL_VAR_DECL] = DEFINE_SYNTAX_ERROR;
    err_def->descriptor[JAVA_E_REF_UNDEFINED] = DEFINE_CONTEXT_ERROR;
    err_def->descriptor[JAVA_E_BREAK_UNBOUND] = DEFINE_CONTEXT_ERROR;
    err_def->descriptor[JAVA_E_CONTINUE_UNBOUND] = DEFINE_CONTEXT_ERROR;

    err_def->descriptor[JAVA_E_AMBIGUITY_START] = DEFINE_RESERVED_ERROR;
    err_def->descriptor[JAVA_E_AMBIGUITY_PATH_1] = DEFINE_RESERVED_ERROR;
    err_def->descriptor[JAVA_E_AMBIGUITY_PATH_2] = DEFINE_RESERVED_ERROR;
    err_def->descriptor[JAVA_E_AMBIGUITY_SEPARATOR] = DEFINE_RESERVED_ERROR;
    err_def->descriptor[JAVA_E_AMBIGUITY_END] = DEFINE_RESERVED_ERROR;

    /* Error Messages */

    err_def->message[JAVA_E_RESERVED] = "(Unregistered error)";
    err_def->message[JAVA_E_FILE_NO_PATH] = "File name is not valid.";
    err_def->message[JAVA_E_FILE_OPEN_FAILED] = "File '%s' failed to open.";
    err_def->message[JAVA_E_FILE_SIZE_NOT_MATCH] = "File '%s' has incorrect loading size comparing to what is reported from file system.";
    err_def->message[JAVA_E_TRAILING_CONTENT] = "Unrecognized trailing content.";
    err_def->message[JAVA_E_PKG_DECL_NO_NAME] = "Expected 'name' in package declaration.";
    err_def->message[JAVA_E_PKG_DECL_NO_SEMICOLON] = ERR_MSG_NO_SEMICOLON;
    err_def->message[JAVA_E_IMPORT_NO_NAME] = "Expected 'name' in import declaration.";
    err_def->message[JAVA_E_IMPORT_NO_SEMICOLON] = ERR_MSG_NO_SEMICOLON;
    err_def->message[JAVA_E_IMPORT_AMBIGUOUS] = "Ambiguous import type name, resolution of same type diverges.";
    err_def->message[JAVA_E_IMPORT_DUPLICATE] = "Duplicated import, discarded.";
    err_def->message[JAVA_E_CLASS_NO_NAME] = "Expected 'name' in class declaration.";
    err_def->message[JAVA_E_CLASS_NO_BODY] = "Expected class body in class declaration.";
    err_def->message[JAVA_E_CLASS_NAME_DUPLICATE] = "Duplicated class name.";
    err_def->message[JAVA_E_CLASS_BODY_ENCLOSE] = ERR_MSG_NO_RIGHT_BRACE;
    err_def->message[JAVA_E_MEMBER_VAR_DUPLICATE] = "Duplicated member variable.";
    err_def->message[JAVA_E_MEMBER_VAR_DIM_AMBIGUOUS] = ERR_MSG_DIM_DEF_DIVERGE;
    err_def->message[JAVA_E_MEMBER_VAR_DIM_DUPLICATE] = ERR_MSG_DIM_DEF_DUPLICATE;
    err_def->message[JAVA_E_MEMBER_VAR_NO_SEMICOLON] = ERR_MSG_NO_SEMICOLON;
    err_def->message[JAVA_E_MEMBER_NO_TYPE] = "Expected 'type' in member declaration.";
    err_def->message[JAVA_E_MEMBER_NO_NAME] = "Expected 'name' in member declaration.";
    err_def->message[JAVA_E_MEMBER_AMBIGUOUS] = "Incomplete member declaration.";
    err_def->message[JAVA_E_EXPRESSION_NO_OPERAND] = "Invalid expression: expected operand.";
    err_def->message[JAVA_E_EXPRESSION_NO_OPERATOR] = "Invalid expression: expected operator.";
    err_def->message[JAVA_E_EXPRESSION_NO_LVALUE] = "Invalid expression: expected lvalue.";
    err_def->message[JAVA_E_EXPRESSION_LITERAL_LVALUE] = "Invalid expression: literal cannot be used as lvalue.";
    err_def->message[JAVA_E_EXPRESSION_NO_SEMICOLON] = ERR_MSG_NO_SEMICOLON;
    err_def->message[JAVA_E_NUMBER_OVERFLOW_INT8] = "Number overflows: literal value exceeds valid range of type 'byte'.";
    err_def->message[JAVA_E_NUMBER_OVERFLOW_INT16] = "Number overflows: literal value exceeds valid range of type 'short'.";
    err_def->message[JAVA_E_NUMBER_OVERFLOW_INT32] = "Number overflows: literal value exceeds valid range of type 'int'.";
    err_def->message[JAVA_E_NUMBER_OVERFLOW_INT64] = "Number overflows: literal value exceeds valid range of type 'long'.";
    err_def->message[JAVA_E_NUMBER_OVERFLOW_U16] = "Number overflows: literal value exceeds valid range of type 'char'.";
    err_def->message[JAVA_E_NUMBER_OVERFLOW_FP32_EXP] = "Number overflows: floarting-point value exceeds valid range of type 'float'.";
    err_def->message[JAVA_E_NUMBER_OVERFLOW_FP64_EXP] = "Number overflows: floarting-point value exceeds valid range of type 'double'.";
    err_def->message[JAVA_E_PART_EXPONENT_OVERFLOW] = "Invalid number: exponent part overflows.";
    err_def->message[JAVA_E_PART_INTEGER_OVERFLOW] = "Invalid number: too many digits.";
    err_def->message[JAVA_E_LOCAL_VAR_DUPLICATE] = "Duplicated local variable name.";
    err_def->message[JAVA_E_LOCAL_VAR_DIM_AMBIGUOUS] = ERR_MSG_DIM_DEF_DIVERGE;
    err_def->message[JAVA_E_LOCAL_VAR_DIM_DUPLICATE] = ERR_MSG_DIM_DEF_DUPLICATE;
    err_def->message[JAVA_E_LOCAL_VAR_NO_SEMICOLON] = ERR_MSG_NO_SEMICOLON;
    err_def->message[JAVA_E_VAR_NO_DECLARATOR] = "Expected declarator name after type.";
    err_def->message[JAVA_E_VAR_NO_ARR_ENCLOSE] = "Expected ']'.";
    err_def->message[JAVA_E_PARAM_DUPLICATE] = "Duplicated parameter name.";
    err_def->message[JAVA_E_PARAM_DIM_AMBIGUOUS] = ERR_MSG_DIM_DEF_DIVERGE;
    err_def->message[JAVA_E_PARAM_DIM_DUPLICATE] = ERR_MSG_DIM_DEF_DUPLICATE;
    err_def->message[JAVA_E_METHOD_DUPLICATE] = "Duplicated method name.";
    err_def->message[JAVA_E_METHOD_DIM_AMBIGUOUS] = ERR_MSG_DIM_DEF_DIVERGE;
    err_def->message[JAVA_E_METHOD_DIM_DUPLICATE] = ERR_MSG_DIM_DEF_DUPLICATE;
    err_def->message[JAVA_E_IF_LOCAL_VAR_DECL] = "Local variable definition not allowed in short 'if'.";
    err_def->message[JAVA_E_WHILE_LOCAL_VAR_DECL] = "Local variable definition not allowed in short 'while'.";
    err_def->message[JAVA_E_REF_UNDEFINED] = "Undefined reference.";
    err_def->message[JAVA_E_BREAK_UNBOUND] = "Unbounded statement: 'break' needs to be bounded by a loop or 'switch'.";
    err_def->message[JAVA_E_CONTINUE_UNBOUND] = "Unbounded statement: 'continue' needs to be bounded by a loop.";

    err_def->message[JAVA_E_AMBIGUITY_START] = NULL;
    err_def->message[JAVA_E_AMBIGUITY_PATH_1] = NULL;
    err_def->message[JAVA_E_AMBIGUITY_PATH_2] = NULL;
    err_def->message[JAVA_E_AMBIGUITY_SEPARATOR] = NULL;
    err_def->message[JAVA_E_AMBIGUITY_END] = NULL;
}

/**
 * Release Error Definitions
*/
void release_error_definition(java_error_definition* err_def)
{
    free(err_def->descriptor);
    free(err_def->message);
}

/**
 * Initialize Error Manager
*/
void init_error_stack(java_error_stack* error, java_error_definition* def)
{
    error->def = def;
    error->data = NULL;
    error->top = NULL;
    error->num_info = 0;
    error->num_warn = 0;
    error->num_err = 0;
}

/**
 * Release Error Manager
*/
void release_error_stack(java_error_stack* error)
{
    clear_error_stack(error);
}

/**
 * Clear Error Stack
*/
void clear_error_stack(java_error_stack* error)
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
java_error_entry* error_stack_top(java_error_stack* error)
{
    return error->top;
}

/**
 * Check is stack is empty
*/
bool error_stack_empty(java_error_stack* error)
{
    return error->data == NULL;
}

/**
 * Stack rewind to a new top
*/
void error_stack_rewind(java_error_stack* error, java_error_entry* new_top)
{
    while (error->top != new_top && error_stack_pop(error));
}

/**
 * Pop stack top element
*/
bool error_stack_pop(java_error_stack* error)
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
void error_stack_push(java_error_stack* error, java_error_entry* item)
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
 * Merge two error stacks
*/
void error_stack_concat(java_error_stack* dest, java_error_stack* src)
{
    if (!dest || !src || !src->data)
    {
        return;
    }

    if (dest->data)
    {
        dest->top->next = src->data;
    }
    else
    {
        dest->data = src->data;
    }

    dest->top = src->top;
    dest->num_err += src->num_err;
    dest->num_warn += src->num_warn;
    dest->num_info += src->num_info;

    // detach
    src->data = NULL;
    src->top = NULL;
}

/**
 * Delete error entry
 *
 * WARNING: make sure the entry is in the list, otherwise it will cause
 * memory leak
*/
void error_stack_entry_delete(java_error_stack* error, java_error_entry* entry)
{
    if (entry->prev)
    {
        entry->prev->next = entry->next;
    }
    else
    {
        error->data = entry->next;
    }

    if (entry->next)
    {
        entry->next->prev = entry->prev;
    }
    else
    {
        error->top = entry->prev;
    }

    free(entry);
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
void error_log(java_error_stack* error, java_error_id id, size_t ln, size_t col)
{
    if (!error || id == JAVA_E_MAX)
    {
        return;
    }

    error_stack_push(error, error_new_entry(id, ln, col));

    switch (error->def->descriptor[id] & ERR_DEF_MASK_LEVEL)
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
size_t error_count(java_error_stack* error, error_descriptor error_level)
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
