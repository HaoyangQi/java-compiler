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
    JNT_TOP_LEVEL,
    JNT_CLASS_DECL,
    JNT_INTERFACE_DECL,
} java_node_query;

/**
 * Compilation Unit
 *
 * not yet have any info on this level
*/
// typedef struct
// {
// } node_data_compilation_unit;

/**
 * Name
*/
typedef struct
{
    linked_list name;
} node_data_name;

/**
 * Package Declaration
 *
 * not yet have any info on this level
*/
// typedef struct
// {
// } node_data_pkg_decl;

/**
 * Import Declaration
*/
typedef struct
{
    /* on demand import */
    bool on_demand;
} node_data_import_decl;

/**
 * Top Level
*/
typedef struct
{
    /* modifier data */
    lbit_flag modifier;
} node_data_top_level;

/**
 * Class Declaration
*/
typedef struct
{
    /* class name */
    java_token id;
} node_data_class_declaration;

/**
 * Interface Declaration
*/
typedef struct
{
    /* interface name */
    java_token id;
} node_data_interface_declaration;

#endif
