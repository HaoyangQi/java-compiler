#include "tree.h"

/**
 * init tree node fields
*/
void init_tree_node(tree_node* node)
{
    node->metadata = 0;
    node->data = NULL;
    node->valid = true;
    node->ambiguous = true;
    node->first_child = NULL;
    node->next_sibling = NULL;
    node->last_sibling = NULL;
}

/**
 * Mutate tree node type into another
*/
void tree_node_mutate(tree_node* node, int meta)
{
    node->metadata = meta;
}

/**
 * attach data to a tree node
*/
void tree_node_attach(tree_node* node, int meta, void* data)
{
    node->metadata = meta;
    node->data = data;
}

/**
 * add a child node to the target node
*/
void tree_node_add_child(tree_node* node, tree_node* child)
{
    if (node->last_sibling)
    {
        node->last_sibling->next_sibling = child;
    }
    else
    {
        node->first_child = child;
    }

    node->last_sibling = child;
    node->valid = node->valid && child->valid;
    node->ambiguous = node->ambiguous || child->ambiguous;
}

/**
 * recursively delete ast tree and data attached to each node
 *
 * if calback is specified, it will invoke that first to process data deletion in detail
 * then delete data itself
 * the callback must be generic and handles every scenario for node data, because the
 * deletion is recursive
 *
 * NOTE: always try deletion subtree before attaching, otherwise it will cause avalanche
 * effect
*/
void tree_node_delete(tree_node* node, node_data_delete_callback cb)
{
    if (!node)
    {
        return;
    }

    // go deeper first
    tree_node_delete(node->first_child, cb);

    // then go to next sibling
    tree_node_delete(node->next_sibling, cb);

    // delete data
    if (cb)
    {
        (*cb)(node->metadata, node->data);
    }
    free(node->data);

    // finally, delete self
    free(node);
}
