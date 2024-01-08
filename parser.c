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
 * IMPORTANT:
 *
 * All production parser functions are reduction of certain production rules.
 *
 * Meaning: once invoked, it implies that the rule is matched for reduction.
 *
 * Also meaning: look-ahead check will be made at call-site.
 *
 * Practice: for a production parse function, start parsing the following
 * without any condition check:
 * 1. start with terminal: all terminal tokens until the next is not
 * 2. start with non-terminal: one reduction of first non-terminal
 * After these, we need to peek before parsing for every single step
 *
 * Example (T is for terminal, N is for non-terminal):
 *
 * Rule1:
 *     T1 T2 N ...
 * Rule2:
 *     N1 T N2 ...
 *
 * parse_Rule1() { parse_T1(); parse_T2(); if (N) ... }
 * parse_Rule2() { parse_N1(); if (T) ... }
*/

#include "parser.h"

/**
 * Initialize parser instance
*/
void init_parser(java_parser* parser, file_buffer* buffer, java_symbol_table* rw, java_expression* expr)
{
    // tokens contains garbage data if not used
    // so the counter is important
    parser->num_token_available = 0;

    parser->buffer = buffer;
    parser->reserved_words = rw;
    parser->ast_root = NULL;
    parser->expression = expr;
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
static tree_node* parse_interface_extends(java_parser* parser);
static tree_node* parse_interface_body(java_parser* parser);
static tree_node* parse_class_body_declaration(java_parser* parser);
static tree_node* parse_static_initializer(java_parser* parser);
static tree_node* parse_block(java_parser* parser);
static tree_node* parse_statement(java_parser* parser, bool allow_variable_declaration);
static tree_node* parse_switch_statement(java_parser* parser);
static tree_node* parse_do_statement(java_parser* parser);
static tree_node* parse_break_statement(java_parser* parser);
static tree_node* parse_continue_statement(java_parser* parser);
static tree_node* parse_return_statement(java_parser* parser);
static tree_node* parse_synchronized_statement(java_parser* parser);
static tree_node* parse_throw_statement(java_parser* parser);
static tree_node* parse_try_statement(java_parser* parser);
static tree_node* parse_if_statement(java_parser* parser);
static tree_node* parse_while_statement(java_parser* parser);
static tree_node* parse_for_statement(java_parser* parser);
static tree_node* parse_label_statement(java_parser* parser);
static tree_node* parse_catch_statement(java_parser* parser);
static tree_node* parse_finally_statement(java_parser* parser);
static tree_node* parse_switch_label(java_parser* parser);
static tree_node* parse_for_init(java_parser* parser);
static tree_node* parse_for_update(java_parser* parser);
static tree_node* parse_constructor_declaration(java_parser* parser);
static tree_node* parse_type(java_parser* parser);
static tree_node* parse_method_declaration(java_parser* parser);
static tree_node* parse_field_declaration(java_parser* parser);
static tree_node* parse_formal_parameter_list(java_parser* parser);
static tree_node* parse_formal_parameter(java_parser* parser);
static tree_node* parse_throws(java_parser* parser);
static tree_node* parse_constructor_body(java_parser* parser);
static tree_node* parse_explicit_constructor_invocation(java_parser* parser);
static tree_node* parse_method_body(java_parser* parser);
static tree_node* parse_variable_declarator(java_parser* parser);
static tree_node* parse_array_initializer(java_parser* parser);
static tree_node* parse_primary_simple(java_parser* parser);
static tree_node* parse_primary_complex(java_parser* parser);
static tree_node* parse_primary_creation(java_parser* parser);
static tree_node* parse_primary_array_creation(java_parser* parser);
static tree_node* parse_primary_class_instance_creation(java_parser* parser);
static tree_node* parse_primary_method_invocation(java_parser* parser);
static tree_node* parse_primary_array_access(java_parser* parser);
static tree_node* parse_primary_class_literal(java_parser* parser);
static tree_node* parse_primary(java_parser* parser);
static tree_node* parse_expression(java_parser* parser);

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
        switch (peek_token_type(parser, TOKEN_PEEK_1st))
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
        type = peek_token_type(parser, TOKEN_PEEK_1st);

        // if not a modifier, we stop
        if (!JAVA_LEXEME_MODIFIER_WORD(type))
        {
            break;
        }

        consume_token(parser, NULL);
        data->modifier |= ((lbit_flag)1 << type);
    }

    type = peek_token_type(parser, TOKEN_PEEK_1st);

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
 *     interface Identifier [InterfaceExtends] InterfaceBody
*/
static tree_node* parse_interface_declaration(java_parser* parser)
{
    tree_node* node = ast_node_interface_declaration();
    node_data_interface_declaration* data = (node_data_interface_declaration*)(node->data);

    // interface
    consume_token(parser, NULL);

    // ID, terminate if incomplete
    if (peek_token_class_is(parser, TOKEN_PEEK_1st, JT_IDENTIFIER))
    {
        consume_token(parser, &data->id);
    }
    else
    {
        fprintf(stderr, "TODO error: expected name after interface declaration\n");
        return node;
    }

    // [Interface Extends]
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_RWD_EXTENDS))
    {
        tree_node_add_child(node, parse_interface_extends(parser));
    }

    // Interface Body
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_BRACE_OPEN))
    {
        tree_node_add_child(node, parse_interface_body(parser));
    }
    else
    {
        fprintf(stderr, "TODO error: expected interface body in interface declaration\n");
    }

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
 * ClassImplements:
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
*/
static tree_node* parse_class_body(java_parser* parser)
{
    tree_node* node = ast_node_class_body();
    java_token* peek;

    // {
    consume_token(parser, NULL);

    // {ClassBodyDeclaration}
    while (true)
    {
        peek = token_peek(parser, TOKEN_PEEK_1st);

        if (!(JAVA_LEXEME_MODIFIER_OR_TYPE_WORD(peek->type) || peek->class == JT_IDENTIFIER))
        {
            break;
        }

        tree_node_add_child(node, parse_class_body_declaration(parser));
    }

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

/**
 * InterfaceExtends:
 *     extends InterfaceTypeList
*/
static tree_node* parse_interface_extends(java_parser* parser)
{
    tree_node* node = ast_node_interface_extends();

    // extends
    consume_token(parser, NULL);

    // interface type list
    // triggered by interface type, so share same trigger with parse_interface_type
    if (parser_trigger_interface_type(parser, TOKEN_PEEK_1st))
    {
        tree_node_add_child(node, parse_interface_type_list(parser));
    }
    else
    {
        fprintf(stderr, "TODO error: expected interface type list after \'extends\'\n");
    }

    return node;
}

/**
 * InterfaceBody:
 *     { [InterfaceMemberDeclarations] }
 *
 * InterfaceMemberDeclarations:
 *     InterfaceMemberDeclaration
 *     InterfaceMemberDeclarations InterfaceMemberDeclaration
*/
static tree_node* parse_interface_body(java_parser* parser)
{
    tree_node* node = ast_node_interface_body();

    // {
    consume_token(parser, NULL);

    /**
     * TODO: [InterfaceMemberDeclarations]
    */

    // }
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_BRACE_CLOSE))
    {
        consume_token(parser, NULL);
    }
    else
    {
        fprintf(stderr, "TODO error: expected '}' at the end of interface body\n");
    }

    return node;
}

/**
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
 * 3. peek(2) = ID (
 * reduce: ConstructorDeclaration
 *
 * 4. peek(1) = type word|ID
 * add: Type (type reserved words, Name, with trailing {[ ]}*)
 *
 * 5 peek(2) = ID (
 * reduce: MethodDeclaration
 *
 * 6 peek(2) = ID [|ID =|ID ,
 * reduce: FieldDeclaration
*/
static tree_node* parse_class_body_declaration(java_parser* parser)
{
    tree_node* node = ast_node_class_body_declaration();
    node_data_class_body_declaration* data = (node_data_class_body_declaration*)(node->data);
    java_lexeme_type type;

    // StaticInitializer
    //
    // this has higher priority because trigger word "static" 
    // is also part of modifier word
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_RWD_STATIC) &&
        peek_token_type_is(parser, TOKEN_PEEK_2nd, JLT_SYM_BRACE_OPEN))
    {
        tree_node_add_child(node, parse_static_initializer(parser));
        return node;
    }

    // {Modifier}, still ambiguous
    while (true)
    {
        type = peek_token_type(parser, TOKEN_PEEK_1st);

        // if not a modifier, we stop
        if (!JAVA_LEXEME_MODIFIER_WORD(type))
        {
            break;
        }

        consume_token(parser, NULL);
        data->modifier |= ((lbit_flag)1 << type);
    }

    // ConstructorDeclaration: {Modifier} ID (
    if (peek_token_class_is(parser, TOKEN_PEEK_1st, JT_IDENTIFIER) &&
        peek_token_type_is(parser, TOKEN_PEEK_2nd, JLT_SYM_PARENTHESIS_OPEN))
    {
        tree_node_add_child(node, parse_constructor_declaration(parser));
        return node;
    }

    // Type, still ambiguous
    if (parser_trigger_type(parser, TOKEN_PEEK_1st))
    {
        tree_node_add_child(node, parse_type(parser));
    }
    else
    {
        fprintf(stderr, "TODO error: expected type name in the declaration\n");
        return node;
    }

    // MethodDeclaration|FieldDeclaration
    if (peek_token_class_is(parser, TOKEN_PEEK_1st, JT_IDENTIFIER))
    {
        switch (peek_token_type(parser, TOKEN_PEEK_2nd))
        {
            case JLT_SYM_PARENTHESIS_OPEN:
                // MethodDeclaration: {Modifier} Type ID (
                tree_node_add_child(node, parse_method_declaration(parser));
                break;
            case JLT_SYM_BRACKET_OPEN:
            case JLT_SYM_EQUAL:
            case JLT_SYM_COMMA:
            case JLT_SYM_SEMICOLON:
                // FieldDeclaration: {Modifier} Type ID [
                // FieldDeclaration: {Modifier} Type ID =
                // FieldDeclaration: {Modifier} Type ID ,
                // FieldDeclaration: {Modifier} Type ID ;
                tree_node_add_child(node, parse_field_declaration(parser));
                break;
            default:
                fprintf(stderr, "TODO error: ambiguous declaration\n");
                break;
        }
    }
    else
    {
        fprintf(stderr, "TODO error: expected a name in declaration\n");
    }

    return node;
}

/**
 * StaticInitializer:
 *     static Block
*/
static tree_node* parse_static_initializer(java_parser* parser)
{
    tree_node* node = ast_node_static_initializer();

    // static
    consume_token(parser, NULL);

    // Block
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_BRACE_OPEN))
    {
        tree_node_add_child(node, parse_block(parser));
    }
    else
    {
        fprintf(stderr, "TODO error: expected block in static initializer\n");
    }

    return node;
}

/**
 * Block:
 *     { [BlockStatements] }
 *
 * BlockStatements:
 *     BlockStatement
 *     BlockStatements BlockStatement
 *
 * BlockStatement:
 *     LocalVariableDeclarationStatement
 *     Statement
 *
 * LocalVariableDeclarationStatement:
 *     LocalVariableDeclaration ;
 * LocalVariableDeclaration:
 *     Type VariableDeclarators
 *
 * Statement:
 *     StatementWithoutTrailingSubstatement
 *     LabeledStatement
 *     IfThenStatement
 *     IfThenElseStatement
 *     WhileStatement
 *     ForStatement
 *
 * StatementWithoutTrailingSubstatement:
 *     Block
 *     EmptyStatement
 *     ExpressionStatement
 *     SwitchStatement
 *     DoStatement
 *     BreakStatement
 *     ContinueStatement
 *     ReturnStatement
 *     SynchronizedStatement
 *     ThrowStatement
 *     TryStatement
 *
 * Looking at those variants, we do not have to worry about those that start with
 * unique keyword terminal, so the ambiguity (with ID) includes:
 * 1. LocalVariableDeclaration: Starts with Type, hence Name, hence ID
 * 2. LabeledStatement: Must start with ID followed by a colon
 * 3. ExpressionStatement: May start with ID
 *
 * So we can see some cross-referencing happen between declaration and statement,
 * therefore we do NOT distinguish 2 types of statements, and all of them will
 * be discussed under same scope: BlockStatement, thus same node struct, different
 * node ID
 *
 * From 3 cases, we see 2 is the easist production, because the trigger is definite:
 * ID followed by a colon (:)
 * Therefore this one takes the highest priority to process with 2 lookaheads
 *
 * Both 1 and 3 can start with ID but with very complex trailing content within the
 * sub-node, so we need to mutate one into another that can cover both cases
 *
 * Observe that: Type is Name, Name is a kind of Primary, Primary is a kind of
 * Expression. So we say: Expression is a superset of Type
 *
 * Therefore, LocalVariableDeclaration will not be triggered by Type, instead, it will
 * parse an Expression first:
 * LocalVariableDeclaration:
 *     Expression VariableDeclarators
 * It does not make sense semantically, but parser should not care about it. In this
 * way, this production can cover only more cases, not less; so semantics check can
 * simply rule out those that do not make sense
 *
 * With this patch, case 3 takes 2nd place of priority, with trigger check as usual,
 * results in an Expression sub-node
 *
 * And finally, case 1 will then find trigger of VariableDeclarators, then look for
 * semicolon. If VariableDeclarators not triggered, then the statement is an
 * ExpressionStatement
 *
*/
static tree_node* parse_block(java_parser* parser)
{
    tree_node* node = ast_node_block();
    tree_node* statement = NULL;

    // {
    consume_token(parser, NULL);

    // {Statement}
    while (parser_trigger_statement(parser, TOKEN_PEEK_1st))
    {
        statement = parse_statement(parser, true);

        if (statement->metadata == JNT_STATEMENT_EMPTY)
        {
            // prune empty statements
            tree_node_delete(node, &parser_ast_node_data_deleter);
        }
        else
        {
            // accept others
            tree_node_add_child(node, statement);
        }
    }

    // }
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_BRACE_CLOSE))
    {
        consume_token(parser, NULL);
    }
    else
    {
        fprintf(stderr, "TODO error: expected '}' at the end of block\n");
    }

    return node;
}

/**
 * Satement
 *
 * See parser_block for more information
 *
 * This is one of few special cases that requires extra parameters to
 * avoid code duplication
*/
static tree_node* parse_statement(java_parser* parser, bool allow_variable_declaration)
{
    // used conditionally, so no initialization here
    tree_node* node = NULL;

    // statement parser dispatch
    switch (peek_token_type(parser, TOKEN_PEEK_1st))
    {
        case JLT_SYM_SEMICOLON:
            node = ast_node_statement();
            tree_node_mutate(node, JNT_STATEMENT_EMPTY);
            consume_token(parser, NULL); // ;
            return node;
        case JLT_SYM_BRACE_OPEN:
            return parse_block(parser);
        case JLT_RWD_SWITCH:
            return parse_switch_statement(parser);
        case JLT_RWD_DO:
            return parse_do_statement(parser);
        case JLT_RWD_BREAK:
            return parse_break_statement(parser);
        case JLT_RWD_CONTINUE:
            return parse_continue_statement(parser);
        case JLT_RWD_RETURN:
            return parse_return_statement(parser);
        case JLT_RWD_SYNCHRONIZED:
            return parse_synchronized_statement(parser);
        case JLT_RWD_THROW:
            return parse_throw_statement(parser);
        case JLT_RWD_TRY:
            return parse_try_statement(parser);
        case JLT_RWD_IF:
            return parse_if_statement(parser);
        case JLT_RWD_WHILE:
            return parse_while_statement(parser);
        case JLT_RWD_FOR:
            return parse_for_statement(parser);
        default:
            // fall-through for remaining cases
            break;
    }

    // order matters here
    if (peek_token_class_is(parser, TOKEN_PEEK_1st, JT_IDENTIFIER) &&
        peek_token_type_is(parser, TOKEN_PEEK_2nd, JLT_SYM_COLON))
    {
        // LabelStatement
        return parse_label_statement(parser);
    }
    else if (parser_trigger_expression(parser, TOKEN_PEEK_1st))
    {
        // first we parse as if it is an expression statement
        node = ast_node_statement();
        tree_node_mutate(node, JNT_STATEMENT_EXPRESSION);
        tree_node_add_child(node, parse_expression(parser));

        // now we mutate as needed
        if (allow_variable_declaration && peek_token_class_is(parser, TOKEN_PEEK_1st, JT_IDENTIFIER))
        {
            tree_node_mutate(node, JNT_STATEMENT_VAR_DECL);
            tree_node_add_child(node, parse_field_declaration(parser));
        }
        else if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_SEMICOLON))
        {
            // ;
            consume_token(parser, NULL);
        }
        else
        {
            fprintf(stderr, "TODO error: expected ';' at the end of expression statement.\n");
        }

        return node;
    }
    else
    {
        fprintf(stderr, "TODO error: expected statment.\n");

        // by default we return an ill-formed node
        return ast_node_statement();
    }
}

/**
 * SwitchStatement:
 *     switch ( Expression ) SwitchBlock
 *
 * SwitchBlock:
 *     { [SwitchBlockStatementGroups] [SwitchLabels] }
 *
 * SwitchBlockStatementGroups:
 *     SwitchBlockStatementGroup
 *     SwitchBlockStatementGroups SwitchBlockStatementGroup
 *
 * SwitchBlockStatementGroup:
 *     SwitchLabels BlockStatements
 *
 * SwitchLabels:
 *     SwitchLabel
 *     SwitchLabels SwitchLabel
 *
 * SwitchLabel:
 *     case ConstantExpression :
 *     default :
 *
 * Looks crazy, but we can simplify...
 *
 * SwitchBlock simply suggests the following:
 * BlockStatements behind SwitchLabel is optional
 *
 * So we have:
 * SwitchStatement:
 *     switch ( Expression ) { {SwitchLabel {SwitchLabel} {Statement}} }
 *
 * If we naively do this, then, there could exist dangling
 * labels in the end without block statements, but this is
 * actually supported in this version
 *
 * If we want every label must have at least one statement, we
 * can simply change "{Statement}" into "Statement"
 *
 * Since switch block statement always groups consecutive labels and
 * consecutive statements, so we do not need separate node level
 * to distinguish
 *
 * We do need node for switch label to log case/default info
*/
static tree_node* parse_switch_statement(java_parser* parser)
{
    tree_node* node = ast_node_statement();
    java_lexeme_type peek;

    // switch
    tree_node_mutate(node, JNT_STATEMENT_SWITCH);
    consume_token(parser, NULL);

    // (
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_PARENTHESIS_OPEN))
    {
        consume_token(parser, NULL);
    }
    else
    {
        fprintf(stderr, "TODO error: expected '(' in switch statement.\n");
        return node;
    }

    // Expression
    if (parser_trigger_expression(parser, TOKEN_PEEK_1st))
    {
        tree_node_add_child(node, parse_expression(parser));
    }
    else
    {
        fprintf(stderr, "TODO error: expected expression in switch statement.\n");
        return node;
    }

    // )
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_PARENTHESIS_CLOSE))
    {
        consume_token(parser, NULL);
    }
    else
    {
        fprintf(stderr, "TODO error: expected ')' in switch statement.\n");
        return node;
    }

    // {
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_BRACE_OPEN))
    {
        consume_token(parser, NULL);
    }
    else
    {
        fprintf(stderr, "TODO error: expected '{' in switch statement.\n");
        return node;
    }

    // {SwitchLabel {SwitchLabel} {Statement}}
    while (true)
    {
        peek = peek_token_type(parser, TOKEN_PEEK_1st);

        // break first to make sure at least one label exists
        if (peek != JLT_RWD_CASE && peek != JLT_RWD_DEFAULT)
        {
            break;
        }

        // SwitchLabel {SwitchLabel}
        // same idea but only for label
        while (true)
        {
            peek = peek_token_type(parser, TOKEN_PEEK_1st);

            // trigger check
            if (peek != JLT_RWD_CASE && peek != JLT_RWD_DEFAULT)
            {
                break;
            }

            tree_node_add_child(node, parse_switch_label(parser));
        }

        // {Statement}
        while (parser_trigger_statement(parser, TOKEN_PEEK_1st))
        {
            tree_node_add_child(node, parse_statement(parser, true));
        }
    }

    // }
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_BRACE_CLOSE))
    {
        consume_token(parser, NULL);
    }
    else
    {
        fprintf(stderr, "TODO error: expected '}' in switch statement.\n");
        return node;
    }

    return node;
}

/**
 * DoStatement:
 *     do Statement while ( Expression ) ;
*/
static tree_node* parse_do_statement(java_parser* parser)
{
    tree_node* node = ast_node_statement();

    // do
    tree_node_mutate(node, JNT_STATEMENT_DO);
    consume_token(parser, NULL);

    // Statement
    if (parser_trigger_statement(parser, TOKEN_PEEK_1st))
    {
        tree_node_add_child(node, parse_statement(parser, false));
    }
    else
    {
        fprintf(stderr, "TODO error: expected statement in do statement.\n");
        return node;
    }

    // while
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_RWD_WHILE))
    {
        consume_token(parser, NULL);
    }
    else
    {
        fprintf(stderr, "TODO error: expected 'while' in do statement.\n");
        return node;
    }

    // (
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_PARENTHESIS_OPEN))
    {
        consume_token(parser, NULL);
    }
    else
    {
        fprintf(stderr, "TODO error: expected '(' in do statement.\n");
        return node;
    }

    // Expression
    if (parser_trigger_expression(parser, TOKEN_PEEK_1st))
    {
        tree_node_add_child(node, parse_expression(parser));
    }
    else
    {
        fprintf(stderr, "TODO error: expected expression in do statement.\n");
        return node;
    }

    // )
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_PARENTHESIS_CLOSE))
    {
        consume_token(parser, NULL);
    }
    else
    {
        fprintf(stderr, "TODO error: expected ')' in do statement.\n");
        return node;
    }

    // ;
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_SEMICOLON))
    {
        consume_token(parser, NULL);
    }
    else
    {
        fprintf(stderr, "TODO error: expected ';' at the end of do statement.\n");
    }

    return node;
}

/**
 * BreakStatement:
 *     break [ID] ;
*/
static tree_node* parse_break_statement(java_parser* parser)
{
    tree_node* node = ast_node_statement();
    node_data_statement* data = (node_data_statement*)(node->data);

    // break
    tree_node_mutate(node, JNT_STATEMENT_BREAK);
    consume_token(parser, NULL);

    // [ID]
    if (peek_token_class_is(parser, TOKEN_PEEK_1st, JT_IDENTIFIER))
    {
        consume_token(parser, &data->id);
    }

    // ;
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_SEMICOLON))
    {
        consume_token(parser, NULL);
    }
    else
    {
        fprintf(stderr, "TODO error: expected ';' at the end of break statement.\n");
    }

    return node;
}

/**
 * ContinueStatement:
 *     continue [ID] ;
*/
static tree_node* parse_continue_statement(java_parser* parser)
{
    tree_node* node = ast_node_statement();
    node_data_statement* data = (node_data_statement*)(node->data);

    // continue
    tree_node_mutate(node, JNT_STATEMENT_CONTINUE);
    consume_token(parser, NULL);

    // [ID]
    if (peek_token_class_is(parser, TOKEN_PEEK_1st, JT_IDENTIFIER))
    {
        consume_token(parser, &data->id);
    }

    // ;
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_SEMICOLON))
    {
        consume_token(parser, NULL);
    }
    else
    {
        fprintf(stderr, "TODO error: expected ';' at the end of continue statement.\n");
    }

    return node;
}

/**
 * ReturnStatement:
 *     return [Expression] ;
*/
static tree_node* parse_return_statement(java_parser* parser)
{
    tree_node* node = ast_node_statement();

    // return
    tree_node_mutate(node, JNT_STATEMENT_RETURN);
    consume_token(parser, NULL);

    // [Expression]
    if (parser_trigger_expression(parser, TOKEN_PEEK_1st))
    {
        tree_node_add_child(node, parse_expression(parser));
    }

    // ;
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_SEMICOLON))
    {
        consume_token(parser, NULL);
    }
    else
    {
        fprintf(stderr, "TODO error: expected ';' at the end of return statement.\n");
    }

    return node;
}

/**
 * SynchronizedStatement:
 *     synchronized ( Expression ) Block
*/
static tree_node* parse_synchronized_statement(java_parser* parser)
{
    tree_node* node = ast_node_statement();

    // throw
    tree_node_mutate(node, JNT_STATEMENT_THROW);
    consume_token(parser, NULL);

    // (
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_PARENTHESIS_OPEN))
    {
        consume_token(parser, NULL);
    }
    else
    {
        fprintf(stderr, "TODO error: expected '(' in synchronized statement.\n");
        return node;
    }

    // Expression
    if (parser_trigger_expression(parser, TOKEN_PEEK_1st))
    {
        tree_node_add_child(node, parse_expression(parser));
    }
    else
    {
        fprintf(stderr, "TODO error: expected expression in synchronized statement.\n");
        return node;
    }

    // )
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_PARENTHESIS_CLOSE))
    {
        consume_token(parser, NULL);
    }
    else
    {
        fprintf(stderr, "TODO error: expected ')' in synchronized statement.\n");
        return node;
    }

    // Block
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_BRACE_OPEN))
    {
        tree_node_add_child(node, parse_block(parser));
    }
    else
    {
        fprintf(stderr, "TODO error: expected block in synchronized statement.\n");
        return node;
    }

    return node;
}

/**
 * ThrowStatement:
 *     throw Expression ;
*/
static tree_node* parse_throw_statement(java_parser* parser)
{
    tree_node* node = ast_node_statement();

    // throw
    tree_node_mutate(node, JNT_STATEMENT_THROW);
    consume_token(parser, NULL);

    // Expression
    if (parser_trigger_expression(parser, TOKEN_PEEK_1st))
    {
        tree_node_add_child(node, parse_expression(parser));
    }
    else
    {
        fprintf(stderr, "TODO error: expected expression in throw statement.\n");
        return node;
    }

    // ;
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_SEMICOLON))
    {
        consume_token(parser, NULL);
    }
    else
    {
        fprintf(stderr, "TODO error: expected ';' at the end of throw statement.\n");
    }

    return node;
}

/**
 * TryStatement:
 *     try Block Catches
 *     try Block [Catches] Finally
 *
 * Catches:
 *     CatchClause
 *     Catches CatchClause
*/
static tree_node* parse_try_statement(java_parser* parser)
{
    tree_node* node = ast_node_statement();

    // try
    tree_node_mutate(node, JNT_STATEMENT_TRY);
    consume_token(parser, NULL);

    // Block
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_BRACE_OPEN))
    {
        tree_node_add_child(node, parse_block(parser));
    }
    else
    {
        fprintf(stderr, "TODO error: expected block in try statement.\n");
        return node;
    }

    // we need to log once to see if catch clause exists
    bool has_catch_clause = peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_RWD_CATCH);

    // {CatchClause}
    while (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_RWD_CATCH))
    {
        tree_node_add_child(node, parse_catch_statement(parser));
    }

    // [Finally]
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_RWD_FINALLY))
    {
        tree_node_add_child(node, parse_finally_statement(parser));
    }
    else if (!has_catch_clause)
    {
        // catch of finally clause must exist
        fprintf(stderr, "TODO error: expected catch and/or finally clause after try block.\n");
    }

    return node;
}

/**
 * TODO: if
*/
static tree_node* parse_if_statement(java_parser* parser)
{
    ;
}

/**
 * WhileStatement:
 *     while ( Expression ) Statement
*/
static tree_node* parse_while_statement(java_parser* parser)
{
    tree_node* node = ast_node_statement();

    // while
    tree_node_mutate(node, JNT_STATEMENT_WHILE);
    consume_token(parser, NULL);

    // (
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_PARENTHESIS_OPEN))
    {
        consume_token(parser, NULL);
    }
    else
    {
        fprintf(stderr, "TODO error: expected '(' in while statement.\n");
        return node;
    }

    // Expression
    if (parser_trigger_expression(parser, TOKEN_PEEK_1st))
    {
        tree_node_add_child(node, parse_expression(parser));
    }
    else
    {
        fprintf(stderr, "TODO error: expected expression in while statement.\n");
        return node;
    }

    // )
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_PARENTHESIS_CLOSE))
    {
        consume_token(parser, NULL);
    }
    else
    {
        fprintf(stderr, "TODO error: expected ')' in while statement.\n");
        return node;
    }

    // Statement
    if (parser_trigger_statement(parser, TOKEN_PEEK_1st))
    {
        tree_node_add_child(node, parse_statement(parser, false));
    }
    else
    {
        fprintf(stderr, "TODO error: expected statement in while statement.\n");
        return node;
    }

    return node;
}

/**
 * ForStatement:
 *     for ( [ForInit] ; [Expression] ; [ForUpdate] ) Statement
 *
 * ForInit:
 *     StatementExpressionList
 *     LocalVariableDeclaration
 *
 * ForUpdate:
 *     StatementExpressionList
 *
 * StatementExpressionList:
 *     StatementExpression
 *     StatementExpressionList , StatementExpression
 *
 * Same idea as the one applied in Statement, ForInit can be simplified as:
 * Statement (separated by comma) that can start with expression only
*/
static tree_node* parse_for_statement(java_parser* parser)
{
    tree_node* node = ast_node_statement();

    // for
    tree_node_mutate(node, JNT_STATEMENT_FOR);
    consume_token(parser, NULL);

    // (
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_PARENTHESIS_OPEN))
    {
        consume_token(parser, NULL);
    }
    else
    {
        fprintf(stderr, "TODO error: expected '(' in while statement.\n");
        return node;
    }

    // [ForInit]
    if (parser_trigger_expression(parser, TOKEN_PEEK_1st))
    {
        tree_node_add_child(node, parse_for_init(parser));
    }

    // ;
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_SEMICOLON))
    {
        consume_token(parser, NULL);
    }
    else
    {
        fprintf(stderr, "TODO error: expected ';' after for initialization list.\n");
        return node;
    }

    // [Expression]
    if (parser_trigger_expression(parser, TOKEN_PEEK_1st))
    {
        tree_node_add_child(node, parse_expression(parser));
    }

    // ;
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_SEMICOLON))
    {
        consume_token(parser, NULL);
    }
    else
    {
        fprintf(stderr, "TODO error: expected ';' before for update list.\n");
        return node;
    }

    // [ForUpdate]
    if (parser_trigger_expression(parser, TOKEN_PEEK_1st))
    {
        tree_node_add_child(node, parse_for_update(parser));
    }

    // )
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_PARENTHESIS_CLOSE))
    {
        consume_token(parser, NULL);
    }
    else
    {
        fprintf(stderr, "TODO error: expected ')' in while statement.\n");
        return node;
    }

    // Statement
    if (parser_trigger_statement(parser, TOKEN_PEEK_1st))
    {
        tree_node_add_child(node, parse_statement(parser, false));
    }
    else
    {
        fprintf(stderr, "TODO error: expected statement in while statement.\n");
        return node;
    }

    return node;
}

/**
 * LabeledStatement:
 *     ID : Statement
*/
static tree_node* parse_label_statement(java_parser* parser)
{
    tree_node* node = ast_node_statement();
    node_data_statement* data = (node_data_statement*)(node->data);

    // ID :
    tree_node_mutate(node, JNT_STATEMENT_LABEL);
    consume_token(parser, &data->id);
    consume_token(parser, NULL);

    // Statement
    if (parser_trigger_statement(parser, TOKEN_PEEK_1st))
    {
        tree_node_add_child(node, parse_statement(parser, false));
    }
    else
    {
        fprintf(stderr, "TODO error: expected statement at the end of label statement.\n");
    }

    return node;
}

/**
 * CatchClause:
 *     catch ( FormalParameter ) Block
*/
static tree_node* parse_catch_statement(java_parser* parser)
{
    tree_node* node = ast_node_statement();

    // catch
    tree_node_mutate(node, JNT_STATEMENT_CATCH);
    consume_token(parser, NULL);

    // (
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_PARENTHESIS_OPEN))
    {
        consume_token(parser, NULL);
    }
    else
    {
        fprintf(stderr, "TODO error: expected '(' in catch clause.\n");
        return node;
    }

    // FormalParameter
    if (parser_trigger_type(parser, TOKEN_PEEK_1st))
    {
        tree_node_add_child(node, parse_formal_parameter(parser));
    }
    else
    {
        fprintf(stderr, "TODO error: expected expression in catch clause.\n");
        return node;
    }

    // )
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_PARENTHESIS_CLOSE))
    {
        consume_token(parser, NULL);
    }
    else
    {
        fprintf(stderr, "TODO error: expected ')' in catch clause.\n");
        return node;
    }

    // Block
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_BRACE_OPEN))
    {
        tree_node_add_child(node, parse_block(parser));
    }
    else
    {
        fprintf(stderr, "TODO error: expected block in catch clause.\n");
        return node;
    }

    return node;
}

/**
 * Finally:
 *     finally Block
*/
static tree_node* parse_finally_statement(java_parser* parser)
{
    tree_node* node = ast_node_statement();

    // finally
    tree_node_mutate(node, JNT_STATEMENT_FINALLY);
    consume_token(parser, NULL);

    // Block
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_BRACE_OPEN))
    {
        tree_node_add_child(node, parse_block(parser));
    }
    else
    {
        fprintf(stderr, "TODO error: expected block in finally clause.\n");
    }

    return node;
}

/**
 * SwitchLabel:
 *     case ConstantExpression :
 *     default :
 *
 * ConstantExpression:
 *     Expression
*/
static tree_node* parse_switch_label(java_parser* parser)
{
    tree_node* node = ast_node_switch_label();
    node_data_switch_label* data = (node_data_switch_label*)(node->data);

    // case/default
    data->is_default = peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_RWD_DEFAULT);
    consume_token(parser, NULL);

    // case label requires an Expression
    if (!data->is_default)
    {
        // Expression
        if (parser_trigger_expression(parser, TOKEN_PEEK_1st))
        {
            tree_node_add_child(node, parse_expression(parser));
        }
        else
        {
            fprintf(stderr, "TODO error: expected expression in case label.\n");
            return node;
        }
    }

    // :
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_COLON))
    {
        consume_token(parser, NULL);
    }
    else
    {
        fprintf(stderr, "TODO error: expected ':' at the end of switch label.\n");
    }

    return node;
}

/**
 * ForInit:
 *     StatementExpressionList
 *     LocalVariableDeclaration
 *
 * StatementExpressionList:
 *     StatementExpression
 *     StatementExpressionList , StatementExpression
 *
 * Thanks to the idea of Statement, ForInit is StatementExpressionList
 * which is:
 *
 * StatementExpressionList:
 *     Statement(true) {, Statement(true)}
*/
static tree_node* parse_for_init(java_parser* parser)
{
    tree_node* node = ast_node_for_init();

    // must use Expression trigger for this (see below)
    tree_node_add_child(node, parse_statement(parser, true));

    // {, Statement}
    while (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_COMMA))
    {
        // ,
        consume_token(parser, NULL);

        // statement that can only start with expression, so trigger
        // here is expression instead of statement
        if (parser_trigger_expression(parser, TOKEN_PEEK_1st))
        {
            tree_node_add_child(node, parse_statement(parser, true));
        }
        else
        {
            fprintf(stderr, "TODO error: expected expression/declaration in for initialization.\n");
            break;
        }
    }

    return node;
}

/**
 * ForUpdate:
 *     StatementExpressionList
 *
 * Different than ForInit, this list does not allow declarations
*/
static tree_node* parse_for_update(java_parser* parser)
{
    tree_node* node = ast_node_for_update();

    // must use Expression trigger for this (see below)
    tree_node_add_child(node, parse_statement(parser, false));

    // {, Statement}
    while (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_COMMA))
    {
        // ,
        consume_token(parser, NULL);

        // statement that can only start with expression, so trigger
        // here is expression instead of statement
        if (parser_trigger_expression(parser, TOKEN_PEEK_1st))
        {
            tree_node_add_child(node, parse_statement(parser, false));
        }
        else
        {
            fprintf(stderr, "TODO error: expected expression/declaration in for initialization.\n");
            break;
        }
    }

    return node;
}

/**
 * ConstructorDeclaration:
 *     ID ( [FormalParameterList] ) [Throws] ConstructorBody
*/
static tree_node* parse_constructor_declaration(java_parser* parser)
{
    tree_node* node = ast_node_constructor_declaration();
    node_data_constructor_declaration* data = (node_data_constructor_declaration*)(node->data);

    // ID (
    consume_token(parser, &data->id);
    consume_token(parser, NULL);

    // [FormalParameterList]
    if (parser_trigger_type(parser, TOKEN_PEEK_1st))
    {
        tree_node_add_child(node, parse_formal_parameter_list(parser));
    }

    // )
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_PARENTHESIS_CLOSE))
    {
        consume_token(parser, NULL);
    }
    else
    {
        fprintf(stderr, "TODO error: expected ')' at constructor declaration\n");
        return node;
    }

    // [Throws]
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_RWD_THROWS))
    {
        tree_node_add_child(node, parse_throws(parser));
    }

    // ConstructorBody
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_BRACE_OPEN))
    {
        tree_node_add_child(node, parse_constructor_body(parser));
    }
    else
    {
        fprintf(stderr, "TODO error: expected constructor body\n");
    }

    return node;
}

/**
 * Type:
 *     type reserved words {[ ]}
 *     ClassType {[ ]}
 *
 * Here we have a conflict to resolve:
 * Name, ClassType, and InterfaceType
 *
 * ClassType and InterfaceType are same (even at SE 20), but
 * out of respect, we add a clause here
 *
 * Now, Name vs ClassType?
 *
 * obviously: all Name can be a valid ClassType;
 * while ClassType may not be a Name
 *
 * The above statement is very true after introducing template,
 * so we add that compatibility now.
*/
static tree_node* parse_type(java_parser* parser)
{
    tree_node* node = ast_node_type();
    node_data_type* data = (node_data_type*)(node->data);

    /**
     * We need to directly accept without checking at the
     * beginning, are we breaking the rule here?
     *
     * This part looks breaking the rule, but it is for
     * different logic for each condition
     *
     * That is why we do not have 'else' clause here
     * for error check
    */
    if (peek_token_is_type_word(parser, TOKEN_PEEK_1st))
    {
        data->primitive = peek_token_type(parser, TOKEN_PEEK_1st);
        consume_token(parser, NULL);
    }
    else if (parser_trigger_class_type(parser, TOKEN_PEEK_1st))
    {
        tree_node_add_child(node, parse_class_type(parser));
    }
    else if (parser_trigger_interface_type(parser, TOKEN_PEEK_1st))
    {
        tree_node_add_child(node, parse_interface_type(parser));
    }

    // {[ ]}
    while (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_BRACKET_OPEN))
    {
        // [
        consume_token(parser, NULL);

        // ]
        if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_BRACKET_CLOSE))
        {
            consume_token(parser, NULL);
        }
        else
        {
            fprintf(stderr, "TODO error: expected ']'\n");
            break;
        }

        // track dimensions
        data->dimension++;
    }

    return node;
}

/**
 * MethodDeclaration:
 *     ID ( [FormalParameterList] ) {[ ]} [Throws] MethodBody
*/
static tree_node* parse_method_declaration(java_parser* parser)
{
    tree_node* node = ast_node_method_declaration();
    node_data_method_declaration* data = (node_data_method_declaration*)(node->data);

    // ID (
    consume_token(parser, &data->id);
    consume_token(parser, NULL);

    // [FormalParameterList]
    if (parser_trigger_type(parser, TOKEN_PEEK_1st))
    {
        tree_node_add_child(node, parse_formal_parameter_list(parser));
    }

    // )
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_PARENTHESIS_CLOSE))
    {
        consume_token(parser, NULL);
    }
    else
    {
        fprintf(stderr, "TODO error: expected ')' at constructor declaration\n");
        return node;
    }

    // [Throws]
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_RWD_THROWS))
    {
        tree_node_add_child(node, parse_throws(parser));
    }

    // MethodBody
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_BRACE_OPEN))
    {
        tree_node_add_child(node, parse_method_body(parser));
    }
    else
    {
        fprintf(stderr, "TODO error: expected method body\n");
    }

    return node;
}

/**
 * FieldDeclaration:
 *     VariableDeclarator {, VariableDeclarator} ;
 *
 * The modification here is: we do not include Type
 * part at the beginning, because many things can
 * start with Type, e.g. method declaration
*/
static tree_node* parse_field_declaration(java_parser* parser)
{
    tree_node* node = ast_node_field_declaration();

    // VariableDeclarator
    tree_node_add_child(node, parse_variable_declarator(parser));

    // {, VariableDeclarator}
    while (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_COMMA) &&
        peek_token_class_is(parser, TOKEN_PEEK_2nd, JT_IDENTIFIER))
    {
        // ,
        consume_token(parser, NULL);

        // VariableDeclarator
        tree_node_add_child(node, parse_variable_declarator(parser));
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
 * FormalParameterList:
 *     FormalParameter
 *     FormalParameterList , FormalParameter
 *
 * since the unit is not a simple non-terminal, so we cannot
 * flatten this level
*/
static tree_node* parse_formal_parameter_list(java_parser* parser)
{
    tree_node* node = ast_node_formal_parameter_list();

    // FormalParameter
    tree_node_add_child(node, parse_formal_parameter(parser));

    // {, FormalParameter}
    while (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_COMMA) &&
        parser_trigger_type(parser, TOKEN_PEEK_2nd))
    {
        // ,
        consume_token(parser, NULL);

        // FormalParameter
        tree_node_add_child(node, parse_formal_parameter(parser));
    }

    return node;
}

/**
 * FormalParameter:
 *     Type VariableDeclaratorId
 *
 * VariableDeclaratorId:
 *     ID {[ ]}
 *
 * we flatten VariableDeclaratorId here
*/
static tree_node* parse_formal_parameter(java_parser* parser)
{
    tree_node* node = ast_node_formal_parameter();
    node_data_formal_parameter* data = (node_data_formal_parameter*)(node->data);

    // Type
    tree_node_add_child(node, parse_type(parser));

    // VariableDeclaratorId => ID {[ ]}
    if (peek_token_class_is(parser, TOKEN_PEEK_1st, JT_IDENTIFIER))
    {
        consume_token(parser, &data->id);

        // {[ ]}
        while (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_BRACKET_OPEN))
        {
            // [
            consume_token(parser, NULL);

            // ]
            if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_BRACKET_CLOSE))
            {
                consume_token(parser, NULL);
            }
            else
            {
                fprintf(stderr, "TODO error: expected ']'\n");
                break;
            }

            // track dimensions
            data->dimension++;
        }
    }
    else
    {
        fprintf(stderr, "TODO error: expected parameter name\n");
    }

    return node;
}

/**
 * Throws:
 *     throws ClassTypeList
 *
 * ClassTypeList:
 *     ClassType
 *     ClassTypeList , ClassType
 *
 * Since the class type list does not expand the unit ClassType any further
 * so we can flatten the rule to reduce complexity of AST
*/
static tree_node* parse_throws(java_parser* parser)
{
    tree_node* node = ast_node_throws();

    // throws
    consume_token(parser, NULL);

    if (parser_trigger_class_type(parser, TOKEN_PEEK_1st))
    {
        // ClassType
        tree_node_add_child(node, parse_class_type(parser));

        // {, ClassType}
        while (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_COMMA) &&
            parser_trigger_class_type(parser, TOKEN_PEEK_2nd))
        {
            // ,
            consume_token(parser, NULL);

            // ClassType
            tree_node_add_child(node, parse_class_type(parser));
        }
    }
    else
    {
        fprintf(stderr, "TODO error: expected class name\n");
    }

    return node;
}

/**
 * ArgumentList:
 *     Expression {, Expression}
*/
static tree_node* parse_argument_list(java_parser* parser)
{
    tree_node* node = ast_node_argument_list();

    // Expression
    tree_node_add_child(node, parse_expression(parser));

    // {, Expression}
    while (peek_token_type(parser, TOKEN_PEEK_1st) == JLT_SYM_COMMA)
    {
        // ,
        consume_token(parser, NULL);

        if (parser_trigger_expression(parser, TOKEN_PEEK_1st))
        {
            // Expression
            tree_node_add_child(node, parse_expression(parser));
        }
        else
        {
            fprintf(stderr, "TODO error: expected expression\n");
            break;
        }
    }

    return node;
}

/**
 * ConstructorBody:
 *     { [ExplicitConstructorInvocation] [BlockStatements] }
*/
static tree_node* parse_constructor_body(java_parser* parser)
{
    tree_node* node = ast_node_constructor_body();

    // {
    consume_token(parser, NULL);

    // [ExplicitConstructorInvocation]
    java_lexeme_type peek_1st = peek_token_type(parser, TOKEN_PEEK_1st);
    if ((peek_1st == JLT_RWD_SUPER || peek_1st == JLT_RWD_THIS) &&
        peek_token_type_is(parser, TOKEN_PEEK_2nd, JLT_SYM_PARENTHESIS_OPEN))
    {
        tree_node_add_child(node, parse_explicit_constructor_invocation(parser));
    }

    // [BlockStatements]
    if (parser_trigger_statement(parser, TOKEN_PEEK_1st))
    {
        tree_node_add_child(node, parse_statement(parser, true));
    }

    // }
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_BRACE_CLOSE))
    {
        consume_token(parser, NULL);
    }
    else
    {
        fprintf(stderr, "TODO error: expected '}' at the end of block\n");
    }

    return node;
}

/**
 * ExplicitConstructorInvocation:
 *     this ( [ArgumentList] ) ;
 *     super ( [ArgumentList] ) ;
 *
 * ArgumentList:
 *     Expression {, Expression}
 *
 * We MUST use 2 lookaheads to trigger because this production is actually
 * covered by Primary
 *
 * This rule exists because constructor invocation must stay at the front
 * of constructor body
*/
static tree_node* parse_explicit_constructor_invocation(java_parser* parser)
{
    tree_node* node = ast_node_constructor_invocation();
    node_data_constructor_invoke* data = (node_data_constructor_invoke*)(node->data);

    // this/super (
    data->is_super = peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_RWD_SUPER);
    consume_token(parser, NULL);
    consume_token(parser, NULL);

    // [ArgumentList]
    if (parser_trigger_expression(parser, TOKEN_PEEK_1st))
    {
        tree_node_add_child(node, parse_argument_list(parser));
    }

    // )
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_PARENTHESIS_CLOSE))
    {
        consume_token(parser, NULL);
    }
    else
    {
        fprintf(stderr, "TODO error: expected ')' at the end of constructor invocation\n");
    }

    return node;
}

/**
 * MethodBody:
 *     Block
 *     ;
*/
static tree_node* parse_method_body(java_parser* parser)
{
    tree_node* node = ast_node_method_body();

    // Block
    switch (peek_token_type(parser, TOKEN_PEEK_1st))
    {
        case JLT_SYM_BRACE_OPEN:
            tree_node_add_child(node, parse_block(parser));
            break;
        case JLT_SYM_SEMICOLON:
            consume_token(parser, NULL);
            break;
        default:
            fprintf(stderr, "TODO error: expected block or ';'\n");
            break;
    }

    return node;
}

/**
 * VariableDeclarator:
 *     VariableDeclaratorId [= VariableInitializer]
 *
 * VariableDeclaratorId:
 *     ID {[ ]}
 *
 * VariableInitializer:
 *     Expression
 *     ArrayInitializer
 *
 * we flatten VariableDeclaratorId & VariableInitializer here
*/
static tree_node* parse_variable_declarator(java_parser* parser)
{
    tree_node* node = ast_node_variable_declarator();
    node_data_variable_declarator* data = (node_data_variable_declarator*)(node->data);

    // VariableDeclaratorId => ID {[ ]}
    consume_token(parser, &data->id);

    // {[ ]}
    while (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_BRACKET_OPEN))
    {
        // [
        consume_token(parser, NULL);

        // ]
        if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_BRACKET_CLOSE))
        {
            consume_token(parser, NULL);
        }
        else
        {
            fprintf(stderr, "TODO error: expected ']'\n");
            return node;
        }

        // track dimensions
        data->dimension++;
    }

    // [= VariableInitializer] => [= Expression|ArrayInitializer]
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_EQUAL))
    {
        consume_token(parser, NULL);

        if (parser_trigger_expression(parser, TOKEN_PEEK_1st))
        {
            // Expression
            tree_node_add_child(node, parse_expression(parser));
        }
        else if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_BRACE_OPEN))
        {
            // ArrayInitializer
            tree_node_add_child(node, parse_array_initializer(parser));
        }
        else
        {
            fprintf(stderr, "TODO error: expected initializer\n");
        }
    }

    return node;
}

/**
 * ArrayInitializer:
 *     { [VariableInitializers] [,] }
 *
 * VariableInitializers:
 *     VariableInitializer
 *     VariableInitializers , VariableInitializer
 *
 * VariableInitializer:
 *     Expression
 *     ArrayInitializer
 *
 * flatten VariableInitializers and VariableInitializer
 * since VariableInitializers is optional, so
 * [VariableInitializers] is actually {VariableInitializers}
 *
 * the comma rule looks messy in here, but in code it looks
 * more elegant
*/
static tree_node* parse_array_initializer(java_parser* parser)
{
    tree_node* node = ast_node_array_initializer();

    // {
    consume_token(parser, NULL);

    // {Expression|ArrayInitializer ,}
    // except: last comma in sequence is optional
    while (true)
    {
        if (parser_trigger_expression(parser, TOKEN_PEEK_1st))
        {
            // Expression
            tree_node_add_child(node, parse_expression(parser));
        }
        else if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_BRACE_OPEN))
        {
            // ArrayInitializer
            // YOLO: recursion!
            tree_node_add_child(node, parse_array_initializer(parser));
        }
        else
        {
            // stop at invalid token as an initializer
            break;
        }

        // [,]
        if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_COMMA))
        {
            consume_token(parser, NULL);
        }
        else
        {
            // it is only optional if no initializer remained and vice versa
            // so if we do not detect a comma, it means initializer list
            // must be done
            break;
        }
    }

    // }
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_BRACE_CLOSE))
    {
        consume_token(parser, NULL);
    }
    else
    {
        // missing comma with dangling initializer also reaches here
        fprintf(stderr, "TODO error: expected '}' at the end of array initializer\n");
    }

    return node;
}

/**
 * PrimarySimple: primary that contains only token type info
*/
static tree_node* parse_primary_simple(java_parser* parser)
{
    tree_node* node = ast_node_primary_simple();

    ((node_data_primary_simple*)(node->data))->type = peek_token_type(parser, TOKEN_PEEK_1st);
    consume_token(parser, NULL);

    return node;
}

/**
 * PrimaryComplex: primary that contains needs token data
*/
static tree_node* parse_primary_complex(java_parser* parser)
{
    tree_node* node = ast_node_primary_complex();

    consume_token(parser, &((node_data_primary_complex*)(node->data))->token);

    return node;
}

/**
 * PrimaryCreationExpression:
 *     new Type PrimaryArrayCreation
 *     new Type PrimaryClassInstanceCreation
*/
static tree_node* parse_primary_creation(java_parser* parser)
{
    tree_node* node = ast_node_primary_creation();

    // new
    consume_token(parser, NULL);

    // Type
    if (parser_trigger_type(parser, TOKEN_PEEK_1st))
    {
        tree_node_add_child(node, parse_type(parser));
    }
    else
    {
        fprintf(stderr, "TODO error: expected type at creation expression\n");
        return node;
    }

    // PrimaryArrayCreation|PrimaryClassInstanceCreation
    switch (peek_token_type(parser, TOKEN_PEEK_1st))
    {
        case JLT_SYM_BRACKET_OPEN:
            tree_node_add_child(node, parse_primary_array_creation(parser));
            break;
        case JLT_SYM_PARENTHESIS_OPEN:
            tree_node_add_child(node, parse_primary_class_instance_creation(parser));
            break;
        default:
            fprintf(stderr, "TODO error: expected array or class instance creation\n");
            break;
    }

    return node;
}

/**
 * PrimaryArrayCreation:
 *     [ Expression ] {[ Expression ]} {[ ]}
 *     [ ] {[ ]} ArrayInitializer
*/
static tree_node* parse_primary_array_creation(java_parser* parser)
{
    tree_node* node = ast_node_primary_array_creation();
    node_data_primary_array_creation* data = (node_data_primary_array_creation*)(node->data);
    bool first_variadic = true;
    bool accepting_variadic;

    // [
    consume_token(parser, NULL);

    // first chunk: determin if it starts with variadic dimension
    if (parser_trigger_expression(parser, TOKEN_PEEK_1st))
    {
        first_variadic = false;

        // Expression
        // YOLO: recursion!
        tree_node_add_child(node, parse_expression(parser));
    }

    // first chunk enclosure
    if (peek_token_type(parser, TOKEN_PEEK_1st) == JLT_SYM_BRACKET_CLOSE)
    {
        // ]
        consume_token(parser, NULL);
        data->dims_var++;
    }
    else
    {
        fprintf(stderr, "TODO error: expected ']'\n");
        return node;
    }

    // {[ Expression ]} {[ ]}
    accepting_variadic = first_variadic;
    while (peek_token_type(parser, TOKEN_PEEK_1st) == JLT_SYM_BRACKET_OPEN)
    {
        // [
        consume_token(parser, NULL);

        if (!accepting_variadic && parser_trigger_expression(parser, TOKEN_PEEK_1st))
        {
            // Expression
            // YOLO: recursion!
            tree_node_add_child(node, parse_expression(parser));
        }
        else
        {
            // variadic dims must stay at the end
            accepting_variadic = true;
        }

        // ]
        if (peek_token_type(parser, TOKEN_PEEK_1st) == JLT_SYM_BRACKET_CLOSE)
        {
            consume_token(parser, NULL);
        }
        else
        {
            fprintf(stderr, "TODO error: expected ']'\n");
            break;
        }

        data->dims_var++;
    }

    // ArrayInitializer
    if (first_variadic)
    {
        if (peek_token_type(parser, TOKEN_PEEK_1st) == JLT_SYM_BRACE_OPEN)
        {
            tree_node_add_child(node, parse_array_initializer(parser));
        }
        else
        {
            fprintf(stderr, "TODO error: expected array initializer\n");
        }
    }

    return node;
}

/**
 * PrimaryClassInstanceCreation:
 *     ( [ArgumentList] ) [ClassBody]
 *
 * ArgumentList:
 *     Expression {, Expression}
*/
static tree_node* parse_primary_class_instance_creation(java_parser* parser)
{
    tree_node* node = ast_node_primary_class_instance_creation();

    // (
    consume_token(parser, NULL);

    // [ArgumentList]
    if (parser_trigger_expression(parser, TOKEN_PEEK_1st))
    {
        tree_node_add_child(node, parse_argument_list(parser));
    }

    // )
    if (peek_token_type(parser, TOKEN_PEEK_1st) == JLT_SYM_PARENTHESIS_CLOSE)
    {
        consume_token(parser, NULL);
    }
    else
    {
        fprintf(stderr, "TODO error: expected ')'\n");
        return node;
    }

    // [ClassBody]
    if (peek_token_type(parser, TOKEN_PEEK_1st) == JLT_SYM_BRACE_OPEN)
    {
        tree_node_add_child(node, parse_class_body(parser));
    }

    return node;
}

/**
 * PrimaryMethodInvocation:
 *     ( [ArgumentList] )
 *
 * ArgumentList:
 *     Expression {, Expression}
*/
static tree_node* parse_primary_method_invocation(java_parser* parser)
{
    tree_node* node = ast_node_primary_method_invocation();

    // (
    consume_token(parser, NULL);

    // [ArgumentList]
    if (parser_trigger_expression(parser, TOKEN_PEEK_1st))
    {
        tree_node_add_child(node, parse_argument_list(parser));
    }

    // )
    if (peek_token_type(parser, TOKEN_PEEK_1st) == JLT_SYM_PARENTHESIS_CLOSE)
    {
        consume_token(parser, NULL);
    }
    else
    {
        fprintf(stderr, "TODO error: expected ')'\n");
        return node;
    }

    return node;
}

/**
 * PrimaryArrayAccess:
 *     [ Expression ] {[ Expression ]}
*/
static tree_node* parse_primary_array_access(java_parser* parser)
{
    tree_node* node = ast_node_primary_array_access();

    // based on our design rule:
    // this loop is guaranteed to have at least 1 iteration
    while (peek_token_type(parser, TOKEN_PEEK_1st) == JLT_SYM_BRACKET_OPEN)
    {
        // [
        consume_token(parser, NULL);

        if (parser_trigger_expression(parser, TOKEN_PEEK_1st))
        {
            // Expression
            // YOLO: recursion!
            tree_node_add_child(node, parse_expression(parser));
        }
        else
        {
            fprintf(stderr, "TODO error: expected expression\n");
            break;
        }

        // ]
        if (peek_token_type(parser, TOKEN_PEEK_1st) == JLT_SYM_BRACKET_CLOSE)
        {
            consume_token(parser, NULL);
        }
        else
        {
            fprintf(stderr, "TODO error: expected ']'\n");
            break;
        }
    }

    return node;
}

/**
 * PrimaryClassLiteral: . class
 *
 * caller needs both tokens checked
*/
static tree_node* parse_primary_class_literal(java_parser* parser)
{
    tree_node* node = ast_node_primary_class_literal();

    consume_token(parser, NULL);
    consume_token(parser, NULL);

    return node;
}

/**
 * To understand Primary, we break it down into 2 parts
 *
 * 1. Simple Primary: building block of Complex Primary which is Primary
 * including: literals, ID, this, new, and parenthesis expression: ( Expression )
 * also includes trailing content: class literal (.class), array access ([]),
 * and argument list (())
 *
 * 2. (Complex) Primary: on top level, we also have connectors,
 * including: field access (.) and method reference (::)
 *
 * Now we need to figure out parser's job for those parts.
 *
 * In a nutshell: keep reading any part, if matched, we accept, terminate otherwise
 *
 * This will result something that does not make any sense,
 * e.g. ....................a
 * Should parser worry if it makes sense? NO!
 * Parser always and only maximizes pattern matching, it does not make sense out of anything
 *
 * Another note about parenthesis expression: ( Expression )
 * We must do it on Primary level to cover cases like the following:
 * (a + 1).toString()
 * also because the syntax of type casting, which is essentially consecutive parenthesis closures
 * since we only match patterns without justification in parser so it implicitly accept sequence
 * like (...)(...), which is the form of type casting
 *
 * Accepting ( Expression ) on this level also means:
 * 1. Expression does not accept parenthesis, instead, it will dispatch it to Primary to handle
 * 2. This supports the precendence meaning of parenthesis because parenthesized expression
 *    will go deeper in the tree, hence higher precendence
*/
static tree_node* parse_primary(java_parser* parser)
{
    tree_node* node = ast_node_primary();
    bool accepting = true;

    while (accepting)
    {
        switch (peek_token_type(parser, TOKEN_PEEK_1st))
        {
            case JLT_SYM_PARENTHESIS_OPEN:
                // ( Expression )
                consume_token(parser, NULL);
                tree_node_add_child(node, parse_expression(parser));
                if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_PARENTHESIS_CLOSE))
                {
                    consume_token(parser, NULL);
                }
                else
                {
                    fprintf(stderr, "TODO error: expected ')'\n");
                }
                break;
            case JLT_SYM_DOT:
            case JLT_SYM_METHOD_REFERENCE:
            case JLT_RWD_TRUE:
            case JLT_RWD_FALSE:
            case JLT_RWD_NULL:
            case JLT_RWD_THIS:
            case JLT_RWD_SUPER:
                tree_node_add_child(node, parse_primary_simple(parser));
                break;
            case JLT_RWD_NEW:
                tree_node_add_child(node, parse_primary_creation(parser));
                break;
            case JLT_LTR_NUMBER:
            case JLT_LTR_CHARACTER:
            case JLT_LTR_STRING:
                tree_node_add_child(node, parse_primary_complex(parser));
                break;
            default:
                if (peek_token_class_is(parser, TOKEN_PEEK_1st, JT_IDENTIFIER))
                {
                    tree_node_add_child(node, parse_primary_complex(parser));

                    // now the trailing content
                    // there could be many, so we keep it going until mismatched
                    while (true)
                    {
                        // 2 look-ahead first, to buffer most #tokens
                        if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_DOT) &&
                            peek_token_type_is(parser, TOKEN_PEEK_2nd, JLT_RWD_CLASS))
                        {
                            tree_node_add_child(node, parse_primary_class_literal(parser));
                        }
                        else if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_PARENTHESIS_OPEN))
                        {
                            tree_node_add_child(node, parse_primary_method_invocation(parser));
                        }
                        else if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_BRACKET_OPEN))
                        {
                            tree_node_add_child(node, parse_primary_array_access(parser));
                        }
                        else
                        {
                            // terminate
                            break;
                        }
                    }
                }
                else
                {
                    // reject by default
                    accepting = false;
                }
                break;
        }
    }

    return node;
}

/**
 * All Primary are Expression, but not all Expression are Primary
 *
 * Shunting Yard Algorithm:
 * https://mathcenter.oxford.emory.edu/site/cs171/shuntingYardAlgorithm/
 *
 * 1. If the incoming symbols is an operand, print it..
 * 2. If the incoming symbol is a left parenthesis, push it on the stack.
 * 3. If the incoming symbol is a right parenthesis: discard the right parenthesis,
 *    pop and print the stack symbols until you see a left parenthesis.
 *    Pop the left parenthesis and discard it.
 * 4. If the incoming symbol is an operator and the stack is empty or contains a
 *    left parenthesis on top, push the incoming operator onto the stack.
 * 5. If the incoming symbol is an operator and has either higher precedence than
 *    the operator on the top of the stack, or has the same precedence as the
 *    operator on the top of the stack and is right associative -- push it on the stack.
 * 6. If the incoming symbol is an operator and has either lower precedence than the operator
 *    on the top of the stack, or has the same precedence as the operator on the top of the
 *    stack and is left associative -- continue to pop the stack until this is not true.
 *    Then, push the incoming operator.
 * 7. At the end of the expression, pop and print all operators on the stack.
 *    (No parentheses should remain.)
 *
 * But in actual implementation, parenthesis is handled in Primary, so Expression will dispatch
 * it to Primary and does deeper in the tree, hence supports precedence
 * Meaning: Step 2 and 3 are not necessary
*/
static tree_node* parse_expression(java_parser* parser)
{
    tree_node* node = ast_node_expression();
    java_expression_worker worker;
    java_lexeme_type token_type;
    java_operator op_type;
    bool next_is_operator;

    // get worker ready
    init_expression_worker(&worker);

    while (true)
    {
        token_type = peek_token_type(parser, TOKEN_PEEK_1st);
        op_type = parser->expression->op_map[token_type];
        next_is_operator = true;

        // token-operator preprocessing: 1-1 mapping
        switch (token_type)
        {
            case JLT_SYM_EQUAL:
            case JLT_SYM_ANGLE_BRACKET_OPEN:
            case JLT_SYM_ANGLE_BRACKET_CLOSE:
            case JLT_SYM_EXCALMATION:
            case JLT_SYM_TILDE:
            case JLT_SYM_ASTERISK:
            case JLT_SYM_FORWARD_SLASH:
            case JLT_SYM_AMPERSAND:
            case JLT_SYM_PIPE:
            case JLT_SYM_CARET:
            case JLT_SYM_PERCENT:
            case JLT_SYM_QUESTION:
            case JLT_SYM_COLON:
            case JLT_SYM_ARROW:
            case JLT_SYM_RELATIONAL_EQUAL:
            case JLT_SYM_LESS_EQUAL:
            case JLT_SYM_GREATER_EQUAL:
            case JLT_SYM_NOT_EQUAL:
            case JLT_SYM_LOGIC_AND:
            case JLT_SYM_LOGIC_OR:
            case JLT_SYM_LEFT_SHIFT:
            case JLT_SYM_RIGHT_SHIFT:
            case JLT_SYM_RIGHT_SHIFT_UNSIGNED:
            case JLT_SYM_ADD_ASSIGNMENT:
            case JLT_SYM_SUBTRACT_ASSIGNMENT:
            case JLT_SYM_MULTIPLY_ASSIGNMENT:
            case JLT_SYM_DIVIDE_ASSIGNMENT:
            case JLT_SYM_BIT_AND_ASSIGNMENT:
            case JLT_SYM_BIT_OR_ASSIGNMENT:
            case JLT_SYM_BIT_XOR_ASSIGNMENT:
            case JLT_SYM_MODULO_ASSIGNMENT:
            case JLT_SYM_LEFT_SHIFT_ASSIGNMENT:
            case JLT_SYM_RIGHT_SHIFT_ASSIGNMENT:
            case JLT_SYM_RIGHT_SHIFT_UNSIGNED_ASSIGNMENT:
                // no-op as the mapped type is correct
                break;
            case JLT_SYM_PLUS:
                // handle ambiguity
                if (!worker.last_push_operand)
                {
                    op_type = parser->expression->definition[OPID_SIGN_POS];
                }
                break;
            case JLT_SYM_MINUS:
                // handle ambiguity
                if (!worker.last_push_operand)
                {
                    op_type = parser->expression->definition[OPID_SIGN_NEG];
                }
                break;
            case JLT_SYM_INCREMENT:
                // handle ambiguity
                if (!worker.last_push_operand)
                {
                    op_type = parser->expression->definition[OPID_PRE_INC];
                }
                break;
            case JLT_SYM_DECREMENT:
                // handle ambiguity
                if (!worker.last_push_operand)
                {
                    op_type = parser->expression->definition[OPID_PRE_DEC];
                }
                break;
            default:
                next_is_operator = false;
                break;
        }

        // acceptance
        if (next_is_operator)
        {
            // now we need to consume the token for sure
            consume_token(parser, NULL);

            // precedence validation
            while (expression_stack_pop_required(&worker, op_type))
            {
                tree_node_add_child(node, expression_stack_parse_top(&worker));
            }

            expression_stack_push(&worker, op_type);
        }
        else if (parser_trigger_primary(parser, TOKEN_PEEK_1st))
        {
            tree_node_add_child(node, parse_primary(parser));
            expression_stack_push(&worker, OPID_UNDEFINED);
        }
        else
        {
            // terminator
            break;
        }
    }

    // pop all operators and push them in sequence
    while (!expression_stack_empty(&worker))
    {
        tree_node_add_child(node, expression_stack_parse_top(&worker));
    }

    // cleanup
    release_expression_worker(&worker);

    return node;
}
