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
 * Node Stack
 *
 * used for node-based tree walk
*/
typedef struct _node_walk_stack
{
    basic_block* ref;
    struct _node_walk_stack* next;
} node_walk_stack;

/**
 * Variable-Specific Data
 *
 * version_counter only increments in algorithm, meaning
 * it incremnts when new version number is generated,
 * but when popping version stack, this counter will
 * not change, by design
*/
typedef struct _variable_ssa_info
{
    // version stack
    variable_version_stack* vvs;
    // version counter
    size_t version_counter;
    // blocks that contain definition of each variable
    index_set blocks;
} variable_ssa_info;

/**
 * this struct contains some static memory chunk and info accquired by routines
 * multiple times
*/
typedef struct
{
    size_t num_nodes;

    // Globals set, candidates for PHI placement
    index_set globals;

    // Variable Data
    variable_ssa_info* variables;
} ssa_builder;

static node_walk_stack* node_stack_push(node_walk_stack* s, basic_block* bb)
{
    node_walk_stack* top = (node_walk_stack*)malloc_assert(sizeof(node_walk_stack));

    top->ref = bb;
    top->next = s;

    return top;
}

static node_walk_stack* node_stack_pop(node_walk_stack* s)
{
    if (!s) { return NULL; }

    node_walk_stack* top = s;

    s = s->next;

    free(top);
    return s;
}

static basic_block* node_stack_top(node_walk_stack* s)
{
    return s ? s->ref : NULL;
}

static void ssa_builder_init(optimizer* om, ssa_builder* builder)
{
    /**
     * Optimizer Context Init
     *
     * instruction array is nullified due to PHI code injection
    */
    optimizer_invalidate_instructions(om);
    optimizer_invalidate_variables(om);
    optimizer_populate_variables(om);

    builder->num_nodes = om->profile.num_nodes;
    builder->variables = (variable_ssa_info*)malloc_assert(sizeof(variable_ssa_info) * om->profile.num_variables);

    init_index_set(&builder->globals, om->profile.num_variables);

    for (size_t i = 0; i < om->profile.num_variables; i++)
    {
        variable_ssa_info* info = &builder->variables[i];

        // ignore all temporary variables
        if (!is_def_user_defined_variable(om->variables[i].ref))
        {
            memset(info, 0, sizeof(variable_ssa_info));
            continue;
        }

        // version stack initialize
        if (varmap_idx_is_member(om, i))
        {
            /**
             * member are firstly defined outside, so it always maintain a valid definition upon entry
             *
             * NOTE:
             * there is a weird exception: the source of member version 0 is NULL
             * we cannot use member init code ref here, because that is NOT immediate assignment
             * context upon code entry (well, not always)
             *
             * so, it is left NULL on purpose, combined with lvalue of the PHI being a member variable,
             * implies that a "null" operand in PHI means very first value of the member upon entry
            */
            info->vvs = (variable_version_stack*)malloc_assert(sizeof(variable_version_stack));
            info->vvs->version = 0;
            info->vvs->source = NULL;
            info->vvs->next = NULL;
            info->version_counter = 1;
        }
        else
        {
            info->vvs = NULL;
            info->version_counter = 0;
        }

        init_index_set(&info->blocks, builder->num_nodes);
    }
}

static void ssa_builder_release(optimizer* om, ssa_builder* builder)
{
    for (size_t i = 0; i < om->profile.num_variables; i++)
    {
        variable_ssa_info* info = &builder->variables[i];
        variable_version_stack* v = info->vvs;
        variable_version_stack* next = NULL;

        while (v)
        {
            next = v->next;
            free(v);
            v = next;
        }

        release_index_set(&info->blocks);
    }

    release_index_set(&builder->globals);
    free(builder->variables);
}

static variable_ssa_info* ssa_get_variable_info(optimizer* om, ssa_builder* builder, const definition* variable)
{
    return &builder->variables[varmap_varid2idx(om, variable)];
}

static void ssa_builder_generate_variable_version(
    optimizer* om,
    ssa_builder* builder,
    reference* ref,
    instruction* source
)
{
    definition* variable = ref2vardef(ref);

    if (!is_def_user_defined_variable(variable)) { return; }

    variable_ssa_info* info = &builder->variables[varmap_varid2idx(om, variable)];
    variable_version_stack* frame = (variable_version_stack*)malloc_assert(sizeof(variable_version_stack));
    variable_version_stack* top = info->vvs;
    size_t i = info->version_counter;

    // push version i onto stack
    frame->version = i;
    frame->source = source;
    frame->next = top;
    info->vvs = frame;

    // increment counter and set reference version properly
    info->version_counter++;
    ref->ver = i;
}

static void ssa_builder_get_variable_version(optimizer* om, ssa_builder* builder, reference* ref)
{
    definition* variable = ref2vardef(ref);

    if (!is_def_user_defined_variable(variable)) { return; }

    ref->ver = ssa_get_variable_info(om, builder, variable)->vvs->version;
}

/**
 *
*/
static instruction* ssa_builder_get_variable_source(optimizer* om, ssa_builder* builder, const definition* variable)
{
    if (!variable) { return NULL; }

    return ssa_get_variable_info(om, builder, variable)->vvs->source;
}

static void ssa_builder_pop_variable_version(optimizer* om, ssa_builder* builder, const definition* variable)
{
    if (!is_def_user_defined_variable(variable)) { return; }

    variable_ssa_info* info = ssa_get_variable_info(om, builder, variable);
    variable_version_stack* top = info->vvs;

    if (!top) { return; }

    info->vvs = top->next;
    free(top);
}

static void optimizer_ssa_build_globals(optimizer* om, ssa_builder* builder)
{
    index_set kill;

    init_index_set(&kill, om->profile.num_variables);

    for (size_t i = 0; i < builder->num_nodes; i++)
    {
        basic_block* bb = om->node_postorder[i];

        index_set_clear(&kill);

        // node order does not matter
        for (instruction* p = bb->inst_first; p != NULL; p = p->next)
        {
            definition* d;
            size_t idx;

            if (p->operand_1)
            {
                d = p->operand_1->def;

                if (is_def_user_defined_variable(d))
                {
                    idx = varmap_varid2idx(om, d);

                    if (!index_set_contains(&kill, idx))
                    {
                        index_set_add(&builder->globals, idx);
                    }
                }
            }

            if (p->operand_2)
            {
                d = p->operand_2->def;

                if (is_def_user_defined_variable(d))
                {
                    idx = varmap_varid2idx(om, d);

                    if (!index_set_contains(&kill, idx))
                    {
                        index_set_add(&builder->globals, idx);
                    }
                }
            }

            if (p->lvalue)
            {
                d = p->lvalue->def;

                if (is_def_user_defined_variable(d))
                {
                    idx = varmap_varid2idx(om, d);
                    index_set_add(&kill, idx);
                    index_set_add(&builder->variables[idx].blocks, bb->id);
                }
            }
        }
    }

    release_index_set(&kill);
}

/**
 * insert phi instruction for given variable
*/
static void optimizer_ssa_place_phi(optimizer* om, ssa_builder* builder, definition* variable, index_set* df)
{
    /**
     * Only "variable" defined by user will be considered
     * (thus: variables written in the source code file)
     *
     * temporary variable, for example, is NOT user-defined,
     * it is IR-defined "holder" that transfer data inside
     * one, and only one block that it belongs to; so SSA
     * does not need to care about them
    */
    if (!is_def_user_defined_variable(variable)) { return; }

    index_set worklist;
    index_set_iterator itor_set;
    size_t n;

    init_index_set_copy(&worklist, &ssa_get_variable_info(om, builder, variable)->blocks);

    while (index_set_pop(&worklist, &n))
    {
        index_set_iterator_init(&itor_set, &df[n]);

        while (!index_set_iterator_end(&itor_set))
        {
            basic_block* node = om->graph->nodes.arr[index_set_iterator_get(&itor_set)];

            if (optimizer_phi_place(om, node, variable))
            {
                index_set_add(&worklist, node->id);
            }

            index_set_iterator_next(&itor_set);
        }

        index_set_iterator_release(&itor_set);
    }

    release_index_set(&worklist);
}

/**
 * Rename Variables
 *
 * Iterative DFS Walk order on dominator tree
*/
static void optimizer_ssa_rename_variable(optimizer* om, ssa_builder* builder, basic_block** idom)
{
    size_t num_nodes = om->profile.num_nodes;
    index_set* domtree_node_children = (index_set*)malloc_assert(sizeof(index_set) * num_nodes);
    bool* domtree_node_visited = (bool*)malloc_assert(sizeof(bool) * num_nodes);
    bool domtree_visiting_next;
    size_t dometree_next;
    node_walk_stack* dometree_node_stack = NULL;
    instruction* inst;

    // initialize dominator tree
    for (size_t i = 0; i < num_nodes; i++)
    {
        init_index_set(&domtree_node_children[i], num_nodes);
        domtree_node_visited[i] = false;
    }

    // construct dominator tree, simply inverse idom array
    for (size_t i = 0; i < num_nodes; i++)
    {
        index_set_add(&domtree_node_children[idom[i]->id], i);
    }

    // start from entry node
    dometree_node_stack = node_stack_push(dometree_node_stack, om->graph->entry);

    while (true)
    {
        basic_block* bb = node_stack_top(dometree_node_stack);

        // if stack is empty, we are done
        if (!bb)
        {
            break;
        }

        /**
         * Pre-order
         *
         * if visit of this node is first time, work on SSA
         * otherwise just skip this part, and do DFS part
        */
        if (!domtree_node_visited[bb->id])
        {
            // for each x = PHI(...)
            for (inst = bb->inst_first; inst && inst->op == IROP_PHI; inst = inst->next)
            {
                ssa_builder_generate_variable_version(om, builder, inst->lvalue, inst);
            }

            // for each x = y OP z
            for (inst = bb->inst_first; inst; inst = inst->next)
            {
                if (inst->op == IROP_PHI) { continue; }

                // order matters here: RHS then LHS
                ssa_builder_get_variable_version(om, builder, inst->operand_1);
                ssa_builder_get_variable_version(om, builder, inst->operand_2);
                ssa_builder_generate_variable_version(om, builder, inst->lvalue, inst);
            }

            // PHI argument insertion on successor CFG nodes
            for (size_t i = 0; i < bb->out.num; i++)
            {
                cfg_edge* out = bb->out.arr[i];

                for (instruction* sp = out->to->inst_first; sp && sp->op == IROP_PHI; sp = sp->next)
                {
                    sp->operand_phi.arr[out->to_phi_operand_index] =
                        ssa_builder_get_variable_source(om, builder, ref2vardef(sp->lvalue));
                }
            }
        }

        /**
         * DFS Walk
         *
         * Now current node has been visited
         * Once reached an unvisited child, push onto stack and start over
        */
        domtree_visiting_next = false;
        domtree_node_visited[bb->id] = true;
        while (index_set_pop(&domtree_node_children[bb->id], &dometree_next))
        {
            if (!domtree_node_visited[dometree_next])
            {
                domtree_visiting_next = true;
                dometree_node_stack = node_stack_push(dometree_node_stack, om->graph->nodes.arr[dometree_next]);
                break;
            }
        }

        /**
         * DFS Walk
         *
         * if no child left, we are done with this node, pop all versions used in this block
        */
        if (!domtree_visiting_next)
        {
            for (inst = bb->inst_first; inst; inst = inst->next)
            {
                ssa_builder_pop_variable_version(om, builder, ref2def(inst->lvalue));
            }

            dometree_node_stack = node_stack_pop(dometree_node_stack);
        }
    }

    // cleanup

    for (size_t i = 0; i < num_nodes; i++)
    {
        release_index_set(&domtree_node_children[i]);
    }

    free(domtree_node_children);
    free(domtree_node_visited);
}

/**
 * Convert CFG To SSA
 *
 * 1. Compute DF
 * 2. Place PHI
 * 3. Rename variables
 *
 * NOTE: it will repopulate optimizer::variables array
 * NOTE: it will nullify optimizer::instructions array
*/
void optimizer_ssa_build(optimizer* om)
{
    ssa_builder builder;
    basic_block** idom = cfg_idom(om->graph, om->node_postorder);
    index_set* df = cfg_dominance_frontiers(om->graph, idom);
    size_t v;

    // initialize builder
    ssa_builder_init(om, &builder);

    // prepare global set
    optimizer_ssa_build_globals(om, &builder);

    // phi placement for every variable in global set
    while (index_set_pop(&builder.globals, &v))
    {
        optimizer_ssa_place_phi(om, &builder, om->variables[v].ref, df);
    }

    // rename variable for each node
    optimizer_ssa_rename_variable(om, &builder, idom);

    // cleanup
    ssa_builder_release(om, &builder);
    cfg_delete_idom(idom);
    cfg_delete_dominance_frontiers(om->graph, df);
}

/**
 * Eliminate SSA instructions
 *
 * The CFG will be cleaned by eliminating all PHI instructions
 *
 * NOTE: it will nullify optimizer::instructions array
*/
void optimizer_ssa_eliminate(optimizer* om)
{
    size_t num_nodes = om->profile.num_nodes;

    // nullify instruction array due to instruction removal
    optimizer_invalidate_instructions(om);

    for (size_t n = 0; n < num_nodes; n++)
    {
        basic_block* b = om->graph->nodes.arr[n];
        instruction* p = b->inst_first;

        while (p && p->op == IROP_PHI)
        {
            delete_instruction(instruction_pop_front(b), true);
            om->profile.num_instructions--;
            p = b->inst_first;
        }
    }
}
