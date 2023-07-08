#pragma once
#ifndef __COMPILER_TREE_H__
#define __COMPILER_TREE_H__

#include "types.h"
#include "tree-node.h"
#include "vecbuf.h"

/**
 * ast node data query
 *
 * the type will help union determine which type
 * the data is binded to the tree node
*/
typedef enum
{
    /* package declaration */
    JNT_PKG_DECL,
    JNT_IMPORT_DECL,
} java_node_query;

/**
 * ast node data selector
 *
 * to control size of the union, pointers are used
*/
typedef union
{
    java_tree_node_pkg_decl* pck_decl;
    java_tree_node_import_decl* import_decl;
} java_node_selector;

typedef struct _tree_node
{
    /* query provides type so selector can fetch right field */
    java_node_query query : 8;
    /* node type collection for query */
    java_node_selector selector;
    /* tree_node children, not using vec to strongly type it */
    struct _tree_node** children;
    size_t num_children;
} tree_node;

typedef struct _tree
{
    tree_node root;
} tree;

#endif
