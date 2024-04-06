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
*/
typedef union
{
    java_lexeme_type simple;
    java_token* complex;
} node_data_id;

/**
 * Aux Data: Import
 *
*/
typedef struct
{
    bool on_demand;
} node_data_import;

/**
 * Aux Data: Top Level Declaration
 *
*/
typedef struct
{
    lbit_flag modifier;
} node_data_top_level;

/**
 * Aux Data: Declarators
 *
 * 1. type header
 * 2. formal parameter
 * 3. method header
 * 4. variable declarator
 * 5. array creation
 *
*/
typedef struct
{
    node_data_id id;
    /* array dimension */
    size_t dimension;
} node_data_declarator;

/**
 * Aux Data: Expression
 *
 * Expression node holds a step of operation,
 * so it contains an operator, and operands
 * are stored on child level
 *
*/
typedef struct
{
    // operator id
    operator_id op;
} node_data_expression;

/**
 * Aux Data: Contructor Invocation
 *
*/
typedef struct
{
    // true if calling from super class, this class otherwise
    bool is_super;
} node_data_contructor_invoke;

/**
 * Aux Data: Switch Label
 *
*/
typedef struct
{
    // true if default label, case label otherwise
    bool is_default;
} node_data_switch_label;

/**
 * Aux Data: Ambiguity Node
 *
*/
typedef struct
{
    // pointer to JAVA_E_AMBIGUITY_START
    java_error_entry* error;
} node_data_ambiguity;

/**
 * Aux Data
 *
 * This is a layer for typing
 *
 * NOTE: pointer-only here
*/
typedef union
{
    node_data_id* id;
    node_data_import * import;
    node_data_top_level* top_level;
    node_data_declarator* declarator;
    node_data_expression* expression;
    node_data_contructor_invoke* constructor_invoke;
    node_data_switch_label* switch_label;
    node_data_ambiguity* ambiguity;
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
    tree_node_data data;
    /* binary way to represent multi-way tree */
    struct _tree_node* first_child;
    struct _tree_node* next_sibling;
    /* aux info for sibling traversal and addition */
    struct _tree_node* last_child;
    struct _tree_node* prev_sibling;
} tree_node;

void init_tree_node(tree_node* node);
void tree_node_add_child(tree_node* node, tree_node* child);
void tree_node_add_first_child(tree_node* node, tree_node* child);
void tree_node_delete(tree_node* node);

tree_node* ast_node_new(java_node_query type);

#endif
