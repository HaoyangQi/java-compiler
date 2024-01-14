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

static void parser_recovery_package_declaration(java_parser* parser);

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
            parser_recovery_package_declaration(parser);
            break;
        default:
            // no-op
            break;
    }
}

/**
 * Package Declaration
 *
 * Skip until reaching its FOLLOW
 *
 * semicolon will be skipped because it contributes to no meaning when standalone
 * on top level
*/
static void parser_recovery_package_declaration(java_parser* parser)
{
    java_lexeme_type type = peek_token_type(parser, TOKEN_PEEK_1st);
    bool skip = type != JLT_MAX;

    while (skip)
    {
        switch (type)
        {
            case JLT_RWD_IMPORT:
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
            case JLT_MAX:
                skip = false;
                break;
            default:
                consume_token(parser, NULL);
                skip = true;
                break;
        }

        type = peek_token_type(parser, TOKEN_PEEK_1st);
    }
}
