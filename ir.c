#include "ir.h"
#include "node.h"

/**
 * initialize semantic analysis
*/
void init_ir(java_ir* ir, java_error* error)
{
    ir->lookup_current_scope = NULL;
    ir->member_initialization = NULL;
    ir->methods = NULL;
    ir->num_methods = 0;
    ir->error = error;

    init_hash_table(&ir->tbl_on_demand_packages, HASH_TABLE_DEFAULT_BUCKET_SIZE);
}

/**
 * release semantic analysis
*/
void release_ir(java_ir* ir)
{
    // delete on-demand import package names
    release_hash_table(&ir->tbl_on_demand_packages, &pair_data_delete_key);
    // delete entire lookup stack
    while (lookup_pop_scope(ir));
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
