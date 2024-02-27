/**
 * IR Back-End: Static Single Assignment Form & Optimization
 *
*/

#include "ir.h"

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
static void ir_ssa_compute_dominators(cfg* g)
{
    node_set all_nodes, work_list, new_set;
    basic_block* node;
    basic_block* probe;

    init_node_set(&all_nodes);
    init_node_set(&work_list);

    // get node set: the set contains all nodes
    for (size_t i = 0; i < g->nodes.num; i++)
    {
        node_set_add(&all_nodes, g->nodes.arr[i]);
    }

    // initialize all dominator set
    for (size_t i = 0; i < g->nodes.num; i++)
    {
        init_node_set_with_copy(&g->nodes.arr[i]->dominators, &all_nodes);
    }

    // initialize worklist
    node_set_add(&work_list, g->entry);

    // algorithm
    while (!node_set_empty(&work_list))
    {
        node = node_set_pop(&work_list);

        if (node->in.num == 0)
        {
            init_node_set(&new_set);
        }
        else
        {
            init_node_set_with_copy(&new_set, &node->in.arr[0]->from->dominators);

            // union all dominators of all predecessors
            for (size_t i = 1; i < node->in.num; i++)
            {
                node_set_intersect(&new_set, &node->in.arr[i]->from->dominators);
            }
        }

        node_set_add(&new_set, node);

        if (node_set_equal(&new_set, &node->dominators))
        {
            // do nothing
            release_node_set(&new_set);
        }
        else
        {
            // dominator of node will be new set
            release_node_set(&node->dominators);
            memcpy(&node->dominators, &new_set, sizeof(node_set));

            // add all successor of node to work list
            for (size_t j = 0; j < node->out.num; j++)
            {
                node_set_add(&work_list, node->out.arr[j]->to);
            }
        }
    }

    // cleanup
    release_node_set(&all_nodes);
    release_node_set(&work_list);
}

/**
 * test if a dominates b
*/
static bool ir_ssa_dominates(const basic_block* a, const basic_block* b)
{
    return node_set_contains(&b->dominators, a);
}

/**
 * test if a strictly dominates b
*/
static bool ir_ssa_strictly_dominates(const basic_block* a, const basic_block* b)
{
    return a != b && ir_ssa_dominates(a, b);
}

/**
 * Dominance Frontier Calculation
 *
*/
static void ir_ssa_compute_dominance_frontier(cfg* g)
{
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
            init_node_set_with_copy(&dom, &node->dominators);

            // for every n in dom(node)
            while (!node_set_empty(&dom))
            {
                n = node_set_pop(&dom);

                // if n not strictly dominates succ
                if (!ir_ssa_strictly_dominates(n, succ))
                {
                    // DF[n] += succ
                    node_set_add(&n->df, succ);
                }
            }

            release_node_set(&dom);
        }
    }
}

/**
 * SSA Builder Entry Point
*/
void ir_ssa_build(cfg_worker* worker)
{
    ir_ssa_compute_dominators(worker->graph);

    printf("all dominator sets:\n");
    for (size_t i = 0; i < worker->graph->nodes.num; i++)
    {
        printf("%zd: ", worker->graph->nodes.arr[i]->id);
        ir_ssa_node_set_println(&worker->graph->nodes.arr[i]->dominators);
    }

    ir_ssa_compute_dominance_frontier(worker->graph);

    printf("all DF sets:\n");
    for (size_t i = 0; i < worker->graph->nodes.num; i++)
    {
        printf("%zd: ", worker->graph->nodes.arr[i]->id);
        ir_ssa_node_set_println(&worker->graph->nodes.arr[i]->df);
    }
}
