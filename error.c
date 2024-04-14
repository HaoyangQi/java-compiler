#include "error.h"

#define ERROR_DEFINITION_ENTRY(def, id, desc, ctx, msg) \
    def[id] = (java_error_definition){ .descriptor = (desc), .context = (ctx), .message = (msg) }

/**
 * Common Error Messages
*/

static char* STR_NULL = "(null)";
static char* ERR_CTX_PKG_DECL = "package declaration";
static char* ERR_CTX_IMPORT_DECL = "import declaration";
static char* ERR_CTX_CLASS_DECL = "class declaration";
static char* ERR_CTX_INTERFACE_DECL = "interface declaration";
static char* ERR_CTX_MEMBER_DECL = "member declaration";
static char* ERR_CTX_VAR_DECL = "variable declaration";
static char* ERR_CTX_EXPR = "expression";
static char* ERR_MSG_MISSING_TOKEN = "Expected '%s' in %s.";
static char* ERR_MSG_MISSING_STATEMENT_TOKEN = "Expected '%s' in %s statement.";
static char* ERR_MSG_DIM_DEF_DIVERGE = "Dimension definition mismatched.";
static char* ERR_MSG_DIM_DEF_DUPLICATE = "Duplicated dimension definition.";

/**
 * Initialize Error Definitions
*/
static java_error_definition* new_error_definitions()
{
    java_error_definition* d = (java_error_definition*)malloc_assert(sizeof(java_error_definition) * JAVA_E_MAX);

    ERROR_DEFINITION_ENTRY(d, JAVA_E_RESERVED, DEFINE_RESERVED_ERROR, NULL, "(Unregistered error)");
    ERROR_DEFINITION_ENTRY(d, JAVA_E_NO_DIGIT, DEFINE_ERROR(JEL_ERROR, JES_LEXICAL), NULL, "At least one digit must be provided after prefix.");
    ERROR_DEFINITION_ENTRY(d, JAVA_E_ILLEGAL_CHARACTER, DEFINE_ERROR(JEL_ERROR, JES_LEXICAL), NULL, "Illegal character: '\\x%02x'.");
    ERROR_DEFINITION_ENTRY(d, JAVA_E_CHARACTER_LITERAL_ENCLOSE, DEFINE_ERROR(JEL_ERROR, JES_LEXICAL), "character literal", ERR_MSG_MISSING_TOKEN);
    ERROR_DEFINITION_ENTRY(d, JAVA_E_STRING_LITERAL_ENCLOSE, DEFINE_ERROR(JEL_ERROR, JES_LEXICAL), "string literal", ERR_MSG_MISSING_TOKEN);
    ERROR_DEFINITION_ENTRY(d, JAVA_E_MULTILINE_COMMENT_ENCLOSE, DEFINE_ERROR(JEL_ERROR, JES_LEXICAL), "multi-line comment", ERR_MSG_MISSING_TOKEN);
    ERROR_DEFINITION_ENTRY(d, JAVA_E_AMBIGUIOUS_ERROR_ENTRY, DEFINE_ERROR(JEL_ERROR, JES_INTERNAL), NULL, "Error stack contains ambiguous error entry, discarded...");
    ERROR_DEFINITION_ENTRY(d, JAVA_E_PASER_SWAP_FAILED, DEFINE_ERROR(JEL_ERROR, JES_INTERNAL), NULL, "Compiler is trying to swap an ill-formed parser instance.");
    ERROR_DEFINITION_ENTRY(d, JAVA_E_PASER_DELETE_FAILED, DEFINE_ERROR(JEL_ERROR, JES_INTERNAL), NULL, "Compiler is trying to delete an ill-formed parser instance.");
    ERROR_DEFINITION_ENTRY(d, JAVA_E_AMBIGUITY_DIVERGE, DEFINE_ERROR(JEL_ERROR, JES_INTERNAL), NULL, "Ambiguity diverges: buffer has different cursor location: (0x%x, 0x%x).");
    ERROR_DEFINITION_ENTRY(d, JAVA_E_FILE_NO_PATH, DEFINE_ERROR(JEL_ERROR, JES_RUNTIME), NULL, "File name is not valid.");
    ERROR_DEFINITION_ENTRY(d, JAVA_E_FILE_OPEN_FAILED, DEFINE_ERROR(JEL_ERROR, JES_RUNTIME), NULL, "File '%s' failed to open.");
    ERROR_DEFINITION_ENTRY(d, JAVA_E_FILE_SIZE_NOT_MATCH, DEFINE_ERROR(JEL_ERROR, JES_RUNTIME), NULL, "File '%s' has incorrect loading size comparing to what is reported from file system.");
    ERROR_DEFINITION_ENTRY(d, JAVA_E_TRAILING_CONTENT, DEFINE_ERROR(JEL_WARNING, JES_LEXICAL), NULL, "Unrecognized trailing content.");
    ERROR_DEFINITION_ENTRY(d, JAVA_E_PKG_DECL_NO_NAME, DEFINE_SYNTAX_ERROR, ERR_CTX_PKG_DECL, ERR_MSG_MISSING_TOKEN);
    ERROR_DEFINITION_ENTRY(d, JAVA_E_PKG_DECL_NO_SEMICOLON, DEFINE_SYNTAX_ERROR, ERR_CTX_PKG_DECL, ERR_MSG_MISSING_TOKEN);
    ERROR_DEFINITION_ENTRY(d, JAVA_E_IMPORT_NO_NAME, DEFINE_SYNTAX_ERROR, ERR_CTX_IMPORT_DECL, ERR_MSG_MISSING_TOKEN);
    ERROR_DEFINITION_ENTRY(d, JAVA_E_IMPORT_NO_SEMICOLON, DEFINE_SYNTAX_ERROR, ERR_CTX_IMPORT_DECL, ERR_MSG_MISSING_TOKEN);
    ERROR_DEFINITION_ENTRY(d, JAVA_E_IMPORT_AMBIGUOUS, DEFINE_CONTEXT_ERROR, ERR_CTX_IMPORT_DECL, "Ambiguous import type name, resolution of same type diverges.");
    ERROR_DEFINITION_ENTRY(d, JAVA_E_IMPORT_DUPLICATE, DEFINE_ERROR(JEL_WARNING, JES_CONTEXT), ERR_CTX_IMPORT_DECL, "Duplicated import, discarded.");
    ERROR_DEFINITION_ENTRY(d, JAVA_E_TOP_LEVEL, DEFINE_SYNTAX_ERROR, NULL, "Expected class or interface declaration.");
    ERROR_DEFINITION_ENTRY(d, JAVA_E_CLASS_NO_NAME, DEFINE_SYNTAX_ERROR, ERR_CTX_CLASS_DECL, ERR_MSG_MISSING_TOKEN);
    ERROR_DEFINITION_ENTRY(d, JAVA_E_CLASS_NO_BODY, DEFINE_SYNTAX_ERROR, ERR_CTX_CLASS_DECL, ERR_MSG_MISSING_TOKEN);
    ERROR_DEFINITION_ENTRY(d, JAVA_E_CLASS_NAME_DUPLICATE, DEFINE_CONTEXT_ERROR, NULL, "Duplicated class name.");
    ERROR_DEFINITION_ENTRY(d, JAVA_E_CLASS_BODY_ENCLOSE, DEFINE_CONTEXT_ERROR, "class body", ERR_MSG_MISSING_TOKEN);
    ERROR_DEFINITION_ENTRY(d, java_E_CLASS_ENTENDS_NO_NAME, DEFINE_SYNTAX_ERROR, "class extends", ERR_MSG_MISSING_TOKEN);
    ERROR_DEFINITION_ENTRY(d, java_E_CLASS_IMPLEMENTS_NO_NAME, DEFINE_SYNTAX_ERROR, "class implements", ERR_MSG_MISSING_TOKEN);
    ERROR_DEFINITION_ENTRY(d, JAVA_E_INTERFACE_NO_NAME, DEFINE_SYNTAX_ERROR, ERR_CTX_INTERFACE_DECL, ERR_MSG_MISSING_TOKEN);
    ERROR_DEFINITION_ENTRY(d, JAVA_E_INTERFACE_NO_BODY, DEFINE_SYNTAX_ERROR, ERR_CTX_INTERFACE_DECL, ERR_MSG_MISSING_TOKEN);
    ERROR_DEFINITION_ENTRY(d, java_E_INTERFACE_ENTENDS_NO_NAME, DEFINE_SYNTAX_ERROR, "interface extends", ERR_MSG_MISSING_TOKEN);
    ERROR_DEFINITION_ENTRY(d, java_E_INTERFACE_ENTENDS_NO_NAME, DEFINE_SYNTAX_ERROR, "interface body", ERR_MSG_MISSING_TOKEN);
    ERROR_DEFINITION_ENTRY(d, JAVA_E_MEMBER_VAR_DUPLICATE, DEFINE_CONTEXT_ERROR, NULL, "Duplicated member variable.");
    ERROR_DEFINITION_ENTRY(d, JAVA_E_MEMBER_VAR_DIM_AMBIGUOUS, DEFINE_CONTEXT_ERROR, NULL, ERR_MSG_DIM_DEF_DIVERGE);
    ERROR_DEFINITION_ENTRY(d, JAVA_E_MEMBER_VAR_DIM_DUPLICATE, DEFINE_ERROR(JEL_WARNING, JES_CONTEXT), NULL, ERR_MSG_DIM_DEF_DUPLICATE);
    ERROR_DEFINITION_ENTRY(d, JAVA_E_MEMBER_VAR_NO_SEMICOLON, DEFINE_SYNTAX_ERROR, "member variable declaration", ERR_MSG_MISSING_TOKEN);
    ERROR_DEFINITION_ENTRY(d, JAVA_E_MEMBER_METHOD_NO_SEMICOLON, DEFINE_SYNTAX_ERROR, "method interface", ERR_MSG_MISSING_TOKEN);
    ERROR_DEFINITION_ENTRY(d, JAVA_E_MEMBER_NO_TYPE, DEFINE_SYNTAX_ERROR, ERR_CTX_MEMBER_DECL, ERR_MSG_MISSING_TOKEN);
    ERROR_DEFINITION_ENTRY(d, JAVA_E_MEMBER_NO_NAME, DEFINE_SYNTAX_ERROR, ERR_CTX_MEMBER_DECL, ERR_MSG_MISSING_TOKEN);
    ERROR_DEFINITION_ENTRY(d, JAVA_E_MEMBER_AMBIGUOUS, DEFINE_SYNTAX_ERROR, NULL, "Incomplete member declaration.");
    ERROR_DEFINITION_ENTRY(d, JAVA_E_STATIC_INITIALIZER_NO_BODY, DEFINE_SYNTAX_ERROR, "static initializer", ERR_MSG_MISSING_TOKEN);
    ERROR_DEFINITION_ENTRY(d, JAVA_E_BLOCK_ENCLOSE, DEFINE_SYNTAX_ERROR, "block", ERR_MSG_MISSING_TOKEN);
    ERROR_DEFINITION_ENTRY(d, JAVA_E_STATEMENT_UNRECOGNIZED, DEFINE_SYNTAX_ERROR, NULL, "Expected statement");
    ERROR_DEFINITION_ENTRY(d, JAVA_E_STATEMENT_SWITCH, DEFINE_SYNTAX_ERROR, "switch", ERR_MSG_MISSING_STATEMENT_TOKEN);
    ERROR_DEFINITION_ENTRY(d, JAVA_E_STATEMENT_DO, DEFINE_SYNTAX_ERROR, "do", ERR_MSG_MISSING_STATEMENT_TOKEN);
    ERROR_DEFINITION_ENTRY(d, JAVA_E_STATEMENT_BREAK, DEFINE_SYNTAX_ERROR, "break", ERR_MSG_MISSING_STATEMENT_TOKEN);
    ERROR_DEFINITION_ENTRY(d, JAVA_E_STATEMENT_CONTINUE, DEFINE_SYNTAX_ERROR, "continue", ERR_MSG_MISSING_STATEMENT_TOKEN);
    ERROR_DEFINITION_ENTRY(d, JAVA_E_STATEMENT_RETURN, DEFINE_SYNTAX_ERROR, "return", ERR_MSG_MISSING_STATEMENT_TOKEN);
    ERROR_DEFINITION_ENTRY(d, JAVA_E_STATEMENT_SYNCHRONIZED, DEFINE_SYNTAX_ERROR, "synchronized", ERR_MSG_MISSING_STATEMENT_TOKEN);
    ERROR_DEFINITION_ENTRY(d, JAVA_E_STATEMENT_THROW, DEFINE_SYNTAX_ERROR, "throw", ERR_MSG_MISSING_STATEMENT_TOKEN);
    ERROR_DEFINITION_ENTRY(d, JAVA_E_STATEMENT_TRY, DEFINE_SYNTAX_ERROR, "try", ERR_MSG_MISSING_STATEMENT_TOKEN);
    ERROR_DEFINITION_ENTRY(d, JAVA_E_STATEMENT_IF, DEFINE_SYNTAX_ERROR, "if", ERR_MSG_MISSING_STATEMENT_TOKEN);
    ERROR_DEFINITION_ENTRY(d, JAVA_E_STATEMENT_ELSE, DEFINE_SYNTAX_ERROR, "else clause", ERR_MSG_MISSING_TOKEN);
    ERROR_DEFINITION_ENTRY(d, JAVA_E_STATEMENT_WHILE, DEFINE_SYNTAX_ERROR, "while", ERR_MSG_MISSING_STATEMENT_TOKEN);
    ERROR_DEFINITION_ENTRY(d, JAVA_E_STATEMENT_FOR, DEFINE_SYNTAX_ERROR, "for", ERR_MSG_MISSING_STATEMENT_TOKEN);
    ERROR_DEFINITION_ENTRY(d, JAVA_E_STATEMENT_FOR_INITIALIZER_NO_SEMICOLON, DEFINE_SYNTAX_ERROR, NULL, "Expected ';' after for-loop initializer.");
    ERROR_DEFINITION_ENTRY(d, JAVA_E_STATEMENT_FOR_CONDITION_NO_SEMICOLON, DEFINE_SYNTAX_ERROR, NULL, "Expected ';' after for-loop condition.");
    ERROR_DEFINITION_ENTRY(d, JAVA_E_STATEMENT_LABEL, DEFINE_SYNTAX_ERROR, "label", ERR_MSG_MISSING_STATEMENT_TOKEN);
    ERROR_DEFINITION_ENTRY(d, JAVA_E_STATEMENT_CATCH, DEFINE_SYNTAX_ERROR, "catch", ERR_MSG_MISSING_STATEMENT_TOKEN);
    ERROR_DEFINITION_ENTRY(d, JAVA_E_STATEMENT_FINALLY, DEFINE_SYNTAX_ERROR, "finally", ERR_MSG_MISSING_STATEMENT_TOKEN);
    ERROR_DEFINITION_ENTRY(d, JAVA_E_STATEMENT_SWITCH_LABEL, DEFINE_SYNTAX_ERROR, "switch label", ERR_MSG_MISSING_STATEMENT_TOKEN);
    ERROR_DEFINITION_ENTRY(d, JAVA_E_CONSTRUCTOR, DEFINE_SYNTAX_ERROR, "constructor", ERR_MSG_MISSING_TOKEN);
    ERROR_DEFINITION_ENTRY(d, JAVA_E_CONSTRUCTOR_INVOKE, DEFINE_SYNTAX_ERROR, "constructor invocation", ERR_MSG_MISSING_TOKEN);
    ERROR_DEFINITION_ENTRY(d, JAVA_E_METHOD_DECLARATION, DEFINE_SYNTAX_ERROR, "method declaration", ERR_MSG_MISSING_TOKEN);
    ERROR_DEFINITION_ENTRY(d, JAVA_E_METHOD_INVOKE, DEFINE_SYNTAX_ERROR, "method invocation", ERR_MSG_MISSING_TOKEN);
    ERROR_DEFINITION_ENTRY(d, JAVA_E_FORMAL_PARAMETER, DEFINE_SYNTAX_ERROR, "formal parameter", ERR_MSG_MISSING_TOKEN);
    ERROR_DEFINITION_ENTRY(d, JAVA_E_THROWS_NO_TYPE, DEFINE_SYNTAX_ERROR, "throws list", ERR_MSG_MISSING_TOKEN);
    ERROR_DEFINITION_ENTRY(d, JAVA_E_NO_ARGUMENT, DEFINE_SYNTAX_ERROR, "argument list", ERR_MSG_MISSING_TOKEN);
    ERROR_DEFINITION_ENTRY(d, JAVA_E_ARRAY_INITIALIZER, DEFINE_SYNTAX_ERROR, "array initializer", ERR_MSG_MISSING_TOKEN);
    ERROR_DEFINITION_ENTRY(d, JAVA_E_ARRAY_CREATION, DEFINE_SYNTAX_ERROR, "array creation", ERR_MSG_MISSING_TOKEN);
    ERROR_DEFINITION_ENTRY(d, JAVA_E_ARRAY_ACCESS, DEFINE_SYNTAX_ERROR, "array access", ERR_MSG_MISSING_TOKEN);
    ERROR_DEFINITION_ENTRY(d, JAVA_E_OBJECT_CREATION, DEFINE_SYNTAX_ERROR, "object creation", ERR_MSG_MISSING_TOKEN);
    ERROR_DEFINITION_ENTRY(d, JAVA_E_INSTANCE_CREATION, DEFINE_SYNTAX_ERROR, "instance expression", ERR_MSG_MISSING_TOKEN);
    ERROR_DEFINITION_ENTRY(d, JAVA_E_EXPRESSION_NO_OPERAND, DEFINE_SYNTAX_ERROR, NULL, "Invalid expression: expected operand.");
    ERROR_DEFINITION_ENTRY(d, JAVA_E_EXPRESSION_TOO_MANY_OPERAND, DEFINE_ERROR(JEL_ERROR, JES_INTERNAL), NULL, "Internal expression error: too many operands, they will be ignored.");
    ERROR_DEFINITION_ENTRY(d, JAVA_E_EXPRESSION_INCOMPLETE_TERNARY, DEFINE_SYNTAX_ERROR, NULL, "Invalid expression: incomplete conditional expression.");
    ERROR_DEFINITION_ENTRY(d, JAVA_E_EXPRESSION_UNHANDLED_BLOCK_CONTEXT, DEFINE_ERROR(JEL_ERROR, JES_INTERNAL), NULL, "Internal expression error: block context is not handled.");
    ERROR_DEFINITION_ENTRY(d, JAVA_E_EXPRESSION_NO_OPERATOR, DEFINE_SYNTAX_ERROR, NULL, "Invalid expression: expected operator.");
    ERROR_DEFINITION_ENTRY(d, JAVA_E_EXPRESSION_NO_LVALUE, DEFINE_SYNTAX_ERROR, NULL, "Invalid expression: expected lvalue.");
    ERROR_DEFINITION_ENTRY(d, JAVA_E_EXPRESSION_LITERAL_LVALUE, DEFINE_SYNTAX_ERROR, NULL, "Invalid expression: literal cannot be used as lvalue.");
    ERROR_DEFINITION_ENTRY(d, JAVA_E_EXPRESSION_NO_SEMICOLON, DEFINE_SYNTAX_ERROR, ERR_CTX_EXPR, ERR_MSG_MISSING_TOKEN);
    ERROR_DEFINITION_ENTRY(d, JAVA_E_NUMBER_OVERFLOW_INT8, DEFINE_ERROR(JEL_WARNING, JES_SYNTAX), NULL, "Number overflows: literal value exceeds valid range of type 'byte'.");
    ERROR_DEFINITION_ENTRY(d, JAVA_E_NUMBER_OVERFLOW_INT16, DEFINE_ERROR(JEL_WARNING, JES_SYNTAX), NULL, "Number overflows: literal value exceeds valid range of type 'short'.");
    ERROR_DEFINITION_ENTRY(d, JAVA_E_NUMBER_OVERFLOW_INT32, DEFINE_ERROR(JEL_WARNING, JES_SYNTAX), NULL, "Number overflows: literal value exceeds valid range of type 'int'.");
    ERROR_DEFINITION_ENTRY(d, JAVA_E_NUMBER_OVERFLOW_INT64, DEFINE_ERROR(JEL_WARNING, JES_SYNTAX), NULL, "Number overflows: literal value exceeds valid range of type 'long'.");
    ERROR_DEFINITION_ENTRY(d, JAVA_E_NUMBER_OVERFLOW_U16, DEFINE_ERROR(JEL_WARNING, JES_SYNTAX), NULL, "Number overflows: literal value exceeds valid range of type 'char'.");
    ERROR_DEFINITION_ENTRY(d, JAVA_E_NUMBER_OVERFLOW_FP32_EXP, DEFINE_ERROR(JEL_WARNING, JES_SYNTAX), NULL, "Number overflows: floarting-point value exceeds valid range of type 'float'.");
    ERROR_DEFINITION_ENTRY(d, JAVA_E_NUMBER_OVERFLOW_FP64_EXP, DEFINE_ERROR(JEL_WARNING, JES_SYNTAX), NULL, "Number overflows: floarting-point value exceeds valid range of type 'double'.");
    ERROR_DEFINITION_ENTRY(d, JAVA_E_PART_EXPONENT_OVERFLOW, DEFINE_CONTEXT_ERROR, NULL, "Invalid number: exponent part overflows.");
    ERROR_DEFINITION_ENTRY(d, JAVA_E_PART_INTEGER_OVERFLOW, DEFINE_CONTEXT_ERROR, NULL, "Invalid number: too many digits.");
    ERROR_DEFINITION_ENTRY(d, JAVA_E_LOCAL_VAR_DUPLICATE, DEFINE_CONTEXT_ERROR, NULL, "Duplicated local variable name.");
    ERROR_DEFINITION_ENTRY(d, JAVA_E_LOCAL_VAR_DIM_AMBIGUOUS, DEFINE_CONTEXT_ERROR, NULL, ERR_MSG_DIM_DEF_DIVERGE);
    ERROR_DEFINITION_ENTRY(d, JAVA_E_LOCAL_VAR_DIM_DUPLICATE, DEFINE_CONTEXT_ERROR, NULL, ERR_MSG_DIM_DEF_DUPLICATE);
    ERROR_DEFINITION_ENTRY(d, JAVA_E_LOCAL_VAR_NO_SEMICOLON, DEFINE_CONTEXT_ERROR, NULL, ERR_MSG_MISSING_TOKEN);
    ERROR_DEFINITION_ENTRY(d, JAVA_E_VAR_NO_DECLARATOR, DEFINE_SYNTAX_ERROR, ERR_CTX_VAR_DECL, ERR_MSG_MISSING_TOKEN);
    ERROR_DEFINITION_ENTRY(d, JAVA_E_VAR_NO_ARR_ENCLOSE, DEFINE_SYNTAX_ERROR, ERR_CTX_VAR_DECL, ERR_MSG_MISSING_TOKEN);
    ERROR_DEFINITION_ENTRY(d, JAVA_E_VAR_NO_INITIALIZER, DEFINE_SYNTAX_ERROR, ERR_CTX_VAR_DECL, ERR_MSG_MISSING_TOKEN);
    ERROR_DEFINITION_ENTRY(d, JAVA_E_PARAM_DUPLICATE, DEFINE_CONTEXT_ERROR, NULL, "Duplicated parameter name.");
    ERROR_DEFINITION_ENTRY(d, JAVA_E_PARAM_DIM_AMBIGUOUS, DEFINE_CONTEXT_ERROR, NULL, ERR_MSG_DIM_DEF_DIVERGE);
    ERROR_DEFINITION_ENTRY(d, JAVA_E_PARAM_DIM_DUPLICATE, DEFINE_CONTEXT_ERROR, NULL, ERR_MSG_DIM_DEF_DUPLICATE);
    ERROR_DEFINITION_ENTRY(d, JAVA_E_METHOD_DUPLICATE, DEFINE_CONTEXT_ERROR, NULL, "Duplicated method name.");
    ERROR_DEFINITION_ENTRY(d, JAVA_E_METHOD_DIM_AMBIGUOUS, DEFINE_CONTEXT_ERROR, NULL, ERR_MSG_DIM_DEF_DIVERGE);
    ERROR_DEFINITION_ENTRY(d, JAVA_E_METHOD_DIM_DUPLICATE, DEFINE_CONTEXT_ERROR, NULL, ERR_MSG_DIM_DEF_DUPLICATE);
    ERROR_DEFINITION_ENTRY(d, JAVA_E_IF_LOCAL_VAR_DECL, DEFINE_SYNTAX_ERROR, NULL, "Local variable definition not allowed in short 'if'.");
    ERROR_DEFINITION_ENTRY(d, JAVA_E_WHILE_LOCAL_VAR_DECL, DEFINE_SYNTAX_ERROR, NULL, "Local variable definition not allowed in short 'while'.");
    ERROR_DEFINITION_ENTRY(d, JAVA_E_REF_UNDEFINED, DEFINE_CONTEXT_ERROR, NULL, "Undefined reference.");
    ERROR_DEFINITION_ENTRY(d, JAVA_E_BREAK_UNBOUND, DEFINE_CONTEXT_ERROR, NULL, "Unbounded statement: 'break' needs to be bounded by a loop or 'switch'.");
    ERROR_DEFINITION_ENTRY(d, JAVA_E_CONTINUE_UNBOUND, DEFINE_CONTEXT_ERROR, NULL, "Unbounded statement: 'continue' needs to be bounded by a loop.");
    ERROR_DEFINITION_ENTRY(d, JAVA_E_EXPRESSION_LIST_INCOMPLETE, DEFINE_SYNTAX_ERROR, NULL, "Expected 'expression' in expression list.");
    ERROR_DEFINITION_ENTRY(d, JAVA_E_EXPRESSION_PARENTHESIS, DEFINE_SYNTAX_ERROR, ERR_CTX_EXPR, ERR_MSG_MISSING_TOKEN);
    ERROR_DEFINITION_ENTRY(d, JAVA_E_TYPE_NO_ARR_ENCLOSE, DEFINE_SYNTAX_ERROR, ERR_CTX_EXPR, ERR_MSG_MISSING_TOKEN);

    return d;
}

/**
 * Delete Error Definitions
*/
static void delete_error_definitions(java_error_definition* err_def)
{
    free(err_def);
}

static void init_error_stack(java_error_stack* stack, java_error_stack* amb_parent);
static void release_error_stack(java_error_stack* stack);
static bool error_stack_empty(const java_error_stack* stack);

static void init_error_summary(java_error_summary* summary)
{
    if (!summary) { return; }

    summary->num_err = 0;
    summary->num_info = 0;
    summary->num_warn = 0;
}

static void release_error_summary(java_error_summary* summary)
{
    // so far there is nothing to do...
}

static void delete_error_summary(java_error_summary* summary)
{
    release_error_summary(summary);
    free(summary);
}

static void error_summary_diff(java_error_summary* diff, java_error_summary* cache, java_error_summary* current)
{
    if (!diff)
    {
        return;
    }
    else
    {
        init_error_summary(diff);

        if (!cache || !current)
        {
            diff->num_err = 0;
            diff->num_warn = 0;
            diff->num_info = 0;
        }
        else
        {
            diff->num_err = current->num_err - cache->num_err;
            diff->num_warn = current->num_warn - cache->num_warn;
            diff->num_info = current->num_info - cache->num_info;
        }
    }
}

static void error_summary_merge(java_error_summary* dest, const java_error_summary* src)
{
    if (!dest || !src) { return; }

    dest->num_err += src->num_err;
    dest->num_warn += src->num_warn;
    dest->num_info += src->num_info;
}

static void error_summary_update(const java_error_definition* def, java_error_summary* summary, java_error_id id)
{
    if (!summary) { return; }

    switch (def[id].descriptor & ERR_DEF_MASK_LEVEL)
    {
        case JEL_INFORMATION:
            summary->num_info++;
            break;
        case JEL_WARNING:
            summary->num_warn++;
            break;
        case JEL_ERROR:
            summary->num_err++;
            break;
        default:
            break;
    }
}

static java_error_entry* new_error_entry(java_error_entry_type type, java_error_id id)
{
    java_error_entry* entry = (java_error_entry*)malloc_assert(sizeof(java_error_entry));

    entry->type = type;
    entry->id = id;
    entry->begin = LINE(0, 0);
    entry->end = LINE(0, 0);
    entry->msg = NULL;
    entry->prev = NULL;
    entry->next = NULL;

    entry->ambiguity.arr = NULL;
    entry->ambiguity.len = 0;
    entry->ambiguity.size = 0;

    return entry;
}

static void delete_error_entry(java_error_entry* entry)
{
    if (!entry) { return; }

    for (size_t i = 0; i < entry->ambiguity.len; i++)
    {
        release_error_stack(&entry->ambiguity.arr[i]);
    }

    free(entry->ambiguity.arr);
    free(entry->msg);
    free(entry);
}

/**
 * resolve an ambiguity entry on stack
*/
static bool error_entry_resolve_ambiguity(java_error_entry* entry, size_t idx)
{
    if (!entry || entry->type != ERROR_ENTRY_AMBIGUITY || idx >= entry->ambiguity.len)
    {
        return false;
    }

    java_error_stack* target = &entry->ambiguity.arr[idx];
    java_error_stack* dest = target->amb_parent;
    java_error_entry* prev = entry->prev;
    java_error_entry* next = entry->next;

    if (prev)
    {
        prev->next = error_stack_empty(target) ? next : target->first;
    }
    else
    {
        dest->first = target->first;
    }

    if (next)
    {
        next->prev = error_stack_empty(target) ? prev : target->last;
    }
    else
    {
        dest->last = target->last;
    }

    error_summary_merge(&dest->summary, &target->summary);

    // for each ambiguity entry in target stack, all ambiguity stack should re-align with new amb_parent
    for (java_error_entry* p = target->first; p != NULL; p = p->next)
    {
        if (p->type == ERROR_ENTRY_AMBIGUITY)
        {
            for (size_t i = 0; i < p->ambiguity.len; i++)
            {
                p->ambiguity.arr[i].amb_parent = dest->amb_parent;
            }
        }
    }

    // detach stack
    target->first = NULL;
    target->last = NULL;

    // delete entry
    delete_error_entry(entry);

    return true;
}

static void init_error_stack(java_error_stack* stack, java_error_stack* amb_parent)
{
    if (!stack) { return; }

    stack->amb_parent = amb_parent;
    stack->num_ambiguity = 0;
    stack->first = NULL;
    stack->last = NULL;

    init_error_summary(&stack->summary);
}

static void release_error_stack(java_error_stack* stack)
{
    if (!stack) { return; }

    java_error_entry* cur;

    while (true)
    {
        cur = stack->first;

        if (!cur) { break; }

        stack->first = cur->next;
        delete_error_entry(cur);
    }
}

static bool error_stack_empty(const java_error_stack* stack)
{
    return !stack || stack->first == NULL || stack->last == NULL;
}

static java_error_entry* error_stack_top(const java_error_stack* stack)
{
    return stack ? stack->last : NULL;
}

static java_error_entry* error_stack_push(java_error_definition* def, java_error_stack* stack, java_error_entry* entry)
{
    if (!stack || !entry) { return NULL; }

    if (error_stack_empty(stack))
    {
        stack->first = entry;
    }
    else
    {
        entry->prev = stack->last;
        stack->last->next = entry;
    }

    stack->last = entry;

    if (entry->type == ERROR_ENTRY_AMBIGUITY)
    {
        stack->num_ambiguity++;
    }
    else
    {
        error_summary_update(def, &stack->summary, entry->id);
    }

    return entry;
}


static java_error_stack* error_stack_branch_into_ambiguity(java_error_definition* def, java_error_stack* stack)
{
    java_error_entry* entry = error_stack_top(stack);

    if (!entry || entry->type != ERROR_ENTRY_AMBIGUITY)
    {
        /**
         * ambiguity entry itself represents an internal error
         * because in final stack there should not exist any ambiguous entry
        */
        entry = error_stack_push(def, stack, new_error_entry(ERROR_ENTRY_AMBIGUITY, JAVA_E_AMBIGUIOUS_ERROR_ENTRY));
    }

    // grow
    if (!entry->ambiguity.arr)
    {
        entry->ambiguity.arr = (java_error_stack*)malloc_assert(sizeof(java_error_stack));
        entry->ambiguity.len = 0;
        entry->ambiguity.size = 1;
    }
    else if (entry->ambiguity.len + 1 > entry->ambiguity.size)
    {
        entry->ambiguity.size = find_next_pow2_size(entry->ambiguity.len + 1);
        entry->ambiguity.arr = (java_error_stack*)
            realloc_assert(entry->ambiguity.arr, sizeof(java_error_stack) * entry->ambiguity.size);
    }

    // initialize
    java_error_stack* s = &entry->ambiguity.arr[entry->ambiguity.len];
    entry->ambiguity.len++;
    init_error_stack(s, stack);

    return s;
}

static void error_stack_get_summary(const java_error_stack* stack, java_error_summary* out);

static void error_stack_get_summary_recursive(const java_error_stack* stack, java_error_summary* out)
{
    if (!stack || !out) { return; }

    error_summary_merge(out, &stack->summary);

    for (java_error_entry* entry = stack->first; entry != NULL; entry = entry->next)
    {
        if (entry->type == ERROR_ENTRY_AMBIGUITY)
        {
            for (size_t i = 0; i < entry->ambiguity.len; i++)
            {
                java_error_summary sum;
                error_stack_get_summary(&entry->ambiguity.arr[i], &sum);
                error_summary_merge(out, &sum);
            }
        }
    }
}

static void error_stack_get_summary(const java_error_stack* stack, java_error_summary* out)
{
    init_error_summary(out);
    error_stack_get_summary_recursive(stack, out);
}

/**
 * Initialize Error Manager
*/
void init_error_logger(java_error_logger* logger)
{
    logger->def = new_error_definitions();
    logger->current_stream = &logger->main_stream;

    init_error_stack(&logger->main_stream, NULL);
}

/**
 * Release Error Manager
*/
void release_error_logger(java_error_logger* logger)
{
    delete_error_definitions(logger->def);
    release_error_stack(&logger->main_stream);
}

/**
 * Clear Error Stack
*/
void clear_error_logger(java_error_logger* logger)
{
    logger->current_stream = &logger->main_stream;

    release_error_stack(&logger->main_stream);
    init_error_stack(&logger->main_stream, NULL);
}

/**
 * Logger's ignore condition for specific ID
*/
bool error_logger_log_ignore(java_error_logger* logger, java_error_id id)
{
    return !logger || id == JAVA_E_MAX;
}

/**
 * Log an error, with provided argument list
 *
 * error logger will assume provided argument matches the string template,
 * because it lacks the nature to determine what data is passed to it
 * (due to the implementation-dependent black box "va_list")
*/
void error_logger_vslog(java_error_logger* logger, line* begin, line* end, java_error_id id, va_list* arguments)
{
    if (error_logger_log_ignore(logger, id)) { return; }

    java_error_entry* entry =
        error_stack_push(logger->def, logger->current_stream, new_error_entry(ERROR_ENTRY_NORMAL, id));
    const char* format = logger->def[id].message;
    va_list args;
    int len;

    // fill line info if applicable
    if (begin) { line_copy(&entry->begin, begin); }
    if (end) { line_copy(&entry->end, end); }

    // get target string length
    va_copy(args, *arguments);
    len = vsnprintf(NULL, 0, format, args);
    va_end(args);

    // if length is invalid, there is not much choice left
    if (len < 0)
    {
        fprintf(stderr, "Error logger failed to construct error message with ID %d.\n", id);
        return;
    }

    entry->msg = (char*)malloc_assert(sizeof(char) * (len + 1));

    // generate error message
    va_copy(args, *arguments);
    vsprintf(entry->msg, format, args);
    va_end(args);
}

/**
 * Log an error
 *
 * this is a warpper with variadic length arguments
 *
 * all customized wrapper can use this as a template
*/
void error_logger_log(java_error_logger* logger, line* begin, line* end, java_error_id id, ...)
{
    va_list args;

    va_start(args, id);
    error_logger_vslog(logger, begin, end, id, &args);
    va_end(args);
}

/**
 * Begin an ambiguity stack
 *
 * if current stack top is ambiguity entry, it will use it,
 * otherwise it will push one
 *
 * then it will allocate one stack and enter that context
 *
*/
void error_logger_ambiguity_begin(java_error_logger* logger)
{
    logger->current_stream = error_stack_branch_into_ambiguity(logger->def, logger->current_stream);
}

/**
 * Finish current ambiguity stack
 *
 * it will treat current stream as an ambiguity stack, and return current
 * stream back to the one that holds it
*/
void error_logger_ambiguity_end(java_error_logger* logger)
{
    if (logger->current_stream->amb_parent)
    {
        logger->current_stream = logger->current_stream->amb_parent;
    }
    else
    {
        logger->current_stream = &logger->main_stream;
    }
}

/**
 * Resolve an ambiguity entry
*/
bool error_logger_ambiguity_resolve(java_error_logger* logger, java_error_entry* entry, size_t idx)
{
    return error_entry_resolve_ambiguity(entry, idx);
}

/**
 * count ambiguity entry on main stack
 *
*/
size_t error_logger_count_main_ambiguity(java_error_logger* logger)
{
    return logger->main_stream.num_ambiguity;
}

/**
 * count ambiguity entry on current stack
 *
*/
size_t error_logger_count_current_ambiguity(java_error_logger* logger)
{
    return logger->current_stream ? logger->current_stream->num_ambiguity : 0;
}

/**
 * get stack top of main stream
*/
java_error_entry* error_logger_get_main_top(const java_error_logger* logger)
{
    return error_stack_top(&logger->main_stream);
}

/**
 * get stack top of current stream
*/
java_error_entry* error_logger_get_current_top(const java_error_logger* logger)
{
    return error_stack_top(logger->current_stream);
}

/**
 * Get summary of main stack
 *
 * this routine executes recursive counting process and might be expensive
*/
void error_logger_count_main_summary(const java_error_logger* logger, java_error_summary* out)
{
    error_stack_get_summary(&logger->main_stream, out);
}

void error_logger_count_current_summary(const java_error_logger* logger, java_error_summary* out)
{
    error_stack_get_summary(logger->current_stream, out);
}

/**
 * Check if main stack has error
 *
 * this routine executes recursive counting process and might be expensive
*/
bool error_logger_if_main_stack_no_error(const java_error_logger* logger)
{
    java_error_summary sum;
    error_stack_get_summary(&logger->main_stream, &sum);

    return sum.num_err == 0;
}

/**
 * Check if current stack has error
 *
 * this routine executes recursive counting process and might be expensive
*/
bool error_logger_if_current_stack_no_error(const java_error_logger* logger)
{
    java_error_summary sum;
    error_stack_get_summary(logger->current_stream, &sum);

    return sum.num_err == 0;
}

/**
 * get context string
*/
char* error_logger_get_context_string(const java_error_logger* logger, java_error_id id)
{
    char* ctx = logger->def[id].context;
    return ctx ? ctx : STR_NULL;
}
