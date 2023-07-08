#include "parser.h"

/**
 * TODO: parse functions has type: static void
*/

void init_parser(java_parser* parser)
{
    // tokens contains garbage data if not used
    // so the counter is important
    parser->num_token_available = 0;
}

void parse_compilation_unit(java_parser* parser)
{}

static void parse_package_declaration(java_parser* parser)
{}
