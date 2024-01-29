#include "ir.h"

/**
 * lookup hierarchy table pair deleter
 *
 * TODO: we may need delete routine for semantic_variable_descriptor in the
 * future
*/
void lookup_scope_deleter(char* k, definition* v)
{
    free(k);
    definition_delete(v);
}

/**
 * push a new lookup node
*/
hash_table* lookup_new_scope(java_ir* ir, lookup_scope_type type)
{
    scope_frame* scope = (scope_frame*)malloc_assert(sizeof(scope_frame));

    // init
    scope->type = type;
    scope->fic = 0;
    scope->table = (hash_table*)malloc_assert(sizeof(hash_table));
    init_hash_table(scope->table, HASH_TABLE_DEFAULT_BUCKET_SIZE);

    // push
    scope->next = ir->scope_stack_top;
    ir->scope_stack_top = scope;

    return scope->table;
}

/**
 * pop current lookup node
 *
 * if merge_global is true, table contents will be merged into global lookup
*/
bool lookup_pop_scope(java_ir* ir, bool merge_global)
{
    scope_frame* top = ir->scope_stack_top;

    // merge
    if (top && merge_global)
    {
        hash_table* table = top->table;

        for (size_t i = 0; i < table->bucket_size; i++)
        {
            hash_pair* p = table->bucket[i];

            if (p)
            {
                while (p)
                {
                    hash_pair* pair = shash_table_get(&ir->tbl_global, p->key);

                    // if key exists, we merge; otherwise simply insert
                    if (pair)
                    {
                        definition_concat(pair->value, p->value);
                    }
                    else
                    {
                        shash_table_insert(&ir->tbl_global, p->key, p->value);
                    }

                    /**
                     * detach all data since we are moving it to global
                     *
                     * NOTE: this is destructive and will make the table
                     * stop functioning
                    */
                    p->key = NULL;
                    p->value = NULL;

                    p = p->next;
                }
            }
        }
    }

    if (top)
    {
        release_hash_table(top->table, &lookup_scope_deleter);
        free(top->table);
        ir->scope_stack_top = top->next;
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
    return ir->scope_stack_top->table;
}

static void __init_type_name(type_name* t)
{
    t->primitive = JLT_MAX;
    t->reference = NULL;
    t->dim = 0;
}

/**
 * name lookup register
 *
 * when passing error code JAVA_E_MAX, no error will be logged
 *
 * NOTE: if failed, desc will be deleted
*/
bool lookup_register(
    java_ir* ir,
    hash_table* table,
    char* name,
    definition* desc,
    java_error_id err
)
{
    if (shash_table_test(table, name))
    {
        if (err != JAVA_E_MAX)
        {
            ir_error(ir, err);
        }

        definition_delete(desc);
        return false;
    }
    else
    {
        shash_table_insert(table, name, desc);
        return true;
    }
}

/**
 * generate definition instance
*/
definition* new_definition(java_node_query type)
{
    definition* v = (definition*)malloc_assert(sizeof(definition));

    v->type = type;
    v->next = NULL;

    switch (type)
    {
        case JNT_IMPORT_DECL:
            v->import.package_name = NULL;
            break;
        case JNT_CLASS_DECL:
            v->class.modifier = 0;
            v->class.extend = NULL;
            v->class.implement = NULL;
            break;
        case JNT_VAR_DECL:
            v->variable.is_class_member = false;
            v->variable.modifier = 0;
            v->variable.version = 0;
            __init_type_name(&v->variable.type);
        case JNT_METHOD_DECL:
            v->method.modifier = 0;
            __init_type_name(&v->method.return_type);
        default:
            break;
    }

    return v;
}

/**
 * im-place concatenate two definition chain
*/
void definition_concat(definition* dest, definition* src)
{
    // locate the end of chain
    while (dest->next != NULL)
    {
        dest++;
    }

    // append
    dest->next = src;
}

/**
 * delete single definition
*/
static definition* __definition_delete_single(definition* v)
{
    definition* n = v->next;

    switch (v->type)
    {
        case JNT_IMPORT_DECL:
            free(v->import.package_name);
            break;
        case JNT_CLASS_DECL:
            free(v->class.extend);
            free(v->class.implement);
            break;
        case JNT_VAR_DECL:
            free(v->variable.type.reference);
            break;
        default:
            // no-op
            break;
    }

    free(v);

    return n;
}

/**
 * delete descriptor content
*/
void definition_delete(definition* v)
{
    while (v)
    {
        v = __definition_delete_single(v);
    }
}

/**
 * copy single definition
*/
definition* __definition_copy_single(definition* v)
{
    definition* w = (definition*)malloc_assert(sizeof(definition));

    // shallow copy first
    memcpy(w, v, sizeof(definition));

    // now deep copy
    switch (v->type)
    {
        case JNT_IMPORT_DECL:
            w->import.package_name = strmcpy_assert(v->import.package_name);
            break;
        case JNT_CLASS_DECL:
            w->class.extend = strmcpy_assert(v->class.extend);
            w->class.implement = strmcpy_assert(v->class.implement);
            break;
        case JNT_VAR_DECL:
            w->variable.type.reference = strmcpy_assert(v->variable.type.reference);
            break;
        default:
            // no-op
            break;
    }

    // clear pointer
    w->next = NULL;

    return w;
}

/**
 * copy definition
*/
definition* definition_copy(definition* v)
{
    definition* head = NULL;
    definition* cur = NULL;

    while (v)
    {
        // if head is defined, so is cur
        if (head)
        {
            cur->next = __definition_copy_single(v);
            cur = cur->next;
        }
        else
        {
            cur = __definition_copy_single(v);
            head = cur;
        }

        v = v->next;
    }

    return head;
}
