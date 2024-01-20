#include "semantics.h"

void semantics_walk_top_level(tree_node* node)
{
    if (!node)
    {
        return;
    }

    // top level does not have siblings
    node = node->first_child;

    /**
     * TODO: handle pkg decl
    */

    /**
     * TODO: handle import
    */

    // traverse
    while (node)
    {
        /**
         * TODO: process
        */

        // go deeper first
        if (node->first_child)
        {
            semantics_walk_top_level(node->first_child);
        }

        // then go next sibling
        node = node->next_sibling;
    }
}
