/**
 * Parser Error Recovery Routines
 *
 * recovery depends on FOLLOW set of every production
 * 1. find the parser function
 * 2. read all error points in the function
 * 3. group all possible recovery points
 * 4. write a function here
 * 5. write a case in parser_recovery_dispatch for dispatch
 *
 * Make sure all error points have registered a unique ID so
 * we can group in any way we want in here
 *
 * TODO: write recovery logic for every production
*/

#include "parser.h"

static void parser_recovery_package_or_import(java_parser* parser);
static void parser_recovery_class_name_skip(java_parser* parser);
static void parser_recovery_top_level(java_parser* parser);
static void parser_recovery_next_class_member(java_parser* parser);

/**
 * Parser Error Recovery Dispatch
 *
 * recovery depends on FOLLOW set of every production
 *
 * TODO: do it case by case
*/
void parser_recovery_dispatch(java_parser* parser, java_error_id id)
{
    java_lexeme_type type;
    bool skip;

    switch (id)
    {
        case JAVA_E_PKG_DECL_NO_NAME:
        case JAVA_E_PKG_DECL_NO_SEMICOLON:
        case JAVA_E_IMPORT_NO_NAME:
        case JAVA_E_IMPORT_NO_SEMICOLON:
            parser_recovery_package_or_import(parser);
            break;
        case JAVA_E_CLASS_NO_NAME:
            parser_recovery_class_name_skip(parser);
            break;
        case JAVA_E_CLASS_NO_BODY:
        case JAVA_E_CLASS_BODY_ENCLOSE:
            parser_recovery_top_level(parser);
            break;
        case JAVA_E_MEMBER_NO_TYPE:
        case JAVA_E_MEMBER_NO_NAME:
        case JAVA_E_MEMBER_AMBIGUOUS:
            parser_recovery_next_class_member(parser);
            break;
        default:
            // no-op
            break;
    }
}

///// FOLLOW SET /////

/**
 * default FOLLOW guard that guarantees recovery exits
*/
static bool is_follow_always(java_lexeme_type type)
{
    return type == JLT_MAX;
}

/**
 * FOLLOW set of top level
*/
static bool is_follow_top_level(java_lexeme_type type)
{
    switch (type)
    {
        case JLT_RWD_CLASS:
        case JLT_RWD_INTERFACE:
        case JLT_RWD_PUBLIC:
        case JLT_RWD_PRIVATE:
        case JLT_RWD_PROTECTED:
        case JLT_RWD_FINAL:
        case JLT_RWD_STATIC:
        case JLT_RWD_ABSTRACT:
        case JLT_RWD_TRANSIENT:
        case JLT_RWD_SYNCHRONIZED:
        case JLT_RWD_VOLATILE:
            return true;
        default:
            return is_follow_always(type);
    }
}

/**
 * FOLLOW set of package or import declaration
*/
static bool is_follow_import_or_top_level(java_lexeme_type type)
{
    return type == JLT_RWD_IMPORT || is_follow_top_level(type);
}

/**
 * FOLLOW set of top level
*/
static bool is_follow_class_decl_no_name(java_lexeme_type type)
{
    switch (type)
    {
        case JLT_RWD_EXTENDS:
        case JLT_RWD_IMPLEMENTS:
        case JLT_SYM_BRACE_OPEN:
            return true;
        default:
            return is_follow_top_level(type);
    }
}

///// RECOVERY ROUTINES /////

/**
 * Package Declaration
 *
 * Skip until reaching its FOLLOW
 *
 * semicolon will be skipped because it contributes to no meaning when standalone
 * on top level
 *
 * import can follow itself, so both package and import declarations have same
 * FOLLOW set
*/
static void parser_recovery_package_or_import(java_parser* parser)
{
    while (!is_follow_import_or_top_level(peek_token_type(parser, TOKEN_PEEK_1st)))
    {
        consume_token(parser, NULL);
    }
}

/**
 * Skip missing class name
 *
 * Following could still be available to parse
*/
static void parser_recovery_class_name_skip(java_parser* parser)
{
    while (!is_follow_class_decl_no_name(peek_token_type(parser, TOKEN_PEEK_1st)))
    {
        consume_token(parser, NULL);
    }
}

/**
 * Skip missing class name
 *
 * Following could still be available to parse
*/
static void parser_recovery_top_level(java_parser* parser)
{
    while (!is_follow_top_level(peek_token_type(parser, TOKEN_PEEK_1st)))
    {
        consume_token(parser, NULL);
    }
}

/**
 * skip missing type in class member declaration
*/
static void parser_recovery_next_class_member(java_parser* parser)
{
    while (true)
    {
        java_token* peek = token_peek(parser, TOKEN_PEEK_1st);

        if (!JAVA_LEXEME_MODIFIER_OR_TYPE_WORD(peek->type) && peek->class != JT_IDENTIFIER)
        {
            break;
        }

        if (!is_follow_always(peek->type))
        {
            break;
        }
    }
}
