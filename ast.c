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
    java_tree_node_name* data =
        (java_tree_node_name*)malloc_assert(sizeof(java_tree_node_name));

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
    java_tree_node_import_decl* data =
        (java_tree_node_import_decl*)malloc_assert(sizeof(java_tree_node_import_decl));

    data->on_demand = false;
    tree_node_attach(node, JNT_IMPORT_DECL, data);
    return node;
}
