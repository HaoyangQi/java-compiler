#include "ir.h"
#include "node.h"

/**
 * initialize semantic analysis
 *
 * code_member_init: no init for it here because it data section is only
 *                   a container for cfg worker
*/
void init_ir(java_ir* ir, java_expression* expression, java_error_stack* error)
{
    ir->local_def_pool = NULL;
    ir->scope_stack_top = NULL;
    ir->arch = NULL;
    ir->expression = expression;
    ir->error = error;
    ir->code_member_init = NULL;
    ir->scope_workers = NULL;
    ir->statement_contexts = NULL;

    init_hash_table(&ir->tbl_on_demand_packages, HASH_TABLE_DEFAULT_BUCKET_SIZE);
    init_hash_table(&ir->tbl_global, HASH_TABLE_DEFAULT_BUCKET_SIZE);
    init_hash_table(&ir->tbl_literal, HASH_TABLE_DEFAULT_BUCKET_SIZE);
}

/**
 * release semantic analysis
*/
void release_ir(java_ir* ir)
{
    // delete on-demand import package names
    release_hash_table(&ir->tbl_on_demand_packages, &pair_data_delete_key);
    // delete global name lookup
    release_hash_table(&ir->tbl_global, &lookup_scope_deleter);
    // delete literal lookup
    release_hash_table(&ir->tbl_literal, &lookup_scope_deleter);
    // delete entire lookup stack
    while (lookup_pop_scope(ir, false));
    // delete member init code
    release_cfg(ir->code_member_init);
    free(ir->code_member_init);
    // delete definition pool
    definition_delete(ir->local_def_pool);
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
    definition* def = HT_STR2DEF(table, registered_name);
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

    // construct package name list
    init_string_list(&sl);
    while (from != stop_before)
    {
        string_list_append(&sl, t2s(from->data->id.complex));
        from = from->next_sibling;
    }

    // now we concat the package name
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
void push_statement_context(java_ir* ir, java_node_query type)
{
    statement_context* c = (statement_context*)malloc_assert(sizeof(statement_context));

    c->type = type;
    c->_break = NULL;
    c->_continue = NULL;
    c->next = ir->statement_contexts;
    ir->statement_contexts = c;
}

/**
 * get closest stack-top statement context
 *
 * a type must be provided due to statement nesting
 * e.g.
 *
 * while(){ switch(){ case: continue; } }
 *
 * now the "continue" need to reach "while", but stack top
 * is "switch"
*/
statement_context* get_statement_context(java_ir* ir, java_node_query type)
{
    statement_context* probe = ir->statement_contexts;

    while (probe && probe->type != type)
    {
        probe = probe->next;
    }

    return probe;
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
