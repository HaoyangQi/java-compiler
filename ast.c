#include "ast.h"

/**
 * AST node generator
*/
static tree_node* ast_node_new()
{
    tree_node* node = (tree_node*)malloc_assert(sizeof(tree_node));

    init_tree_node(node);
    return node;
}

/**
 * AST node generator
 *
 * compilation unit
*/
tree_node* ast_node_compilation_unit()
{
    tree_node* node = ast_node_new();

    tree_node_attach(node, JNT_UNIT, NULL);
    return node;
}

/**
 * AST node generator
 *
 * name unit
*/
tree_node* ast_node_name_unit()
{
    tree_node* node = ast_node_new();
    node_data_name_unit* data =
        (node_data_name_unit*)malloc_assert(sizeof(node_data_name_unit));

    init_token(&data->id);
    tree_node_attach(node, JNT_NAME_UNIT, data);
    return node;
}

/**
 * AST node generator
 *
 * name
*/
tree_node* ast_node_name()
{
    tree_node* node = ast_node_new();

    tree_node_attach(node, JNT_NAME, NULL);
    return node;
}

/**
 * AST node generator
 *
 * class type unit
*/
tree_node* ast_node_class_type_unit()
{
    tree_node* node = ast_node_new();
    node_data_class_type_unit* data =
        (node_data_class_type_unit*)malloc_assert(sizeof(node_data_class_type_unit));

    init_token(&data->id);
    tree_node_attach(node, JNT_CLASS_TYPE_UNIT, data);
    return node;
}

/**
 * AST node generator
 *
 * class type
*/
tree_node* ast_node_class_type()
{
    tree_node* node = ast_node_new();

    tree_node_attach(node, JNT_CLASS_TYPE, NULL);
    return node;
}

/**
 * AST node generator
 *
 * interface type unit
*/
tree_node* ast_node_interface_type_unit()
{
    tree_node* node = ast_node_new();
    node_data_interface_type_unit* data =
        (node_data_interface_type_unit*)malloc_assert(sizeof(node_data_interface_type_unit));

    init_token(&data->id);
    tree_node_attach(node, JNT_INTERFACE_TYPE_UNIT, data);
    return node;
}

/**
 * AST node generator
 *
 * interface type
*/
tree_node* ast_node_interface_type()
{
    tree_node* node = ast_node_new();

    tree_node_attach(node, JNT_INTERFACE_TYPE, NULL);
    return node;
}

/**
 * AST node generator
 *
 * interface type list
*/
tree_node* ast_node_interface_type_list()
{
    tree_node* node = ast_node_new();

    tree_node_attach(node, JNT_INTERFACE_TYPE_LIST, NULL);
    return node;
}

/**
 * AST node generator
 *
 * package declaration
*/
tree_node* ast_node_package_declaration()
{
    tree_node* node = ast_node_new();

    tree_node_attach(node, JNT_PKG_DECL, NULL);
    return node;
}

/**
 * AST node generator
 *
 * import declaration
*/
tree_node* ast_node_import_declaration()
{
    tree_node* node = ast_node_new();
    node_data_import_decl* data =
        (node_data_import_decl*)malloc_assert(sizeof(node_data_import_decl));

    data->on_demand = false;
    tree_node_attach(node, JNT_IMPORT_DECL, data);
    return node;
}

/**
 * AST node generator
 *
 * class/interface top level declaration
*/
tree_node* ast_node_top_level()
{
    tree_node* node = ast_node_new();
    node_data_top_level* data =
        (node_data_top_level*)malloc_assert(sizeof(node_data_top_level));

    data->modifier = 0;
    tree_node_attach(node, JNT_TOP_LEVEL, data);
    return node;
}

/**
 * AST node generator
 *
 * class declaration
*/
tree_node* ast_node_class_declaration()
{
    tree_node* node = ast_node_new();
    node_data_class_declaration* data =
        (node_data_class_declaration*)malloc_assert(sizeof(node_data_class_declaration));

    init_token(&data->id);
    tree_node_attach(node, JNT_CLASS_DECL, data);
    return node;
}

/**
 * AST node generator
 *
 * interface declaration
*/
tree_node* ast_node_interface_declaration()
{
    tree_node* node = ast_node_new();
    node_data_interface_declaration* data =
        (node_data_interface_declaration*)malloc_assert(sizeof(node_data_interface_declaration));

    init_token(&data->id);
    tree_node_attach(node, JNT_INTERFACE_DECL, data);
    return node;
}

/**
 * AST node generator
 *
 * class extends
*/
tree_node* ast_node_class_extends()
{
    tree_node* node = ast_node_new();

    tree_node_attach(node, JNT_CLASS_EXTENDS, NULL);
    return node;
}

/**
 * AST node generator
 *
 * class implements
*/
tree_node* ast_node_class_implements()
{
    tree_node* node = ast_node_new();

    tree_node_attach(node, JNT_CLASS_IMPLEMENTS, NULL);
    return node;
}

/**
 * AST node generator
 *
 * class body
*/
tree_node* ast_node_class_body()
{
    tree_node* node = ast_node_new();

    tree_node_attach(node, JNT_CLASS_BODY, NULL);
    return node;
}

/**
 * AST node generator
 *
 * interface extends
*/
tree_node* ast_node_interface_extends()
{
    tree_node* node = ast_node_new();

    tree_node_attach(node, JNT_INTERFACE_EXTENDS, NULL);
    return node;
}

/**
 * AST node generator
 *
 * interface body
*/
tree_node* ast_node_interface_body()
{
    tree_node* node = ast_node_new();

    tree_node_attach(node, JNT_INTERFACE_BODY, NULL);
    return node;
}

/**
 * AST node generator
 *
 * static initializer
*/
tree_node* ast_node_static_initializer()
{
    tree_node* node = ast_node_new();

    tree_node_attach(node, JNT_STATIC_INIT, NULL);
    return node;
}

/**
 * AST node generator
 *
 * block
*/
tree_node* ast_node_block()
{
    tree_node* node = ast_node_new();

    tree_node_attach(node, JNT_BLOCK, NULL);
    return node;
}

/**
 * AST node generator
 *
 * class body declaration
*/
tree_node* ast_node_class_body_declaration()
{
    tree_node* node = ast_node_new();
    node_data_class_body_declaration* data = (node_data_class_body_declaration*)
        malloc_assert(sizeof(node_data_class_body_declaration));

    data->modifier = 0;
    tree_node_attach(node, JNT_CLASS_BODY_DECL, data);
    return node;
}

/**
 * AST node generator
 *
 * constructor declaration
*/
tree_node* ast_node_constructor_declaration()
{
    tree_node* node = ast_node_new();
    node_data_constructor_declaration* data = (node_data_constructor_declaration*)
        malloc_assert(sizeof(node_data_constructor_declaration));

    init_token(&data->id);
    tree_node_attach(node, JNT_CTOR_DECL, data);
    return node;
}

/**
 * AST node generator
 *
 * type
*/
tree_node* ast_node_type()
{
    tree_node* node = ast_node_new();
    node_data_type* data = (node_data_type*)malloc_assert(sizeof(node_data_type));

    data->primitive = JLT_MAX;
    data->dimension = 0;

    tree_node_attach(node, JNT_TYPE, data);
    return node;
}

/**
 * AST node generator
 *
 * method declaration
*/
tree_node* ast_node_method_declaration()
{
    tree_node* node = ast_node_new();
    node_data_method_declaration* data = (node_data_method_declaration*)
        malloc_assert(sizeof(node_data_method_declaration));

    init_token(&data->id);
    tree_node_attach(node, JNT_METHOD_DECL, data);
    return node;
}

/**
 * AST node generator
 *
 * field declaration
*/
tree_node* ast_node_field_declaration()
{
    tree_node* node = ast_node_new();

    tree_node_attach(node, JNT_FIELD_DECL, NULL);
    return node;
}

/**
 * AST node generator
 *
 * formal parameter list
*/
tree_node* ast_node_formal_parameter_list()
{
    tree_node* node = ast_node_new();

    tree_node_attach(node, JNT_FORMAL_PARAM_LIST, NULL);
    return node;
}

/**
 * AST node generator
 *
 * formal parameter
*/
tree_node* ast_node_formal_parameter()
{
    tree_node* node = ast_node_new();
    node_data_formal_parameter* data = (node_data_formal_parameter*)
        malloc_assert(sizeof(node_data_formal_parameter));

    init_token(&data->id);
    data->dimension = 0;

    tree_node_attach(node, JNT_FORMAL_PARAM, data);
    return node;
}

/**
 * AST node generator
 *
 * throws
*/
tree_node* ast_node_throws()
{
    tree_node* node = ast_node_new();

    tree_node_attach(node, JNT_THROWS, NULL);
    return node;
}

/**
 * AST node generator
 *
 * argument list
*/
tree_node* ast_node_argument_list()
{
    tree_node* node = ast_node_new();

    tree_node_attach(node, JNT_ARGUMENT_LIST, NULL);
    return node;
}

/**
 * AST node generator
 *
 * constructor body
*/
tree_node* ast_node_constructor_body()
{
    tree_node* node = ast_node_new();

    tree_node_attach(node, JNT_CTOR_BODY, NULL);
    return node;
}

/**
 * AST node generator
 *
 * method body
*/
tree_node* ast_node_method_body()
{
    tree_node* node = ast_node_new();

    tree_node_attach(node, JNT_METHOD_BODY, NULL);
    return node;
}

/**
 * AST node generator
 *
 * variable declarator
*/
tree_node* ast_node_variable_declarator()
{
    tree_node* node = ast_node_new();
    node_data_variable_declarator* data = (node_data_variable_declarator*)
        malloc_assert(sizeof(node_data_variable_declarator));

    init_token(&data->id);
    data->dimension = 0;

    tree_node_attach(node, JNT_VAR_DECL, data);
    return node;
}

/**
 * AST node generator
 *
 * array initializer
*/
tree_node* ast_node_array_initializer()
{
    tree_node* node = ast_node_new();

    tree_node_attach(node, JNT_ARRAY_INIT, NULL);
    return node;
}

/**
 * AST node generator
 *
 * primary
*/
tree_node* ast_node_primary()
{
    tree_node* node = ast_node_new();

    tree_node_attach(node, JNT_PRIMARY, NULL);
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
    tree_node* node = ast_node_new();
    node_data_primary_simple* data =
        (node_data_primary_simple*)malloc_assert(sizeof(node_data_primary_simple));

    data->type = JLT_MAX;

    tree_node_attach(node, JNT_PRIMARY_SIMPLE, data);
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
    tree_node* node = ast_node_new();
    node_data_primary_complex* data =
        (node_data_primary_complex*)malloc_assert(sizeof(node_data_primary_complex));

    init_token(&data->token);
    tree_node_attach(node, JNT_PRIMARY_COMPLEX, data);
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
    tree_node* node = ast_node_new();

    tree_node_attach(node, JLT_SYM_METHOD_REFERENCE, NULL);
    return node;
}

/**
 * AST node generator
 *
 * primary creation expression
*/
tree_node* ast_node_primary_creation()
{
    tree_node* node = ast_node_new();

    tree_node_attach(node, JNT_PRIMARY_CREATION, NULL);
    return node;
}

/**
 * AST node generator
 *
 * primary array creation expression
*/
tree_node* ast_node_primary_array_creation()
{
    tree_node* node = ast_node_new();
    node_data_primary_array_creation* data = (node_data_primary_array_creation*)
        malloc_assert(sizeof(node_data_primary_array_creation));

    data->dims_var = 0;
    tree_node_attach(node, JNT_PRIMARY_ARR_CREATION, data);
    return node;
}

/**
 * AST node generator
 *
 * primary class instance creation expression
*/
tree_node* ast_node_primary_class_instance_creation()
{
    tree_node* node = ast_node_new();

    tree_node_attach(node, JNT_PRIMARY_CLS_CREATION, NULL);
    return node;
}

/**
 * AST node generator
 *
 * primary method invocation
*/
tree_node* ast_node_primary_method_invocation()
{
    tree_node* node = ast_node_new();

    tree_node_attach(node, JNT_PRIMARY_METHOD_INVOKE, NULL);
    return node;
}

/**
 * AST node generator
 *
 * primary array access
*/
tree_node* ast_node_primary_array_access()
{
    tree_node* node = ast_node_new();

    tree_node_attach(node, JNT_PRIMARY_ARR_ACCESS, NULL);
    return node;
}

/**
 * AST node generator
 *
 * primary class literal
*/
tree_node* ast_node_primary_class_literal()
{
    tree_node* node = ast_node_new();

    tree_node_attach(node, JNT_PRIMARY_CLS_LITERAL, NULL);
    return node;
}

/**
 * AST node generator
 *
 * expression
*/
tree_node* ast_node_expression()
{
    tree_node* node = ast_node_new();

    tree_node_attach(node, JNT_EXPRESSION, NULL);
    return node;
}

/**
 * AST node generator
 *
 * operator
*/
tree_node* ast_node_operator()
{
    tree_node* node = ast_node_new();
    node_data_operator* data = (node_data_operator*)malloc_assert(sizeof(node_data_operator));

    data->op = 0;
    tree_node_attach(node, JNT_OPERATOR, data);
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
tree_node* ast_node_statement()
{
    tree_node* node = ast_node_new();
    node_data_statement* data = (node_data_statement*)malloc_assert(sizeof(node_data_statement));

    init_token(&data->id);
    tree_node_attach(node, JNT_STATEMENT, data);
    return node;
}

/**
 * AST node generator
 *
 * constructor invocation
*/
tree_node* ast_node_constructor_invocation()
{
    tree_node* node = ast_node_new();
    node_data_constructor_invoke* data =
        (node_data_constructor_invoke*)malloc_assert(sizeof(node_data_constructor_invoke));

    data->is_super = false;
    tree_node_attach(node, JNT_CTOR_INVOCATION, data);
    return node;
}

/**
 * AST node generator
 *
 * switch label
*/
tree_node* ast_node_switch_label()
{
    tree_node* node = ast_node_new();
    node_data_switch_label* data =
        (node_data_switch_label*)malloc_assert(sizeof(node_data_switch_label));

    data->is_default = false;
    tree_node_attach(node, JNT_SWITCH_LABEL, data);
    return node;
}

/**
 * AST node generator
 *
 * for-loop initialization
*/
tree_node* ast_node_for_init()
{
    tree_node* node = ast_node_new();

    tree_node_attach(node, JNT_FOR_INIT, NULL);
    return node;
}

/**
 * AST node generator
 *
 * for-loop update
*/
tree_node* ast_node_for_update()
{
    tree_node* node = ast_node_new();

    tree_node_attach(node, JNT_FOR_UPDATE, NULL);
    return node;
}
