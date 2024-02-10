/**
 * CFG Data Structure Utils
 *
*/

#include "ir.h"

/**
 * edge array size increment factor
 *
 * this value is carefully selected because:
 * common branch in CFG is binary, hence 2 edges,
 * with 2 more nodes
*/
#define EDGE_ARRAY_SIZE_INCREMENT_FACTOR (2)
#define NODE_ARRAY_SIZE_INCREMENT_FACTOR (2)

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
static void edge_array_init(edge_array* edges, size_t s)
{
    edges->arr = (cfg_edge**)malloc_assert(sizeof(cfg_edge*) * s);
    edges->num = 0;
    edges->size = s;

    memset(edges->arr, 0, sizeof(cfg_edge*) * s);
}

/**
 * edge array resize
*/
static void edge_array_resize(edge_array* edges, size_t s)
{
    if (edges->num + 1 > edges->size)
    {
        edges->size += s;
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
static void node_array_init(node_array* nodes, size_t s)
{
    nodes->arr = (basic_block**)malloc_assert(sizeof(basic_block*) * s);
    nodes->num = 0;
    nodes->size = s;

    memset(nodes->arr, 0, sizeof(basic_block*) * s);
}

/**
 * node array resize
*/
static void node_array_resize(node_array* nodes, size_t s)
{
    if (nodes->num + 1 > nodes->size)
    {
        nodes->size += s;
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
*/
static void basic_block_add_edge(basic_block* block, cfg_edge* edge, basic_block_edge_type type)
{
    edge_array* edges = type == BLOCK_EDGE_IN ? &block->in : &block->out;

    if (edges->arr)
    {
        edge_array_resize(edges, EDGE_ARRAY_SIZE_INCREMENT_FACTOR);
    }
    else
    {
        edge_array_init(edges, EDGE_ARRAY_SIZE_INCREMENT_FACTOR);
    }

    edges->arr[edges->num] = edge;
    edges->num++;
}

/**
 * initialize CFG
*/
void init_cfg(cfg* g)
{
    node_array_init(&g->nodes, NODE_ARRAY_SIZE_INCREMENT_FACTOR);
    edge_array_init(&g->edges, EDGE_ARRAY_SIZE_INCREMENT_FACTOR);
    g->entry = NULL;
    g->exit = NULL;
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
    node_array_resize(&g->nodes, NODE_ARRAY_SIZE_INCREMENT_FACTOR);

    // allocate
    basic_block* n = (basic_block*)malloc_assert(sizeof(basic_block));
    memset(n, 0, sizeof(basic_block));

    // align minimum graph case
    if (g->nodes.num == 0)
    {
        g->entry = n;
        g->exit = n;
    }

    // register
    g->nodes.arr[g->nodes.num] = n;
    g->nodes.num++;

    return n;
}

/**
 * add an edge to CFG
*/
void cfg_new_edge(cfg* g, basic_block* from, basic_block* to)
{
    // reallocate if reaching maximum
    edge_array_resize(&g->edges, EDGE_ARRAY_SIZE_INCREMENT_FACTOR);

    // new edge
    cfg_edge* edge = (cfg_edge*)malloc_assert(sizeof(cfg_edge));
    edge->from = from;
    edge->to = to;

    // register
    g->edges.arr[g->edges.num] = edge;
    g->edges.num++;

    // connect blocks
    basic_block_add_edge(from, edge, BLOCK_EDGE_OUT);
    basic_block_add_edge(to, edge, BLOCK_EDGE_IN);

    // update entry
    if ((!g->entry || g->entry->in.num != 0) && from->in.num == 0)
    {
        // if new from is also an entry candidate, we do NOT update
        // because old block could like be the entry
        g->entry = from;
    }
    else if (g->entry && g->entry->in.num != 0)
    {
        // if old expired, find a new one
        for (size_t i = 0; i < g->nodes.num; i++)
        {
            g->entry = g->nodes.arr[i];

            if (g->entry->in.num == 0)
            {
                break;
            }

            g->entry = NULL;
        }
    }

    // update exit
    if (!g->exit || to->out.num == 0)
    {
        // if new to is also an exit candidate, we update
        // because new block could likely be the exit
        g->exit = to;
    }
    else if (g->exit && g->exit->out.num != 0)
    {
        // if old exit expired, find a new one
        for (size_t i = 0; i < g->nodes.num; i++)
        {
            g->exit = g->nodes.arr[i];

            if (g->exit->out.num == 0)
            {
                break;
            }

            g->exit = NULL;
        }
    }
}

/**
 * connect 2 graphs
 *
 * if source graph has only one node, it simply merge the block
 * into dest
 *
 * if dest graph is NULL; source graph will be returned
 * if source graph is NULL, dest graph will be returned
 * otherwise source graph wrapper will be deleted, return dest graph
*/
cfg* cfg_connect(cfg* g, cfg* src_graph)
{
    if (!g)
    {
        return src_graph;
    }
    else if (!src_graph)
    {
        return g;
    }
    else if (src_graph->nodes.num == 1)
    {
        // if node is the graph, graph merge is block merge
        basic_block* src = src_graph->entry;

        // append
        g->exit->inst_last->next = src->inst_first;
        g->exit->inst_last = src->inst_last;

        // detach
        src->inst_first = NULL;
        src->inst_last = NULL;
    }
    else
    {
        // array resizing
        node_array_resize(&g->nodes, src_graph->nodes.num);
        edge_array_resize(&g->edges, src_graph->edges.num);

        size_t node_arr_mem_size = src_graph->nodes.num * sizeof(basic_block*);
        size_t edge_arr_mem_size = src_graph->edges.num * sizeof(cfg_edge*);

        // merge array
        memcpy(g->nodes.arr + g->nodes.num, src_graph->nodes.arr, node_arr_mem_size);
        memcpy(g->edges.arr + g->edges.num, src_graph->edges.arr, edge_arr_mem_size);

        // detach
        memset(src_graph->nodes.arr, 0, node_arr_mem_size);
        memset(src_graph->edges.arr, 0, edge_arr_mem_size);

        // roughly estimate new exit to save efforts during edge creation
        g->exit = src_graph->exit;

        // connect by creating new edge
        cfg_new_edge(g, g->exit, src_graph->entry);

        // reset source graph for safer deletion
        src_graph->entry = NULL;
        src_graph->exit = NULL;
        src_graph->nodes.num = 0;
        src_graph->edges.num = 0;
    }

    // delete
    release_cfg(src_graph);

    return g;
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
    if (destructive)
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

    free(inst);
}

/**
 * new reference
*/
reference* new_reference()
{
    reference* ref = (reference*)malloc_assert(sizeof(reference));

    ref->type = IR_ASN_REF_UNDEFINED;
    ref->doi = NULL;
    ref->ver = 0;

    return ref;
}

/**
 * copy reference
 *
 * doi is not the case here (they are references anyway)
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
    // so far all doi will be referenced, so no deletion
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
 * push instruction at the beginning
*/
bool instruction_push_front(basic_block* node, instruction* inst)
{
    return instruction_insert(node, NULL, inst);
}
