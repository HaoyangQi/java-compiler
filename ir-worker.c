#include "ir.h"

/**
 * Initialize CFG worker
 *
 * next_outbound_strategy: upon next grow, the connect edge will be set
 *                         to this type
 * execute_inverse: when set, instruction will stack from start of the block
 * grow_insert: upon next grow, the outbound info will be moved to the
 *              new block, while inbound stays as-is
 *
 * NOTE: do NOT initialize optimizer here because it needs a complete CFG
 *
*/
void init_cfg_worker(cfg_worker* worker)
{
    worker->graph = (cfg*)malloc_assert(sizeof(cfg));
    worker->cur_blk = NULL;
    worker->next_outbound_strategy = EDGE_ANY;
    worker->execute_inverse = false;
    worker->grow_insert = false;
    worker->is_next_asn_init = false;
    worker->optimizer = NULL;

    init_cfg(worker->graph);
    init_definition_pool(&worker->variables);
}

/**
 * Release CFG worker
 *
 * if a move target is provided, the CFG will be moved
 *
 * NOTE: make sure the move target is not initialized
 * whatsoever because the target will become the worker
 * graph
 *
 * when moving, cfg_detach on the worker graph is not
 * sufficient because of node and edge array, so we
 * simply make move target become the worker graph,
 * and wipe out worker graph data completely
 *
 * merge_to_pool: if set, the definition pool will be
 * merged into an initialized pool, otherwise it will
 * be released
*/
void release_cfg_worker(cfg_worker* worker, cfg* move_to, definition_pool* merge_to_pool)
{
    if (!worker) { return; }

    if (move_to)
    {
        memcpy(move_to, worker->graph, sizeof(cfg));
        memset(worker->graph, 0, sizeof(cfg));
    }
    else
    {
        release_cfg(worker->graph);
    }

    if (merge_to_pool)
    {
        definition_pool_merge(merge_to_pool, &worker->variables);
    }

    cfg_worker_ssa_release(worker);
    free(worker->graph);
    release_definition_pool(&worker->variables);
}

/**
 * get current block
*/
basic_block* cfg_worker_current_block(cfg_worker* worker)
{
    return worker->cur_blk;
}

/**
 * start new block
 *
 * the new block will be connected to current block, once
 * connect, next outbound strategy will be set to default
 *
 * IDIOM: since we are reading the code linearly, so the very first
 * block added IS the entry node of entire graph
*/
basic_block* cfg_worker_grow(cfg_worker* worker)
{
    basic_block* b = cfg_new_basic_block(worker->graph);

    // if we are inserting, move outbound info
    if (worker->cur_blk && worker->grow_insert)
    {
        memcpy(&b->out, &worker->cur_blk->out, sizeof(edge_array));

        // detach
        worker->cur_blk->out.arr = NULL;
        worker->cur_blk->out.num = 0;
        worker->cur_blk->out.size = 0;
    }

    if (worker->cur_blk)
    {
        cfg_new_edge(worker->graph, worker->cur_blk, b, worker->next_outbound_strategy);
    }
    else
    {
        // first block added IS the entry node
        worker->graph->entry = b;
    }

    // update state
    worker->cur_blk = b;
    worker->next_outbound_strategy = EDGE_ANY;
    worker->grow_insert = false;

    return b;
}

/**
 * mark next outbound strategy of current block
*/
void cfg_worker_next_outbound_strategy(cfg_worker* worker, edge_type type)
{
    worker->next_outbound_strategy = type;
}

/**
 * mark next instruction appending location as "push front"
*/
void cfg_worker_execution_strategy(cfg_worker* worker, bool inverse)
{
    worker->execute_inverse = inverse;
}

/**
 * mark next grow strategy
*/
void cfg_worker_next_grow_strategy(cfg_worker* worker, bool insert)
{
    worker->grow_insert = insert;
}

/**
 * mark next IROP_ASN strategy
*/
void cfg_worker_next_asn_strategy(cfg_worker* worker, bool is_init)
{
    worker->is_next_asn_init = is_init;
}

/**
 * set current block type
*/
void cfg_worker_set_current_block_type(cfg_worker* worker, block_type t)
{
    worker->cur_blk->type = t;
}

/**
 * jump from current to a destination
 *
 * change_cur: if true, current block trancker will be changed
 * edge: if true, an edge will be created
 *
 * NOTE: again, make sure you are using right data
 * otherwise the method will behave as messy as parameters
*/
void cfg_worker_jump(cfg_worker* worker, basic_block* to, bool change_cur, bool edge)
{
    if (edge)
    {
        cfg_new_edge(worker->graph, worker->cur_blk, to, worker->next_outbound_strategy);
        worker->next_outbound_strategy = EDGE_ANY;
    }

    if (change_cur)
    {
        worker->cur_blk = to;
    }
}

/**
 * connect two graphs
 *
 * just like cfg_worker_grow, it grows the graph from current
 * block, but instead of appending a new block, it appends
 * a new graph
 *
 * NOTE:
 * once merged, src worker will be released AND deleted
 * so do NOT use stack variable as source worker
*/
void cfg_worker_grow_with_graph(cfg_worker* dest, cfg_worker* src)
{
    if (!src) { return; }

    cfg* dest_graph = dest->graph;
    cfg* src_graph = src->graph;

    // only do heavy work if graph is not empty
    // now an entry must be determined otherwise it will crash purposefully
    if (!cfg_empty(src->graph))
    {
        // array resizing
        node_array_resize(&dest_graph->nodes, src_graph->nodes.num);
        edge_array_resize(&dest_graph->edges, src_graph->edges.num);

        // merge array
        memcpy(
            dest_graph->nodes.arr + dest_graph->nodes.num,
            src_graph->nodes.arr,
            src_graph->nodes.num * sizeof(basic_block*)
        );
        memcpy(
            dest_graph->edges.arr + dest_graph->edges.num,
            src_graph->edges.arr,
            src_graph->edges.num * sizeof(cfg_edge*)
        );

        // update node id
        for (size_t id = dest_graph->nodes.num, i = 0; i < src_graph->nodes.num; id++, i++)
        {
            src_graph->nodes.arr[i]->id = id;
        }

        // update counter
        dest_graph->nodes.num += src_graph->nodes.num;
        dest_graph->edges.num += src_graph->edges.num;

        // order matters here
        if (!dest_graph->entry)
        {
            // otherwise it means we have an empty graph, so transfer header info
            dest_graph->entry = src->graph->entry;
        }
        else if (dest->cur_blk)
        {
            // connect, if we have to
            cfg_new_edge(dest_graph, dest->cur_blk, src->graph->entry, dest->next_outbound_strategy);
        }

        dest->cur_blk = src->cur_blk;
        dest->next_outbound_strategy = src->next_outbound_strategy;
    }

    // cleanup
    cfg_detach(src->graph);
    release_cfg_worker(src, NULL, &dest->variables);
    free(src);
}

/**
 * code generation interface
 *
 * all operands will be moved, and stay as-is if failed
 *
 * TODO: specially process following:
 * 1. ternary operator(? :)
 * 2. short-circuit for logical operators
*/
instruction* cfg_worker_execute(
    java_ir* ir,
    cfg_worker* worker,
    operation irop,
    reference** lvalue,
    reference** operand_1,
    reference** operand_2
)
{
    instruction* inst = new_instruction();
    basic_block* block = worker->cur_blk;

    if (!block)
    {
        block = cfg_worker_grow(worker);
    }

    // prepare the instruction first
    // so we can delete instance without touching any operands
    if (!(worker->execute_inverse ? instruction_push_front(block, inst) : instruction_push_back(block, inst)))
    {
        delete_instruction(inst, false);
        return NULL;
    }

    // fill
    inst->op = irop;
    inst->lvalue = lvalue ? *lvalue : NULL;
    inst->operand_1 = operand_1 ? *operand_1 : NULL;
    inst->operand_2 = operand_2 ? *operand_2 : NULL;
    inst->node = block;

    // detach
    if (lvalue) { *lvalue = NULL; }
    if (operand_1) { *operand_1 = NULL; }
    if (operand_2) { *operand_2 = NULL; }

    /**
     * validate lvalue
     *
     * NOTE: validate what we can here, leave the rest
     * for further steps
    */
    if (inst->lvalue && inst->lvalue->type != IR_ASN_REF_DEFINITION)
    {
        ir_error(ir, JAVA_E_EXPRESSION_LITERAL_LVALUE);
    }

    /**
     * def-use info update
     *
     * NOTE:
     * irop == IROP_ASN here is not sufficient, because any operator
     * can implicitly contain an assignment if lvalue is set
    */
    if (!worker->is_next_asn_init && inst->lvalue && irop != IROP_INIT)
    {
        inst->lvalue->ver = ++((definition*)inst->lvalue->doi)->def_count;
    }

    /**
     * special worker state update
     *
     * NOTE: do NOT grow node here -- let parser do it
     * because valid code dtructure will scope them correctly
     * so that return/break/continue is the last operation in
     * current block
     *
     * and those are not valid can stay as-is so that we can
     * use this context to issue warning and skip statements
     * that will never executed
    */
    switch (irop)
    {
        case IROP_RET:
            block->type = BLOCK_RETURN;
            break;
        case IROP_TEST:
            block->type = BLOCK_TEST;
            break;
        default:
            break;
    }

    worker->is_next_asn_init = false;
    return inst;
}

/**
 * undo last execution
*/
instruction* cfg_worker_undo_last_execution(java_ir* ir, cfg_worker* worker)
{
    basic_block* block = worker->cur_blk;
    instruction* inst = instruction_pop_back(block);

    if (!inst) { return NULL; }

    inst->node = NULL;

    // ill-formed node will not recover secondary type
    switch (inst->op)
    {
        case IROP_RET:
        case IROP_TEST:
            block->type = BLOCK_ANY;
            break;
        default:
            break;
    }

    return inst;
}

/**
 * test if current block is empty
 *
 * if graph is empty, then it also return true
*/
bool cfg_worker_current_block_empty(const cfg_worker* worker)
{
    return !(worker->cur_blk && worker->cur_blk->inst_first);
}

/**
 * split current block at a specific instruction
 *
 * this method will always generate a new node and insert it after current block
 *
 * "at" can be NULL, which is equivalent to adding a block in front of current
 * one, and in this case
 *
 * it returns the predecessor, remainder will become current block
*/
basic_block* cfg_worker_current_block_split(
    java_ir* ir,
    cfg_worker* worker,
    instruction* at,
    edge_type to_remainder,
    bool split_before
)
{
    // go to target block
    if (at)
    {
        cfg_worker_jump(worker, at->node, true, false);
    }

    basic_block* test = worker->cur_blk;
    basic_block* cur;

    // if split before, change it into "split after"
    if (at && split_before)
    {
        at = at->prev;
    }

    cfg_worker_next_outbound_strategy(worker, to_remainder);
    cfg_worker_next_grow_strategy(worker, true);
    cur = cfg_worker_grow(worker);

    // split instruction list
    if (!at)
    {
        // new node has everything
        cur->inst_first = test->inst_first;
        cur->inst_last = test->inst_last;
        test->inst_first = NULL;
        test->inst_last = NULL;
    }
    else if (!at->next)
    {
        // new node has nothing
        cur->inst_first = NULL;
        cur->inst_last = NULL;
    }
    else
    {
        // split in the middle
        cur->inst_first = at->next;
        cur->inst_last = test->inst_last;
        cur->inst_first->prev = NULL;
        test->inst_last = at;
        test->inst_last->next = NULL;
    }

    // update ownership
    at = cur->inst_first;
    while (at)
    {
        at->node = cur;
        at = at->next;
    }

    return test;
}

/**
 * Logical Precedence Expansion
 *
 * It re-parses a block and expand following IROP :
 * IROP_LAND
 * IROP_LOR
 * IROP_TC
 * IROP_TB
 *
 * logical connector AND/OR uses short-circuit
 * control flow to operate
 *
 * it stops at the exit node
 *
*/
void cfg_worker_expand_logical_precedence(java_ir* ir, cfg_worker* worker)
{
    basic_block* test;
    instruction* inst_phi;
    instruction* inst_op1;
    instruction* inst_op2;
    edge_type edge_continue, edge_continue_inv;

    if (!worker->cur_blk) { return; }

    // scan top-down
    inst_phi = worker->cur_blk->inst_first;

    /**
     * Logical Connector Implementation
     *
     * on top level, it normalizes the expression stack into following
     * format (top(entry)-down):
     *
     * OP1_operand_left_inst
     * OP1_operand_right_inst
     * OP1
     * OP2_operand_left_inst
     * OP2_operand_right_inst
     * OP2
     * ...
     *
     * if operand is not an instruction, a STORE will be inserted
     *
     * the logical structure is like following:
     *
     * A && B
     *                             A         <--- test node
     *                            / \
     *                          F/   \T
     *                          /     \
     *      exit node --->    PHI <--- B     <--- continue node
     *
     * A || B
     *                             A         <--- test node
     *                            / \
     *                          T/   \F
     *                          /     \
     *      exit node --->    PHI <--- B     <--- continue node
     *
     * we do not care edge type from B to PHI because
     * PHI node must use the value from B if it is
     * triggered
     *
    */
    while (inst_phi)
    {
        // A -> B edge type
        switch (inst_phi->op)
        {
            case IROP_LAND:
                edge_continue = EDGE_TRUE;
                edge_continue_inv = EDGE_FALSE;
                break;
            case IROP_LOR:
                edge_continue = EDGE_FALSE;
                edge_continue_inv = EDGE_TRUE;
                break;
            default:
                inst_phi = inst_phi->next;
                continue;
        }

        /**
         * we need to put current PHI in new node, because
         * PHI always implies a new convergence
        */
        test = cfg_worker_current_block_split(ir, worker, inst_phi, EDGE_ANY, true);

        inst_op1 = inst_phi->operand_1->type == IR_ASN_REF_INSTRUCTION ? inst_phi->operand_1->doi : NULL;
        inst_op2 = inst_phi->operand_2->type == IR_ASN_REF_INSTRUCTION ? inst_phi->operand_2->doi : NULL;

        /**
         * STORE operand 1
         *
         * it is a bit tricky here...
         * in test node, the instruction stack may look like the following:
         *
         * other code
         * [ insertion point ]
         * OPERAND_2 code
         *
         * so we need to locate start of operand 2 (if applicable)
        */
        if (!inst_op1)
        {
            cfg_worker_jump(worker, test, true, false);

            // if no operand 2 code, simply append
            inst_op1 = cfg_worker_execute(ir, worker, IROP_STORE, NULL, &inst_phi->operand_1, NULL);

            if (inst_op2)
            {
                // if operand 2 has code, locate the start and insert before
                cfg_worker_undo_last_execution(ir, worker);
                instruction_insert(test, instruction_locate_enclosure_start(inst_op2)->prev, inst_op1);
            }
        }

        // continue branch after operand 1
        cfg_worker_current_block_split(ir, worker, inst_op1, edge_continue, false);

        // STORE operand 2
        if (!inst_op2)
        {
            inst_op2 = cfg_worker_execute(ir, worker, IROP_STORE, NULL, &inst_phi->operand_2, NULL);
        }

        /**
         * Branch code for operand 1
        */
        cfg_worker_jump(worker, inst_op1->node, true, false);
        cfg_worker_execute(ir, worker, IROP_TEST, NULL, NULL, NULL);
        cfg_worker_next_outbound_strategy(worker, edge_continue_inv);
        cfg_worker_jump(worker, inst_phi->node, true, true);

        /**
         * mutate instruction into PHI
        */
        inst_phi->op = IROP_PHI;
        if (!inst_phi->operand_1)
        {
            inst_phi->operand_1 = new_reference(IR_ASN_REF_INSTRUCTION, inst_op1);
        }
        if (!inst_phi->operand_2)
        {
            inst_phi->operand_2 = new_reference(IR_ASN_REF_INSTRUCTION, inst_op2);
        }

        inst_phi = inst_phi->next;
    }
}
