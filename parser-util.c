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

    // buffer it if not yet available
    for (size_t i = parser->num_token_available; i <= idx; i++)
    {
        get_next_token(parser->tokens + i, parser->buffer, parser->reserved_words);

        // discard comments
        while (parser->tokens[i].class == JT_COMMENT)
        {
            get_next_token(parser->tokens + i, parser->buffer, parser->reserved_words);
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

/**
 * ast node data deletion util
*/
void parser_ast_node_data_deleter(int metadata, void* data)
{
    switch (metadata)
    {
        default:
            break;
    }
}
