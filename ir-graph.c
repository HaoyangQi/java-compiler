/**
 * CFG Data Structure Utils
 *
 * TODO: deletion of:
 * 1. instruction
 * 2. basic block
 * 3. graph & graph edges
*/

#include "ir.h"

/**
 * edge array size increment factor
 *
 * this value is carefully selected because:
 * common branch in CFG is binary, hence 2 blocks
*/
#define EDGE_ARRAY_SIZE_INCREMENT_FACTOR (2)

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
static void edge_array_resize(edge_array* edges)
{
    if (edges->num + 1 > edges->size)
    {
        edges->size += EDGE_ARRAY_SIZE_INCREMENT_FACTOR;
        edges->arr = (cfg_edge**)realloc_assert(edges->arr, sizeof(cfg_edge*) * (edges->size));
    }
}

/**
 * TODO:edge array deletion
 *
 * edge array deletion will delete entire connected graph (CFG)
 * because those edges are expected to be the only reference to the blocks
*/

/**
 * allocate and initialize a new basic block
 *
 * for in&out edges, by default we set to NULL to save some memory
*/
basic_block* new_basic_block()
{
    basic_block* n = (basic_block*)malloc_assert(sizeof(basic_block));
    memset(n, 0, sizeof(basic_block));
    return n;
}

/**
 * add inbound edge
*/
void basic_block_add_edge(basic_block* block, cfg_edge* edge, basic_block_edge_type type)
{
    edge_array* edges = type == BLOCK_EDGE_IN ? &block->in : &block->out;

    if (edges->arr)
    {
        edge_array_resize(edges);
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
    edge_array_init(&g->edges, EDGE_ARRAY_SIZE_INCREMENT_FACTOR);
    g->entry = NULL;
}

/**
 * add an edge to CFG
*/
void cfg_new_edge(cfg* g, basic_block* from, basic_block* to)
{
    // reallocate if reaching maximum
    edge_array_resize(&g->edges);

    // new edge
    cfg_edge* edge = (cfg_edge*)malloc_assert(sizeof(cfg_edge));
    edge->from = from;
    edge->to = to;
    g->edges.arr[g->edges.num] = edge;

    // connect blocks
    basic_block_add_edge(from, edge, BLOCK_EDGE_OUT);
    basic_block_add_edge(to, edge, BLOCK_EDGE_IN);

    // increment boundary
    g->edges.num++;
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
*/
void delete_instruction(instruction* inst)
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

    free(inst);
    return next;
}

/**
 * instruction insertion
 *
 * if prev is NULL, it inserts at the beginning
 * if prev->next is NULL, it inserts at the end
 * otherwise it does regular inserts
*/
void instruction_insert(basic_block* node, instruction* prev, instruction* inst)
{
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
}

/**
 * append instruction at the end
*/
void instruction_push_back(basic_block* node, instruction* inst)
{
    instruction_insert(node, node->inst_last, inst);
}

/**
 * push instruction at the beginning
*/
void instruction_push_front(basic_block* node, instruction* inst)
{
    instruction_insert(node, NULL, inst);
}
