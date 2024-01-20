#pragma once
#ifndef __COMPILER_SEMANTICS_H__
#define __COMPILER_SEMANTICS_H__

#include "types.h"
#include "hash-table.h"
#include "token.h"
#include "expression.h"
#include "tree.h"
#include "error.h"

/**
 * Symbol Lookup Hierarchy
 *
 * LCRS Bi-Directional Muti-Way Tree
 *
 * Every node represents a Block
 *
 * key is a copy of name string
 * value is a copy of semantic_variable_descriptor
 *
 * TODO: need semantic_block* reference back to a block
 * if it is a function or a loop
*/
typedef struct _lookup_hierarchy
{
    // name -> variable_descriptor
    hash_table table;

    struct _lookup_hierarchy* parent;
    struct _lookup_hierarchy* first_child;
    struct _lookup_hierarchy* next_sibling;
} lookup_hierarchy;

/**
 * variable descriptor
 *
 * TODO: need to describe type
*/
typedef struct
{
    // use counter, 0 means definition
    size_t version;
} semantic_variable_descriptor;

/**
 * value type
*/
typedef enum
{
    SVT_NAME,
    SVT_LITERAL,
} semantic_value_type;

/**
 * value
*/
typedef struct
{
    semantic_value_type type;

    // value description
    union
    {
        // reference type
        struct
        {
            /**
             * TODO: do we need key here?
            */
            // char* s;
            semantic_variable_descriptor* definition;
            size_t version;
        } name;

        // primitive type
        struct
        {
            java_lexeme_type type;
            java_number_bit_length bits;
            void* data;
        } literal;
    } describe;
} semantic_value;

/**
 * single assignment form
 *
 * v <- aux_1 op aux_2
 *
 * multiple statements are chained in order
 * when they are in same block
 *
 * the simplist form is an assignment expression:
 * a ;
 * it takes one value, and that is it
 *
 * but there is allowed 3 values at most because
 * it is a single assignment form
 *
 * what about "? :"
 * it is considered as a syntatic suger instead of
 * a simple assignment, because it branches statements
 * and diverges the pathway; it needs to be broken
 * down into simpler version in SSA
 *
 * TODO:
*/
typedef struct _semantic_assignment
{
    // at least one value is mandatory
    semantic_value v;
    // another two values are optional
    semantic_value* aux_1;
    semantic_value* aux_2;
    // operator is also optional
    java_operator op;

    struct _semantic_assignment* next;
} semantic_assignment;

typedef enum
{
    SBT_SIMPLE,
    SBT_IF,
    SBT_LOOP,
} semantic_block_branch_type;

/**
 * A block of assignments
 *
 * Connecting together we have SSA
*/
typedef struct _semantic_block
{
    // lookupup table of current scope
    lookup_hierarchy* lookup_from;
    // assignments within the block
    semantic_assignment* assignment;
    // branch type
    semantic_block_branch_type branch_type;

    // branch form
    union
    {
        struct
        {
            struct _semantic_block* next;
        } single;

        struct
        {
            struct _semantic_block* yes;
            struct _semantic_block* no;
        } binary;

        struct
        {
            struct _semantic_block* condition_block;
        } loop;
    } branch_like;
} semantic_block;

/**
 * Top Level of Semantics
 *
 * on top level, each method occupies a tree of blocks
 * additionally, member initializers will occupy a
 * tree;
*/
typedef struct
{
    // symbol lookup tree
    lookup_hierarchy* lookup_root;
    // member initializer block
    semantic_block* member_initialization;
    // method SSAs
    semantic_block** methods;
    // toplevel block count
    size_t num_methods;
} java_semantics;

void init_semantics(java_semantics* se);
void release_semantics(java_semantics* se);
void contextualize(java_semantics* se, tree_node* compilation_unit);

/**
 * TODO: more
*/

#endif
