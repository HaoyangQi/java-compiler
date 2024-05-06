#pragma once
#ifndef __COMPILER_OPTIMIZER_H__
#define __COMPILER_OPTIMIZER_H__

#include "types.h"
#include "ir.h"

typedef struct _optimizer
{
    // graph reference
    cfg* graph;

    /**
     * Dominance Analysis
     *
     * postorder: node list in post-order, index is used in the algorithm
     * idom:      immediate dominator of every node, indexed by node's ID
     * frontier:  dominance frontier set of every node, indexed by node's ID
     *
     * according to definition: immediate dominator of node n, IDOM(n), has
     * exactly one node: for any node p, there does not exist node q, such that:
     * p DOM q DOM n, AND p DOM n
     *
     * To construct dominator set (as well as the dominator tree) from idom:
     *
     * chain access from target node using idom[idom[idom[...idom[n]]]] until
     * result reaches entry node
     * 1. and all elements accessed constructs dominator set of n
     * 2. the path constructs the path in dominator tree
     *
    */
    struct
    {
        basic_block** idom;
        index_set* dom;
        index_set* frontier;
    } dominance;
} optimizer;

void init_optimizer(optimizer* om, cfg* code);
void release_optimizer(optimizer* om);

#endif
