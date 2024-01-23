#include "ir.h"

/**
 * lookup hierarchy table pair deleter
 *
 * TODO: we may need delete routine for semantic_variable_descriptor in the
 * future
*/
static void lookup_scope_deleter(char* k, lookup_value_descriptor* v)
{
    free(k);
    lookup_value_descriptor_delete(v);
}

/**
 * push a new lookup node
*/
hash_table* lookup_new_scope(java_ir* ir, lookup_scope_type type)
{
    lookup_hierarchy* scope = (lookup_hierarchy*)malloc_assert(sizeof(lookup_hierarchy));

    // init
    scope->type = type;
    init_hash_table(&scope->table, HASH_TABLE_DEFAULT_BUCKET_SIZE);

    // push
    scope->next = ir->lookup_current_scope;
    ir->lookup_current_scope = scope;

    return &scope->table;
}

/**
 * pop current lookup node
*/
bool lookup_pop_scope(java_ir* ir)
{
    lookup_hierarchy* top = ir->lookup_current_scope;

    if (top)
    {
        release_hash_table(&top->table, &lookup_scope_deleter);
        ir->lookup_current_scope = top->next;
        free(top);

        return true;
    }

    return false;
}

/**
 * get current scope
*/
hash_table* lookup_current_scope(java_ir* ir)
{
    return &ir->lookup_current_scope->table;
}

/**
 * generate lookup_value_descriptor instance
*/
lookup_value_descriptor* new_lookup_value_descriptor(java_node_query type)
{
    lookup_value_descriptor* v = (lookup_value_descriptor*)malloc_assert(sizeof(lookup_value_descriptor));

    v->type = type;

    switch (type)
    {
        case JNT_IMPORT_DECL:
            v->import.package_name = NULL;
            break;
        default:
            break;
    }

    return v;
}

/**
 * delete descriptor content
*/
void lookup_value_descriptor_delete(lookup_value_descriptor* v)
{
    switch (v->type)
    {
        case JNT_IMPORT_DECL:
            free(v->import.package_name);
            break;
        default:
            // no-op
            break;
    }

    free(v);
}
