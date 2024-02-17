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
void init_parser(
    java_parser* parser,
    file_buffer* buffer,
    hash_table* rw,
    java_expression* expr,
    java_error_stack* err
)
{
    // tokens contains garbage data if not used
    // so the counter is important
    parser->num_token_available = 0;

    parser->buffer = buffer;
    parser->reserved_words = rw;
    parser->ast_root = NULL;
    parser->expression = expr;
    parser->error = err;
}

/**
 * Copy parser instance
 *
 * a copy of parser instance has shallow copy of all static data,
 * and does not copy AST
*/
void copy_parser(java_parser* from, java_parser* to)
{
    memcpy(to, from, sizeof(java_parser));

    /**
     * a copy of parser insatnce should never affect current AST
     *
     * AST is complex, and should always manipulate only from
     * the original reference
    */
    to->ast_root = NULL;

    /**
     * error stack needs to be isolated
     *
     * but error definitions are still shared
    */
    to->error = (java_error_stack*)malloc_assert(sizeof(java_error_stack));
    init_error_stack(to->error, from->error->def);

    /**
     * file buffer state needs to be isolated
     *
     * the content buffer though does not need deep copy because
     * it is static
    */
    to->buffer = (file_buffer*)malloc_assert(sizeof(file_buffer));
    memcpy(to->buffer, from->buffer, sizeof(file_buffer));
    to->buffer->error = to->error; // redirect error stack
}

/**
 * Mutate Parser Instance
 *
 * Mutate main instance with copy instance
 * and copy instance will override the parser state
 *
 * NOTE: copied fields are released by calling release_parser
 *
 * WARNING: the parser instance has shallow reference of data
 * by definition, so during swap all pointer references must
 * stay the same and perform deep copy as needed
*/
void mutate_parser(java_parser* parser, java_parser* copy)
{
    // lookaheads: deep copy
    memcpy(parser->tokens, copy->tokens, sizeof(java_token) * 4);
    parser->num_token_available = copy->num_token_available;

    // file buffer deep copy
    // only cur pointer is isolated, others are static so no copy
    parser->buffer->cur = copy->buffer->cur;

    // error stack should be merged
    error_stack_concat(parser->error, copy->error);

    // reserved words and expression data are static, so no copy

    // guard: copy instance should not have AST
    if (copy->ast_root)
    {
        fprintf(stderr, "TODO error: internal error: compiler is trying to swap an ill-formed parser instance.\n");
    }
}

/**
 * Release parser instance
 *
 * if deleting a copy of parser instance, the AST data must be NULL
*/
void release_parser(java_parser* parser, bool is_copy)
{
    if (is_copy)
    {
        if (parser->ast_root)
        {
            fprintf(stderr, "TODO error: internal error: compiler is trying to delete an ill-formed parser instance.\n");
        }

        // a copy of instance has isolated file buffer
        // no need to release, because only the struct is the copy
        free(parser->buffer);

        // delete error stack
        release_error_stack(parser->error);
        free(parser->error);
    }
    else
    {
        tree_node_delete(parser->ast_root);
    }
}

/* HELPER FUNCTIONS */

typedef tree_node* (*parser_func)(java_parser*);

/**
 * Parser wrapper with ambiguity resolution
 *
 * WARNING: do NOT abuse this function due to its complexity and limitation
 *
 * ALWAYS: make sure both productions are terminated by same symbol, otherwise
 * the result may cause incorrect behavior for future parsing
 * if the terminator is included in both productions, use JLT_MAX as terminator
 * if a production is not clearly bounded by a terminal, do NOT use it
 *
 * TODO: we probably need a clever way to propagate error messages
*/
static tree_node* parse_binary_ambiguity(
    java_parser* parser,
    parser_func f1,
    parser_func f2,
    java_lexeme_type terminator)
{
    tree_node* node = NULL;
    tree_node* n1 = NULL;
    tree_node* n2 = NULL;
    bool n1_valid = false;
    bool n2_valid = false;

    // copy parser instance
    java_parser* parser_1 = (java_parser*)malloc_assert(sizeof(java_parser));
    java_parser* parser_2 = (java_parser*)malloc_assert(sizeof(java_parser));
    copy_parser(parser, parser_1);
    copy_parser(parser, parser_2);

    // parse path 1
    n1 = (*f1)(parser_1);
    n1_valid = parser_1->error->num_err == 0;

    // parse path 2
    n2 = (*f2)(parser_2);
    n2_valid = parser_2->error->num_err == 0;

    // verify terminator and determine final validity status
    n1_valid = n1_valid &&
        (terminator == JLT_MAX || peek_token_type_is(parser_1, TOKEN_PEEK_1st, terminator));
    n2_valid = n2_valid &&
        (terminator == JLT_MAX || peek_token_type_is(parser_2, TOKEN_PEEK_1st, terminator));

    if (n1_valid && n2_valid)
    {
        // keep both: mutate using n1
        node = ast_node_ambiguous();
        tree_node_add_child(node, n1);
        tree_node_add_child(node, n2);

        // convergence test
        if (parser_1->buffer->cur != parser_2->buffer->cur)
        {
            fprintf(stderr,
                "TODO error: internal error: ambiguity diverges: termination differs (0x%x 0x%x).\n",
                *(parser_1->buffer->cur), *(parser_2->buffer->cur)
            );
        }

        /**
         * when keeping both, error stack will enclose errors from both paths
         * with special flags
         *
         * for convenience, parser will be mutated using n1
        */

        // first pathway
        parser_error(parser, JAVA_E_AMBIGUITY_START);
        node->data->ambiguity.error = error_stack_top(parser->error);
        mutate_parser(parser, parser_1);

        // second pathway
        parser_error(parser, JAVA_E_AMBIGUITY_SEPARATOR);
        error_stack_concat(parser->error, parser_2->error);
        parser_error(parser, JAVA_E_AMBIGUITY_END);
    }
    else if (n2_valid || !n2->ambiguous)
    {
        /**
         * if n2 is valid OR
         * n2 pathway is uniquely deducted
         *
         * keep n2
        */
        node = n2;
        tree_node_delete(n1);
    }
    else
    {
        /**
         * if n1 is valid OR
         * n1 pathway is uniquely deducted OR
         * no correct pathway
         *
         * keep n1
        */
        node = n1;
        tree_node_delete(n2);
    }

    // mutate parser accordingly
    if (node->type != JNT_AMBIGUOUS)
    {
        mutate_parser(parser, node == n1 ? parser_1 : parser_2);
    }

    // delete parser instance copy
    release_parser(parser_1, true);
    release_parser(parser_2, true);
    free(parser_1);
    free(parser_2);

    return node;
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
static tree_node* parse_interface_body_declaration(java_parser* parser);
static tree_node* parse_class_body_declaration(java_parser* parser);
static tree_node* parse_static_initializer(java_parser* parser);
static tree_node* parse_block(java_parser* parser);
static tree_node* parse_statement(java_parser* parser);
static tree_node* parse_expression_statement(java_parser* parser);
static tree_node* parse_local_variable_declaration(java_parser* parser);
static tree_node* parse_local_variable_declaration_statement(java_parser* parser);
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
static tree_node* parse_expression_list(java_parser* parser);
static tree_node* parse_for_init(java_parser* parser);
static tree_node* parse_for_update(java_parser* parser);
static tree_node* parse_constructor_declaration(java_parser* parser);
static tree_node* parse_type(java_parser* parser);
static tree_node* parse_method_header(java_parser* parser);
static tree_node* parse_method_declaration(java_parser* parser);
static tree_node* parse_variable_declarators(java_parser* parser);
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
                // standalone semicolon on top level can be safely ignored
                consume_token(parser, NULL);
                break;
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

    if (!peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_MAX))
    {
        parser_error(parser, JAVA_E_TRAILING_CONTENT);
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

    // ID
    consume_token(parser, node->data->id.complex);

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

    // ID
    consume_token(parser, node->data->id.complex);

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

    // ID
    consume_token(parser, node->data->id.complex);

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
        parser_error(parser, JAVA_E_PKG_DECL_NO_NAME);
        return node;
    }

    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_SEMICOLON))
    {
        consume_token(parser, NULL);
    }
    else
    {
        parser_error(parser, JAVA_E_PKG_DECL_NO_SEMICOLON);
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

    // import
    consume_token(parser, NULL);

    // Name, terminate if incomplete
    if (parser_trigger_name(parser, TOKEN_PEEK_1st))
    {
        tree_node_add_child(node, parse_name(parser));
    }
    else
    {
        parser_error(parser, JAVA_E_IMPORT_NO_NAME);
        return node;
    }

    // [. *] on-demand sequence
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_DOT) &&
        peek_token_type_is(parser, TOKEN_PEEK_2nd, JLT_SYM_ASTERISK))
    {
        consume_token(parser, NULL);
        consume_token(parser, NULL);
        node->data->import.on_demand = true;
    }

    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_SEMICOLON))
    {
        consume_token(parser, NULL);
    }
    else
    {
        parser_error(parser, JAVA_E_IMPORT_NO_SEMICOLON);
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
        node->data->top_level_declaration.modifier |= ((lbit_flag)1 << type);
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

    // class
    consume_token(parser, NULL);

    // ID, skip if incomplete
    if (peek_token_class_is(parser, TOKEN_PEEK_1st, JT_IDENTIFIER))
    {
        consume_token(parser, node->data->id.complex);
    }
    else
    {
        parser_error(parser, JAVA_E_CLASS_NO_NAME);
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
        parser_error(parser, JAVA_E_CLASS_NO_BODY);
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

    // interface
    consume_token(parser, NULL);

    // ID, terminate if incomplete
    if (peek_token_class_is(parser, TOKEN_PEEK_1st, JT_IDENTIFIER))
    {
        consume_token(parser, node->data->id.complex);
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
        parser_error(parser, JAVA_E_CLASS_BODY_ENCLOSE);
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
 *     { {InterfaceMemberDeclaration} }
*/
static tree_node* parse_interface_body(java_parser* parser)
{
    tree_node* node = ast_node_interface_body();
    java_token* peek;

    // {
    consume_token(parser, NULL);

    // {InterfaceMemberDeclaration}
    while (true)
    {
        peek = token_peek(parser, TOKEN_PEEK_1st);

        if (!(JAVA_LEXEME_MODIFIER_OR_TYPE_WORD(peek->type) || peek->class == JT_IDENTIFIER))
        {
            break;
        }

        tree_node_add_child(node, parse_interface_body_declaration(parser));
    }

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
 * InterfaceMemberDeclaration:
 *     ConstantDeclaration
 *     AbstractMethodDeclaration
 *
 * ConstantDeclaration:
 *     FieldDeclaration
 *
 * AbstractMethodDeclaration:
 *     MethodHeader ;
 *
 * FieldDeclaration:
 *     [FieldModifiers] Type VariableDeclarators ;
 *
 * MethodHeader:
 *     [Modifiers] Type MethodDeclarator [Throws]
 *
 * Just like what we did for class body declaration, we discuss the following:
 * 1. [Modifiers] Type VariableDeclarators ;
 * 2. [Modifiers] Type MethodDeclarator [Throws]
 *
 * Approach is similar, so we do not repeat here.
*/
static tree_node* parse_interface_body_declaration(java_parser* parser)
{
    tree_node* node = ast_node_interface_body_declaration();
    java_lexeme_type type;

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
        node->data->top_level_declaration.modifier |= ((lbit_flag)1 << type);
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

    // AbstractMethodDeclaration|FieldDeclaration
    if (peek_token_class_is(parser, TOKEN_PEEK_1st, JT_IDENTIFIER))
    {
        switch (peek_token_type(parser, TOKEN_PEEK_2nd))
        {
            case JLT_SYM_PARENTHESIS_OPEN:
                // AbstractMethodDeclaration: {Modifier} Type ID (
                tree_node_add_child(node, parse_method_header(parser));

                // ;
                if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_SEMICOLON))
                {
                    consume_token(parser, NULL);
                }
                else
                {
                    fprintf(stderr, "TODO error: expected ';'\n");
                }
                break;
            case JLT_SYM_BRACKET_OPEN:
            case JLT_SYM_EQUAL:
            case JLT_SYM_COMMA:
            case JLT_SYM_SEMICOLON:
                // FieldDeclaration: {Modifier} Type ID [
                // FieldDeclaration: {Modifier} Type ID =
                // FieldDeclaration: {Modifier} Type ID ,
                // FieldDeclaration: {Modifier} Type ID ;
                tree_node_add_child(node, parse_variable_declarators(parser));

                // ;
                if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_SEMICOLON))
                {
                    consume_token(parser, NULL);
                }
                else
                {
                    fprintf(stderr, "TODO error: expected ';'\n");
                }
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
 * 6 peek(2) = ID [|ID =|ID ,|ID ;
 * reduce: FieldDeclaration
*/
static tree_node* parse_class_body_declaration(java_parser* parser)
{
    tree_node* node = ast_node_class_body_declaration();
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
        node->data->top_level_declaration.modifier |= ((lbit_flag)1 << type);
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
        // if missing, we terminate current reduction and start at next declaration
        parser_error(parser, JAVA_E_MEMBER_NO_TYPE);
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
                tree_node_add_child(node, parse_variable_declarators(parser));

                // ;
                if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_SEMICOLON))
                {
                    consume_token(parser, NULL);
                }
                else
                {
                    parser_error(parser, JAVA_E_MEMBER_VAR_NO_SEMICOLON);
                }
                break;
            default:
                parser_error(parser, JAVA_E_MEMBER_AMBIGUOUS);
                break;
        }
    }
    else
    {
        parser_error(parser, JAVA_E_MEMBER_NO_NAME);
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
 *     { {Statement} }
 *
 * Empty statement will be discarded
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
        statement = parse_statement(parser);

        if (statement->type == JNT_STATEMENT_EMPTY)
        {
            // prune empty statements
            tree_node_delete(node);
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
 * Statement:
 *     LocalVariableDeclarationStatement
 *     ExpressionStatement
 *     LabeledStatement
 *     IfThenStatement
 *     IfThenElseStatement
 *     WhileStatement
 *     ForStatement
 *     Block
 *     EmptyStatement
 *     SwitchStatement
 *     DoStatement
 *     BreakStatement
 *     ContinueStatement
 *     ReturnStatement
 *     SynchronizedStatement
 *     ThrowStatement
 *     TryStatement
 *
 * AMBIGUITY: When trigger is ID, ambiguity occurs between:
 * 1. LocalVariableDeclarationStatement
 * 2. ExpressionStatement
 *
 * As a solution, we are using parallel deduction approach here:
 * reduce both simultaneously
 *
 * This approach works because both production end with same token:
 * semicolon.
 *
 * Same terminator is very important when applying this approach,
 * because it guarantees that successful reduction ends at same place,
 * hence avoids avalanche effect in future parsing.
 *
 * If both succeeds, we keep both;
 * If one succeeds, we keep it;
 * Otherwise, we randomly choose one, and generate error message
 *
 * Why can't we use Expression to compensate the ambiguous part,
 * in this case: Type?
 * It is because there are contents Expression cannot handle
 * e.g. array dimensions definition: String[] ss;
 * also, in future version, generic type: T<A> B;
 *
 * One to note is that generic type syntax is more painful because
 * it uses operator "<" and ">" as enclosure symbol, so it is
 * impossible to distinguish this on Expression level
 *
 * So in conclusion, parallel deduction is the most robust way to go.
*/
static tree_node* parse_statement(java_parser* parser)
{
    // used conditionally, so no initialization here
    tree_node* node = NULL;

    // statement parser dispatch
    switch (peek_token_type(parser, TOKEN_PEEK_1st))
    {
        case JLT_SYM_SEMICOLON:
            node = ast_node_statement(false);
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
        case JLT_RWD_BOOLEAN:
        case JLT_RWD_DOUBLE:
        case JLT_RWD_BYTE:
        case JLT_RWD_INT:
        case JLT_RWD_SHORT:
        case JLT_RWD_VOID:
        case JLT_RWD_CHAR:
        case JLT_RWD_LONG:
        case JLT_RWD_FLOAT:
            // type words guarantees variable declaration
            return parse_local_variable_declaration_statement(parser);
        default:
            // fall-through for remaining cases
            break;
    }

    /**
     * ID trigger must be discussed separately due to potential ambiguity
     *
     * Expression trigger must stay behind ID trigger because it can also
     * be triggered by ID
    */
    if (peek_token_class_is(parser, TOKEN_PEEK_1st, JT_IDENTIFIER))
    {
        // :
        if (peek_token_type_is(parser, TOKEN_PEEK_2nd, JLT_SYM_COLON))
        {
            // LabelStatement
            return parse_label_statement(parser);
        }
        else
        {
            /**
             * here we have ambiguity:
             * 1. Local variable declaration statement
             * 2. Expression statement
             *
             * 1 starts with Type, which has form that makes it not ambiguous
            */
            return parse_binary_ambiguity(
                parser,
                &parse_local_variable_declaration_statement,
                &parse_expression_statement,
                JLT_MAX
            );
        }
    }
    else if (parser_trigger_expression(parser, TOKEN_PEEK_1st))
    {
        // ExpressionStatement
        // now it is triggered by non-ID
        return parse_expression_statement(parser);
    }
    else
    {
        fprintf(stderr, "TODO error: expected statment.\n");

        // by default we return an ill-formed node
        return ast_node_statement(false);
    }
}

/**
 * ExpressionStatement:
 *     Expression ;
 *
 * This statement, without additional context, is ambiguous
 * but it can be validated once terminated by semicolon
 *
 * NOTE: due to potential ambiguity, this parser function
 * should not generate any error messages
 * (or at least pending it before call site determines)
*/
static tree_node* parse_expression_statement(java_parser* parser)
{
    tree_node* node = ast_node_statement(false);

    // Expression
    tree_node_mutate(node, JNT_STATEMENT_EXPRESSION);
    tree_node_add_child(node, parse_expression(parser));

    // ;
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_SEMICOLON))
    {
        consume_token(parser, NULL);
    }
    else
    {
        parser_error(parser, JAVA_E_LOCAL_VAR_NO_SEMICOLON);
    }

    return node;
}

/**
 * LocalVariableDeclaration:
 *     Type VariableDeclarators
*/
static tree_node* parse_local_variable_declaration(java_parser* parser)
{
    tree_node* node = ast_node_statement(false);

    // Type
    tree_node_mutate(node, JNT_LOCAL_VAR_DECL);
    tree_node_add_child(node, parse_type(parser));

    // VariableDeclarators
    if (peek_token_class_is(parser, TOKEN_PEEK_1st, JT_IDENTIFIER))
    {
        tree_node_add_child(node, parse_variable_declarators(parser));
    }
    else
    {
        parser_error(parser, JAVA_E_VAR_NO_DECLARATOR);
    }

    return node;
}

/**
 * LocalVariableDeclarationStatement:
 *     LocalVariableDeclaration ;
*/
static tree_node* parse_local_variable_declaration_statement(java_parser* parser)
{
    tree_node* node = ast_node_statement(false);

    // LocalVariableDeclaration
    tree_node_mutate(node, JNT_STATEMENT_VAR_DECL);
    tree_node_add_child(node, parse_local_variable_declaration(parser));

    // ;
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_SEMICOLON))
    {
        consume_token(parser, NULL);
    }
    else
    {
        parser_error(parser, JAVA_E_LOCAL_VAR_NO_SEMICOLON);
    }

    return node;
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
    tree_node* node = ast_node_statement(false);
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
            tree_node_add_child(node, parse_statement(parser));
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
    tree_node* node = ast_node_statement(false);

    // do
    tree_node_mutate(node, JNT_STATEMENT_DO);
    consume_token(parser, NULL);

    // Statement
    if (parser_trigger_statement(parser, TOKEN_PEEK_1st))
    {
        tree_node_add_child(node, parse_statement(parser));
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
    tree_node* node = ast_node_statement(true);

    // break
    tree_node_mutate(node, JNT_STATEMENT_BREAK);
    consume_token(parser, NULL);

    // [ID]
    if (peek_token_class_is(parser, TOKEN_PEEK_1st, JT_IDENTIFIER))
    {
        consume_token(parser, node->data->id.complex);
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
    tree_node* node = ast_node_statement(true);

    // continue
    tree_node_mutate(node, JNT_STATEMENT_CONTINUE);
    consume_token(parser, NULL);

    // [ID]
    if (peek_token_class_is(parser, TOKEN_PEEK_1st, JT_IDENTIFIER))
    {
        consume_token(parser, node->data->id.complex);
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
    tree_node* node = ast_node_statement(false);

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
    tree_node* node = ast_node_statement(false);

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
    tree_node* node = ast_node_statement(false);

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
    tree_node* node = ast_node_statement(false);

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
 * IfStatement:
 *     if ( Expression ) Statement [else Statement]
 *
 * Dangling else ambiguity?
 * Solve it by attaching else to nearby if statement
 * This solution is implicitly supported by recursive parsing
*/
static tree_node* parse_if_statement(java_parser* parser)
{
    tree_node* node = ast_node_statement(false);

    // if
    tree_node_mutate(node, JNT_STATEMENT_IF);
    consume_token(parser, NULL);

    // (
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_PARENTHESIS_OPEN))
    {
        consume_token(parser, NULL);
    }
    else
    {
        fprintf(stderr, "TODO error: expected '(' in if statement.\n");
        return node;
    }

    // Expression
    if (parser_trigger_expression(parser, TOKEN_PEEK_1st))
    {
        tree_node_add_child(node, parse_expression(parser));
    }
    else
    {
        fprintf(stderr, "TODO error: expected expression in if statement.\n");
        return node;
    }

    // )
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_PARENTHESIS_CLOSE))
    {
        consume_token(parser, NULL);
    }
    else
    {
        fprintf(stderr, "TODO error: expected ')' in if statement.\n");
        return node;
    }

    // Statement
    // only statement in block allows variable declarations
    if (parser_trigger_statement(parser, TOKEN_PEEK_1st))
    {
        tree_node_add_child(node, parse_statement(parser));
    }
    else
    {
        fprintf(stderr, "TODO error: expected statement in if clause.\n");
        return node;
    }

    // [else Statement]
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_RWD_ELSE))
    {
        // else
        consume_token(parser, NULL);

        // Statement
        // only statement in block allows variable declarations
        if (parser_trigger_statement(parser, TOKEN_PEEK_1st))
        {
            tree_node_add_child(node, parse_statement(parser));
        }
        else
        {
            fprintf(stderr, "TODO error: expected statement in else clause.\n");
            return node;
        }
    }

    return node;
}

/**
 * WhileStatement:
 *     while ( Expression ) Statement
*/
static tree_node* parse_while_statement(java_parser* parser)
{
    tree_node* node = ast_node_statement(false);

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
        tree_node_add_child(node, parse_statement(parser));
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
*/
static tree_node* parse_for_statement(java_parser* parser)
{
    tree_node* node = ast_node_statement(false);

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
    if (parser_trigger_type(parser, TOKEN_PEEK_1st) || parser_trigger_expression(parser, TOKEN_PEEK_1st))
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
        tree_node_add_child(node, parse_statement(parser));
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
    tree_node* node = ast_node_statement(true);

    // ID :
    tree_node_mutate(node, JNT_STATEMENT_LABEL);
    consume_token(parser, node->data->id.complex);
    consume_token(parser, NULL);

    // Statement
    if (parser_trigger_statement(parser, TOKEN_PEEK_1st))
    {
        tree_node_add_child(node, parse_statement(parser));
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
    tree_node* node = ast_node_statement(false);

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
    tree_node* node = ast_node_statement(false);

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
 *
 * AMBIGUITY: the trailing colon(:) is a valid operator in "? :"
 * so Expression will handle the case and not consume ":" if it is
 * not dominated by "?"
*/
static tree_node* parse_switch_label(java_parser* parser)
{
    tree_node* node = ast_node_switch_label();

    // case/default
    node->data->switch_label.is_default = peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_RWD_DEFAULT);
    consume_token(parser, NULL);

    // case label requires an Expression
    if (!node->data->switch_label.is_default)
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
 * StatementExpressionList:
 *     Expression {, Expression}
*/
static tree_node* parse_expression_list(java_parser* parser)
{
    tree_node* node = ast_node_expression_list();

    // Expression
    tree_node_add_child(node, parse_statement(parser));

    // {, Statement}
    while (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_COMMA))
    {
        // ,
        consume_token(parser, NULL);

        // statement that can only start with expression, so trigger
        // here is expression instead of statement
        if (parser_trigger_expression(parser, TOKEN_PEEK_1st))
        {
            tree_node_add_child(node, parse_statement(parser));
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
 * ForInit:
 *     StatementExpressionList
 *     LocalVariableDeclaration
 *
 * AMBIGUITY: both are terminated by semicolon on ForStatement level
 * so we can use parallel deduction here
*/
static tree_node* parse_for_init(java_parser* parser)
{
    tree_node* node = ast_node_for_init();

    // optimization: for some cases, trigger will not cause ambiguity
    if (peek_token_is_primitive_type(parser, TOKEN_PEEK_1st))
    {
        // LocalVariableDeclaration
        tree_node_add_child(node, parse_local_variable_declaration(parser));
    }
    else
    {
        tree_node_add_child(node,
            parse_binary_ambiguity(
                parser, &parse_local_variable_declaration, &parse_expression_list, JLT_SYM_SEMICOLON));
    }

    return node;
}

/**
 * ForUpdate:
 *     StatementExpressionList
*/
static tree_node* parse_for_update(java_parser* parser)
{
    tree_node* node = ast_node_for_update();

    // StatementExpressionList
    tree_node_add_child(node, parse_expression_list(parser));

    return node;
}

/**
 * ConstructorDeclaration:
 *     ID ( [FormalParameterList] ) [Throws] ConstructorBody
*/
static tree_node* parse_constructor_declaration(java_parser* parser)
{
    tree_node* node = ast_node_constructor_declaration();

    // ID (
    consume_token(parser, node->data->id.complex);
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
    if (peek_token_is_primitive_type(parser, TOKEN_PEEK_1st))
    {
        // primitive type word makes a Type uniquely produced
        node->ambiguous = false;
        node->data->declarator.id.simple = peek_token_type(parser, TOKEN_PEEK_1st);
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
        node->data->declarator.dimension++;
    }

    // array dimension makes a Type uniquely produced
    node->ambiguous = node->ambiguous && node->data->declarator.dimension == 0;

    return node;
}

/**
 * MethodHeader:
 *     ID ( [FormalParameterList] ) {[ ]} [Throws]
 *
 * the {[ ]} is legacy syntax that marks return value as "array type";
 * and this syntax has been deprecated and should not be supported in
 * newer versions
*/
static tree_node* parse_method_header(java_parser* parser)
{
    tree_node* node = ast_node_method_header();

    // ID (
    consume_token(parser, node->data->declarator.id.complex);
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
        node->data->declarator.dimension++;
    }

    // [Throws]
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_RWD_THROWS))
    {
        tree_node_add_child(node, parse_throws(parser));
    }

    return node;
}

/**
 * MethodDeclaration:
 *     MethodHeader MethodBody
*/
static tree_node* parse_method_declaration(java_parser* parser)
{
    tree_node* node = ast_node_method_declaration();

    // MethodHeader
    tree_node_add_child(node, parse_method_header(parser));

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
 * VariableDeclarators:
 *     VariableDeclarator {, VariableDeclarator}
*/
static tree_node* parse_variable_declarators(java_parser* parser)
{
    tree_node* node = ast_node_variable_declarators();

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

    // Type
    tree_node_add_child(node, parse_type(parser));

    // VariableDeclaratorId => ID {[ ]}
    if (peek_token_class_is(parser, TOKEN_PEEK_1st, JT_IDENTIFIER))
    {
        consume_token(parser, node->data->declarator.id.complex);

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
            node->data->declarator.dimension++;
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
 *     { [ExplicitConstructorInvocation] {Statement} }
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

    // {Statement}
    while (parser_trigger_statement(parser, TOKEN_PEEK_1st))
    {
        tree_node_add_child(node, parse_statement(parser));
    }

    // }
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_BRACE_CLOSE))
    {
        consume_token(parser, NULL);
    }
    else
    {
        fprintf(stderr, "TODO error: expected '}' at the end of constructor body\n");
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

    // this/super (
    node->data->constructor_invoke.is_super = peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_RWD_SUPER);
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

    // VariableDeclaratorId => ID {[ ]}
    consume_token(parser, node->data->declarator.id.complex);

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
            parser_error(parser, JAVA_E_VAR_NO_ARR_ENCLOSE);
            return node;
        }

        // track dimensions
        node->data->declarator.dimension++;
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

    node->data->id.simple = peek_token_type(parser, TOKEN_PEEK_1st);
    consume_token(parser, NULL);

    return node;
}

/**
 * PrimaryComplex: primary that contains needs token data
*/
static tree_node* parse_primary_complex(java_parser* parser)
{
    tree_node* node = ast_node_primary_complex();

    // ID
    consume_token(parser, node->data->id.complex);

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
        node->data->declarator.dimension++;
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

        node->data->declarator.dimension++;
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
 * Top-Level rules
 * 1. separators cannot stay consecutively
 * 2. ( Expression ) can be followed by anything, thus type casting
 * 3. other valid Primary form can only be followed by separators
 *
 * and we need another trick for trailing content of ID trigger:
 * 1. array access
 * 2. method call
 * 3. class literal (. class)
 * so that they will be separately accepted and stay isolated from top-level rules
 *
 * Accepting ( Expression ) on this level also means:
 * 1. Expression does not accept parenthesis, instead, it will dispatch it to Primary to handle
 * 2. This supports the precendence meaning of parenthesis because parenthesized expression
 *    will go deeper in the tree, hence higher precendence
 *
 * AMBIGUITY: parenthesized expression may have 2 forms:
 * 1. ( Expression )
 * 2. ( Type )
 * they are ambiguous, but they both terminate at ")" so we can use parallel deduction here
 * both may result in same tree, but we still need to keep the mechanism here because Type
 * may have form that is not accepted by Expression
 * e.g. (int[])a
 * is a valid expression but "int[]" part has unique interpretation, which is Type
*/
static tree_node* parse_primary(java_parser* parser)
{
    tree_node* node = ast_node_primary();
    java_lexeme_type peek;
    bool accepting = true;
    bool last_separator = false;
    bool allow_all = true;
    bool is_separator;

    while (accepting)
    {
        // here we buffer 2 tokens to speed up during "( Expression/Type" ambiguity resolution
        token_peek(parser, TOKEN_PEEK_2nd);

        peek = peek_token_type(parser, TOKEN_PEEK_1st);
        is_separator = peek == JLT_SYM_DOT || peek == JLT_SYM_METHOD_REFERENCE;

        /**
         * Guards
         *
         * 1. if last is parenthesized expression: allow everything
         * 2. if last is separator (.|::): block separator
         * 3. if last is other: allow separator only
         *
         * condition 1 allows forms of type casting
        */
        if ((last_separator && is_separator) || (!allow_all && !last_separator && !is_separator))
        {
            break;
        }

        last_separator = false;
        allow_all = false;

        switch (peek)
        {
            case JLT_SYM_PARENTHESIS_OPEN:
                // ( Expression )
                allow_all = true;

                // (
                consume_token(parser, NULL);
                peek = peek_token_type(parser, TOKEN_PEEK_1st);

                // optimization: for some cases, trigger will not cause ambiguity
                if (is_lexeme_primitive_type(peek))
                {
                    // Type
                    tree_node_add_child(node, parse_type(parser));
                }
                else if (is_lexeme_literal(peek))
                {
                    // Expression
                    tree_node_add_child(node, parse_expression(parser));
                }
                else
                {
                    tree_node_add_child(node,
                        parse_binary_ambiguity(parser, &parse_expression, &parse_type, JLT_SYM_PARENTHESIS_CLOSE));
                }

                // )
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
                last_separator = true;
                consume_token(parser, NULL);
                break;
            case JLT_SYM_METHOD_REFERENCE:
                // here we keep the token because we need to distinguish it from DOT
                // :: is more rarely used than DOT so we log :: to save some memory
                last_separator = true;
                tree_node_add_child(node, parse_primary_simple(parser));
                break;
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
 * it to Primary and goes deeper in the tree, hence supports precedence
 * Meaning: Step 2 and 3 are not necessary
 *
 * To prune some dumb cases, we have following rules:
 * Two Primary cannot stay consecutively in a valid Expression;
 * Suprisingly, operators can, for example: unary operators
*/
static tree_node* parse_expression(java_parser* parser)
{
    tree_node* node = ast_node_expression();
    java_expression_worker worker;
    java_lexeme_type token_type;
    operator_id op_type;
    size_t num_op_question = 0;
    bool next_is_operator;
    bool allow_primary = true; // allow in 1st iteration

    // get worker ready
    init_expression_worker(&worker);

    while (true)
    {
        token_type = peek_token_type(parser, TOKEN_PEEK_1st);
        op_type = expr_tid2opid(parser->expression, token_type);
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
            case JLT_SYM_QUESTION:
                // only same number of ":" will be dominated
                num_op_question++;
                break;
            case JLT_SYM_COLON:
                /**
                 * AMBIGUITY: colon cannot be considered as operator if it is not
                 * dominated by "?", because it causes ambiguity with "case label"
                 * syntax, which also ends with a colon
                 *
                 * if ":" will become a standalone operator in the future, this
                 * approach will no longer work
                */
                next_is_operator = num_op_question > 0;
                if (next_is_operator)
                {
                    num_op_question--;
                }
                break;
            case JLT_SYM_PLUS:
                // handle ambiguity
                if (!worker.last_push_operand)
                {
                    op_type = OPID_SIGN_POS;
                }
                break;
            case JLT_SYM_MINUS:
                // handle ambiguity
                if (!worker.last_push_operand)
                {
                    op_type = OPID_SIGN_NEG;
                }
                break;
            case JLT_SYM_INCREMENT:
                // handle ambiguity
                if (!worker.last_push_operand)
                {
                    op_type = OPID_PRE_INC;
                }
                break;
            case JLT_SYM_DECREMENT:
                // handle ambiguity
                if (!worker.last_push_operand)
                {
                    op_type = OPID_PRE_DEC;
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
            while (expression_stack_pop_required(parser->expression, &worker, op_type))
            {
                tree_node_add_child(node, expression_stack_parse_top(&worker));
            }

            expression_stack_push(&worker, op_type);
            allow_primary = true;
        }
        else if (allow_primary && parser_trigger_primary(parser, TOKEN_PEEK_1st))
        {
            tree_node_add_child(node, parse_primary(parser));
            expression_stack_push(&worker, OPID_UNDEFINED);

            // 2 primary cannot stay consecutively
            allow_primary = false;
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
