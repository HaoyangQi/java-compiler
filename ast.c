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
 * name
*/
tree_node* ast_node_name()
{
    tree_node* node = ast_node_new();
    node_data_name* data =
        (node_data_name*)malloc_assert(sizeof(node_data_name));

    init_linked_list(&data->name);
    tree_node_attach(node, JNT_NAME, data);
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
