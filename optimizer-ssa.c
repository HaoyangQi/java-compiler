#include "optimizer.h"

/**
 * Variable version stack used during variable renaming
*/
typedef struct _variable_version_stack
{
    size_t version;
    instruction* source;
    struct _variable_version_stack* next;
} variable_version_stack;

/**
 * this struct contains some static memory chunk and info accquired by routines
 * multiple times
*/
typedef struct
{
    size_t num_nodes;

    /**
     * PHI placement data
    */
    struct
    {
        size_t sz_flags;
        byte* inserted;
        byte* added;
    } phi;

    /**
     * Variable renaming data
    */
    struct
    {
        variable_version_stack** variable;
    } rename;
} ssa_builder;

static void ssa_builder_init(optimizer* om, ssa_builder* builder)
{
    size_t sz_version_stack = sizeof(variable_version_stack*) * (om->profile.num_variables);

    builder->num_nodes = om->graph->nodes.num;
    builder->phi.sz_flags = sizeof(bool) * builder->num_nodes;
    builder->phi.inserted = (byte*)malloc_assert(builder->phi.sz_flags);
    builder->phi.added = (byte*)malloc_assert(builder->phi.sz_flags);
    builder->rename.variable = (variable_version_stack**)malloc_assert(sz_version_stack);

    memset(builder->rename.variable, 0, sz_version_stack);

    /**
     * rename version for members
     *
     * members are not defined in CFG scope, but they are valid, default to version 0
    */
    for (size_t i = 0; i < om->profile.num_members; i++)
    {
        builder->rename.variable[i] = (variable_version_stack*)malloc_assert(sizeof(variable_version_stack));
        builder->rename.variable[i]->version = 0;
        builder->rename.variable[i]->source = NULL;
        builder->rename.variable[i]->next = NULL;
    }
}

static void ssa_builder_release(optimizer* om, ssa_builder* builder)
{
    free(builder->phi.inserted);
    free(builder->phi.added);

    for (size_t i = 0; i < om->profile.num_variables; i++)
    {
        variable_version_stack* v = builder->rename.variable[i];
        variable_version_stack* next = NULL;

        while (v)
        {
            next = v->next;
            free(v);
            v = next;
        }
    }

    free(builder->rename.variable);
}

static size_t ssa_builder_generate_variable_version(
    optimizer* om,
    ssa_builder* builder,
    const definition* variable,
    instruction* source
)
{
    size_t v = varmap_varid2idx(om, variable);
    variable_version_stack* frame = (variable_version_stack*)malloc_assert(sizeof(variable_version_stack));
    size_t ver = builder->rename.variable[v]->version;

    if (builder->rename.variable[v])
    {
        frame->version = ver + 1;
    }
    else
    {
        frame->version = 0;
    }

    frame->source = source;
    frame->next = builder->rename.variable[v];
    builder->rename.variable[v] = frame;

    return ver;
}

static size_t ssa_builder_get_variable_version(optimizer* om, ssa_builder* builder, const definition* variable)
{
    if (!variable) { return 0; }

    variable_version_stack* top = builder->rename.variable[varmap_varid2idx(om, variable)];
    return top ? top->version : 0;
}

static instruction* ssa_builder_get_variable_source(optimizer* om, ssa_builder* builder, const definition* variable)
{
    if (!variable) { return 0; }

    variable_version_stack* top = builder->rename.variable[varmap_varid2idx(om, variable)];
    return top ? top->source : NULL;
}

static void ssa_builder_pop_variable_version(optimizer* om, ssa_builder* builder, const definition* variable)
{
    if (!variable) { return; }

    size_t v = varmap_varid2idx(om, variable);
    variable_version_stack* top = builder->rename.variable[v];

    builder->rename.variable[v] = top->next;
    free(top);
}

/**
 * insert phi instruction for given variable
*/
static void optimizer_ssa_place_phi(
    optimizer* om,
    ssa_builder* builder,
    definition* variable,
    const index_set* df
)
{
    if (!variable) { return; }

    index_set worklist;
    index_set_iterator itor_set;
    size_t n;
    basic_block* node;

    init_index_set(&worklist, builder->num_nodes);

    // reset
    memset(builder->phi.inserted, 0, builder->phi.sz_flags);
    memset(builder->phi.added, 0, builder->phi.sz_flags);
    index_set_clear(&worklist);

    // initialize
    for (n = 0; n < builder->num_nodes; n++)
    {
        node = om->graph->nodes.arr[n];
        instruction* p = node->inst_first;

        while (p)
        {
            if (p->lvalue && p->lvalue->def == variable)
            {
                index_set_add(&worklist, node->id);
                builder->phi.added[node->id] = 1;
                break;
            }

            p = p->next;
        }
    }

    // main loop
    while (index_set_pop(&worklist, &n))
    {
        index_set_iterator_init(&itor_set, (index_set*)(&df[n]));

        while (!index_set_iterator_end(&itor_set))
        {
            n = index_set_iterator_get(&itor_set);
            node = om->graph->nodes.arr[n];

            if (!builder->phi.inserted[n])
            {
                optimizer_phi_place(node, variable, om->profile.num_instructions);
                builder->phi.inserted[n] = 1;
                om->profile.num_instructions++;

                // mutate worklist
                if (!builder->phi.added[n])
                {
                    builder->phi.added[n] = 1;
                    index_set_add(&worklist, n);
                }
            }

            index_set_iterator_next(&itor_set);
        }

        // cleanup
        index_set_iterator_release(&itor_set);
    }
}

/**
 * Rename Variables
 *
 * Iterative DFS Walk
*/
static void optimizer_ssa_rename_variable(optimizer* om, ssa_builder* builder)
{
    size_t num_nodes = om->graph->nodes.num;
    size_t snti = 0; // stack's next-top idx
    size_t rnti = 0; // result's next idx
    basic_block** stack = (basic_block**)malloc_assert(sizeof(basic_block*) * num_nodes);
    size_t* nc = (size_t*)malloc_assert(sizeof(size_t) * num_nodes); // next-child-to-visit index
    char* visited = (char*)malloc_assert(sizeof(char) * num_nodes);

    // mark all as not visited at the beginning
    memset(visited, 0, sizeof(char) * num_nodes);
    memset(nc, 0, sizeof(size_t) * num_nodes);

    // initialize: from entry node
    stack[snti++] = om->graph->entry;
    visited[om->graph->entry->id] = 1;

    // main loop
    while (snti != 0)
    {
        // read current top
        basic_block* cur = stack[snti - 1];

        /**
         * Preorder: name generation on current node
        */
        for (instruction* i = cur->inst_first; i != NULL; i = i->next)
        {
            // order matters here: RHS then LHS
            i->operand_1->ver = ssa_builder_get_variable_version(om, builder, ref2vardef(i->operand_1));
            i->operand_2->ver = ssa_builder_get_variable_version(om, builder, ref2vardef(i->operand_2));
            i->lvalue->ver = ssa_builder_generate_variable_version(om, builder, ref2vardef(i->lvalue), i);
        }

        /**
         * Preorder: PHI argument insertion on successor nodes
        */
        for (size_t i = 0; i < cur->out.num; i++)
        {
            size_t phi_index = cur->out.arr[i]->to_phi_operand_index;

            for (instruction* sp = cur->out.arr[i]->to->inst_first; sp->op == IROP_PHI; sp = sp->next)
            {
                optimizer_phi_set_operand(
                    cur, phi_index, ssa_builder_get_variable_source(om, builder, ref2vardef(sp->lvalue)));
            }
        }

        // locate next-to-visit child
        for (size_t* pnc = &nc[cur->id]; *pnc < cur->out.num && visited[cur->out.arr[*pnc]->to->id]; (*pnc)++);

        // traverse next
        if (nc[cur->id] >= cur->out.num)
        {
            /**
             * pop all versions defined in this node
             *
             * popping from the last instruction is more semantically correct, but
             * logically speaking there is no difference
            */
            for (instruction* i = cur->inst_first; i != NULL; i = i->next)
            {
                ssa_builder_pop_variable_version(om, builder, ref2def(i->lvalue));
            }

            // pop: when all children are visited
            snti--;
        }
        else
        {
            // push: next un-visited element
            basic_block* next = cur->out.arr[nc[cur->id]]->to;

            stack[snti++] = next;
            visited[next->id] = 1;
            nc[cur->id]++;
        }
    }

    // cleanup
    free(nc);
    free(stack);
    free(visited);
}

/**
 * Convert CFG To SSA
 *
 * 1. Compute DF
 * 2. Place PHI
 * 3. Rename variables
*/
void optimizer_ssa_build(optimizer* om)
{
    ssa_builder builder;
    basic_block** idom = cfg_idom(om->graph, om->node_postorder);
    index_set* df = cfg_dominance_frontiers(om->graph, idom);

    // initialize builder
    ssa_builder_init(om, &builder);

    // phi placement for every variable
    for (size_t v = 0; v < om->profile.num_variables; v++)
    {
        optimizer_ssa_place_phi(om, &builder, om->variables[v].ref, df);
    }

    // rename variable for each node
    optimizer_ssa_rename_variable(om, &builder);

    // cleanup
    ssa_builder_release(om, &builder);
    cfg_delete_idom(idom);
    cfg_delete_dominance_frontiers(om->graph, df);
}

/**
 * Eliminate SSA instructions
 *
 * The CFG will be cleaned by eliminating all PHI instructions
*/
void optimizer_ssa_eliminate(optimizer* om)
{
    size_t num_nodes = om->graph->nodes.num;

    for (size_t n = 0; n < num_nodes; n++)
    {
        basic_block* b = om->graph->nodes.arr[n];
        instruction* p = b->inst_first;

        while (p && p->op == IROP_PHI)
        {
            delete_instruction(instruction_pop_front(b), true);
            p = b->inst_first;
        }
    }
}
