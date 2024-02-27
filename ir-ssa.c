/**
 * IR Back-End: Static Single Assignment Form & Optimization
 *
 * methods here converts CFG into SSA form for further
 * analysis and optimization
 *
*/

#include "ir.h"

#define __ssa_node_dom(worker, node) (&(worker)->optimizer[(node)->id].dominators)

void ir_ssa_node_set_println(const node_set* s)
{
    hash_pair* b;

    for (size_t index = 0; index < s->tbl.bucket_size; index++)
    {
        b = s->tbl.bucket[index];

        while (b)
        {
            // use ((basic_block*)b->value)->id to check node id
            printf("%zd ", *((size_t*)b->key));
            b = b->next;
        }
    }

    printf("\n");
}

/**
 * Dominator set calculation: Worklist Algorithm
 *
 * in CFG, node d dominates n if following condition is met:
 *
 * for every path from entry node to n, they must all go
 * through d
*/
static void ir_ssa_compute_dominators(cfg_worker* worker)
{
    cfg* g = worker->graph;
    node_set work_list, new_set;
    node_set* node_dom;
    basic_block* node;
    basic_block* probe;

    // initialize worklist
    init_node_set(&work_list);
    node_set_add(&work_list, g->entry);

    // algorithm
    while (!node_set_empty(&work_list))
    {
        node = node_set_pop(&work_list);
        node_dom = __ssa_node_dom(worker, node);

        if (node->in.num == 0)
        {
            init_node_set(&new_set);
        }
        else
        {
            init_node_set_with_copy(&new_set, __ssa_node_dom(worker, node->in.arr[0]->from));

            // union all dominators of all predecessors
            for (size_t i = 1; i < node->in.num; i++)
            {
                node_set_intersect(&new_set, __ssa_node_dom(worker, node->in.arr[i]->from));
            }
        }

        node_set_add(&new_set, node);

        if (node_set_equal(&new_set, node_dom))
        {
            // do nothing
            release_node_set(&new_set);
        }
        else
        {
            // dominator of node will be new set
            release_node_set(node_dom);
            memcpy(node_dom, &new_set, sizeof(node_set));

            // add all successor of node to work list
            for (size_t j = 0; j < node->out.num; j++)
            {
                node_set_add(&work_list, node->out.arr[j]->to);
            }
        }
    }

    // cleanup
    release_node_set(&work_list);
}

/**
 * test if a dominates b
*/
static bool ir_ssa_dominates(const cfg_worker* worker, const basic_block* a, const basic_block* b)
{
    return node_set_contains(__ssa_node_dom(worker, b), a);
}

/**
 * test if a strictly dominates b
*/
static bool ir_ssa_strictly_dominates(const cfg_worker* worker, const basic_block* a, const basic_block* b)
{
    return a != b && ir_ssa_dominates(worker, a, b);
}

/**
 * Dominance Frontier Calculation
 *
*/
static void ir_ssa_compute_dominance_frontier(cfg_worker* worker)
{
    cfg* g = worker->graph;
    basic_block* node;
    basic_block* succ;
    basic_block* n;
    node_set dom;

    for (size_t i = 0; i < g->nodes.num; i++)
    {
        node = g->nodes.arr[i];

        // for every edge node->succ
        for (size_t j = 0; j < node->out.num; j++)
        {
            succ = node->out.arr[j]->to;
            init_node_set_with_copy(&dom, __ssa_node_dom(worker, node));

            // for every n in dom(node)
            while (!node_set_empty(&dom))
            {
                n = node_set_pop(&dom);

                // if n not strictly dominates succ
                if (!ir_ssa_strictly_dominates(worker, n, succ))
                {
                    // DF[n] += succ
                    node_set_add(&worker->optimizer[n->id].df, succ);
                }
            }

            release_node_set(&dom);
        }
    }
}

/**
 * Optimizer Initializer
 *
 * this interface is not exposed because cfg_worker_ssa_build will handle this
*/
static void ir_ssa_init(cfg_worker* worker)
{
    cfg* g = worker->graph;
    size_t num_nodes = g->nodes.num;
    node_set all_nodes;

    worker->optimizer = (ssa*)malloc_assert(sizeof(ssa) * num_nodes);
    init_node_set(&all_nodes);

    // get node set: the set contains all nodes
    for (size_t i = 0; i < g->nodes.num; i++)
    {
        node_set_add(&all_nodes, g->nodes.arr[i]);
    }

    // initialize optimizer of every node
    for (size_t i = 0; i < num_nodes; i++)
    {
        worker->optimizer[i].node = g->nodes.arr[i];
        init_node_set_with_copy(&worker->optimizer[i].dominators, &all_nodes);
        init_node_set(&worker->optimizer[i].df);
    }

    // cleanup
    release_node_set(&all_nodes);
}

void cfg_worker_ssa_release(cfg_worker* worker)
{
    if (!worker->optimizer) { return; }

    for (size_t i = 0; i < worker->graph->nodes.num; i++)
    {
        release_node_set(&worker->optimizer[i].dominators);
        release_node_set(&worker->optimizer[i].df);
    }

    free(worker->optimizer);
}

/**
 * SSA Builder Entry Point
*/
void cfg_worker_ssa_build(cfg_worker* worker)
{
    ir_ssa_init(worker);
    ir_ssa_compute_dominators(worker);

    printf("all dominator sets:\n");
    for (size_t i = 0; i < worker->graph->nodes.num; i++)
    {
        printf("%zd: ", worker->optimizer[i].node->id);
        ir_ssa_node_set_println(&worker->optimizer[i].dominators);
    }

    ir_ssa_compute_dominance_frontier(worker);

    printf("all DF sets:\n");
    for (size_t i = 0; i < worker->graph->nodes.num; i++)
    {
        printf("%zd: ", worker->optimizer[i].node->id);
        ir_ssa_node_set_println(&worker->optimizer[i].df);
    }
}
