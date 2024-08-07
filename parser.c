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
    java_lexer* lexer,
    hash_table* rw,
    java_expression* expr,
    java_error_logger* logger
)
{
    parser->is_copy = false; // MUST be false from this routine

    // tokens contains garbage data if not used
    // so the counter is important
    parser->num_token_available = 0;

    parser->lexer = lexer;
    parser->reserved_words = rw;
    parser->ast_root = NULL;
    parser->expression = expr;
    parser->logger = logger;
}

/**
 * Copy parser instance
 *
 * a copy of parser instance has shallow copy of all static data,
 * and does not copy AST
 *
 * 1. copy parser instance MUST ALWAYS start from a new AST
 * 2. error logger is a singleton so no copy is allowed
 * 3. lexer needs a copy, while the file map in file buffer does not
*/
void copy_parser(java_parser* from, java_parser* to)
{
    memcpy(to, from, sizeof(java_parser));

    // set copy flag
    to->is_copy = true;

    /**
     * a copy of parser insatnce should never affect current AST
     *
     * AST is complex, and should always manipulate only from
     * the original reference
    */
    to->ast_root = NULL;

    // copy lexer
    to->lexer = copy_lexer(from->lexer);
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
 *
 * 1. lookaheads requires copy
 * 2. lexer needs to be (shallow) copied
 * 3. error logger is a singleton, no copy
 * 4. reserved words and expression data are static, no copy
*/
void mutate_parser(java_parser* parser, java_parser* copy)
{
    // lookaheads: deep copy
    memcpy(parser->tokens, copy->tokens, sizeof(java_token) * 4);
    parser->num_token_available = copy->num_token_available;

    /**
     * Override Lexer
     *
     * file buffer is a bit tricky because there are 2 instances but
     * pointer cannot be directly override
     *
     * luckily... from file buffer "cur" is only thing needed so
     * override can be hancked here
    */
    file_buffer* buf = parser->lexer->buffer;
    buf->cur = copy->lexer->buffer->cur;
    memcpy(parser->lexer, copy->lexer, sizeof(java_lexer));
    parser->lexer->buffer = buf;

    // guard: copy instance should not have AST
    if (!copy->is_copy || copy->ast_root)
    {
        parser_error(parser, JAVA_E_PASER_SWAP_FAILED);
    }
}

/**
 * Release parser instance
 *
 * if deleting a copy of parser instance, the AST data must be NULL
*/
void release_parser(java_parser* parser)
{
    if (parser->is_copy)
    {
        if (parser->ast_root)
        {
            parser_error(parser, JAVA_E_PASER_DELETE_FAILED);
        }

        // delete lexer instance
        release_lexer(parser->lexer);
        free(parser->lexer);
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

    // error logger follows singleton design
    java_error_logger* logger = parser->logger;

    // copy parser instance
    java_parser* parser_1 = (java_parser*)malloc_assert(sizeof(java_parser));
    java_parser* parser_2 = (java_parser*)malloc_assert(sizeof(java_parser));
    copy_parser(parser, parser_1);
    copy_parser(parser, parser_2);

    // parse path 1
    java_error_stack* tmp_cur = logger->current_stream;
    error_logger_ambiguity_begin(logger);
    n1 = (*f1)(parser_1);
    n1_valid = error_logger_if_current_stack_no_error(logger);
    error_logger_ambiguity_end(logger);

    // parse path 2
    error_logger_ambiguity_begin(logger);
    n2 = (*f2)(parser_2);
    n2_valid = error_logger_if_current_stack_no_error(logger);
    error_logger_ambiguity_end(logger);

    // now current top should be the ambiguity entry
    java_error_entry* entry_amb = error_logger_get_current_top(logger);

    // verify terminator and determine final validity status
    n1_valid = n1_valid &&
        (terminator == JLT_MAX || peek_token_type_is(parser_1, TOKEN_PEEK_1st, terminator));
    n2_valid = n2_valid &&
        (terminator == JLT_MAX || peek_token_type_is(parser_2, TOKEN_PEEK_1st, terminator));

    if (n1_valid && n2_valid)
    {
        // keep both: mutate using n1
        node = ast_node_new(JNT_AMBIGUOUS);
        tree_node_add_child(node, n1);
        tree_node_add_child(node, n2);

        // convergence test
        if (parser_1->lexer->buffer->cur != parser_2->lexer->buffer->cur)
        {
            parser_error(parser, JAVA_E_AMBIGUITY_DIVERGE, *(parser_1->lexer->buffer->cur), *(parser_2->lexer->buffer->cur));
        }

        // cache the ambiguity error entry in node
        node->data.ambiguity->error = entry_amb;
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
        error_logger_ambiguity_resolve(logger, entry_amb, 1);
        // assert(error_logger_get_current_top(logger) != entry_amb);
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
        error_logger_ambiguity_resolve(logger, entry_amb, 0);
        // assert(error_logger_get_current_top(logger) != entry_amb);
    }

    // mutate parser accordingly
    if (node->type != JNT_AMBIGUOUS)
    {
        mutate_parser(parser, node == n1 ? parser_1 : parser_2);
    }

    // delete parser instance copy
    release_parser(parser_1);
    release_parser(parser_2);
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
    parser->ast_root = ast_node_new(JNT_UNIT);

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
    tree_node* node = ast_node_new(JNT_NAME_UNIT);

    // ID
    consume_token(parser, node->data.id->complex);

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
    tree_node* node = ast_node_new(JNT_NAME);

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
    tree_node* node = ast_node_new(JNT_CLASS_TYPE_UNIT);

    // ID
    consume_token(parser, node->data.id->complex);

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
    tree_node* node = ast_node_new(JNT_CLASS_TYPE);

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
    tree_node* node = ast_node_new(JNT_INTERFACE_TYPE_UNIT);

    // ID
    consume_token(parser, node->data.id->complex);

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
    tree_node* node = ast_node_new(JNT_INTERFACE_TYPE);

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
    tree_node* node = ast_node_new(JNT_INTERFACE_TYPE_LIST);

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
    tree_node* node = ast_node_new(JNT_PKG_DECL);

    // package
    consume_token(parser, NULL);

    // Name, terminate if incomplete
    if (parser_trigger_name(parser, TOKEN_PEEK_1st))
    {
        tree_node_add_child(node, parse_name(parser));
    }
    else
    {
        parser_error_missing_token(parser, JAVA_E_PKG_DECL_NO_NAME, "name");
        return node;
    }

    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_SEMICOLON))
    {
        consume_token(parser, NULL);
    }
    else
    {
        parser_error_missing_token(parser, JAVA_E_PKG_DECL_NO_SEMICOLON, ";");
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
    tree_node* node = ast_node_new(JNT_IMPORT_DECL);

    // import
    consume_token(parser, NULL);

    // Name, terminate if incomplete
    if (parser_trigger_name(parser, TOKEN_PEEK_1st))
    {
        tree_node_add_child(node, parse_name(parser));
    }
    else
    {
        parser_error_missing_token(parser, JAVA_E_IMPORT_NO_NAME, "name");
        return node;
    }

    // [. *] on-demand sequence
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_DOT) &&
        peek_token_type_is(parser, TOKEN_PEEK_2nd, JLT_SYM_ASTERISK))
    {
        consume_token(parser, NULL);
        consume_token(parser, NULL);
        node->data.import->on_demand = true;
    }

    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_SEMICOLON))
    {
        consume_token(parser, NULL);
    }
    else
    {
        parser_error_missing_token(parser, JAVA_E_IMPORT_NO_SEMICOLON, ";");
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
    tree_node* node = ast_node_new(JNT_TOP_LEVEL);
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
        node->data.top_level->modifier |= ((lbit_flag)1 << type);
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
        parser_error(parser, JAVA_E_TOP_LEVEL);
    }

    return node;
}

/**
 * ClassDeclaration:
 *     class Identifier [ClassExtends] [ClassImplements] ClassBody
*/
static tree_node* parse_class_declaration(java_parser* parser)
{
    tree_node* node = ast_node_new(JNT_CLASS_DECL);

    // class
    consume_token(parser, NULL);

    // ID, skip if incomplete
    if (peek_token_class_is(parser, TOKEN_PEEK_1st, JT_IDENTIFIER))
    {
        consume_token(parser, node->data.id->complex);
    }
    else
    {
        parser_error_missing_token(parser, JAVA_E_CLASS_NO_NAME, "name");
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
        parser_error_missing_token(parser, JAVA_E_CLASS_NO_BODY, "class body");
    }

    return node;
}

/**
 * InterfaceDeclaration:
 *     interface Identifier [InterfaceExtends] InterfaceBody
*/
static tree_node* parse_interface_declaration(java_parser* parser)
{
    tree_node* node = ast_node_new(JNT_INTERFACE_DECL);

    // interface
    consume_token(parser, NULL);

    // ID, terminate if incomplete
    if (peek_token_class_is(parser, TOKEN_PEEK_1st, JT_IDENTIFIER))
    {
        consume_token(parser, node->data.id->complex);
    }
    else
    {
        parser_error_missing_token(parser, JAVA_E_INTERFACE_NO_NAME, "name");
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
        parser_error_missing_token(parser, JAVA_E_INTERFACE_NO_BODY, "class body");
    }

    return node;
}

/**
 * ClassExtends:
 *     extends ClassType
*/
static tree_node* parse_class_extends(java_parser* parser)
{
    tree_node* node = ast_node_new(JNT_CLASS_EXTENDS);

    // extends
    consume_token(parser, NULL);

    // class type
    if (parser_trigger_class_type(parser, TOKEN_PEEK_1st))
    {
        tree_node_add_child(node, parse_class_type(parser));
    }
    else
    {
        parser_error_missing_token(parser, java_E_CLASS_ENTENDS_NO_NAME, "reference type name");
    }

    return node;
}

/**
 * ClassImplements:
 *     implements InterfaceTypeList
*/
static tree_node* parse_class_implements(java_parser* parser)
{
    tree_node* node = ast_node_new(JNT_CLASS_IMPLEMENTS);

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
        parser_error_missing_token(parser, java_E_CLASS_IMPLEMENTS_NO_NAME, "one or more reference type name");
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
    tree_node* node = ast_node_new(JNT_CLASS_BODY);
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
        parser_error_missing_token(parser, JAVA_E_CLASS_BODY_ENCLOSE, "}");
    }

    return node;
}

/**
 * InterfaceExtends:
 *     extends InterfaceTypeList
*/
static tree_node* parse_interface_extends(java_parser* parser)
{
    tree_node* node = ast_node_new(JNT_INTERFACE_EXTENDS);

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
        parser_error_missing_token(parser, java_E_INTERFACE_ENTENDS_NO_NAME, "reference type name");
    }

    return node;
}

/**
 * InterfaceBody:
 *     { {InterfaceMemberDeclaration} }
*/
static tree_node* parse_interface_body(java_parser* parser)
{
    tree_node* node = ast_node_new(JNT_INTERFACE_BODY);
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
        parser_error_missing_token(parser, JAVA_E_INTERFACE_BODY_ENCLOSE, "}");
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
    tree_node* node = ast_node_new(JNT_INTERFACE_BODY_DECL);
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
        node->data.top_level->modifier |= ((lbit_flag)1 << type);
    }

    // Type, still ambiguous
    if (parser_trigger_type(parser, TOKEN_PEEK_1st))
    {
        tree_node_add_child(node, parse_type(parser));
    }
    else
    {
        parser_error_missing_token(parser, JAVA_E_MEMBER_NO_TYPE, "type name");
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
                    parser_error_missing_token(parser, JAVA_E_MEMBER_METHOD_NO_SEMICOLON, ";");
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
                    parser_error_missing_token(parser, JAVA_E_MEMBER_VAR_NO_SEMICOLON, ";");
                }
                break;
            default:
                parser_error(parser, JAVA_E_MEMBER_AMBIGUOUS);
                break;
        }
    }
    else
    {
        parser_error_missing_token(parser, JAVA_E_MEMBER_NO_NAME, "name");
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
    tree_node* node = ast_node_new(JNT_CLASS_BODY_DECL);
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
        node->data.top_level->modifier |= ((lbit_flag)1 << type);
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
        parser_error_missing_token(parser, JAVA_E_MEMBER_NO_TYPE, "type name");
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
                    parser_error_missing_token(parser, JAVA_E_MEMBER_VAR_NO_SEMICOLON, ";");
                }
                break;
            default:
                parser_error(parser, JAVA_E_MEMBER_AMBIGUOUS);
                break;
        }
    }
    else
    {
        parser_error_missing_token(parser, JAVA_E_MEMBER_NO_NAME, "name");
    }

    return node;
}

/**
 * StaticInitializer:
 *     static Block
*/
static tree_node* parse_static_initializer(java_parser* parser)
{
    tree_node* node = ast_node_new(JNT_STATIC_INIT);

    // static
    consume_token(parser, NULL);

    // Block
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_BRACE_OPEN))
    {
        tree_node_add_child(node, parse_block(parser));
    }
    else
    {
        parser_error_missing_token(parser, JAVA_E_STATIC_INITIALIZER_NO_BODY, "block");
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
    tree_node* node = ast_node_new(JNT_BLOCK);
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
        parser_error_missing_token(parser, JAVA_E_BLOCK_ENCLOSE, "}");
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
            node = ast_node_new(JNT_STATEMENT_EMPTY);
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
        parser_error(parser, JAVA_E_STATEMENT_UNRECOGNIZED);

        // by default we return an ill-formed node
        return ast_node_new(JNT_STATEMENT);
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
    tree_node* node = ast_node_new(JNT_STATEMENT_EXPRESSION);

    // Expression
    tree_node_add_child(node, parse_expression(parser));

    // ;
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_SEMICOLON))
    {
        consume_token(parser, NULL);
    }
    else
    {
        parser_error_missing_token(parser, JAVA_E_EXPRESSION_NO_SEMICOLON, ";");
    }

    return node;
}

/**
 * LocalVariableDeclaration:
 *     Type VariableDeclarators
*/
static tree_node* parse_local_variable_declaration(java_parser* parser)
{
    tree_node* node = ast_node_new(JNT_LOCAL_VAR_DECL);

    // Type
    tree_node_add_child(node, parse_type(parser));

    // VariableDeclarators
    if (peek_token_class_is(parser, TOKEN_PEEK_1st, JT_IDENTIFIER))
    {
        tree_node_add_child(node, parse_variable_declarators(parser));
    }
    else
    {
        parser_error_missing_token(parser, JAVA_E_VAR_NO_DECLARATOR, "name");
    }

    return node;
}

/**
 * LocalVariableDeclarationStatement:
 *     LocalVariableDeclaration ;
*/
static tree_node* parse_local_variable_declaration_statement(java_parser* parser)
{
    tree_node* node = ast_node_new(JNT_STATEMENT_VAR_DECL);

    // LocalVariableDeclaration
    tree_node_add_child(node, parse_local_variable_declaration(parser));

    // ;
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_SEMICOLON))
    {
        consume_token(parser, NULL);
    }
    else
    {
        parser_error_missing_token(parser, JAVA_E_LOCAL_VAR_NO_SEMICOLON, ";");
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
    tree_node* node = ast_node_new(JNT_STATEMENT_SWITCH);
    java_lexeme_type peek;

    // switch
    consume_token(parser, NULL);

    // (
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_PARENTHESIS_OPEN))
    {
        consume_token(parser, NULL);
    }
    else
    {
        parser_error_missing_token(parser, JAVA_E_STATEMENT_SWITCH, "(");
        return node;
    }

    // Expression
    if (parser_trigger_expression(parser, TOKEN_PEEK_1st))
    {
        tree_node_add_child(node, parse_expression(parser));
    }
    else
    {
        parser_error_missing_token(parser, JAVA_E_STATEMENT_SWITCH, "expression");
        return node;
    }

    // )
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_PARENTHESIS_CLOSE))
    {
        consume_token(parser, NULL);
    }
    else
    {
        parser_error_missing_token(parser, JAVA_E_STATEMENT_SWITCH, ")");
        return node;
    }

    // {
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_BRACE_OPEN))
    {
        consume_token(parser, NULL);
    }
    else
    {
        parser_error_missing_token(parser, JAVA_E_STATEMENT_SWITCH, "{");
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
        parser_error_missing_token(parser, JAVA_E_STATEMENT_SWITCH, "}");
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
    tree_node* node = ast_node_new(JNT_STATEMENT_DO);

    // do
    consume_token(parser, NULL);

    // Statement
    if (parser_trigger_statement(parser, TOKEN_PEEK_1st))
    {
        tree_node_add_child(node, parse_statement(parser));
    }
    else
    {
        parser_error_missing_token(parser, JAVA_E_STATEMENT_DO, "statement");
        return node;
    }

    // while
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_RWD_WHILE))
    {
        consume_token(parser, NULL);
    }
    else
    {
        parser_error_missing_token(parser, JAVA_E_STATEMENT_DO, "while clause");
        return node;
    }

    // (
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_PARENTHESIS_OPEN))
    {
        consume_token(parser, NULL);
    }
    else
    {
        parser_error_missing_token(parser, JAVA_E_STATEMENT_DO, "(");
        return node;
    }

    // Expression
    if (parser_trigger_expression(parser, TOKEN_PEEK_1st))
    {
        tree_node_add_child(node, parse_expression(parser));
    }
    else
    {
        parser_error_missing_token(parser, JAVA_E_STATEMENT_DO, "expression");
        return node;
    }

    // )
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_PARENTHESIS_CLOSE))
    {
        consume_token(parser, NULL);
    }
    else
    {
        parser_error_missing_token(parser, JAVA_E_STATEMENT_DO, ")");
        return node;
    }

    // ;
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_SEMICOLON))
    {
        consume_token(parser, NULL);
    }
    else
    {
        parser_error_missing_token(parser, JAVA_E_STATEMENT_DO, ";");
    }

    return node;
}

/**
 * BreakStatement:
 *     break [ID] ;
*/
static tree_node* parse_break_statement(java_parser* parser)
{
    tree_node* node = ast_node_new(JNT_STATEMENT_BREAK);

    // break
    consume_token(parser, NULL);

    // [ID]
    if (peek_token_class_is(parser, TOKEN_PEEK_1st, JT_IDENTIFIER))
    {
        consume_token(parser, node->data.id->complex);
    }

    // ;
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_SEMICOLON))
    {
        consume_token(parser, NULL);
    }
    else
    {
        parser_error_missing_token(parser, JAVA_E_STATEMENT_BREAK, ";");
    }

    return node;
}

/**
 * ContinueStatement:
 *     continue [ID] ;
*/
static tree_node* parse_continue_statement(java_parser* parser)
{
    tree_node* node = ast_node_new(JNT_STATEMENT_CONTINUE);

    // continue
    consume_token(parser, NULL);

    // [ID]
    if (peek_token_class_is(parser, TOKEN_PEEK_1st, JT_IDENTIFIER))
    {
        consume_token(parser, node->data.id->complex);
    }

    // ;
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_SEMICOLON))
    {
        consume_token(parser, NULL);
    }
    else
    {
        parser_error_missing_token(parser, JAVA_E_STATEMENT_CONTINUE, ";");
    }

    return node;
}

/**
 * ReturnStatement:
 *     return [Expression] ;
*/
static tree_node* parse_return_statement(java_parser* parser)
{
    tree_node* node = ast_node_new(JNT_STATEMENT_RETURN);

    // return
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
        parser_error_missing_token(parser, JAVA_E_STATEMENT_RETURN, ";");
    }

    return node;
}

/**
 * SynchronizedStatement:
 *     synchronized ( Expression ) Block
*/
static tree_node* parse_synchronized_statement(java_parser* parser)
{
    tree_node* node = ast_node_new(JNT_STATEMENT_THROW);

    // throw
    consume_token(parser, NULL);

    // (
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_PARENTHESIS_OPEN))
    {
        consume_token(parser, NULL);
    }
    else
    {
        parser_error_missing_token(parser, JAVA_E_STATEMENT_SYNCHRONIZED, "(");
        return node;
    }

    // Expression
    if (parser_trigger_expression(parser, TOKEN_PEEK_1st))
    {
        tree_node_add_child(node, parse_expression(parser));
    }
    else
    {
        parser_error_missing_token(parser, JAVA_E_STATEMENT_SYNCHRONIZED, "expression");
        return node;
    }

    // )
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_PARENTHESIS_CLOSE))
    {
        consume_token(parser, NULL);
    }
    else
    {
        parser_error_missing_token(parser, JAVA_E_STATEMENT_SYNCHRONIZED, ")");
        return node;
    }

    // Block
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_BRACE_OPEN))
    {
        tree_node_add_child(node, parse_block(parser));
    }
    else
    {
        parser_error_missing_token(parser, JAVA_E_STATEMENT_SYNCHRONIZED, "block");
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
    tree_node* node = ast_node_new(JNT_STATEMENT_THROW);

    // throw
    consume_token(parser, NULL);

    // Expression
    if (parser_trigger_expression(parser, TOKEN_PEEK_1st))
    {
        tree_node_add_child(node, parse_expression(parser));
    }
    else
    {
        parser_error_missing_token(parser, JAVA_E_STATEMENT_THROW, "expression");
        return node;
    }

    // ;
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_SEMICOLON))
    {
        consume_token(parser, NULL);
    }
    else
    {
        parser_error_missing_token(parser, JAVA_E_STATEMENT_THROW, ";");
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
    tree_node* node = ast_node_new(JNT_STATEMENT_TRY);

    // try
    consume_token(parser, NULL);

    // Block
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_BRACE_OPEN))
    {
        tree_node_add_child(node, parse_block(parser));
    }
    else
    {
        parser_error_missing_token(parser, JAVA_E_STATEMENT_TRY, "block");
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
        parser_error_missing_token(parser, JAVA_E_STATEMENT_TRY, "catch and/or finally clause");
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
    tree_node* node = ast_node_new(JNT_STATEMENT_IF);

    // if
    consume_token(parser, NULL);

    // (
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_PARENTHESIS_OPEN))
    {
        consume_token(parser, NULL);
    }
    else
    {
        parser_error_missing_token(parser, JAVA_E_STATEMENT_IF, "(");
        return node;
    }

    // Expression
    if (parser_trigger_expression(parser, TOKEN_PEEK_1st))
    {
        tree_node_add_child(node, parse_expression(parser));
    }
    else
    {
        parser_error_missing_token(parser, JAVA_E_STATEMENT_IF, "expression");
        return node;
    }

    // )
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_PARENTHESIS_CLOSE))
    {
        consume_token(parser, NULL);
    }
    else
    {
        parser_error_missing_token(parser, JAVA_E_STATEMENT_IF, ")");
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
        parser_error_missing_token(parser, JAVA_E_STATEMENT_IF, "statement");
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
            parser_error_missing_token(parser, JAVA_E_STATEMENT_ELSE, "statement");
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
    tree_node* node = ast_node_new(JNT_STATEMENT_WHILE);

    // while
    consume_token(parser, NULL);

    // (
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_PARENTHESIS_OPEN))
    {
        consume_token(parser, NULL);
    }
    else
    {
        parser_error_missing_token(parser, JAVA_E_STATEMENT_WHILE, "(");
        return node;
    }

    // Expression
    if (parser_trigger_expression(parser, TOKEN_PEEK_1st))
    {
        tree_node_add_child(node, parse_expression(parser));
    }
    else
    {
        parser_error_missing_token(parser, JAVA_E_STATEMENT_WHILE, "expression");
        return node;
    }

    // )
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_PARENTHESIS_CLOSE))
    {
        consume_token(parser, NULL);
    }
    else
    {
        parser_error_missing_token(parser, JAVA_E_STATEMENT_WHILE, ")");
        return node;
    }

    // Statement
    if (parser_trigger_statement(parser, TOKEN_PEEK_1st))
    {
        tree_node_add_child(node, parse_statement(parser));
    }
    else
    {
        parser_error_missing_token(parser, JAVA_E_STATEMENT_WHILE, "statement");
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
    tree_node* node = ast_node_new(JNT_STATEMENT_FOR);

    // for
    consume_token(parser, NULL);

    // (
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_PARENTHESIS_OPEN))
    {
        consume_token(parser, NULL);
    }
    else
    {
        parser_error_missing_token(parser, JAVA_E_STATEMENT_FOR, "(");
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
        parser_error(parser, JAVA_E_STATEMENT_FOR_INITIALIZER_NO_SEMICOLON);
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
        parser_error(parser, JAVA_E_STATEMENT_FOR_CONDITION_NO_SEMICOLON);
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
        parser_error_missing_token(parser, JAVA_E_STATEMENT_FOR, ")");
        return node;
    }

    // Statement
    if (parser_trigger_statement(parser, TOKEN_PEEK_1st))
    {
        tree_node_add_child(node, parse_statement(parser));
    }
    else
    {
        parser_error_missing_token(parser, JAVA_E_STATEMENT_FOR, "statement");
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
    tree_node* node = ast_node_new(JNT_STATEMENT_LABEL);

    // ID :
    consume_token(parser, node->data.id->complex);
    consume_token(parser, NULL);

    // Statement
    if (parser_trigger_statement(parser, TOKEN_PEEK_1st))
    {
        tree_node_add_child(node, parse_statement(parser));
    }
    else
    {
        parser_error_missing_token(parser, JAVA_E_STATEMENT_LABEL, "statement");
    }

    return node;
}

/**
 * CatchClause:
 *     catch ( FormalParameter ) Block
*/
static tree_node* parse_catch_statement(java_parser* parser)
{
    tree_node* node = ast_node_new(JNT_STATEMENT_CATCH);

    // catch
    consume_token(parser, NULL);

    // (
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_PARENTHESIS_OPEN))
    {
        consume_token(parser, NULL);
    }
    else
    {
        parser_error_missing_token(parser, JAVA_E_STATEMENT_CATCH, "(");
        return node;
    }

    // FormalParameter
    if (parser_trigger_type(parser, TOKEN_PEEK_1st))
    {
        tree_node_add_child(node, parse_formal_parameter(parser));
    }
    else
    {
        parser_error_missing_token(parser, JAVA_E_STATEMENT_CATCH, "expression");
        return node;
    }

    // )
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_PARENTHESIS_CLOSE))
    {
        consume_token(parser, NULL);
    }
    else
    {
        parser_error_missing_token(parser, JAVA_E_STATEMENT_CATCH, ")");
        return node;
    }

    // Block
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_BRACE_OPEN))
    {
        tree_node_add_child(node, parse_block(parser));
    }
    else
    {
        parser_error_missing_token(parser, JAVA_E_STATEMENT_CATCH, "block");
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
    tree_node* node = ast_node_new(JNT_STATEMENT_FINALLY);

    // finally
    consume_token(parser, NULL);

    // Block
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_BRACE_OPEN))
    {
        tree_node_add_child(node, parse_block(parser));
    }
    else
    {
        parser_error_missing_token(parser, JAVA_E_STATEMENT_FINALLY, "block");
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
    tree_node* node = ast_node_new(JNT_SWITCH_LABEL);

    // case/default
    node->data.switch_label->is_default = peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_RWD_DEFAULT);
    consume_token(parser, NULL);

    // case label requires an Expression
    if (!node->data.switch_label->is_default)
    {
        // Expression
        if (parser_trigger_expression(parser, TOKEN_PEEK_1st))
        {
            tree_node_add_child(node, parse_expression(parser));
        }
        else
        {
            parser_error_missing_token(parser, JAVA_E_STATEMENT_SWITCH_LABEL, "expression");
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
        parser_error_missing_token(parser, JAVA_E_STATEMENT_SWITCH_LABEL, ":");
    }

    return node;
}

/**
 * StatementExpressionList:
 *     Expression {, Expression}
*/
static tree_node* parse_expression_list(java_parser* parser)
{
    tree_node* node = ast_node_new(JNT_EXPRESSION_LIST);

    // Expression
    tree_node_add_child(node, parse_expression(parser));

    // {, Expression}
    while (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_COMMA))
    {
        // ,
        consume_token(parser, NULL);

        // Expression
        if (parser_trigger_expression(parser, TOKEN_PEEK_1st))
        {
            tree_node_add_child(node, parse_expression(parser));
        }
        else
        {
            parser_error(parser, JAVA_E_EXPRESSION_LIST_INCOMPLETE);
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
    tree_node* node = ast_node_new(JNT_FOR_INIT);

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
    tree_node* node = ast_node_new(JNT_FOR_UPDATE);

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
    tree_node* node = ast_node_new(JNT_CTOR_DECL);

    // ID (
    consume_token(parser, node->data.declarator->id.complex);
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
        parser_error_missing_token(parser, JAVA_E_CONSTRUCTOR, ")");
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
        parser_error_missing_token(parser, JAVA_E_CONSTRUCTOR, "block");
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
    tree_node* node = ast_node_new(JNT_TYPE);

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
        node->data.declarator->id.simple = peek_token_type(parser, TOKEN_PEEK_1st);
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
            parser_error(parser, JAVA_E_TYPE_NO_ARR_ENCLOSE);
            break;
        }

        // track dimensions
        node->data.declarator->dimension++;
    }

    // array dimension makes a Type uniquely produced
    node->ambiguous = node->ambiguous && node->data.declarator->dimension == 0;

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
    tree_node* node = ast_node_new(JNT_METHOD_HEADER);

    // ID (
    consume_token(parser, node->data.declarator->id.complex);
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
        parser_error_missing_token(parser, JAVA_E_METHOD_DECLARATION, ")");
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
            parser_error_missing_token(parser, JAVA_E_METHOD_DECLARATION, "]");
            break;
        }

        // track dimensions
        node->data.declarator->dimension++;
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
    tree_node* node = ast_node_new(JNT_METHOD_DECL);

    // MethodHeader
    tree_node_add_child(node, parse_method_header(parser));

    // MethodBody
    if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_BRACE_OPEN))
    {
        tree_node_add_child(node, parse_method_body(parser));
    }
    else
    {
        parser_error_missing_token(parser, JAVA_E_METHOD_DECLARATION, "block");
    }

    return node;
}

/**
 * VariableDeclarators:
 *     VariableDeclarator {, VariableDeclarator}
*/
static tree_node* parse_variable_declarators(java_parser* parser)
{
    tree_node* node = ast_node_new(JNT_VAR_DECLARATORS);

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
    tree_node* node = ast_node_new(JNT_FORMAL_PARAM_LIST);

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
    tree_node* node = ast_node_new(JNT_FORMAL_PARAM);

    // Type
    tree_node_add_child(node, parse_type(parser));

    // VariableDeclaratorId => ID {[ ]}
    if (peek_token_class_is(parser, TOKEN_PEEK_1st, JT_IDENTIFIER))
    {
        consume_token(parser, node->data.declarator->id.complex);

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
                parser_error_missing_token(parser, JAVA_E_FORMAL_PARAMETER, "]");
                break;
            }

            // track dimensions
            node->data.declarator->dimension++;
        }
    }
    else
    {
        parser_error_missing_token(parser, JAVA_E_FORMAL_PARAMETER, "name");
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
    tree_node* node = ast_node_new(JNT_THROWS);

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
        parser_error_missing_token(parser, JAVA_E_THROWS_NO_TYPE, "one or more reference type name");
    }

    return node;
}

/**
 * ArgumentList:
 *     Expression {, Expression}
*/
static tree_node* parse_argument_list(java_parser* parser)
{
    tree_node* node = ast_node_new(JNT_ARGUMENT_LIST);

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
            parser_error_missing_token(parser, JAVA_E_NO_ARGUMENT, "expression");
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
    tree_node* node = ast_node_new(JNT_CTOR_BODY);

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
        parser_error_missing_token(parser, JAVA_E_CONSTRUCTOR, "}");
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
    tree_node* node = ast_node_new(JNT_CTOR_INVOCATION);

    // this/super (
    node->data.constructor_invoke->is_super = peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_RWD_SUPER);
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
        parser_error_missing_token(parser, JAVA_E_CONSTRUCTOR_INVOKE, ")");
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
    tree_node* node = ast_node_new(JNT_METHOD_BODY);

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
            parser_error_missing_token(parser, JAVA_E_METHOD_DECLARATION, "block/;");
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
    tree_node* node = ast_node_new(JNT_VAR_DECL);

    // VariableDeclaratorId => ID {[ ]}
    consume_token(parser, node->data.declarator->id.complex);

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
        node->data.declarator->dimension++;
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
            parser_error_missing_token(parser, JAVA_E_VAR_NO_INITIALIZER, "variable initializer");
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
    tree_node* node = ast_node_new(JNT_ARRAY_INIT);

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
        parser_error_missing_token(parser, JAVA_E_ARRAY_INITIALIZER, "}");
    }

    return node;
}

/**
 * PrimarySimple: primary that contains only token type info
*/
static tree_node* parse_primary_simple(java_parser* parser)
{
    tree_node* node = ast_node_new(JNT_PRIMARY_SIMPLE);

    node->data.id->simple = peek_token_type(parser, TOKEN_PEEK_1st);
    consume_token(parser, NULL);

    return node;
}

/**
 * PrimaryComplex: primary that contains needs token data
*/
static tree_node* parse_primary_complex(java_parser* parser)
{
    tree_node* node = ast_node_new(JNT_PRIMARY_COMPLEX);

    // ID
    consume_token(parser, node->data.id->complex);

    return node;
}

/**
 * PrimaryCreationExpression:
 *     new Type PrimaryArrayCreation
 *     new Type PrimaryClassInstanceCreation
*/
static tree_node* parse_primary_creation(java_parser* parser)
{
    tree_node* node = ast_node_new(JNT_PRIMARY_CREATION);

    // new
    consume_token(parser, NULL);

    // Type
    if (parser_trigger_type(parser, TOKEN_PEEK_1st))
    {
        tree_node_add_child(node, parse_type(parser));
    }
    else
    {
        parser_error_missing_token(parser, JAVA_E_OBJECT_CREATION, "type");
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
            parser_error_missing_token(parser, JAVA_E_OBJECT_CREATION, "array/instance creation");
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
    tree_node* node = ast_node_new(JNT_PRIMARY_ARR_CREATION);
    bool first_variadic = true;
    bool accepting_variadic;

    // [
    consume_token(parser, NULL);

    // first chunk: determine if it starts with variadic dimension
    if (parser_trigger_expression(parser, TOKEN_PEEK_1st))
    {
        first_variadic = false;

        // Expression
        tree_node_add_child(node, parse_expression(parser));
    }

    // first chunk enclosure
    if (peek_token_type(parser, TOKEN_PEEK_1st) == JLT_SYM_BRACKET_CLOSE)
    {
        // ]
        consume_token(parser, NULL);
        node->data.declarator->dimension++;
    }
    else
    {
        parser_error_missing_token(parser, JAVA_E_ARRAY_CREATION, "]");
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
            parser_error_missing_token(parser, JAVA_E_ARRAY_CREATION, "]");
            break;
        }

        node->data.declarator->dimension++;
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
            parser_error_missing_token(parser, JAVA_E_ARRAY_CREATION, "array initializer");
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
    tree_node* node = ast_node_new(JNT_PRIMARY_CLS_CREATION);

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
        parser_error_missing_token(parser, JAVA_E_INSTANCE_CREATION, ")");
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
    tree_node* node = ast_node_new(JNT_PRIMARY_METHOD_INVOKE);

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
        parser_error_missing_token(parser, JAVA_E_METHOD_INVOKE, ")");
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
    tree_node* node = ast_node_new(JNT_PRIMARY_ARR_ACCESS);

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
            parser_error_missing_token(parser, JAVA_E_ARRAY_ACCESS, "index expression");
            break;
        }

        // ]
        if (peek_token_type(parser, TOKEN_PEEK_1st) == JLT_SYM_BRACKET_CLOSE)
        {
            consume_token(parser, NULL);
        }
        else
        {
            parser_error_missing_token(parser, JAVA_E_ARRAY_ACCESS, "]");
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
    tree_node* node = ast_node_new(JNT_PRIMARY_CLS_LITERAL);

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
    tree_node* node = ast_node_new(JNT_PRIMARY);
    tree_node* amb;
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
                    // Expression: collapse the JNT_PRIMARY layer as it is unecessary
                    tree_node_delete(node);
                    node = parse_expression(parser);
                }
                else
                {
                    amb = parse_binary_ambiguity(parser, &parse_expression, &parse_type, JLT_SYM_PARENTHESIS_CLOSE);

                    if (amb->type = JNT_EXPRESSION)
                    {
                        // collapse the JNT_PRIMARY layer as it is unecessary
                        tree_node_delete(node);
                        node = amb;
                    }
                    else
                    {
                        tree_node_add_child(node, amb);
                    }
                }

                // )
                if (peek_token_type_is(parser, TOKEN_PEEK_1st, JLT_SYM_PARENTHESIS_CLOSE))
                {
                    consume_token(parser, NULL);
                }
                else
                {
                    parser_error(parser, JAVA_E_EXPRESSION_PARENTHESIS);
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
    tree_node* node;
    java_expression_worker worker;
    java_lexeme_type token_type;
    operator_id op_type;
    size_t num_op_question = 0;
    bool next_is_operator;
    bool allow_primary = true; // allow in 1st iteration

    // get worker ready
    init_expression_worker(&worker, parser->expression);

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
            consume_token(parser, NULL); // consume the token
            expression_worker_push(&worker, expr_opid2node(op_type));
            allow_primary = true;
        }
        else if (allow_primary && parser_trigger_primary(parser, TOKEN_PEEK_1st))
        {
            expression_worker_push(&worker, parse_primary(parser));
            allow_primary = false; // 2 primary cannot stay consecutively
        }
        else
        {
            // terminator
            break;
        }

        // register error
        parser_error(parser, worker.last_error);
    }

    // collapse everything, log error is applicable, and export expression tree
    expression_worker_complete(&worker);
    parser_error(parser, worker.last_error);
    node = expression_worker_export(&worker);

    // cleanup
    release_expression_worker(&worker);

    return node;
}
