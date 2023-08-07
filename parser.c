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
 * Initialize parser instance
*/
void init_parser(java_parser* parser, file_buffer* buffer, java_symbol_table* rw)
{
    // tokens contains garbage data if not used
    // so the counter is important
    parser->num_token_available = 0;

    parser->buffer = buffer;
    parser->reserved_words = rw;
    parser->ast_root = NULL;
}

/**
 * Release parser instance
*/
void release_parser(java_parser* parser)
{
    tree_node_delete(parser->ast_root, &parser_ast_node_data_deleter);
}

/* FORWARD DECLARATIONS OF PARSER FUNCTIONS */

static tree_node* parse_name(java_parser* parser);
static tree_node* parse_package_declaration(java_parser* parser);
static tree_node* parse_import_declaration(java_parser* parser);

/**
 * parser entry point
*/
void parse(java_parser* parser)
{
    parser->ast_root = ast_node_compilation_unit();

    // package declaration
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_RWD_PACKAGE))
    {
        tree_node_add_child(parser->ast_root, parse_package_declaration(parser));
    }

    // import declarations
    while (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_RWD_IMPORT))
    {
        tree_node_add_child(parser->ast_root, parse_import_declaration(parser));
    }
}

/**
 * Name:
 *     SimpleName
 *     QualifiedName
 * SimpleName:
 *     Identifier
 * QualifiedName:
 *     Name . Identifier
*/
static tree_node* parse_name(java_parser* parser)
{
    tree_node* node = ast_node_name();
    java_tree_node_name* data = (java_tree_node_name*)(node->data);

    // ID
    consume_token(parser, new_token_buffer(parser));
    linked_list_append(&data->name, parser->token_buffer);

    // {. ID}*
    while (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_DOT)
        && peek_token_class_is(parser, TOKEN_PEEK_2nd, JT_IDENTIFIER))
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

    // Name, terminate if incomplete
    if (parser_trigger_name(parser))
    {
        tree_node_add_child(node, parse_name(parser));
    }
    else
    {
        fprintf(stderr, "TODO error: expected name after package declaration\n");
        return node;
    }

    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_SEMICOLON))
    {
        consume_token(parser, NULL);
    }
    else
    {
        fprintf(stderr, "TODO error: expected ';'\n");
    }

    return node;
}

/**
 * ImportDeclaration:
 *     SingleTypeImportDeclaration
 *     TypeImportOnDemandDeclaration
 *
 * SingleTypeImportDeclaration:
 *     import Name ;
 *
 * TypeImportOnDemandDeclaration:
 *     import Name . * ;
*/
static tree_node* parse_import_declaration(java_parser* parser)
{
    tree_node* node = ast_node_import_declaration();
    java_tree_node_import_decl* data = (java_tree_node_import_decl*)(node->data);

    // import
    consume_token(parser, NULL);
    data->on_demand = false;

    // Name, terminate if incomplete
    if (parser_trigger_name(parser))
    {
        tree_node_add_child(node, parse_name(parser));
    }
    else
    {
        fprintf(stderr, "TODO error: expected name after import declaration\n");
        return node;
    }

    // [. *] on-demand sequence
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_DOT) &&
        peek_token_type_is(parser, TOKEN_PEEK_2nd, JLT_SYM_ASTERISK))
    {
        consume_token(parser, NULL);
        consume_token(parser, NULL);
        data->on_demand = true;
    }

    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_SEMICOLON))
    {
        consume_token(parser, NULL);
    }
    else
    {
        fprintf(stderr, "TODO error: expected ';'\n");
    }

    return node;
}
