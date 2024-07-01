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
void init_ir(java_ir* ir, java_expression* expression, java_error_logger* logger)
{
    ir->working_top_level = NULL;
    ir->scope_stack_top = NULL;
    ir->arch = NULL;
    ir->expression = expression;
    ir->logger = logger;
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
    error_logger_log(ir->logger, 0, 0, id);
}

void ir_walk_state_init(java_ir* ir)
{
    ir->walk_state.num_member_variable = 0;
    ir->walk_state.num_local_variable = 0;
    ir->walk_state.num_member_variable_use = 0;
    ir->walk_state.num_worker_instruction = 0;
    ir->walk_state.num_field_init_instruction = 0;
    ir->walk_state.num_field_init_member_variable_use = 0;
}

/**
 * set current walk state
 *
 * it will reset corresponding counters so that ir_walk_state_allocate_id
 * could retrive information from the beginning
 *
 * IR_WALK_CODE_FIELD_INIT: it still uses IR_WALK_CODE_WORKER
 * to actively tracking ID, except that every walk_field()
 * starts, num_field_init_instruction will be copied into
 * num_worker_instruction
 * therefore, this routine will never reset num_field_init_instruction,
 * walk_field() will sync it with num_worker_instruction everytime
 * it finishes
*/
void ir_walk_state_mutate(java_ir* ir, ir_walk_state_type type)
{
    switch (type)
    {
        case IR_WALK_CODE_WORKER:
            ir->walk_state.num_worker_instruction = 0;
            ir->walk_state.num_local_variable = 0;
            ir->walk_state.num_member_variable_use = 0;
            break;
        case IR_WALK_CODE_FIELD_INIT:
            // sync back to worker state
            ir->walk_state.num_worker_instruction = ir->walk_state.num_field_init_instruction;
            ir->walk_state.num_local_variable = 0; // unused
            ir->walk_state.num_member_variable_use = ir->walk_state.num_field_init_member_variable_use;
            break;
        case IR_WALK_DEF_MEMBER_VAR:
            ir->walk_state.num_member_variable = 0;
            break;
        case IR_WALK_DEF_LOCAL_VAR:
        case IR_WALK_USE_MEMBER_VAR:
            /**
             * default and implicit behavior
             *
             * managed by IR_WALK_CODE_WORKER
            */
            break;
        default:
            break;
    }
}

/**
 * Sync state info
 *
 * This is used when one state is shared by multiple walks
 * alternating, the transient info will be accumulated
 * in another fields
 *
 * Synced fields will be reset to 0
*/
void ir_walk_state_sync(java_ir* ir, ir_walk_state_type type)
{
    switch (type)
    {
        case IR_WALK_CODE_FIELD_INIT:
            ir->walk_state.num_field_init_instruction = ir->walk_state.num_worker_instruction;
            ir->walk_state.num_field_init_member_variable_use = ir->walk_state.num_member_variable_use;
            break;
        default:
            // no-op
            break;
    }
}

/**
 * It returns an index ID, also progresses the counter
 *
*/
size_t ir_walk_state_allocate_id(java_ir* ir, ir_walk_state_type type)
{
    switch (type)
    {
        case IR_WALK_USE_MEMBER_VAR:
            return ir->walk_state.num_member_variable_use++;
        case IR_WALK_DEF_MEMBER_VAR:
            return ir->walk_state.num_member_variable++;
        case IR_WALK_DEF_LOCAL_VAR:
            return ir->walk_state.num_local_variable++;
        case IR_WALK_CODE_FIELD_INIT:
            // not used in this way, but add the logic anyway
            return ir->walk_state.num_field_init_instruction++;
        case IR_WALK_CODE_WORKER:
            return ir->walk_state.num_worker_instruction++;
        default:
            // not managed
            return 0;
    }
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
    top->num_fields = 0;
    top->num_methods = 0;
    top->code_member_init = NULL;
    top->node_first_body_decl = NULL;

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
