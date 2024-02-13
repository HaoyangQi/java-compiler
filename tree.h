#pragma once
#ifndef __COMPILER_TREE_H__
#define __COMPILER_TREE_H__

#include "types.h"
#include "langspec.h"
#include "token.h"
#include "node.h"
#include "error.h"

/**
 * Aux Data: ID
 *
 * nodes only require an identification
 *
 * NOTE: control the size of this union carefully
*/
typedef union
{
    java_lexeme_type simple;
    java_token* complex;
} node_data_id;

/**
 * Aux Data
 *
 * NOTE: control the size of this union carefully
*/
typedef union
{
    // nodes only require an identification
    node_data_id id;

    // import-specific
    struct
    {
        bool on_demand;
    } import;

    // top-level (declaration) modifier
    struct
    {
        lbit_flag modifier;
    } top_level_declaration;

    /**
     * declarator form
     *
     * 1. type header
     * 2. formal parameter
     * 3. method header
     * 4. variable declarator
     * 5. array creation
    */
    struct
    {
        node_data_id id;
        /* array dimension */
        size_t dimension;
    } declarator;

    // operator
    struct
    {
        // let's decouple external definitions here
        // and use simple type for enum member

        // operator id, type: operator_id
        int id;
        // IR-use only, type: instruction*
        void* instruction;
    } operator;

    // constructor invocation
    struct
    {
        /* true if calling from super class, this class otherwise */
        bool is_super;
    } constructor_invoke;

    // switch-case label
    struct
    {
        /* true if default label, case label otherwise */
        bool is_default;
    } switch_label;

    // ambiguity data
    struct
    {
        // pointer to JAVA_E_AMBIGUITY_START
        java_error_entry* error;
    } ambiguity;
} tree_node_data;

/**
 * Abstract Syntax Tree Node
 *
 * Left-Child, Right-Sibling (LCRS) Muti-Way Tree
 *
 * LCRS uses binary way to represent multi-way tree
 * child: goes deeper in the tree and points to left-most child
 * sibling: goes along same level and points to next sibling on same level
 *
 * e.g.
 *        A
 *       /
 *      B -> C -> D -> E -> F
 *     /         /         /
 *    G         H->I->J   K->L
 *
 * A's children are: BCDEF, and child points to B
 * B's sibling points to C points to D points E points F
 * B's children are G, and child points to G
 * same idea for D and F
 *
 * TODO: control valid and ambiguous field
*/
typedef struct _tree_node
{
    /* node type */
    java_node_query type;
    /* false if production uniquely determines input */
    bool ambiguous;
    /* aux data */
    tree_node_data* data;
    /* binary way to represent multi-way tree */
    struct _tree_node* first_child;
    struct _tree_node* next_sibling;
    /* aux info for sibling traversal and addition */
    struct _tree_node* last_child;
    struct _tree_node* prev_sibling;
} tree_node;

void init_tree_node(tree_node* node);
void tree_node_mutate(tree_node* node, java_node_query type);
void tree_node_add_child(tree_node* node, tree_node* child);
void tree_node_delete(tree_node* node);

#endif
