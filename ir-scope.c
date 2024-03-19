#include "ir.h"

/**
 * lookup hierarchy table pair deleter
 *
*/
void definition_lookup_deleter(char* k, definition* v)
{
    free(k);
    definition_delete(v);
}

/**
 * push a new lookup node
*/
hash_table* lookup_new_scope(java_ir* ir)
{
    scope_frame* scope = (scope_frame*)malloc_assert(sizeof(scope_frame));

    // init
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
 *
 * TODO: deprecate merge_global logic
 * instead of merging into the global table, we merge it into a pool that only contains
 * local definitions: the pool is a linked list, O(1) insert front and we do not
 * care the order because we only book-keep the data so that code graphs can reference
 * from it
*/
bool lookup_pop_scope(java_ir* ir, definition_pool* pool)
{
    scope_frame* top = ir->scope_stack_top;

    // flush all definitions into the pool
    if (top)
    {
        hash_table* table = top->table;

        if (pool)
        {
            for (size_t i = 0; i < table->bucket_size; i++)
            {
                hash_pair* p = table->bucket[i];

                while (p)
                {
                    // move to pool
                    definition_pool_add(pool, p->value);
                    p->value = NULL;
                    p = p->next;
                }
            }
        }

        // release table
        release_hash_table(top->table, &definition_lookup_deleter);
        free(top->table);
        ir->scope_stack_top = top->next;
        free(top);

        return true;
    }

    return false;
}

/**
 * get current global scope
 *
 * TYPE: map<string, global_top_level*>
*/
hash_table* lookup_global_scope(java_ir* ir)
{
    return &ir->tbl_global;
}

/**
 * get current top-level scope
 *
 * TYPE: global_top_level.tbl_member: map<string, definition*>
*/
hash_table* lookup_top_level_scope(java_ir* ir)
{
    return ir->working_top_level ? &ir->working_top_level->tbl_member : NULL;
}

/**
 * get current top-level literal scope
 *
 * TYPE: global_top_level.tbl_literal: map<string, definition*>
*/
hash_table* lookup_top_level_literal_scope(java_ir* ir)
{
    return ir->working_top_level ? &ir->working_top_level->tbl_literal : NULL;
}

/**
 * get hierarchical scope
 *
 * TYPE: map<string, definition*>
 *
 * all definitions can go as high as ir->working_top_level, global
 * is higher but it only stores reserved names (global_top_level)
 * instead of definitions
*/
hash_table* lookup_working_scope(java_ir* ir)
{
    return ir->scope_stack_top ? ir->scope_stack_top->table : lookup_top_level_scope(ir);
}

/**
 * attach top level definition to lookup hierarchy
*/
void lookup_top_level_begin(java_ir* ir, global_top_level* desc)
{
    ir->working_top_level = desc;
}

/**
 * detach top level definition from lookup hierarchy
*/
void lookup_top_level_end(java_ir* ir)
{
    ir->working_top_level = NULL;
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
 * do NOT use this method if you are not sure which table to use
 *
 * when passing error code JAVA_E_MAX, no error will be logged
 *
 * NOTE: if failed, name and desc will stay as-is
*/
bool lookup_register(
    java_ir* ir,
    hash_table* table,
    char** name,
    void** desc,
    java_error_id err
)
{
    if (shash_table_test(table, *name))
    {
        ir_error(ir, err);
        return false;
    }
    else
    {
        shash_table_insert(table, *name, *desc);

        // detach
        *name = NULL;
        *desc = NULL;

        return true;
    }
}

/**
 * generate definition instance
*/
definition* new_definition(definition_type type)
{
    definition* v = (definition*)malloc_assert(sizeof(definition));

    v->type = type;
    v->def_count = 0;

    switch (type)
    {
        case DEFINITION_VARIABLE:
            v->variable.is_class_member = false;
            v->variable.modifier = JLT_UNDEFINED;
            __init_type_name(&v->variable.type);
            break;
        case DEFINITION_METHOD:
            v->method.modifier = JLT_UNDEFINED;
            __init_type_name(&v->method.return_type);
            init_definition_pool(&v->method.local_variables);
            // no code CFG initialization here as parser will do it
            break;
        case DEFINITION_NUMBER:
        case DEFINITION_BOOLEAN:
        case DEFINITION_CHARACTER:
            v->li_number.type = IRPV_MAX;
            v->li_number.imm = 0;
            break;
        case DEFINITION_STRING:
            v->li_string.stream = NULL;
            v->li_string.length = 0;
            v->li_string.wide_char = false;
            break;
        case DEFINITION_NULL:
        default:
            break;
    }

    return v;
}

/**
 * delete single definition
*/
void definition_delete(definition* v)
{
    if (!v) { return; }

    switch (v->type)
    {
        case DEFINITION_VARIABLE:
            free(v->variable.type.reference);
            break;
        case DEFINITION_METHOD:
            free(v->method.return_type.reference);
            release_definition_pool(&v->method.local_variables);
            release_cfg(&v->method.code);
            break;
        case DEFINITION_STRING:
            free(v->li_string.stream);
            break;
        case DEFINITION_NUMBER:
        case DEFINITION_BOOLEAN:
        case DEFINITION_CHARACTER:
        case DEFINITION_NULL:
        default:
            // no-op
            break;
    }

    free(v);
}

/**
 * copy single definition
*/
definition* definition_copy(definition* v)
{
    definition* w = (definition*)malloc_assert(sizeof(definition));

    // shallow copy first
    memcpy(w, v, sizeof(definition));

    // now deep copy
    switch (v->type)
    {
        case DEFINITION_VARIABLE:
            w->variable.type.reference = strmcpy_assert(v->variable.type.reference);
            break;
        case DEFINITION_METHOD:
            /**
             * TODO: so far there is no use case for CFG copy
             * so we leave it empty
            */
            fprintf(stderr, "TODO ERROR: internal error: method copy detected, but it is not implemented yet.\n");
            w->method.return_type.reference = strmcpy_assert(v->method.return_type.reference);
            memset(&w->method.code, 0, sizeof(cfg));
            break;
        default:
            // no-op
            break;
    }

    return w;
}
