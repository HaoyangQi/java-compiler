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
static tree_node* parse_class_type(java_parser* parser);
static tree_node* parse_interface_type(java_parser* parser);
static tree_node* parse_interface_type_list(java_parser* parser);
static tree_node* parse_package_declaration(java_parser* parser);
static tree_node* parse_import_declaration(java_parser* parser);
static tree_node* parse_top_level(java_parser* parser);
static tree_node* parse_class_declaration(java_parser* parser);
static tree_node* parse_interface_declaration(java_parser* parser);
static tree_node* parse_class_extends(java_parser* parser);
static tree_node* parse_class_implements(java_parser* parser);
static tree_node* parse_class_body(java_parser* parser);

/**
 * parser entry point
*/
void parse(java_parser* parser)
{
    bool loop_continue;
    parser->ast_root = ast_node_compilation_unit();

    // [package declaration]
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_RWD_PACKAGE))
    {
        tree_node_add_child(parser->ast_root, parse_package_declaration(parser));
    }

    // {import declaration}
    while (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_RWD_IMPORT))
    {
        tree_node_add_child(parser->ast_root, parse_import_declaration(parser));
    }

    // {top level}
    loop_continue = true;
    while (loop_continue)
    {
        java_lexeme_type type = token_peek(parser, TOKEN_PEEK_1st)->type;

        switch (type)
        {
            case JLT_SYM_SEMICOLON: // empty
            case JLT_RWD_CLASS:     // keyword
            case JLT_RWD_INTERFACE:
            case JLT_RWD_PUBLIC:    // modifier
            case JLT_RWD_PRIVATE:
            case JLT_RWD_PROTECTED:
            case JLT_RWD_FINAL:
            case JLT_RWD_STATIC:
            case JLT_RWD_ABSTRACT:
            case JLT_RWD_TRANSIENT:
            case JLT_RWD_SYNCHRONIZED:
            case JLT_RWD_VOLATILE:
                tree_node_add_child(parser->ast_root, parse_top_level(parser));
                break;
            default:
                loop_continue = false;
                break;
        }
    }
}

/**
 * NameUnit:
 *     Identifier
 *
 * this wrapper allows ID to be sequenced under Name
 * and also allows potential to enhance Name
*/
static tree_node* __parse_name_unit(java_parser* parser)
{
    tree_node* node = ast_node_name_unit();
    node_data_name_unit* data = (node_data_name_unit*)(node->data);

    // ID
    consume_token(parser, &data->id);

    return node;
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
 * Name is just ID sequence
*/
static tree_node* parse_name(java_parser* parser)
{
    tree_node* node = ast_node_name();

    // Unit
    tree_node_add_child(node, __parse_name_unit(parser));

    // {. Unit}*
    while (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_DOT)
        && parser_trigger_name(parser, TOKEN_PEEK_2nd))
    {
        consume_token(parser, NULL);
        tree_node_add_child(node, __parse_name_unit(parser));
    }

    return node;
}

/**
 * ClassTypeUnit:
 *     Identifier
 *
 * this wrapper allows ID to be sequenced under ClassType
 * and also allows potential to enhance ClassType
*/
static tree_node* __parse_class_type_unit(java_parser* parser)
{
    tree_node* node = ast_node_class_type_unit();
    node_data_class_type_unit* data = (node_data_class_type_unit*)(node->data);

    // ID
    consume_token(parser, &data->id);

    return node;
}

/**
 * ClassType:
 *     Name
 *
 * As of current this is just a duplication of Name, but
 * in the future, this type name would become complex:
 *
 * In Java SE 20, class type name is a DOT-separated
 * sequence with following unit:
 *
 * {Annotation} TypeIdentifier [TypeArguments]
*/
static tree_node* parse_class_type(java_parser* parser)
{
    tree_node* node = ast_node_class_type();

    // Unit
    tree_node_add_child(node, __parse_class_type_unit(parser));

    // {. Unit}*
    while (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_DOT)
        && parser_trigger_class_type(parser, TOKEN_PEEK_2nd))
    {
        consume_token(parser, NULL);
        tree_node_add_child(node, __parse_class_type_unit(parser));
    }

    return node;
}

/**
 * InterfaceTypeUnit:
 *     Identifier
 *
 * this wrapper allows ID to be sequenced under InterfaceType
 * and also allows potential to enhance InterfaceType
*/
static tree_node* __parse_interface_type_unit(java_parser* parser)
{
    tree_node* node = ast_node_interface_type_unit();
    node_data_class_type_unit* data = (node_data_class_type_unit*)(node->data);

    // ID
    consume_token(parser, &data->id);

    return node;
}

/**
 * InterfaceType:
 *     Name
 *
 * As of current this is just a duplication of Name, but
 * in the future, this type name would become complex:
 *
 * In Java SE 20, class type name is a DOT-separated
 * sequence with following unit:
 *
 * {Annotation} TypeIdentifier [TypeArguments]
*/
static tree_node* parse_interface_type(java_parser* parser)
{
    tree_node* node = ast_node_interface_type();

    // Unit
    tree_node_add_child(node, __parse_interface_type_unit(parser));

    // {. Unit}*
    while (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_DOT)
        && parser_trigger_interface_type(parser, TOKEN_PEEK_2nd))
    {
        consume_token(parser, NULL);
        tree_node_add_child(node, __parse_interface_type_unit(parser));
    }

    return node;
}

/**
 * InterfaceTypeList:
 *     InterfaceType
 *     InterfaceTypeList , InterfaceType
*/
static tree_node* parse_interface_type_list(java_parser* parser)
{
    tree_node* node = ast_node_interface_type_list();

    // interface type
    tree_node_add_child(node, parse_interface_type(parser));

    // {, interface type}*
    while (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_COMMA)
        && parser_trigger_interface_type(parser, TOKEN_PEEK_2nd))
    {
        consume_token(parser, NULL);
        tree_node_add_child(node, parse_interface_type(parser));
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
    if (parser_trigger_name(parser, TOKEN_PEEK_1st))
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
    node_data_import_decl* data = (node_data_import_decl*)(node->data);

    // import
    consume_token(parser, NULL);
    data->on_demand = false;

    // Name, terminate if incomplete
    if (parser_trigger_name(parser, TOKEN_PEEK_1st))
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

/**
 * TopLevel:
 *     {Modifier} ClassDeclaration
 *     {Modifier} InterfaceDeclaration
 *     ;
 *
 * since class/interface both can start with modifier(s), so TopLevel
 * is necessary to wrap each class/interface declaration to avoid
 * ambiguous backtrack
*/
static tree_node* parse_top_level(java_parser* parser)
{
    tree_node* node = ast_node_top_level();
    node_data_top_level* data = (node_data_top_level*)(node->data);
    java_lexeme_type type;

    // {Modifier}
    while (true)
    {
        type = token_peek(parser, TOKEN_PEEK_1st)->type;

        // if not a modifier, we stop
        if (!JAVA_LEXEME_MODIFIER(type))
        {
            break;
        }

        consume_token(parser, NULL);
        data->modifier |= ((lbit_flag)1 << type);
    }

    type = token_peek(parser, TOKEN_PEEK_1st)->type;

    // class...|interface...|;
    switch (type)
    {
        case JLT_RWD_CLASS:
            tree_node_add_child(node, parse_class_declaration(parser));
            return node;
        case JLT_RWD_INTERFACE:
            tree_node_add_child(node, parse_interface_declaration(parser));
            return node;
        case JLT_SYM_SEMICOLON:
            consume_token(parser, NULL);
            return node;
        default:
            break;

    }

    if (!peek_token_class_is(parser, TOKEN_PEEK_1st, JT_EOF))
    {
        fprintf(stderr, "TODO error: expected class or interface declaration\n");
    }

    return node;
}

/**
 * ClassDeclaration:
 *     class Identifier [ClassExtends] [ClassImplements] ClassBody
*/
static tree_node* parse_class_declaration(java_parser* parser)
{
    tree_node* node = ast_node_class_declaration();
    node_data_class_declaration* data = (node_data_class_declaration*)(node->data);

    // class
    consume_token(parser, NULL);

    // ID, terminate if incomplete
    if (peek_token_class_is(parser, TOKEN_PEEK_1st, JT_IDENTIFIER))
    {
        consume_token(parser, &data->id);
    }
    else
    {
        fprintf(stderr, "TODO error: expected name after class declaration\n");
        return node;
    }

    // [Class Extends]
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_RWD_EXTENDS))
    {
        tree_node_add_child(node, parse_class_extends(parser));
    }

    // [Class Implements]
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_RWD_IMPLEMENTS))
    {
        tree_node_add_child(node, parse_class_implements(parser));
    }

    // Class Body
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_BRACE_OPEN))
    {
        tree_node_add_child(node, parse_class_body(parser));
    }
    else
    {
        fprintf(stderr, "TODO error: expected class body in class declaration\n");
    }

    return node;
}

/**
 * InterfaceDeclaration:
 *     interface Identifier [ExtendsInterfaces] InterfaceBody
*/
static tree_node* parse_interface_declaration(java_parser* parser)
{
    tree_node* node = ast_node_interface_declaration();
    node_data_interface_declaration* data = (node_data_interface_declaration*)(node->data);

    // interface
    consume_token(parser, NULL);

    // Name, terminate if incomplete
    if (peek_token_class_is(parser, TOKEN_PEEK_1st, JT_IDENTIFIER))
    {
        consume_token(parser, &data->id);
    }
    else
    {
        fprintf(stderr, "TODO error: expected name after interface declaration\n");
        return node;
    }

    /**
     * TODO:
    */

    return node;
}

/**
 * ClassExtends:
 *     extends ClassType
*/
static tree_node* parse_class_extends(java_parser* parser)
{
    tree_node* node = ast_node_class_extends();

    // extends
    consume_token(parser, NULL);

    // class type
    if (parser_trigger_class_type(parser, TOKEN_PEEK_1st))
    {
        tree_node_add_child(node, parse_class_type(parser));
    }
    else
    {
        fprintf(stderr, "TODO error: expected class type after \'extends\'\n");
    }

    return node;
}

/**
 * Interfaces:
 *     implements InterfaceTypeList
*/
static tree_node* parse_class_implements(java_parser* parser)
{
    tree_node* node = ast_node_class_implements();

    // implements
    consume_token(parser, NULL);

    // interface type list
    // triggered by interface type, so share same trigger with parse_interface_type
    if (parser_trigger_interface_type(parser, TOKEN_PEEK_1st))
    {
        tree_node_add_child(node, parse_interface_type_list(parser));
    }
    else
    {
        fprintf(stderr, "TODO error: expected interface type list after \'implements\'\n");
    }

    return node;
}

/**
 * ClassBody:
 *     { [ClassBodyDeclarations] }
 *
 * ClassBodyDeclarations:
 *     ClassBodyDeclaration
 *     ClassBodyDeclarations ClassBodyDeclaration
 *
 * ClassBodyDeclaration:
 *     ClassMemberDeclaration
 *     StaticInitializer
 *     ConstructorDeclaration
 *
 * ClassMemberDeclaration:
 *     FieldDeclaration
 *     MethodDeclaration
 *
 * Peek Logic, on ClassBodyDeclaration level:
 *
 * 1. peek(2) = static {
 * reduce: StaticInitializer
 *
 * 2. peek(1) = modifier word
 * add: Modifiers (modifier reserved words)
 *
 * 3. peek(1) = type word|ID
 * add: Type (type reserved words, Name, with trailing {[ ]}*)
 *
 * 4.1 peek(1) = (
 * reduce: ConstructorDeclaration
 *
 * 4.2 peek(2) = ID (
 * reduce: MethodDeclaration
 *
 * 4.3 peek(2) = ID [|ID =|ID ,
 * reduce: FieldDeclaration
 *
 * Step 3 is ugly, but quite sufficient for parser
 * We'll sort things out when handling context
 * e.g. constructor name must be an ID but rule 3 accepts all Type
 * as valid name for the reduction
*/
static tree_node* parse_class_body(java_parser* parser)
{
    tree_node* node = ast_node_class_body();

    // {
    consume_token(parser, NULL);

    /**
     * TODO:
    */

    // }
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_BRACE_CLOSE))
    {
        consume_token(parser, NULL);
    }
    else
    {
        fprintf(stderr, "TODO error: expected '}' at the end of class body\n");
    }

    return node;
}
