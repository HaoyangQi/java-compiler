#include "optimizer.h"

/**
 * initialize optimizer for given code graph
*/
void init_optimizer(optimizer* om, cfg* code)
{
    if (!om) { return; }

    om->graph = code;

    if (!code) { return; }

    om->dominance.idom = cfg_idom(code);
    om->dominance.dom = cfg_dominators(code, om->dominance.idom);
    om->dominance.frontier = cfg_dominance_frontiers(code, om->dominance.idom);
}

/**
 * release optimizer
*/
void release_optimizer(optimizer* om)
{
    if (!om || !om->graph) { return; }

    cfg_delete_idom(om->dominance.idom);
    cfg_delete_dominators(om->graph, om->dominance.dom);
    cfg_delete_dominance_frontiers(om->graph, om->dominance.frontier);
}
