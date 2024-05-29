/**
 * CFG Data Structure Utils
 *
*/

#include "ir.h"
#include "utils.h"

/**
 * edge instance deletion
 *
 * nodes in edge are only refernce, so we only need to free the element
*/
static void edge_delete(cfg_edge* edge)
{
    free(edge);
}

/**
 * basic block instance deletion
 *
 * edges in node are only references, so we only need to free the array
*/
static void node_delete(basic_block* block)
{
    instruction* inst = block->inst_first;

    // delete all instructions
    while (inst)
    {
        instruction* c = inst;
        inst = inst->next;

        delete_instruction(c, true);
    }

    // edges in node are only references, so we only need to free the array
    free(block->in.arr);
    free(block->out.arr);

    // free node
    free(block);
}

/**
 * initialize edge array
*/
static void edge_array_init(edge_array* edges)
{
    edges->arr = (cfg_edge**)malloc_assert(sizeof(cfg_edge*) * 2);
    edges->num = 0;
    edges->size = 2;

    memset(edges->arr, 0, sizeof(cfg_edge*) * 2);
}

/**
 * edge array resize
*/
void edge_array_resize(edge_array* edges, size_t by)
{
    size_t new_size = edges->num + by;

    if (new_size > edges->size)
    {
        edges->size = find_next_pow2_size(new_size);
        edges->arr = (cfg_edge**)realloc_assert(edges->arr, sizeof(cfg_edge*) * (edges->size));
    }
}

/**
 * edge array delete
*/
static void edge_array_delete(edge_array* edges)
{
    for (size_t i = 0; i < edges->num; i++)
    {
        edge_delete(edges->arr[i]);
    }

    free(edges->arr);
}

/**
 * initialize node array
*/
static void node_array_init(node_array* nodes)
{
    nodes->arr = (basic_block**)malloc_assert(sizeof(basic_block*) * 2);
    nodes->num = 0;
    nodes->size = 2;

    memset(nodes->arr, 0, sizeof(basic_block*) * 2);
}

/**
 * node array resize
*/
void node_array_resize(node_array* nodes, size_t by)
{
    size_t new_size = nodes->num + by;

    if (new_size > nodes->size)
    {
        nodes->size = find_next_pow2_size(new_size);
        nodes->arr = (basic_block**)realloc_assert(nodes->arr, sizeof(basic_block*) * (nodes->size));
    }
}

/**
 * node array delete
*/
static void node_array_delete(node_array* nodes)
{
    for (size_t i = 0; i < nodes->num; i++)
    {
        node_delete(nodes->arr[i]);
    }

    free(nodes->arr);
}

/**
 * add inbound edge
 *
 * if inbound = true: it is an inbound edge
 * otherwise: it is an outbound edge
*/
static void basic_block_add_edge(basic_block* block, cfg_edge* edge, bool inbound)
{
    edge_array* edges = inbound ? &block->in : &block->out;

    if (edges->arr)
    {
        edge_array_resize(edges, 1);
    }
    else
    {
        edge_array_init(edges);
    }

    edges->arr[edges->num] = edge;
    edges->num++;
}

/**
 * generate a CFG container
 *
 * create a CFG memory chunk with all fields set to 0
*/
cfg* new_cfg_container()
{
    cfg* g = (cfg*)malloc_assert(sizeof(cfg));
    memset(g, 0, sizeof(cfg));
    return g;
}

/**
 * initialize CFG
*/
void init_cfg(cfg* g)
{
    node_array_init(&g->nodes);
    edge_array_init(&g->edges);
    g->entry = NULL;
}

/**
 * release CFG
*/
void release_cfg(cfg* g)
{
    if (!g)
    {
        return;
    }

    node_array_delete(&g->nodes);
    edge_array_delete(&g->edges);
}

/**
 * allocate and initialize a new basic block
 *
*/
basic_block* cfg_new_basic_block(cfg* g)
{
    // reallocate if reaching maximum
    node_array_resize(&g->nodes, 1);

    // allocate
    basic_block* n = (basic_block*)malloc_assert(sizeof(basic_block));
    memset(n, 0, sizeof(basic_block));

    // initialize block
    n->id = g->nodes.num;
    n->type = BLOCK_ANY;

    // register
    g->nodes.arr[g->nodes.num] = n;
    g->nodes.num++;

    return n;
}

/**
 * add an edge to CFG
*/
void cfg_new_edge(cfg* g, basic_block* from, basic_block* to, edge_type type)
{
    // reallocate if reaching maximum
    edge_array_resize(&g->edges, 1);

    // new edge
    cfg_edge* edge = (cfg_edge*)malloc_assert(sizeof(cfg_edge));
    edge->type = type;
    edge->to_phi_operand_index = to->in.num;
    edge->from = from;
    edge->to = to;

    // register
    g->edges.arr[g->edges.num] = edge;
    g->edges.num++;

    // connect blocks
    basic_block_add_edge(from, edge, false);
    basic_block_add_edge(to, edge, true);
}

/**
 * test if graph is empty
*/
bool cfg_empty(const cfg* g)
{
    return g->nodes.num == 0;
}

/**
 * detach CFG data
 *
 * nodes.arr and edges.arr cannot be set to NULL
 * because the array itself needs to be freed
 *
 * this is lazy release as node and edge arrays
 * will not be cleared, based on algorithm in
 * node_array_delete and edge_array_delete, we
 * only need to clear the counter
*/
void cfg_detach(cfg* g)
{
    g->entry = NULL;
    g->nodes.num = 0;
    g->edges.num = 0;
}

/**
 * Iterative DFS Walk To Generate A Node Order
 *
 * it will generate an array of nodes with size of #nodes in graph,
 * and the order of node will be either preorder or postorder,
 * depending on the given argument
 *
 * the returned array is a reference array, bb should not be freed
 * with this array
*/
basic_block** cfg_node_order(const cfg* g, cfg_dfs_order order)
{
    size_t num_nodes = g->nodes.num;
    size_t snti = 0; // stack's next-top idx
    size_t rnti = 0; // result's next idx
    basic_block** r = (basic_block**)malloc_assert(sizeof(basic_block*) * num_nodes);
    basic_block** stack = (basic_block**)malloc_assert(sizeof(basic_block*) * num_nodes);
    size_t* nc = (size_t*)malloc_assert(sizeof(size_t) * num_nodes); // next-child-to-visit index
    char* visited = (char*)malloc_assert(sizeof(char) * num_nodes);

    // mark all as not visited at the beginning
    memset(visited, 0, sizeof(char) * num_nodes);
    memset(nc, 0, sizeof(size_t) * num_nodes);

    // initialize: from entry node
    stack[snti++] = g->entry;
    visited[g->entry->id] = 1;

    // main loop
    while (snti != 0)
    {
        // read current top
        basic_block* cur = stack[snti - 1];

        if (order == DFS_PREORDER) { r[rnti++] = cur; }

        // locate next-to-visit child
        for (size_t* pnc = &nc[cur->id]; *pnc < cur->out.num && visited[cur->out.arr[*pnc]->to->id]; (*pnc)++);

        if (nc[cur->id] >= cur->out.num)
        {
            // pop: when all children are visited
            snti--;
            if (order == DFS_POSTORDER) { r[rnti++] = cur; }
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

    return r;
}

/**
 * node order list deleter
*/
void cfg_delete_node_order(basic_block** list)
{
    free(list);
}

/**
 * Calculate Immediate Dominator Set
 *
 * according to definition: immediate dominator of node n, IDOM(n), has
 * exactly one node: for any node p, there does not exist node q, such that:
 * p DOM q DOM n, AND p DOM n
 *
 * Algorithm is based on:
 * A Simple, Fast Dominance Algorithm by Cooper et al.
 *
 * it returns an array of node reference, where each element
 * represents the IDOM of the node with ID equals to the index
*/
basic_block** cfg_idom(const cfg* g, basic_block** postorder)
{
    size_t num_nodes = g->nodes.num;
    size_t* idx_node2post = (size_t*)malloc_assert(sizeof(size_t) * num_nodes);
    basic_block** idom = (basic_block**)malloc_assert(sizeof(basic_block*) * num_nodes);
    bool changed = true;

    // initialize dominators array
    memset(idom, 0, sizeof(basic_block*) * num_nodes);

    // initialize node-to-postorder-index map
    for (size_t i = 0; i < num_nodes; i++)
    {
        idx_node2post[postorder[i]->id] = i;
    }

    idom[g->entry->id] = g->entry;

    while (changed)
    {
        changed = false;

        for (size_t i = 0; i < num_nodes; i++)
        {
            basic_block* b = postorder[num_nodes - i - 1]; // traverse in reverse postorder

            if (b == g->entry) { continue; }

            basic_block* new_idom = b->in.arr[0]->from; // pick first predecessor ?

            // for all other predecessors
            for (size_t j = 1; j < b->in.num; j++)
            {
                basic_block* pred = b->in.arr[j]->from;

                if (idom[pred->id] != NULL)
                {
                    // fast intersect(pred, new_idom), using their postorder index
                    // converge to same node, hence a dominator
                    while (new_idom != pred)
                    {
                        while (idx_node2post[new_idom->id] < idx_node2post[pred->id])
                        {
                            new_idom = idom[new_idom->id];
                        }
                        while (idx_node2post[pred->id] < idx_node2post[new_idom->id])
                        {
                            pred = idom[pred->id];
                        }
                    }
                }
            }

            // update
            if (idom[b->id] != new_idom)
            {
                idom[b->id] = new_idom;
                changed = true;
            }
        }
    }

    // cleanup
    free(idx_node2post);

    return idom;
}

/**
 * IDOM array deleter
*/
void cfg_delete_idom(basic_block** idom)
{
    free(idom);
}

/**
 * Calculate Dominators Set
 *
 * It uses IDOM sets to generate DOM sets
 *
 * it returns an array of index set, where each element
 * represents the DOM set of the node with ID equals to the index
*/
index_set* cfg_dominators(const cfg* g, const basic_block** idom)
{
    size_t num_nodes = g->nodes.num;
    index_set* dom = (index_set*)malloc_assert(sizeof(index_set) * num_nodes);

    // initialize
    for (size_t i = 0; i < num_nodes; i++)
    {
        init_index_set(&dom[i], num_nodes);
        index_set_add(&dom[i], i); // n DOM n is always true
    }

    for (size_t i = 0; i < num_nodes; i++)
    {
        basic_block* b = (basic_block*)(idom[i]);

        while (true)
        {
            index_set_add(&dom[i], b->id);

            if (b == g->entry) { break; }

            b = (basic_block*)(idom[b->id]);
        }
    }

    return dom;
}

/**
 * helper for deletion of dominator sets
*/
void cfg_delete_dominators(const cfg* g, index_set* dom)
{
    for (size_t i = 0; i < g->nodes.num; i++)
    {
        release_index_set(&dom[i]);
    }

    free(dom);
}

/**
 * Calculate Dominance Frontier (DF) Set
 *
 * Algorithm is based on:
 * A Simple, Fast Dominance Algorithm by Cooper et al.
 *
 * it returns an array of index set, where each element
 * represents the DF set of the node with ID equals to the index
 *
 * each set is also a node set, but only indicies of nodes are stored
*/
index_set* cfg_dominance_frontiers(const cfg* g, const basic_block** idom)
{
    size_t num_nodes = g->nodes.num;
    index_set* df = (index_set*)malloc_assert(sizeof(index_set) * num_nodes);

    // initialize
    for (size_t i = 0; i < num_nodes; i++)
    {
        init_index_set(&df[i], num_nodes);
    }

    for (size_t i = 0; i < num_nodes; i++)
    {
        basic_block* b = g->nodes.arr[i];

        if (b->in.num >= 2)
        {
            for (size_t j = 0; j < b->in.num; j++)
            {
                basic_block* probe = b->in.arr[j]->from;

                while (probe != idom[b->id])
                {
                    // add b to probe's df set
                    index_set_add(&df[probe->id], b->id);
                    probe = (basic_block*)(idom[probe->id]);
                }
            }
        }
    }

    return df;
}

/**
 * helper for deletion of dominance frontier sets
*/
void cfg_delete_dominance_frontiers(const cfg* g, index_set* df)
{
    for (size_t i = 0; i < g->nodes.num; i++)
    {
        release_index_set(&df[i]);
    }

    free(df);
}

/**
 * allocate and initialize a new instruction
*/
instruction* new_instruction()
{
    instruction* inst = (instruction*)malloc_assert(sizeof(instruction));
    memset(inst, 0, sizeof(instruction));
    return inst;
}

/**
 * delete an instruction
 *
 * this routine will maintain integrity of the instruction sequence
 *
 * if destructive is set to true, it only deletes inst, the list
 * will be broken
*/
void delete_instruction(instruction* inst, bool destructive)
{
    if (!destructive)
    {
        instruction* prev = inst->prev;
        instruction* next = inst->next;

        if (prev)
        {
            prev->next = next;
        }

        if (next)
        {
            next->prev = prev;
        }
    }

    delete_reference(inst->lvalue);
    delete_reference(inst->operand_1);
    delete_reference(inst->operand_2);

    for (size_t i = 0; i < inst->operand_aux.num; i++)
    {
        delete_reference(inst->operand_aux.arr[i]);
    }

    free(inst->operand_aux.arr);
    free(inst->operand_phi.arr);
    free(inst);
}

/**
 * Push Aux Operand Into Instruction
 *
 * NOTE: PHI instruction should NOT use this one, use operand_phi instead
*/
void instruction_aux_operand_push(instruction* inst, reference* ref)
{
    inst->operand_aux.arr = (reference**)realloc_assert(inst->operand_aux.arr, sizeof(reference*) * (inst->operand_aux.num + 1));
    inst->operand_aux.arr[inst->operand_aux.num++] = ref;
}

/**
 * Test if it is an SSA PHI statement
*/
bool instruction_is_ssa_phi(const instruction* inst)
{
    return inst && inst->operand_phi.arr != NULL;
}

/**
 * new reference
*/
reference* new_reference(reference_type t, definition* def)
{
    reference* ref = (reference*)malloc_assert(sizeof(reference));

    ref->type = t;
    ref->def = def;
    ref->ver = 0;

    return ref;
}

/**
 * copy reference
 *
 * def is not the case here (they are references anyway)
 * the case is the version number
*/
reference* copy_reference(const reference* r)
{
    if (!r)
    {
        return NULL;
    }

    reference* ref = (reference*)malloc_assert(sizeof(reference));
    memcpy(ref, r, sizeof(reference));
    return ref;
}

/**
 * delete reference
*/
void delete_reference(reference* ref)
{
    // so far all def will be referenced, so no deletion
    free(ref);
}

/**
 * instruction insertion
 *
 * if prev is NULL, it inserts at the beginning
 * if prev->next is NULL, it inserts at the end
 * otherwise it does regular inserts
 *
 * NOTE: it is only valid for single instruction insertion
 * except when prev->next is NULL
*/
bool instruction_insert(basic_block* node, instruction* prev, instruction* inst)
{
    // guard: sequence insertion not allowed if instruction is part of a list
    if (!node || inst->prev || inst->next)
    {
        return false;
    }

    inst->prev = prev;
    inst->node = node;

    if (prev)
    {
        inst->next = prev->next;
        prev->next = inst;
    }
    else
    {
        inst->next = node->inst_first;
        node->inst_first = inst;
    }

    if (!inst->next)
    {
        node->inst_last = inst;
    }

    return true;
}

/**
 * append instruction at the end
*/
bool instruction_push_back(basic_block* node, instruction* inst)
{
    return instruction_insert(node, node->inst_last, inst);
}

/**
 * pop instruction at the end
*/
instruction* instruction_pop_back(basic_block* node)
{
    instruction* inst = node->inst_last;

    if (inst)
    {
        node->inst_last = inst->prev;
        inst->prev = NULL;
        inst->next = NULL;

        if (node->inst_last)
        {
            node->inst_last->next = NULL;
        }
        else
        {
            node->inst_first = NULL;
        }
    }

    return inst;
}

/**
 * pop instruction at the beginning
*/
instruction* instruction_pop_front(basic_block* node)
{
    instruction* inst = node->inst_first;

    if (inst)
    {
        node->inst_first = inst->next;
        inst->prev = NULL;
        inst->next = NULL;

        if (node->inst_first)
        {
            node->inst_first->prev = NULL;
        }
        else
        {
            node->inst_last = NULL;
        }
    }

    return inst;
}

/**
 * push instruction at the beginning
*/
bool instruction_push_front(basic_block* node, instruction* inst)
{
    return instruction_insert(node, NULL, inst);
}
