/**
 * Parser Logic
 *
 * Bottom line: use recusion at minimum amount, AND without backtrace
 *
 * To avoid backtrace we are going to need lookaheads and a bit of
 * contextual analysis to make production rule deterministic
 *
 * On top level we have bunch of "sentences", like statements, declarations,
 * etc; so we determine reduction of sentence at the beginning of every rule
 *
 * as an example, for package declaration, keyword "package" will trigger
 * the reduction of sentence "package Name ;", as the keyword is not ambiguous
 *
 * before reduction occurs, it only manipulates lookahead buffer, which is
 * incremental if no consumption, so there is no backtrace
 *
 * all rpoduction parser functions are reduction of certain production rules,
 * meaning: once invoked, it implies that the rule is matched for reduction;
 * also meaning: look-ahead check will be made at call-site.
*/

#include "parser.h"

/**
 * peek a token, and load into buffer if not yet buffered
*/
static java_token* token_peek(java_parser* parser, size_t idx)
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
static void consume_token(java_parser* parser, java_token* dest)
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

/**
 * peek next token and test if it is expected reserved word
 *
 * TODO: should we test deprecation and disable flag here?
*/
static bool peek_next_token_is_reserved_word(java_parser* parser, size_t idx, rwid id)
{
    java_token* peek = token_peek(parser, TOKEN_PEEK_1st);
    return peek->keyword && peek->keyword->id == id;
}

static bool peek_next_token_is_id(java_parser* parser, size_t idx)
{
    java_token* peek = token_peek(parser, TOKEN_PEEK_1st);
    return peek->type == JT_IDENTIFIER;
}

/**
 * peek next token and test if it is expected separator
*/
static bool peek_next_token_is_separator(java_parser* parser, size_t idx, java_separator_type sp)
{
    java_token* peek = token_peek(parser, TOKEN_PEEK_1st);
    return peek->type == JT_SEPARATOR && peek->subtype.sp == sp;
}

/**
 * generates a new token memory to transfer lookahead to AST node
 *
 * NOTE: it does NOT check current pointer and assumes the memory will be referenced somewhere else
*/
static java_token* new_token_buffer(java_parser* parser)
{
    parser->token_buffer = (java_token*)malloc_assert(sizeof(java_token));
    return parser->token_buffer;
}


////////// PARSER IMPLEMENTATIONS //////////


void init_parser(java_parser* parser, file_buffer* buffer, java_symbol_table* rw)
{
    // tokens contains garbage data if not used
    // so the counter is important
    parser->num_token_available = 0;

    parser->buffer = buffer;
    parser->reserved_words = rw;
    parser->ast_root = NULL;
}

/* FORWARD DECLARATIONS OF PARSER FUNCTIONS */

static tree_node* parse_package_declaration(java_parser* parser);
static tree_node* parse_name(java_parser* parser);

/**
 * parser entry point
*/
void parse_compilation_unit(java_parser* parser)
{
    parser->ast_root = ast_node_compilation_unit();
}

/**
 * Name:
 *     SimpleName
 *     QualifiedName
 * SimpleName:
 *     Identifier
 * QualifiedName:
 *     Name . Identifier
 *
 * TODO: consume ID tokens
*/
static tree_node* parse_name(java_parser* parser)
{
    tree_node* node = ast_node_name();
    java_tree_node_name* data = (java_tree_node_name*)(node->data);

    // ID
    consume_token(parser, new_token_buffer(parser));
    linked_list_append(&data->name, parser->token_buffer);

    // {. ID}*
    while (peek_next_token_is_separator(parser, TOKEN_PEEK_1st, JT_SP_DOT)
        && peek_next_token_is_id(parser, TOKEN_PEEK_2nd))
    {
        consume_token(parser, NULL);
        consume_token(parser, new_token_buffer(parser));
        linked_list_append(&data->name, parser->token_buffer);
    }

    return node;
}

/**
 * PackageDeclaration:
 *     package Name ;
*/
static tree_node* parse_package_declaration(java_parser* parser)
{
    tree_node* node = ast_node_package_declaration();

    // package
    consume_token(parser, NULL);
    tree_node_add_child(node, parse_name(parser));

    if (peek_next_token_is_separator(parser, TOKEN_PEEK_1st, JT_SP_SC))
    {
        consume_token(parser, NULL);
    }
    else
    {
        fprintf(stderr, "TODO error: expected ';'\n");
    }

    return node;
}
