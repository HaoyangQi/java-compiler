#pragma once
#ifndef __COMPILER_TREE_NODE_H__
#define __COMPILER_TREE_NODE_H__

#include "types.h"
#include "token.h"

/**
 * ast node data query
 *
 * the type will help node determine which type
 * the data is binded to the tree node
*/
typedef enum
{
    JNT_UNIT,
    JNT_NAME_UNIT,
    JNT_NAME,
    JNT_CLASS_TYPE_UNIT,
    JNT_CLASS_TYPE,
    JNT_INTERFACE_TYPE_UNIT,
    JNT_INTERFACE_TYPE,
    JNT_INTERFACE_TYPE_LIST,
    JNT_PKG_DECL,
    JNT_IMPORT_DECL,
    JNT_TOP_LEVEL,
    JNT_CLASS_DECL,
    JNT_INTERFACE_DECL,
    JNT_CLASS_EXTENDS,
    JNT_CLASS_IMPLEMENTS,
    JNT_CLASS_BODY,
    JNT_INTERFACE_EXTENDS,
    JNT_INTERFACE_BODY,
} java_node_query;

/**
 * Name Unit
 *
 * unit for Name sequence
 * e.g. ID1.ID2.ID3
*/
typedef struct
{
    java_token id;
} node_data_name_unit;

/**
 * Class Type Unit
*/
typedef struct
{
    java_token id;
} node_data_class_type_unit;

/**
 * Interface Type Unit
*/
typedef struct
{
    java_token id;
} node_data_interface_type_unit;

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
