#include "ir.h"
#include "node.h"
#include "utils.h"

/**
 * import lookup deleter
*/
static void import_lookup_deleter(char* k, global_import* v)
{
    free(k);
    delete_global_import(v);
}

/**
 * top-level lookup deleter
*/
static void top_level_lookup_deleter(char* k, global_top_level* v)
{
    free(k);
    delete_global_top_level(v);
}

/**
 * initialize semantic analysis
*/
void init_ir(java_ir* ir, java_expression* expression, java_error_stack* error)
{
    ir->working_top_level = NULL;
    ir->scope_stack_top = NULL;
    ir->arch = NULL;
    ir->expression = expression;
    ir->error = error;
    ir->scope_workers = NULL;
    ir->statement_contexts = NULL;

    init_hash_table(&ir->tbl_import, HASH_TABLE_DEFAULT_BUCKET_SIZE);
    init_hash_table(&ir->tbl_implicit_import, HASH_TABLE_DEFAULT_BUCKET_SIZE);
    init_hash_table(&ir->tbl_global, HASH_TABLE_DEFAULT_BUCKET_SIZE);
}

/**
 * release semantic analysis
*/
void release_ir(java_ir* ir)
{
    // delete import package names
    release_hash_table(&ir->tbl_import, &import_lookup_deleter);
    // delete implicit import names
    release_hash_table(&ir->tbl_implicit_import, &import_lookup_deleter);
    // delete global name lookup
    release_hash_table(&ir->tbl_global, &top_level_lookup_deleter);
    // delete entire lookup stack
    while (lookup_pop_scope(ir, NULL));
}

/**
 * Error Logger
 *
 * TODO: we need a clever way to get line info from java_ir instance
*/
void ir_error(java_ir* ir, java_error_id id)
{
    error_log(ir->error, id, 0, 0);
}

/**
 * Token-To-String Helper
 *
 * return a copy of string that a token consists
 *
 * no validation here because we are beyond that
 * (thus not in job description :P)
*/
char* t2s(java_token* token)
{
    size_t len = buffer_count(token->from, token->to);
    char* content = (char*)malloc_assert(sizeof(char) * (len + 1));

    buffer_substring(content, token->from, len);

    return content;
}

/**
 * Token-To-Definition Helper
 *
 * a definition associated to a name cannot be NULL, so if
 * undefined it returns NULL
 *
 * returned definition is a reference, not a copy
*/
definition* t2d(hash_table* table, java_token* token)
{
    char* registered_name = t2s(token);
    definition* def = shash_table_find(table, registered_name);
    free(registered_name);

    return def;
}

/**
 * name unit concatenation routine
*/
char* name_unit_concat(tree_node* from, tree_node* stop_before)
{
    string_list sl;
    char* s;

    // construct name list
    init_string_list(&sl);
    while (from != stop_before)
    {
        string_list_append(&sl, t2s(from->data.id->complex), false);
        from = from->next_sibling;
    }

    // now we concat the name
    s = string_list_concat(&sl, ".");
    release_string_list(&sl);

    return s;
}

/**
 * push a new scope worker
 *
*/
void push_scope_worker(java_ir* ir)
{
    cfg_worker_context* c = (cfg_worker_context*)malloc_assert(sizeof(cfg_worker_context));
    c->worker = (cfg_worker*)malloc_assert(sizeof(cfg_worker));

    init_cfg_worker(c->worker);
    c->next = ir->scope_workers;
    ir->scope_workers = c;
}

/**
 * get stack-top scope worker
*/
cfg_worker* get_scope_worker(java_ir* ir)
{
    return ir->scope_workers ? ir->scope_workers->worker : NULL;
}

/**
 * pop top scope worker
 *
 * it will return the worker
*/
cfg_worker* pop_scope_worker(java_ir* ir)
{
    cfg_worker* w = NULL;
    cfg_worker_context* c = ir->scope_workers;

    if (c)
    {
        ir->scope_workers = c->next;
        w = c->worker;
        free(c);
    }

    return w;
}

/**
 * push a new statement context
 *
 * it will return the context
*/
statement_context* push_statement_context(java_ir* ir, statement_context_query type)
{
    statement_context* c = (statement_context*)malloc_assert(sizeof(statement_context));

    c->type = type;
    c->_break = NULL;
    c->_continue = NULL;
    c->_test = NULL;
    c->next = ir->statement_contexts;
    ir->statement_contexts = c;

    return c;
}

/**
 * get closest stack-top statement context
 *
 * a query must be provided due to statement nesting
 * e.g.
 *
 * while(){ switch(){ case: continue; } }
 *
 * now the "continue" need to reach "while", but stack top
 * is "switch"
*/
statement_context* get_statement_context(java_ir* ir, statement_context_query query)
{
    statement_context* probe = ir->statement_contexts;

    while (probe)
    {
        if (probe->type & query)
        {
            return probe;
        }

        probe = probe->next;
    }

    return NULL;
}

/**
 * pop top statement context
 *
 * data contained are all references, so only delete stack frame
*/
void pop_statement_context(java_ir* ir)
{
    statement_context* c = ir->statement_contexts;

    if (c)
    {
        ir->statement_contexts = c->next;
    }

    free(c);
}

void init_definition_pool(definition_pool* pool)
{
    size_t sz = sizeof(definition*) * 2;

    pool->arr = (definition**)malloc_assert(sz);
    pool->num = 0;
    pool->size = 2;

    memset(pool->arr, 0, sz);
}

void release_definition_pool(definition_pool* pool)
{
    for (size_t i = 0; i < pool->num; i++)
    {
        definition_delete(pool->arr[i]);
    }

    free(pool->arr);
}

void definition_pool_grow(definition_pool* pool, size_t by)
{
    size_t new_size = pool->num + by;

    if (new_size > pool->size)
    {
        pool->size = find_next_pow2_size(new_size);
        pool->arr = (definition**)realloc_assert(pool->arr, sizeof(definition*) * (pool->size));
    }
}

void definition_pool_add(definition_pool* pool, definition* def)
{
    if (pool->arr)
    {
        definition_pool_grow(pool, 1);
    }
    else
    {
        init_definition_pool(pool);
    }

    pool->arr[pool->num] = def;
    pool->num++;
}

void definition_pool_merge(definition_pool* dest, definition_pool* src)
{
    definition_pool_grow(dest, src->num);
    memcpy(dest->arr + dest->num, src->arr, sizeof(definition*) * src->num);
    dest->num += src->num;

    // lazy detach
    src->num = 0;
}

global_import* new_global_import()
{
    global_import* i = (global_import*)malloc_assert(sizeof(global_import));

    i->package_name = NULL;

    return i;
}

void delete_global_import(global_import* i)
{
    if (!i) { return; }

    free(i->package_name);
    free(i);
}

/**
 * initialize a top-level descriptor
 *
 * NOTE: do NOT initialize code_member_init as it will be properly
 *       handled in walk_class
*/
global_top_level* new_global_top_level(top_level_type type)
{
    global_top_level* top = (global_top_level*)malloc_assert(sizeof(global_top_level));

    top->type = type;
    top->modifier = JLT_RWD_PRIVATE;
    top->extend = NULL;
    top->implement = NULL;
    top->num_implement = 0;
    top->code_member_init = NULL;
    top->node_first_body_decl = NULL;
    top->num_member_variable = 0;

    init_definition_pool(&top->member_init_variables);
    init_hash_table(&top->tbl_member, HASH_TABLE_DEFAULT_BUCKET_SIZE);
    init_hash_table(&top->tbl_literal, HASH_TABLE_DEFAULT_BUCKET_SIZE);

    return top;
}

void delete_global_top_level(global_top_level* top)
{
    if (!top) { return; }

    // release all implement names
    for (size_t i = 0; i < top->num_implement; i++)
    {
        free(top->implement[i]);
    }

    // delete member init code
    release_definition_pool(&top->member_init_variables);
    release_cfg(top->code_member_init);

    // delete all members
    release_hash_table(&top->tbl_member, &definition_lookup_deleter);
    release_hash_table(&top->tbl_literal, &definition_lookup_deleter);

    // cleanup
    free(top->extend);
    free(top->implement);
    free(top->code_member_init);
    free(top);
}
