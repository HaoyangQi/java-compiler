/**
 * Jave Parser Reduction Trigger
 *
 * Only include those might be triggered multiple times during parsing
*/

#include "parser.h"

bool parser_trigger_name(java_parser* parser)
{
    return peek_token_class_is(parser, TOKEN_PEEK_1st, JT_IDENTIFIER);
}
