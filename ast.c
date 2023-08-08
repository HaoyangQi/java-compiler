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
