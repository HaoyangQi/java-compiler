#pragma once
#ifndef __COMPILER_OPTIMIZER_H__
#define __COMPILER_OPTIMIZER_H__

#include "types.h"
#include "ir.h"
#include "index-set.h"

/**
 * Variable Allocation Type
 *
 * VAR_ALLOC_UNDEFINED:
 * when a variable is detected by optimizer such that it can be optimized out
 * somehow, then allocation info is undefined
*/
typedef enum _variable_allocation_type
{
    VAR_ALLOC_UNDEFINED = 0,
    VAR_ALLOC_REGISTER,
    VAR_ALLOC_STACK,
} variable_allocation_type;

typedef struct _variable_item
{
    definition* ref;
    variable_allocation_type alloc_type;
    size_t alloc_loc;
    size_t ud_loop_inside;
    size_t ud_loop_outside;
} variable_item;

typedef struct _instruction_item
{
    instruction* ref;
    index_set def;
    index_set use;
    index_set in;
    index_set out;
} instruction_item;

typedef struct _optimizer_profile
{
    size_t num_nodes;
    size_t num_members;
    size_t num_locals;
    size_t num_variables;
    size_t num_instructions;

    // max number of register available
    size_t reg_count;
    // number of variable allocated on stack
    size_t num_var_on_stack;
} optimizer_profile;

typedef struct _optimizer
{
    cfg* graph;

    /**
     * Register Profile
    */
    optimizer_profile profile;

    /**
     * CFG nodes in post order traversal
    */
    basic_block** node_postorder;

    /**
     * Variable Info Item Array
     *
     * Indexed by member ID and local ID
     *
     * all variables in current program routine, including:
     * 1. all member variables (indexed by mid)
     * 2. parameters + local + temporary (indexed by lid)
     *
     * 3 parts are stored in order: 1->2
     * NULL slot means this variable does not appear in CFG
    */
    variable_item* variables;

    /**
     * Instruction Info Item Array
     *
     * indexed by instruction id
    */
    instruction_item* instructions;

    /**
     * Spilled Code Temp Variable Pool
     *
     * All temp variables generated by code spilling will be stored here
    */
    definition_pool spill_pool;
} optimizer;

definition* ref2def(const reference* r);
definition* ref2vardef(const reference* r);
size_t varmap_varid2idx(optimizer* om, const definition* variable);
size_t varmap_idx2varid(optimizer* om, size_t var_map_index);
size_t varmap_lid2idx(optimizer* om, size_t lid);
size_t varmap_idx2lid(optimizer* om, size_t var_map_index);
bool varmap_idx_is_member(optimizer* om, size_t var_map_index);
instruction* optimizer_phi_locate(basic_block* node, const definition* variable);
bool optimizer_phi_place(optimizer* om, basic_block* node, definition* variable);
void optimizer_populate_variables(optimizer* om);
void optimizer_populate_instructions(optimizer* om);
void optimizer_invalidate_variables(optimizer* om);
void optimizer_invalidate_instructions(optimizer* om);
void optimizer_profile_copy(optimizer* om, optimizer_profile* profile);
void optimizer_profile_apply(optimizer* om, optimizer_profile* profile);

void optimizer_defuse_analyze(optimizer* om);
void optimizer_liveness_analyze(optimizer* om);
void optimizer_ssa_build(optimizer* om);
void optimizer_ssa_eliminate(optimizer* om);
void optimizer_allocator_heuristic(optimizer* om, size_t num_avail_registers);
void optimizer_allocator_linear(optimizer* om, size_t num_avail_registers);

void init_optimizer(optimizer* om);
void release_optimizer(optimizer* om);
void optimizer_attach(optimizer* om, global_top_level* top_level, definition* target);
void optimizer_detach(optimizer* om);
void optimizer_execute(optimizer* om);

#endif
