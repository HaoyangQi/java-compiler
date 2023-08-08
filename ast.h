#pragma once
#ifndef __COMPILER_AST_H__
#define __COMPILER_AST_H__

#include "types.h"
#include "tree.h"
#include "node.h"

tree_node* ast_node_compilation_unit();
tree_node* ast_node_name_unit();
tree_node* ast_node_name();
tree_node* ast_node_class_type_unit();
tree_node* ast_node_class_type();
tree_node* ast_node_interface_type_unit();
tree_node* ast_node_interface_type();
tree_node* ast_node_interface_type_list();
tree_node* ast_node_package_declaration();
tree_node* ast_node_import_declaration();
tree_node* ast_node_top_level();
tree_node* ast_node_class_declaration();
tree_node* ast_node_interface_declaration();
tree_node* ast_node_class_extends();
tree_node* ast_node_class_implements();
tree_node* ast_node_class_body();
tree_node* ast_node_interface_extends();
tree_node* ast_node_interface_body();

#endif
