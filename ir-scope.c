#include "ir.h"

/**
 * lookup hierarchy table pair deleter
 *
 * TODO: we may need delete routine for semantic_variable_descriptor in the
 * future
*/
void lookup_scope_deleter(char* k, lookup_value_descriptor* v)
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
hash_table* lookup_global_scope(java_ir* ir)
{
    return &ir->tbl_global;
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
        case JNT_CLASS_DECL:
            v->class.modifier = 0;
            v->class.extend = NULL;
            break;
        case JNT_VAR_DECL:
            v->member_variable.modifier = 0;
            v->member_variable.type.primitive = JLT_MAX;
            v->member_variable.type.reference = NULL;
            v->member_variable.type.dim = 0;
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
        case JNT_CLASS_DECL:
            free(v->class.extend);
            break;
        case JNT_VAR_DECL:
            free(v->member_variable.type.reference);
            break;
        default:
            // no-op
            break;
    }

    free(v);
}

/**
 * copy descriptor content
*/
lookup_value_descriptor* lookup_value_descriptor_copy(lookup_value_descriptor* v)
{
    lookup_value_descriptor* w = (lookup_value_descriptor*)malloc_assert(sizeof(lookup_value_descriptor));

    // shallow copy first
    memcpy(w, v, sizeof(lookup_value_descriptor));

    // now deep copy
    switch (v->type)
    {
        case JNT_IMPORT_DECL:
            w->import.package_name = strmcpy_assert(v->import.package_name);
            break;
        case JNT_CLASS_DECL:
            w->class.extend = strmcpy_assert(v->class.extend);
            break;
        case JNT_VAR_DECL:
            w->member_variable.type.reference = strmcpy_assert(v->member_variable.type.reference);
            break;
        default:
            // no-op
            break;
    }

    return w;
}

/**
 * name lookup register
 *
 * when passing error code JAVA_E_MAX, no error will be logged
*/
bool lookup_register(
    java_ir* ir,
    hash_table* table,
    char* name,
    lookup_value_descriptor* desc,
    java_error_id err
)
{
    if (shash_table_test(table, name))
    {
        if (err != JAVA_E_MAX)
        {
            ir_error(ir, err);
        }

        return false;
    }
    else
    {
        shash_table_insert(table, name, desc);
        return true;
    }
}
