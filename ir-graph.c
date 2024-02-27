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
 *
 * and when size needs to grow, the new size is
 * calculated by: cur_size * factor
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
static void edge_array_init(edge_array* edges)
{
    edges->arr = (cfg_edge**)malloc_assert(sizeof(cfg_edge*) * EDGE_ARRAY_SIZE_INCREMENT_FACTOR);
    edges->num = 0;
    edges->size = EDGE_ARRAY_SIZE_INCREMENT_FACTOR;

    memset(edges->arr, 0, sizeof(cfg_edge*) * EDGE_ARRAY_SIZE_INCREMENT_FACTOR);
}

/**
 * edge array resize
*/
void edge_array_resize(edge_array* edges, size_t by)
{
    size_t old_size = edges->size;

    if (edges->num + by <= old_size)
    {
        return;
    }

    // yes this is dumb, but let's keep it this way
    while (edges->num + by > edges->size)
    {
        edges->size *= EDGE_ARRAY_SIZE_INCREMENT_FACTOR;
    }

    if (edges->size > old_size)
    {
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
    nodes->arr = (basic_block**)malloc_assert(sizeof(basic_block*) * NODE_ARRAY_SIZE_INCREMENT_FACTOR);
    nodes->num = 0;
    nodes->size = NODE_ARRAY_SIZE_INCREMENT_FACTOR;

    memset(nodes->arr, 0, sizeof(basic_block*) * NODE_ARRAY_SIZE_INCREMENT_FACTOR);
}

/**
 * node array resize
*/
void node_array_resize(node_array* nodes, size_t by)
{
    size_t old_size = nodes->size;

    if (nodes->num + by <= old_size)
    {
        return;
    }

    // yes this is dumb, but let's keep it this way
    while (nodes->num + by > nodes->size)
    {
        nodes->size *= NODE_ARRAY_SIZE_INCREMENT_FACTOR;
    }

    if (nodes->size > old_size)
    {
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

    free(inst);
}

/**
 * new reference
*/
reference* new_reference(reference_type t, void* doi)
{
    reference* ref = (reference*)malloc_assert(sizeof(reference));

    ref->type = t;
    ref->doi = doi;
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
 * push instruction at the beginning
*/
bool instruction_push_front(basic_block* node, instruction* inst)
{
    return instruction_insert(node, NULL, inst);
}

/**
 * find the very first instrcution that belongs to current instruction
*/
instruction* instruction_locate_enclosure_start(instruction* inst)
{
    while (inst)
    {
        // order matters here
        if (inst->operand_1 && inst->operand_1->type == IR_ASN_REF_INSTRUCTION)
        {
            // operand 1 code covers operand 2
            inst = inst->operand_1->doi;
        }
        else if (inst->operand_2 && inst->operand_2->type == IR_ASN_REF_INSTRUCTION)
        {
            inst = inst->operand_2->doi;
        }
        else
        {
            // if neither requires enclosure, then this is the one
            break;
        }
    }

    return inst;
}
