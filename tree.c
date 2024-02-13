#include "tree.h"

/**
 * init tree node fields
*/
void init_tree_node(tree_node* node)
{
    node->type = JNT_MAX;
    node->ambiguous = true;
    node->data = NULL;
    node->first_child = NULL;
    node->next_sibling = NULL;
    node->last_child = NULL;
    node->prev_sibling = NULL;
}

/**
 * Mutate tree node type into another
*/
void tree_node_mutate(tree_node* node, java_node_query type)
{
    node->type = type;
}

/**
 * add a child node to the target node
*/
void tree_node_add_child(tree_node* node, tree_node* child)
{
    child->prev_sibling = node->last_child;

    if (node->last_child)
    {
        node->last_child->next_sibling = child;
    }
    else
    {
        node->first_child = child;
    }

    node->last_child = child;
    node->ambiguous = node->ambiguous || child->ambiguous;
}

/**
 * recursively delete ast and data attached to each node
 *
 * NOTE: always try deletion subtree before attaching, otherwise it will cause avalanche
 * effect
*/
void tree_node_delete(tree_node* node)
{
    if (!node)
    {
        return;
    }

    // go deeper first
    tree_node_delete(node->first_child);

    // then go to next sibling
    tree_node_delete(node->next_sibling);

    // delete data
    if (node->data)
    {
        switch (node->type)
        {
            case JNT_NAME_UNIT:
            case JNT_CLASS_TYPE_UNIT:
            case JNT_INTERFACE_TYPE_UNIT:
            case JNT_CLASS_DECL:
            case JNT_INTERFACE_DECL:
            case JNT_CTOR_DECL:
            case JNT_PRIMARY_COMPLEX:
            case JNT_STATEMENT_BREAK:
            case JNT_STATEMENT_CONTINUE:
            case JNT_STATEMENT_LABEL:
                free(node->data->id.complex);
                break;
            case JNT_METHOD_HEADER:
            case JNT_FORMAL_PARAM:
            case JNT_VAR_DECL:
                free(node->data->declarator.id.complex);
                break;
            default:
                // no-op
                break;
        }

        free(node->data);
    }

    // finally, delete self
    free(node);
}
