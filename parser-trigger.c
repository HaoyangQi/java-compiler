/**
 * Jave Parser Reduction Trigger
 *
 * Only include those might be triggered multiple times during parsing
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
