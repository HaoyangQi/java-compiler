/**
 * Jave Parser Reduction Trigger
 *
 * Only include those might be triggered multiple times during parsing
 * Must use peek APIs ONLY to contrstruct those triggers
*/

#include "parser.h"

bool parser_trigger_name(java_parser* parser, size_t peek_from)
{
    return peek_token_class_is(parser, peek_from, JT_IDENTIFIER);
}

bool parser_trigger_class_type(java_parser* parser, size_t peek_from)
{
    return peek_token_class_is(parser, peek_from, JT_IDENTIFIER);
}

bool parser_trigger_interface_type(java_parser* parser, size_t peek_from)
{
    return peek_token_class_is(parser, peek_from, JT_IDENTIFIER);
}

bool parser_trigger_type(java_parser* parser, size_t peek_from)
{
    return peek_token_is_type_word(parser, peek_from) ||
        peek_token_class_is(parser, peek_from, JT_IDENTIFIER);
}

/**
 * Primary triggers
 *
 * left parenthesis here is for: ( Expression )
*/
bool parser_trigger_primary(java_parser* parser, size_t peek_from)
{
    switch (peek_token_type(parser, peek_from))
    {
        case JLT_SYM_PARENTHESIS_OPEN:
        case JLT_LTR_NUMBER:
        case JLT_LTR_CHARACTER:
        case JLT_LTR_STRING:
        case JLT_RWD_TRUE:
        case JLT_RWD_FALSE:
        case JLT_RWD_NULL:
        case JLT_RWD_THIS:
        case JLT_RWD_NEW:
        case JLT_RWD_SUPER:
            return true;
        default:
            return peek_token_class_is(parser, peek_from, JT_IDENTIFIER);
    }
}

/**
 * Expression Trigger
 *
 * For operators:
 * Only unary operators and primary can trigger an expression,
 * other operators only allowed in the middle and/or end of expression
 *
 * NOTE: parenthesis is handled in Primary, see comment for reasoning
*/
bool parser_trigger_expression(java_parser* parser, size_t peek_from)
{
    switch (peek_token_type(parser, peek_from))
    {
        case JLT_SYM_EXCALMATION:
        case JLT_SYM_TILDE:
        case JLT_SYM_PLUS:
        case JLT_SYM_MINUS:
        case JLT_SYM_INCREMENT:
        case JLT_SYM_DECREMENT:
            return true;
        default:
            return parser_trigger_primary(parser, peek_from);
    }
}
