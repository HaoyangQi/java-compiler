#include "ir.h"
#include "node.h"

/**
 * initialize semantic analysis
*/
void init_ir(java_ir* ir, java_error* error)
{
    ir->scope_stack_top = NULL;
    ir->num_methods = 0;
    ir->arch = NULL;
    ir->error = error;
    ir->code_member_init = NULL;

    init_hash_table(&ir->tbl_on_demand_packages, HASH_TABLE_DEFAULT_BUCKET_SIZE);
    init_hash_table(&ir->tbl_global, HASH_TABLE_DEFAULT_BUCKET_SIZE);
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
    // delete entire lookup stack
    while (lookup_pop_scope(ir, false));
    // delete member init code
    release_cfg(ir->code_member_init);
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
