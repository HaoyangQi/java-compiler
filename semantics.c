#include "semantics.h"
#include "node.h"

/**
 * add a lookup node to the parent node
*/
static lookup_hierarchy* lookup_add(lookup_hierarchy* parent)
{
    lookup_hierarchy* child = (lookup_hierarchy*)malloc_assert(sizeof(lookup_hierarchy));

    memset(child, 0, sizeof(lookup_hierarchy));
    init_hash_table(&child->table, 0);

    child->parent = parent;

    if (parent)
    {
        child->next_sibling = parent->first_child;
        parent->first_child = child;
    }

    return child;
}

/**
 * lookup hierarchy table pair deleter
 *
 * TODO: we may need delete routine for semantic_variable_descriptor in the
 * future
*/
static void lookup_table_pair_deleter(char* k, semantic_variable_descriptor* v)
{
    free(k);
    free(v);
}

/**
 * lookup tree delete
*/
static void lookup_delete(lookup_hierarchy* node)
{
    if (!node)
    {
        return;
    }

    // go deeper first
    lookup_delete(node->first_child);

    // then go to next sibling
    lookup_delete(node->next_sibling);

    // delete data
    release_hash_table(&node->table, &lookup_table_pair_deleter);

    // finally, delete self
    free(node);
}

/**
 * initialize semantic analysis
*/
void init_semantics(java_semantics* se)
{
    se->lookup_root = lookup_add(NULL);
    se->member_initialization = NULL;
    se->methods = NULL;
    se->num_methods = 0;
}

/**
 * release semantic analysis
*/
void release_semantics(java_semantics* se)
{
    lookup_delete(se->lookup_root);
}

/**
 * SSA Builder
*/
void contextualize(java_semantics* se, tree_node* compilation_unit)
{
    // compilation unit does not have siblings
    tree_node* node = compilation_unit->first_child;

    // package
    if (node && node->type == JNT_PKG_DECL)
    {
        /**
         * TODO: handle pkg decl
        */
        node = node->next_sibling;
    }

    // imports
    while (node && node->type == JNT_IMPORT_DECL)
    {
        /**
         * TODO: handle import decl
        */
        node = node->next_sibling;
    }

    // top-levels
    while (node && node->type == JNT_TOP_LEVEL)
    {
        /**
         * TODO: handle import decl
        */
        node = node->next_sibling;
    }
}
