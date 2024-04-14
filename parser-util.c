#include "parser.h"

/**
 * peek a token, and load into buffer if not yet buffered
*/
java_token* token_peek(java_parser* parser, size_t idx)
{
    // guard
    if (idx >= 4)
    {
        return NULL;
    }

    // access
    if (idx + 1 <= parser->num_token_available)
    {
        return parser->tokens + idx;
    }

    // buffer it if not yet available
    for (size_t i = parser->num_token_available; i <= idx; i++)
    {
        lexer_next_token(parser->lexer, parser->tokens + i);

        // discard comments
        while (parser->tokens[i].class == JT_COMMENT)
        {
            lexer_next_token(parser->lexer, parser->tokens + i);
        }
    }

    // update counter only when buffer loads new token
    if (parser->num_token_available <= idx)
    {
        parser->num_token_available = idx + 1;
    }

    // return the queried peek
    return parser->tokens + idx;
}

/**
 * consume first token in buffer, and write a copy to dest
 * if dest not specified, token will be dropped
 * if buffer is empty, it is no-op
*/
void consume_token(java_parser* parser, java_token* dest)
{
    if (parser->num_token_available == 0)
    {
        return;
    }

    // save a copy to dest
    if (dest)
    {
        memcpy(dest, parser->tokens, sizeof(java_token));
    }

    // shift buffer to left by 1 unit
    for (size_t i = 1; i < parser->num_token_available; i++)
    {
        memcpy(parser->tokens + i - 1, parser->tokens + i, sizeof(java_token));
    }

    parser->num_token_available--;
}

java_token_class peek_token_class(java_parser* parser, size_t idx)
{
    return token_peek(parser, idx)->class;
}

java_lexeme_type peek_token_type(java_parser* parser, size_t idx)
{
    return token_peek(parser, idx)->type;
}

bool peek_token_class_is(java_parser* parser, size_t idx, java_token_class class)
{
    java_token* peek = token_peek(parser, idx);
    return peek->class == class;
}

bool peek_token_type_is(java_parser* parser, size_t idx, java_lexeme_type type)
{
    java_token* peek = token_peek(parser, idx);
    return peek->type == type;
}

bool peek_token_is_primitive_type(java_parser* parser, size_t idx)
{
    return is_lexeme_primitive_type(peek_token_type(parser, idx));
}

bool peek_token_is_literal(java_parser* parser, size_t idx)
{
    return is_lexeme_literal(peek_token_type(parser, idx));
}

bool is_lexeme_primitive_type(java_lexeme_type type)
{
    switch (type)
    {
        case JLT_RWD_BOOLEAN:
        case JLT_RWD_DOUBLE:
        case JLT_RWD_BYTE:
        case JLT_RWD_INT:
        case JLT_RWD_SHORT:
        case JLT_RWD_VOID:
        case JLT_RWD_CHAR:
        case JLT_RWD_LONG:
        case JLT_RWD_FLOAT:
            return true;
        default:
            break;
    }

    return false;
}

bool is_lexeme_literal(java_lexeme_type type)
{
    switch (type)
    {
        case JLT_LTR_NUMBER:
        case JLT_LTR_CHARACTER:
        case JLT_LTR_STRING:
            return true;
        default:
            break;
    }

    return false;
}

/**
 * Error Logger
 *
 * TODO: what line info do we need here?
 * TODO: need line_end info for future snapshot info creation
*/
void parser_error(java_parser* parser, java_error_id id, ...)
{
    va_list args;

    va_start(args, id);
    error_logger_vslog(parser->logger, &parser->lexer->ln_cur, NULL, id, &args);
    va_end(args);

    parser_recovery_dispatch(parser, id);
}
