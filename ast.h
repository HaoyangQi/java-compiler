#pragma once
#ifndef __COMPILER_AST_H__
#define __COMPILER_AST_H__

#include "types.h"
#include "tree.h"
#include "node.h"

tree_node* ast_node_compilation_unit();
tree_node* ast_node_name();
tree_node* ast_node_package_declaration();

#endif
