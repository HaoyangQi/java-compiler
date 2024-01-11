#pragma once
#ifndef __COMPILER_TREE_H__
#define __COMPILER_TREE_H__

#include "types.h"

typedef void (*node_data_delete_callback)(int metadata, void* data);

/**
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
    /* metadata describes type of data */
    int metadata;
    /* node data */
    void* data;
    /* true if production completes */
    bool valid;
    /* false if production uniquely determines input */
    bool ambiguous;
    /* binary way to represent multi-way tree */
    struct _tree_node* first_child;
    struct _tree_node* next_sibling;
    /* aux info for sibling traversal and addition */
    struct _tree_node* last_sibling;
} tree_node;

void init_tree_node(tree_node* node);
void tree_node_mutate(tree_node* node, int meta);
void tree_node_attach(tree_node* node, int meta, void* data);
void tree_node_add_child(tree_node* node, tree_node* child);
void tree_node_delete(tree_node* node, node_data_delete_callback cb);

#endif
