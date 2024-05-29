#include "optimizer.h"

/**
 * Def-Use Analysis
 *
 * It will fill the following data in optimizer object:
 * 1. def
 * 2. use
 *
 * it looks trivial, but it implies the following:
 * it does not count phi's operands
 *
 * the fact above is necessary for liveness analysis on SSA,
 * liveness does not have to analyze phi's operands
*/
void optimizer_defuse_analyze(optimizer* om)
{
    size_t num_nodes = om->graph->nodes.num;

    for (size_t i = 0; i < om->profile.num_instructions; i++)
    {
        init_index_set(&om->instructions[i].def, om->profile.num_variables);
        init_index_set(&om->instructions[i].use, om->profile.num_variables);
    }

    for (size_t i = 0; i < num_nodes; i++)
    {
        basic_block* b = om->node_postorder[num_nodes - i - 1];

        for (instruction* p = b->inst_first; p != NULL; p = p->next)
        {
            definition* d;

            d = ref2vardef(p->lvalue);
            if (d) { index_set_add(&om->instructions[p->id].def, varmap_varid2idx(om, d)); }

            d = ref2vardef(p->operand_1);
            if (d) { index_set_add(&om->instructions[p->id].use, varmap_varid2idx(om, d)); }

            d = ref2vardef(p->operand_2);
            if (d) { index_set_add(&om->instructions[p->id].use, varmap_varid2idx(om, d)); }
        }
    }
}
