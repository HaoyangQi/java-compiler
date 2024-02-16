#include "ir.h"

/**
 * Initialize CFG worker
 *
*/
void init_cfg_worker(cfg_worker* worker)
{
    worker->graph = (cfg*)malloc_assert(sizeof(cfg));
    worker->cur_blk = NULL;
    worker->next_outbound_strategy = EDGE_ANY;

    init_cfg(worker->graph);
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
*/
void release_cfg_worker(cfg_worker* worker, cfg* move_to)
{
    if (move_to)
    {
        memcpy(move_to, worker->graph, sizeof(cfg));
        memset(worker->graph, 0, sizeof(cfg));
    }
    else
    {
        release_cfg(worker->graph);
    }

    free(worker->graph);
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
 * NOTE: do NOT use this method to insert between nodes
 * (and general insertion should not be necessary)
 *
 * IDIOM: since we are reading the code linearly, so the very first
 * block added IS the entry node of entire graph
*/
basic_block* cfg_worker_grow(cfg_worker* worker)
{
    basic_block* b = cfg_new_basic_block(worker->graph);

    if (worker->cur_blk)
    {
        cfg_new_edge(worker->graph, worker->cur_blk, b, worker->next_outbound_strategy);
    }
    else
    {
        // first block added IS the entry node
        worker->graph->entry = b;
    }

    // update state (outbound strategy is reset)
    worker->cur_blk = b;
    worker->next_outbound_strategy = EDGE_ANY;

    return b;
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
basic_block* cfg_worker_jump(cfg_worker* worker, basic_block* to, bool change_cur, bool edge)
{
    if (edge)
    {
        cfg_new_edge(worker->graph, worker->cur_blk, to, EDGE_JUMP);
    }

    if (change_cur)
    {
        worker->cur_blk = to;
    }

    return worker->cur_blk;
}

/**
 * connect two graphs
 *
 * just like cfg_worker_grow, it grows the graph from current
 * block, but instead of appending a new block, it appends
 * a new graph
*/
void cfg_worker_grow_with_graph(cfg_worker* dest, cfg_worker* src)
{
    cfg* dest_graph = dest->graph;
    cfg* src_graph = src->graph;

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

    cfg_new_edge(dest_graph, dest->cur_blk, src->graph->entry, dest->next_outbound_strategy);
    dest->cur_blk = src->cur_blk;
    dest->next_outbound_strategy = src->next_outbound_strategy;

    cfg_detach(src->graph);
}

/**
 * Tcode generation interface
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
    if (!instruction_push_back(block, inst))
    {
        delete_instruction(inst, false);
        return NULL;
    }

    // fill
    inst->op = irop;
    inst->lvalue = lvalue ? *lvalue : NULL;
    inst->operand_1 = operand_1 ? *operand_1 : NULL;
    inst->operand_2 = operand_2 ? *operand_2 : NULL;

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
    if (inst->lvalue && inst->lvalue->type == IR_ASN_REF_LITERAL)
    {
        ir_error(ir, JAVA_E_EXPRESSION_LITERAL_LVALUE);
    }

    /**
     * special worker state update
    */
    switch (irop)
    {
        case IROP_RET:
            /**
             * if it is a return statement:
             *
             * mark the node as an exit node, AND
             * grow the graph
            */
            block->type = BLOCK_EXIT;
            cfg_worker_grow(worker);
            break;
        case IROP_TEST:
            /**
             * if it is a logical test:
             *
             * mark the node as a test node
             * (parser should handle the graph grow
             * because ountbound can be many)
            */
            block->type = BLOCK_TEST;
            break;
        default:
            break;
    }

    return inst;
}
