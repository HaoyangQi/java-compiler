#include "optimizer.h"

/**
 * initialize optimizer for given code graph
*/
void init_optimizer(optimizer* om)
{
    memset(om, 0, sizeof(optimizer));
    init_definition_pool(&om->spill_pool);
}

/**
 * release optimizer
*/
void release_optimizer(optimizer* om)
{
    if (!om || !om->graph) { return; }

    optimizer_invalidate_instructions(om);
    optimizer_invalidate_variables(om);
    cfg_delete_node_order(om->node_postorder);
    release_definition_pool(&om->spill_pool);
}

/**
 * attach optimizer for given code graph
 *
 * NOTE: this process only fills information, no data will be allocated
 *
 * e.g.
 * instruction-indexed array will not be initialized here, because its size
 * needs to count phi statements generated by SSA process
 *
 * TODO: probably just register using top-level,
 * and do everything within?
*/
void optimizer_attach(optimizer* om, global_top_level* top_level, definition* target)
{
    if (!om || !top_level || !target) { return; }

    /**
     * TODO: how to handle member init code?
     * we do not optimize: because member init code cannot use "local" variables anyway
     * but we do generate temp variables, that could be tricky
     * so we probably have to do it anyway?
    */
    switch (target->type)
    {
        case DEFINITION_METHOD:
            om->graph = &target->method->code;
            om->node_postorder = cfg_node_order(om->graph, DFS_POSTORDER);

            /**
             * Fill Optimizer Profile
             *
             * NOTE:
             * om->profile.num_members counts total number of member variables
             * on top level, instead of target->method->member_variable_use_count,
             * because the index id "mid" is used to index variables, so all
             * slots need to be reserved
            */
            om->profile.num_nodes = om->graph->nodes.num;
            om->profile.num_locals = target->method->local_variables.num;
            om->profile.num_members = top_level->num_fields;
            om->profile.num_variables = om->profile.num_members + om->profile.num_locals;
            om->profile.num_instructions = target->method->instruction_count;
            break;
        default:
            // halt if target is invalid
            return;
    }

    /**
     * This loop guarantees every node has at least one instruction
     *
     * this aid helps to build clean algorithm in optimizer, as an example,
     * liveness analysis, and any other algorithms that require to access
     * neighbors of instruction along control flow, will be built in cleaner
     * way based on this assumption.
     *
     * node order does not matter here
    */
    for (size_t i = 0; i < om->profile.num_nodes; i++)
    {
        basic_block* n = om->node_postorder[i];

        // if node is empty, insert a placeholder
        // it contribute to no logical meaning, but it is still an "instruction"
        // so algorithms can reference it when necessary
        if (!n->inst_first)
        {
            instruction* inst = new_instruction();

            // prepare instruction
            inst->op = IROP_NOOP;
            inst->node = n;
            instruction_insert(n, NULL, inst);

            // update profile
            om->profile.num_instructions++;
        }
    }
}

/**
 * Detach optimizer from current code graph
*/
void optimizer_detach(optimizer* om)
{
    release_optimizer(om);
    init_optimizer(om);
}

/**
 * execute optimization
 *
 * execution order here matters
 *
 * all processes involve instruction-id-indexed-array:
 * its size needs to count phi statements generated by SSA process
*/
void optimizer_execute(optimizer* om)
{
    if (!om->graph) { return; }

    // SSA begin
    optimizer_ssa_build(om);

    /**
     * TODO:
     * here, in SSA form, do optimizations that depend on it;
     * call optimizer_populate_instructions if necessary,
     * but need to re-populate after eliminating SSA form
    */

    // SSA end
    optimizer_ssa_eliminate(om);

    // register allocation
    // optimizer_allocator_heuristic(om, 4);
    optimizer_allocator_linear(om, 4);
}
