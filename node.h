#pragma once
#ifndef __COMPILER_TREE_NODE_H__
#define __COMPILER_TREE_NODE_H__

#include "types.h"
#include "token.h"
#include "ll.h"

/**
 * ast node data query
 *
 * the type will help node determine which type
 * the data is binded to the tree node
*/
typedef enum
{
    JNT_UNIT,
    JNT_NAME,
    JNT_PKG_DECL,
    JNT_IMPORT_DECL,
} java_node_query;

/**
 * Compilation Unit
 *
 * not yet have any info on this level
*/
// typedef struct
// {
// } java_tree_node_compilation_unit;

/**
 * Name
*/
typedef struct
{
    linked_list name;
} java_tree_node_name;

/**
 * Package Declaration
 *
 * not yet have any info on this level
*/
// typedef struct
// {
// } java_tree_node_pkg_decl;

/**
 * Import Declaration
*/
typedef struct
{
    /* on demand import */
    bool on_demand;
} java_tree_node_import_decl;

#endif
