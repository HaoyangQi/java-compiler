#include "optimizer.h"

/**
 * Liveness Analysis
 *
 * It will fill the following data in optimizer object:
 * 1. in
 * 2. out
 *
 * Fact: variables that live
 *
 * gen(n): use(n)
 * kill(n): def(n)
 * out(n): union in(m), m is every successor of n
 * in(n): (out(n) - kill(n)) union gen(n)
 *
 * NOTE:
 * This algorithm works based on assumption that makes sure
 * every node in CFG has at least one instruction. See
 * optimizer_attach() on how it fulfills this assumption
*/
void optimizer_liveness_analyze(optimizer* om)
{
    size_t num_nodes = om->profile.num_nodes;
    size_t idx;
    index_set worklist;

    for (size_t i = 0; i < om->profile.num_instructions; i++)
    {
        init_index_set(&om->instructions[i].in, om->profile.num_variables);
        init_index_set(&om->instructions[i].out, om->profile.num_variables);
    }

    // initialize worklist with all instructions
    init_index_set_fill(&worklist, om->profile.num_instructions, true);

    // main loop
    while (index_set_pop(&worklist, &idx))
    {
        basic_block* n;
        instruction* s = om->instructions[idx].ref;
        index_set* live_in = &om->instructions[idx].in;
        index_set* live_out = &om->instructions[idx].out;
        index_set old_in;

        // save copy of old in[s]
        init_index_set_copy(&old_in, live_in);

        // out(n) = union(in[p]), p = every successor of n
        index_set_clear(live_out);
        if (s->next)
        {
            index_set_union(live_out, &om->instructions[s->next->id].in);
        }
        else
        {
            n = s->node;

            for (size_t i = 0; i < n->out.num; i++)
            {
                index_set_union(live_out, &om->instructions[n->out.arr[i]->to->inst_first->id].in);
            }
        }

        // in(n) = (out(n) - kill(n)) union gen(n)
        index_set_clear(live_in);
        index_set_union(live_in, live_out);
        index_set_subtract(live_in, &om->instructions[s->id].def);
        index_set_union(live_in, &om->instructions[s->id].use);

        // muutate worklist
        if (!index_set_equal(live_in, &old_in))
        {
            // worklist = worklist union predecessor(n)
            if (s->prev)
            {
                index_set_add(&worklist, s->prev->id);
            }
            else
            {
                n = s->node;
                for (size_t i = 0; i < n->in.num; i++)
                {
                    index_set_add(&worklist, n->in.arr[i]->from->inst_last->id);
                }
            }
        }

        // cleanup
        release_index_set(&old_in);
    }
}
