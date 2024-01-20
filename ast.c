#include "ast.h"

/**
 * AST node generator
*/
static tree_node* ast_node_new(java_node_query type)
{
    tree_node* node = (tree_node*)malloc_assert(sizeof(tree_node));

    init_tree_node(node);
    node->type = type;

    return node;
}

/**
 * AST node data generator
*/
static tree_node_data* ast_node_data_new()
{
    tree_node_data* d = (tree_node_data*)malloc_assert(sizeof(tree_node_data));
    memset(d, 0, sizeof(tree_node_data));
    return d;
}

/**
 * AST node data token allocator
*/
static java_token* ast_node_data_new_token()
{
    java_token* t = (java_token*)malloc_assert(sizeof(java_token));
    init_token(t);
    return t;
}

/**
 * AST node generator
 *
 * compilation unit
*/
tree_node* ast_node_compilation_unit()
{
    tree_node* node = ast_node_new(JNT_UNIT);

    return node;
}

/**
 * AST node generator
 *
 * name unit
*/
tree_node* ast_node_name_unit()
{
    tree_node* node = ast_node_new(JNT_NAME_UNIT);

    node->data = ast_node_data_new();
    node->data->id.complex = ast_node_data_new_token();

    return node;
}

/**
 * AST node generator
 *
 * name
*/
tree_node* ast_node_name()
{
    tree_node* node = ast_node_new(JNT_NAME);

    return node;
}

/**
 * AST node generator
 *
 * class type unit
*/
tree_node* ast_node_class_type_unit()
{
    tree_node* node = ast_node_new(JNT_CLASS_TYPE_UNIT);

    node->data = ast_node_data_new();
    node->data->id.complex = ast_node_data_new_token();

    return node;
}

/**
 * AST node generator
 *
 * class type
*/
tree_node* ast_node_class_type()
{
    tree_node* node = ast_node_new(JNT_CLASS_TYPE);

    return node;
}

/**
 * AST node generator
 *
 * interface type unit
*/
tree_node* ast_node_interface_type_unit()
{
    tree_node* node = ast_node_new(JNT_INTERFACE_TYPE_UNIT);

    node->data = ast_node_data_new();
    node->data->id.complex = ast_node_data_new_token();

    return node;
}

/**
 * AST node generator
 *
 * interface type
*/
tree_node* ast_node_interface_type()
{
    tree_node* node = ast_node_new(JNT_INTERFACE_TYPE);

    return node;
}

/**
 * AST node generator
 *
 * interface type list
*/
tree_node* ast_node_interface_type_list()
{
    tree_node* node = ast_node_new(JNT_INTERFACE_TYPE_LIST);

    return node;
}

/**
 * AST node generator
 *
 * package declaration
*/
tree_node* ast_node_package_declaration()
{
    tree_node* node = ast_node_new(JNT_PKG_DECL);

    return node;
}

/**
 * AST node generator
 *
 * import declaration
*/
tree_node* ast_node_import_declaration()
{
    tree_node* node = ast_node_new(JNT_IMPORT_DECL);

    node->data = ast_node_data_new();
    node->data->import.on_demand = false;

    return node;
}

/**
 * AST node generator
 *
 * class/interface top level declaration
*/
tree_node* ast_node_top_level()
{
    tree_node* node = ast_node_new(JNT_TOP_LEVEL);

    node->data = ast_node_data_new();
    node->data->top_level_declaration.modifier = 0;

    return node;
}

/**
 * AST node generator
 *
 * class declaration
*/
tree_node* ast_node_class_declaration()
{
    tree_node* node = ast_node_new(JNT_CLASS_DECL);

    node->data = ast_node_data_new();
    node->data->id.complex = ast_node_data_new_token();

    return node;
}

/**
 * AST node generator
 *
 * interface declaration
*/
tree_node* ast_node_interface_declaration()
{
    tree_node* node = ast_node_new(JNT_INTERFACE_DECL);

    node->data = ast_node_data_new();
    node->data->id.complex = ast_node_data_new_token();

    return node;
}

/**
 * AST node generator
 *
 * class extends
*/
tree_node* ast_node_class_extends()
{
    tree_node* node = ast_node_new(JNT_CLASS_EXTENDS);

    return node;
}

/**
 * AST node generator
 *
 * class implements
*/
tree_node* ast_node_class_implements()
{
    tree_node* node = ast_node_new(JNT_CLASS_IMPLEMENTS);

    return node;
}

/**
 * AST node generator
 *
 * class body
*/
tree_node* ast_node_class_body()
{
    tree_node* node = ast_node_new(JNT_CLASS_BODY);

    return node;
}

/**
 * AST node generator
 *
 * interface extends
*/
tree_node* ast_node_interface_extends()
{
    tree_node* node = ast_node_new(JNT_INTERFACE_EXTENDS);

    return node;
}

/**
 * AST node generator
 *
 * interface body
*/
tree_node* ast_node_interface_body()
{
    tree_node* node = ast_node_new(JNT_INTERFACE_BODY);

    return node;
}

/**
 * AST node generator
 *
 * static initializer
*/
tree_node* ast_node_static_initializer()
{
    tree_node* node = ast_node_new(JNT_STATIC_INIT);

    return node;
}

/**
 * AST node generator
 *
 * block
*/
tree_node* ast_node_block()
{
    tree_node* node = ast_node_new(JNT_BLOCK);

    return node;
}

/**
 * AST node generator
 *
 * interface body declaration
*/
tree_node* ast_node_interface_body_declaration()
{
    tree_node* node = ast_node_new(JNT_INTERFACE_BODY_DECL);

    node->data = ast_node_data_new();
    node->data->top_level_declaration.modifier = 0;

    return node;
}

/**
 * AST node generator
 *
 * class body declaration
*/
tree_node* ast_node_class_body_declaration()
{
    tree_node* node = ast_node_new(JNT_CLASS_BODY_DECL);

    node->data = ast_node_data_new();
    node->data->top_level_declaration.modifier = 0;

    return node;
}

/**
 * AST node generator
 *
 * constructor declaration
*/
tree_node* ast_node_constructor_declaration()
{
    tree_node* node = ast_node_new(JNT_CTOR_DECL);

    node->data = ast_node_data_new();
    node->data->id.complex = ast_node_data_new_token();

    return node;
}

/**
 * AST node generator
 *
 * type
*/
tree_node* ast_node_type()
{
    tree_node* node = ast_node_new(JNT_TYPE);

    node->data = ast_node_data_new();
    node->data->declarator.id.simple = JLT_MAX;
    node->data->declarator.dimension = 0;

    return node;
}

/**
 * AST node generator
 *
 * method header
*/
tree_node* ast_node_method_header()
{
    tree_node* node = ast_node_new(JNT_METHOD_HEADER);

    node->data = ast_node_data_new();
    node->data->declarator.id.complex = ast_node_data_new_token();
    node->data->declarator.dimension = 0;

    return node;
}

/**
 * AST node generator
 *
 * method declaration
*/
tree_node* ast_node_method_declaration()
{
    tree_node* node = ast_node_new(JNT_METHOD_DECL);

    return node;
}

/**
 * AST node generator
 *
 * variable declarators
*/
tree_node* ast_node_variable_declarators()
{
    tree_node* node = ast_node_new(JNT_VAR_DECLARATORS);

    return node;
}

/**
 * AST node generator
 *
 * formal parameter list
*/
tree_node* ast_node_formal_parameter_list()
{
    tree_node* node = ast_node_new(JNT_FORMAL_PARAM_LIST);

    return node;
}

/**
 * AST node generator
 *
 * formal parameter
*/
tree_node* ast_node_formal_parameter()
{
    tree_node* node = ast_node_new(JNT_FORMAL_PARAM);

    node->data = ast_node_data_new();
    node->data->declarator.id.complex = ast_node_data_new_token();
    node->data->declarator.dimension = 0;

    return node;
}

/**
 * AST node generator
 *
 * throws
*/
tree_node* ast_node_throws()
{
    tree_node* node = ast_node_new(JNT_THROWS);

    return node;
}

/**
 * AST node generator
 *
 * argument list
*/
tree_node* ast_node_argument_list()
{
    tree_node* node = ast_node_new(JNT_ARGUMENT_LIST);

    return node;
}

/**
 * AST node generator
 *
 * constructor body
*/
tree_node* ast_node_constructor_body()
{
    tree_node* node = ast_node_new(JNT_CTOR_BODY);

    return node;
}

/**
 * AST node generator
 *
 * method body
*/
tree_node* ast_node_method_body()
{
    tree_node* node = ast_node_new(JNT_METHOD_BODY);

    return node;
}

/**
 * AST node generator
 *
 * variable declarator
*/
tree_node* ast_node_variable_declarator()
{
    tree_node* node = ast_node_new(JNT_VAR_DECL);

    node->data = ast_node_data_new();
    node->data->declarator.id.complex = ast_node_data_new_token();
    node->data->declarator.dimension = 0;

    return node;
}

/**
 * AST node generator
 *
 * array initializer
*/
tree_node* ast_node_array_initializer()
{
    tree_node* node = ast_node_new(JNT_ARRAY_INIT);

    return node;
}

/**
 * AST node generator
 *
 * primary
*/
tree_node* ast_node_primary()
{
    tree_node* node = ast_node_new(JNT_PRIMARY);

    return node;
}

/**
 * AST node generator
 *
 * primary part: simple
 *
 * it is not necessarily a primary trigger, exception is
 * method reference symbol (::), it has to occupy a node
 * to decorate what is behind
*/
tree_node* ast_node_primary_simple()
{
    tree_node* node = ast_node_new(JNT_PRIMARY_SIMPLE);

    node->data = ast_node_data_new();
    node->data->id.simple = JLT_MAX;

    return node;
}

/**
 * AST node generator
 *
 * primary part: complex
 *
 * complimentary part type of simple primary
*/
tree_node* ast_node_primary_complex()
{
    tree_node* node = ast_node_new(JNT_PRIMARY_COMPLEX);

    node->data = ast_node_data_new();
    node->data->id.complex = ast_node_data_new_token();

    return node;
}

/**
 * AST node generator
 *
 * primary part indicator: method reference (::)
 *
 * indicates that next primary part os method reference
*/
tree_node* ast_node_method_reference()
{
    tree_node* node = ast_node_new(JLT_SYM_METHOD_REFERENCE);

    return node;
}

/**
 * AST node generator
 *
 * primary creation expression
*/
tree_node* ast_node_primary_creation()
{
    tree_node* node = ast_node_new(JNT_PRIMARY_CREATION);

    return node;
}

/**
 * AST node generator
 *
 * primary array creation expression
*/
tree_node* ast_node_primary_array_creation()
{
    tree_node* node = ast_node_new(JNT_PRIMARY_ARR_CREATION);

    node->data = ast_node_data_new();
    node->data->declarator.id.simple = JLT_MAX; // unused
    node->data->declarator.dimension = 0;

    return node;
}

/**
 * AST node generator
 *
 * primary class instance creation expression
*/
tree_node* ast_node_primary_class_instance_creation()
{
    tree_node* node = ast_node_new(JNT_PRIMARY_CLS_CREATION);

    return node;
}

/**
 * AST node generator
 *
 * primary method invocation
*/
tree_node* ast_node_primary_method_invocation()
{
    tree_node* node = ast_node_new(JNT_PRIMARY_METHOD_INVOKE);

    return node;
}

/**
 * AST node generator
 *
 * primary array access
*/
tree_node* ast_node_primary_array_access()
{
    tree_node* node = ast_node_new(JNT_PRIMARY_ARR_ACCESS);

    return node;
}

/**
 * AST node generator
 *
 * primary class literal
*/
tree_node* ast_node_primary_class_literal()
{
    tree_node* node = ast_node_new(JNT_PRIMARY_CLS_LITERAL);

    return node;
}

/**
 * AST node generator
 *
 * expression
*/
tree_node* ast_node_expression()
{
    tree_node* node = ast_node_new(JNT_EXPRESSION);

    return node;
}

/**
 * AST node generator
 *
 * operator
*/
tree_node* ast_node_operator()
{
    tree_node* node = ast_node_new(JNT_OPERATOR);

    node->data = ast_node_data_new();
    node->data->operator.id = 0;

    return node;
}

/**
 * AST node generator
 *
 * statement
 *
 * JNT_STATEMENT is an abstract type, which does not represent any statement
 * if it exists in AST, it means this node is invalid
 * valid type names have form: JNT_STATEMENT_*
*/
tree_node* ast_node_statement(bool need_data)
{
    tree_node* node = ast_node_new(JNT_STATEMENT);

    if (need_data)
    {
        node->data = ast_node_data_new();
        node->data->id.complex = ast_node_data_new_token();
    }

    return node;
}

/**
 * AST node generator
 *
 * constructor invocation
*/
tree_node* ast_node_constructor_invocation()
{
    tree_node* node = ast_node_new(JNT_CTOR_INVOCATION);

    node->data = ast_node_data_new();
    node->data->constructor_invoke.is_super = false;

    return node;
}

/**
 * AST node generator
 *
 * switch label
*/
tree_node* ast_node_switch_label()
{
    tree_node* node = ast_node_new(JNT_SWITCH_LABEL);

    node->data = ast_node_data_new();
    node->data->switch_label.is_default = false;

    return node;
}

/**
 * AST node generator
 *
 * for-loop initialization
*/
tree_node* ast_node_for_init()
{
    tree_node* node = ast_node_new(JNT_FOR_INIT);

    return node;
}

/**
 * AST node generator
 *
 * for-loop update
*/
tree_node* ast_node_for_update()
{
    tree_node* node = ast_node_new(JNT_FOR_UPDATE);

    return node;
}

/**
 * AST node generator
 *
 * ambiguous node
 *
 * each child represents a possible interpretation
*/
tree_node* ast_node_ambiguous()
{
    tree_node* node = ast_node_new(JNT_AMBIGUOUS);

    return node;
}

/**
 * AST node generator
 *
 * expression list
*/
tree_node* ast_node_expression_list()
{
    tree_node* node = ast_node_new(JNT_EXPRESSION_LIST);

    return node;
}
