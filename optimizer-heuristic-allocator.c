/**
 * Optimistic Allocator Heuristic
 *
 * The allocator uses interference graph to find optimal use of
 * registers, and spill the rest into memory
 *
 * This approach is considered to be expensive in general
*/

#include "optimizer.h"

typedef enum _allocator_state
{
    ALLOCATOR_STATE_BUILD,
    ALLOCATOR_STATE_SIMPLIFY,
    ALLOCATOR_STATE_COALESCE,
    ALLOCATOR_STATE_FREEZE,
    ALLOCATOR_STATE_SPILL,
    ALLOCATOR_STATE_SELECT,
    ALLOCATOR_STATE_DONE,
} allocator_state;

/**
 * Graph Planes
 *
 * IG_BIT_PLANE_MUTABLE:
 * IG_BIT_PLANE_FULL:
 * both starts as the interference graph, but IG_BIT_PLANE_MUTABLE
 * will change during the algorithm
 *
 * IG_BIT_PLANE_MOVE_RELATED:
 * g[i][j] = 1: nodes i and j are move-related
 * (hence exists instruction like "i := j")
 *
 * IG_BIT_PLANE_COALESCE:
 * coalesce graph, starts with identity matrix
 * this matrix is NOT persymmetric, only read from "row" level
*/
typedef enum _interference_graph_plane
{
    IG_BIT_PLANE_MUTABLE = 0x01,
    IG_BIT_PLANE_FULL = 0x02,
    IG_BIT_PLANE_MOVE_RELATED = 0x04,
    IG_BIT_PLANE_COALESCE = 0x08,
    IG_BIT_PLANE_RESERVE_5 = 0x10,
    IG_BIT_PLANE_RESERVE_6 = 0x20,
    IG_BIT_PLANE_RESERVE_7 = 0x40,
    IG_BIT_PLANE_RESERVE_8 = 0x80,
} interference_graph_plane;

/**
 * Interference Graph Types
*/
typedef byte ig_cell;
typedef ig_cell* ig_row;
typedef ig_row* ig_matrix;

/**
 * Node's Color State
*/
typedef struct _ig_node_color_state
{
    // #color remained for this node
    size_t num_color_available;
    // use status for each color, 1 means occupied, 0 means available
    byte* colors;
} ig_node_color_state;

/**
 * Interference Graph
 *
 * it is an N x N byte matrix, where:
 * N = number of local variables
 * cell = boolean flag indicating an edge (1 means connected)
 *
 * this matrix is persymmetric
 *
 * The graph is coded bit-wise, so the matrix can hold
 * up to 8 planes, defined by interference_graph_plane
*/
typedef struct _interference_graph
{
    // graph plane
    ig_matrix matrix;
    // dimension of graph plane
    size_t dim;

    // degree of every node on plane IG_BIT_PLANE_MUTABLE
    size_t* deg_graph;
    // degree of every node on plane IG_BIT_PLANE_MOVE_RELATED
    size_t* deg_move;
    // track if a node exists in graph IG_BIT_PLANE_MUTABLE
    // (because deg=0 does not imply the node is not in graph)
    byte* mutable_nodes;

    // color state for each node
    ig_node_color_state* color_state;
} interference_graph;

/**
 * Color Stack
*/
typedef struct _color_stack_frame
{
    size_t node;
    bool spill;

    struct _color_stack_frame* next;
} color_stack_frame;

/**
 * Allocator Instance
*/
typedef struct _heuristic_allocator
{
    optimizer* om;
    optimizer_profile* profile;
    interference_graph ig;
    allocator_state state;
    size_t num_registers;
    size_t num_spilled;
    color_stack_frame* color_stack;
} heuristic_allocator;

static void __debug_print_interference_graph_plane(const interference_graph* ig, interference_graph_plane plane)
{
    switch (plane)
    {
        case IG_BIT_PLANE_MUTABLE:
            printf("IG Plane: IG_BIT_PLANE_MUTABLE\n");
            break;
        case IG_BIT_PLANE_FULL:
            printf("IG Plane: IG_BIT_PLANE_FULL\n");
            break;
        case IG_BIT_PLANE_MOVE_RELATED:
            printf("IG Plane: IG_BIT_PLANE_MOVE_RELATED\n");
            break;
        case IG_BIT_PLANE_COALESCE:
            printf("IG Plane: IG_BIT_PLANE_COALESCE\n");
            break;
        default:
            printf("IG Plane: (unknown: 0x%02x)", plane);
            return;
    }

    for (size_t i = 0; i < ig->dim; i++)
    {
        printf("%zd -> {", i);

        for (size_t j = 0, k = 0; j < ig->dim; j++)
        {
            if (ig->matrix[i][j] & plane)
            {
                if (k) { printf(", "); }
                printf("%zd", j);
                k = 1;
            }
        }

        printf("}\n");
    }
}

static void __debug_print_heuristic_allocator(heuristic_allocator* allocator)
{
    interference_graph* ig = &allocator->ig;

    printf("\n===== GRAPH COLORING ALLOCATOR INFO =====\n");

    printf("No. Registers: %zd\nNo. Spilled: %zd\nGraph Dimension: %zd\n",
        allocator->num_registers,
        allocator->num_spilled,
        ig->dim
    );

    __debug_print_interference_graph_plane(ig, IG_BIT_PLANE_MUTABLE);
    __debug_print_interference_graph_plane(ig, IG_BIT_PLANE_FULL);
    __debug_print_interference_graph_plane(ig, IG_BIT_PLANE_MOVE_RELATED);
    __debug_print_interference_graph_plane(ig, IG_BIT_PLANE_COALESCE);

    printf("IG_BIT_PLANE_MUTABLE node degree:\n");
    for (size_t i = 0; i < ig->dim; i++)
    {
        printf("%zd ", ig->deg_graph[i]);
    }
    printf("\n");

    printf("IG_BIT_PLANE_MOVE_RELATED node degree:\n");
    for (size_t i = 0; i < ig->dim; i++)
    {
        printf("%zd ", ig->deg_move[i]);
    }
    printf("\n");

    printf("IG_BIT_PLANE_MUTABLE node existence:\n");
    for (size_t i = 0; i < ig->dim; i++)
    {
        printf("%d ", (int)(ig->mutable_nodes[i]));
    }
    printf("\n");

    printf("Color Result:\n");
    for (size_t i = 0; i < ig->dim; i++)
    {
        variable_item* v = &allocator->om->variables[varmap_lid2idx(allocator->om, i)];

        switch (v->allocation.type)
        {
            case REG_ALLOC_UNDEFINED:
                printf("_, ");
                break;
            case REG_ALLOC_STACK:
                printf("s%zd, ", v->allocation.location);
                break;
            case REG_ALLOC_REGISTER:
                printf("r%zd, ", v->allocation.location);
                break;
            default:
                printf("?, ");
                break;
        }
    }
    printf("\n");
}

static void ig_init(interference_graph* ig, size_t n, size_t num_registers)
{
    ig->dim = n;
    ig->matrix = (ig_matrix)malloc_assert(sizeof(ig_row) * n);
    ig->deg_graph = (size_t*)malloc_assert(sizeof(size_t) * n);
    ig->deg_move = (size_t*)malloc_assert(sizeof(size_t) * n);
    ig->mutable_nodes = (byte*)malloc_assert(sizeof(byte) * n);
    ig->color_state = (ig_node_color_state*)malloc_assert(sizeof(ig_node_color_state) * n);

    for (size_t i = 0; i < n; i++)
    {
        ig->deg_graph[i] = 0;
        ig->deg_move[i] = 0;
        ig->matrix[i] = (ig_row)malloc_assert(sizeof(ig_cell) * n);
        ig->mutable_nodes[i] = 0;
        ig->color_state[i].num_color_available = num_registers;
        ig->color_state[i].colors = (byte*)malloc_assert(sizeof(byte) * num_registers);

        memset(ig->matrix[i], 0, sizeof(ig_cell) * n);
        memset(ig->color_state[i].colors, 0, sizeof(byte) * num_registers);
    }

    /**
     * initialize coalesce graph
     *
     * by default it is an identity matrix, meaning node coalesces with
     * nothing except itself
    */
    for (size_t i = 0; i < n; i++)
    {
        ig->matrix[i][i] |= IG_BIT_PLANE_COALESCE;
    }
}

static void ig_release(interference_graph* ig)
{
    for (size_t i = 0; i < ig->dim; i++)
    {
        free(ig->matrix[i]);
        free(ig->color_state[i].colors);
    }

    free(ig->matrix);
    free(ig->deg_graph);
    free(ig->deg_move);
    free(ig->mutable_nodes);
    free(ig->color_state);
}

static bool ig_empty(const interference_graph* ig)
{
    for (size_t i = 0; i < ig->dim; i++)
    {
        if (ig->mutable_nodes[i])
        {
            return false;
        }
    }

    return true;
}

static ig_cell ig_cell_get(interference_graph* ig, interference_graph_plane planes, size_t row, size_t col)
{
    return ig->matrix[row][col] & planes;
}

static ig_cell ig_cell_set(interference_graph* ig, interference_graph_plane planes, size_t row, size_t col)
{
    return ig->matrix[row][col] |= planes;
}

static ig_cell ig_cell_reset(interference_graph* ig, interference_graph_plane planes, size_t row, size_t col)
{
    return ig->matrix[row][col] &= ~planes;
}

static void ig_node_add_mutable(interference_graph* ig, size_t n)
{
    ig->mutable_nodes[n] = 1;
}

/**
 * add an edge
 *
 * NOTE: when connecting two nodes in IG_BIT_PLANE_MUTABLE, it implies that
 * both nodes are in the graph, call ig_node_remove to remove a node
*/
static void ig_connect(interference_graph* ig, interference_graph_plane planes, size_t row, size_t col)
{
    size_t cell = ig_cell_get(ig, IG_BIT_PLANE_MUTABLE | IG_BIT_PLANE_MOVE_RELATED, row, col);

    if ((planes & IG_BIT_PLANE_MUTABLE) && !(cell & IG_BIT_PLANE_MUTABLE))
    {
        ig->deg_graph[row]++;
        ig->deg_graph[col]++;

        ig_node_add_mutable(ig, row);
        ig_node_add_mutable(ig, col);
    }
    if ((planes & IG_BIT_PLANE_MOVE_RELATED) && !(cell & IG_BIT_PLANE_MOVE_RELATED))
    {
        ig->deg_move[row]++;
        ig->deg_move[col]++;
    }

    ig->matrix[row][col] |= planes;
    ig->matrix[col][row] |= planes;
}

/**
 * remove an edge
 *
 * WARNING: this routine does not "remove" the node from IG_BIT_PLANE_MUTABLE
 * to do that, call ig_node_remove
*/
static void ig_disconnect(interference_graph* ig, interference_graph_plane planes, size_t row, size_t col)
{
    size_t cell = ig_cell_get(ig, IG_BIT_PLANE_MUTABLE | IG_BIT_PLANE_MOVE_RELATED, row, col);

    if ((planes & IG_BIT_PLANE_MUTABLE) && (cell & IG_BIT_PLANE_MUTABLE))
    {
        ig->deg_graph[row]--;
        ig->deg_graph[col]--;
    }
    if ((planes & IG_BIT_PLANE_MOVE_RELATED) && (cell & IG_BIT_PLANE_MOVE_RELATED))
    {
        ig->deg_move[row]--;
        ig->deg_move[col]--;
    }

    ig->matrix[row][col] &= ~planes;
    ig->matrix[col][row] &= ~planes;
}

static void ig_node_remove(interference_graph* ig, interference_graph_plane planes, size_t n)
{
    for (size_t i = 0; i < ig->dim; i++)
    {
        ig_disconnect(ig, planes, n, i);
    }

    if (planes & IG_BIT_PLANE_MUTABLE)
    {
        ig->mutable_nodes[n] = 0;
    }
}

static size_t ig_node_coalesced_by(interference_graph* ig, size_t n)
{
    if (ig_cell_get(ig, IG_BIT_PLANE_COALESCE, n, n))
    {
        return n;
    }

    for (size_t i = 0; i < ig->dim; i++)
    {
        if (ig_cell_get(ig, IG_BIT_PLANE_MUTABLE, n, i))
        {
            return i;
        }
    }
}

static size_t ig_node_get_coalesce_site(interference_graph* ig, size_t n)
{
    while (!ig_cell_get(ig, IG_BIT_PLANE_COALESCE, n, n))
    {
        for (size_t i = 0; i < ig->dim; i++)
        {
            if (ig_cell_get(ig, IG_BIT_PLANE_MUTABLE, n, i))
            {
                n = i;
                break;
            }
        }
    }

    return n;
}

/**
 * Coalesce node src to dest
 *
 * dest will be redirected to coalesce site if dest is
 * already coalesced
 *
 * this function will delete src node from all perspectives
 * except coalesce graph
 *
 * in coalesce graph
 *     g[src][src] = 0 (src is coalesced)
 *     g[src][dest] = 1 (src is coalesced with dest)
 *     g[dest][src] = 1 (dest is coalesced with src)
 *
 * in mutable graph
 *     g[dest] connect g[src][i] for every i
 *
 * in move-related graph
 *     same idea as above
*/
static size_t ig_node_coalesce(interference_graph* ig, size_t dest, size_t src)
{
    // every node is coalesced by itself by default, no need to do anything
    if (dest == src) { return dest; }

    // makes sure dest is not the one that is already coalesced
    dest = ig_node_get_coalesce_site(ig, dest);

    ig_cell_reset(ig, IG_BIT_PLANE_COALESCE, src, src);
    ig_connect(ig, IG_BIT_PLANE_COALESCE, dest, src);

    for (size_t i = 0; i < ig->dim; i++)
    {
        size_t cell = ig_cell_get(ig, IG_BIT_PLANE_MUTABLE | IG_BIT_PLANE_MOVE_RELATED, src, i);

        if (cell & IG_BIT_PLANE_MUTABLE)
        {
            ig_connect(ig, IG_BIT_PLANE_MUTABLE, dest, i);
        }

        if (cell & IG_BIT_PLANE_MOVE_RELATED)
        {
            ig_connect(ig, IG_BIT_PLANE_MOVE_RELATED, dest, i);
        }
    }

    ig_node_remove(ig, IG_BIT_PLANE_MUTABLE | IG_BIT_PLANE_MOVE_RELATED, src);

    return dest;
}

/**
 * Initialize allocator
 *
 * interference graph will not be initialized here because it is done in Build Stage
*/
static void init_heuristic_allocator(
    heuristic_allocator* allocator,
    optimizer* om,
    optimizer_profile* next_profile,
    size_t num_reg
)
{
    allocator->om = om;
    allocator->profile = next_profile;
    allocator->state = ALLOCATOR_STATE_BUILD;
    allocator->num_registers = num_reg;
    allocator->num_spilled = 0;
    allocator->ig.matrix = NULL;
    allocator->ig.deg_graph = NULL;
    allocator->ig.dim = 0;
    allocator->color_stack = NULL;

    // by default, profile is current one
    optimizer_profile_copy(om, allocator->profile);
}

/**
 * Release allocator
*/
static void release_heuristic_allocator(heuristic_allocator* allocator)
{
    color_stack_frame* top = allocator->color_stack;

    ig_release(&allocator->ig);

    while (top)
    {
        allocator->color_stack = top->next;
        free(top);
        top = allocator->color_stack;
    }
}

static void allocator_push_color_ig_node(heuristic_allocator* allocator, size_t n)
{
    color_stack_frame* f = (color_stack_frame*)malloc_assert(sizeof(color_stack_frame));

    f->node = n;
    f->spill = false;
    f->next = allocator->color_stack;

    allocator->color_stack = f;

    ig_node_remove(&allocator->ig, IG_BIT_PLANE_MUTABLE, n);
}

static void allocator_push_spill_ig_node(heuristic_allocator* allocator, size_t n)
{
    color_stack_frame* f = (color_stack_frame*)malloc_assert(sizeof(color_stack_frame));

    f->node = n;
    f->spill = true;
    f->next = allocator->color_stack;

    allocator->color_stack = f;

    ig_node_remove(&allocator->ig, IG_BIT_PLANE_MUTABLE, n);
}

/**
 * Node Spill Cost
 *
 * lower the number, lower the cost, hence higher the priority
 *
 * 1. high degree: live at many program points
 * 2. not used/defined very often: no need to access stack often
*/
static float allocator_node_spill_cost(heuristic_allocator* allocator, size_t n)
{
    interference_graph* ig = &allocator->ig;
    float p = 0.0f;

    for (size_t i = 0; i < ig->dim; i++)
    {
        if (ig_cell_get(ig, IG_BIT_PLANE_COALESCE, n, i))
        {
            variable_item* var = &allocator->om->variables[varmap_lid2idx(allocator->om, i)];

            p += (float)(var->ud_loop_outside + var->ud_loop_inside * 10);
        }
    }

    return p / (float)ig->deg_graph[n];
}

/**
 * Spill Code Injection (Read)
 *
 * var' <- (null) IROP_READ (null) [operand_rw_stack_loc]
 *
 * WARNING: do NOT update allocator::om::profile because it is useful to safely release
 * old optimizer instance; use allocator::profile instead
*/
static void allocator_spill_code_read(heuristic_allocator* allocator, instruction* target, const variable_item* var_item)
{
    definition* var = var_item->ref;

    // if no read, then no-op
    if (target->operand_1->def != var && target->operand_2->def != var)
    {
        return;
    }

    instruction* inst = new_instruction();
    definition* var_new = new_definition(DEFINITION_VARIABLE);

    // prepare variable
    var_new->variable->kind = VARIABLE_KIND_TEMPORARY;
    var_new->lid = allocator->profile->num_locals;
    definition_pool_add(&allocator->om->spill_pool, var_new);

    // prepare instruction
    inst->op = IROP_READ;
    inst->node = target->node;
    inst->lvalue = new_reference(IR_ASN_REF_DEFINITION, var_new);
    inst->operand_rw_stack_loc = var_item->allocation.location;
    instruction_insert(target->node, target->prev, inst);

    // replace operand reference with new variable
    if (target->operand_1->def == var)
    {
        delete_reference(target->operand_1);
        target->operand_1 = new_reference(IR_ASN_REF_DEFINITION, var_new);
    }

    // replace operand reference with new variable
    if (target->operand_2->def == var)
    {
        delete_reference(target->operand_2);
        target->operand_2 = new_reference(IR_ASN_REF_DEFINITION, var_new);
    }

    // update profile
    allocator->profile->num_instructions++;
    allocator->profile->num_locals++;
    allocator->profile->num_variables++;
}

/**
 * Spill Code Injection (Write)
 *
 * [operand_rw_stack_loc] <- var IROP_WRITE (null)
 *
 * WARNING: do NOT update allocator::om::profile because it is useful to safely release
 * old optimizer instance; use allocator::profile instead
*/
static void allocator_spill_code_write(heuristic_allocator* allocator, instruction* target, const variable_item* var_item)
{
    definition* var = var_item->ref;

    // if no read, then no-op
    if (target->lvalue->def != var)
    {
        return;
    }

    instruction* inst = new_instruction();

    // prepare instruction
    inst->op = IROP_WRITE;
    inst->node = target->node;
    inst->operand_1 = new_reference(IR_ASN_REF_DEFINITION, var);
    inst->operand_rw_stack_loc = var_item->allocation.location;
    instruction_insert(target->node, target, inst);

    // update profile
    allocator->profile->num_instructions++;
}

/**
 * Flush array of variables into interference graph
 *
 * the array represents that all variables interfere with each other
 *
 * NOTE:
 * "buf" is the varmap set, indices from om->variables, use varmap_* APIs
 * to convert from varmap index to lid
*/
static void allocator_ig_connect_varmap_set(heuristic_allocator* allocator, size_t* buf, size_t len)
{
    interference_graph* ig = &allocator->ig;
    optimizer* om = allocator->om;

    for (size_t i = 0; i < len; i++)
    {
        // ignore member variables
        if (varmap_idx_is_member(om, buf[i])) { continue; }

        // make sure every node goes into graph IG_BIT_PLANE_MUTABLE
        ig_node_add_mutable(ig, varmap_idx2lid(om, buf[i]));

        // generate edges
        for (size_t j = i + 1; j < len; j++)
        {
            // ignore member variables
            if (varmap_idx_is_member(om, buf[j])) { continue; }

            ig_connect(
                ig,
                IG_BIT_PLANE_MUTABLE | IG_BIT_PLANE_FULL,
                varmap_idx2lid(om, buf[i]),
                varmap_idx2lid(om, buf[j])
            );
        }
    }
}

static void allocator_ig_build_from_instruction(heuristic_allocator* allocator, instruction_item* inst_item)
{
    interference_graph* ig = &allocator->ig;
    size_t* buf = (size_t*)malloc_assert(sizeof(size_t) * allocator->om->profile.num_locals);
    size_t len;

    // live-in set
    len = index_set_to_array(&inst_item->in, buf);
    allocator_ig_connect_varmap_set(allocator, buf, len);

    // live-out set
    len = index_set_to_array(&inst_item->out, buf);
    allocator_ig_connect_varmap_set(allocator, buf, len);

    free(buf);
}

/**
 * Color a node
 *
 * if color is depleted, process will fail and return false
 */
static bool allocator_ig_node_assign_color(heuristic_allocator* allocator, size_t n)
{
    interference_graph* ig = &allocator->ig;
    size_t color;

    // do nothing is color depleted
    if (!ig->color_state[n].num_color_available) { return false; }

    // locate the color
    for (color = 0; color < allocator->num_registers && ig->color_state[n].colors[color]; color++);

    // color it, along with all coalesced ones
    // coalesce graph includes the node itself so the loop covers both cases
    for (size_t i = 0; i < ig->dim; i++)
    {
        if (!ig_cell_get(ig, IG_BIT_PLANE_COALESCE, n, i)) { continue; }

        variable_item* var = &allocator->om->variables[varmap_lid2idx(allocator->om, i)];

        var->allocation.type = REG_ALLOC_REGISTER;
        var->allocation.location = color;

        // propagate the color use info to neighbors
        for (size_t j = 0; j < ig->dim; j++)
        {
            if (ig_cell_get(ig, IG_BIT_PLANE_FULL, i, j))
            {
                ig->color_state[j].colors[color] = 1;
                ig->color_state[j].num_color_available--;
            }
        }
    }

    return true;
}

/**
 * Build Stage
 *
 * Build inteference graph, and detect move-related variables
 * This stage is very expensive
*/
static void optimizer_allocator_build(heuristic_allocator* allocator)
{
    interference_graph* ig = &allocator->ig;

    ig_init(ig, allocator->om->profile.num_locals, allocator->num_registers);

    // build interference graph
    for (size_t i = 0; i < allocator->om->profile.num_instructions; i++)
    {
        allocator_ig_build_from_instruction(allocator, &allocator->om->instructions[i]);
    }

    // connect move-related pairs (pairs in instruction with form "a := b")
    for (size_t i = 0; i < allocator->om->profile.num_instructions; i++)
    {
        instruction* inst = allocator->om->instructions[i].ref;

        // skip incorrect form
        if (!inst->lvalue || !inst->operand_1) { continue; }

        definition* v1 = inst->lvalue->def;
        definition* v2 = inst->operand_1->def;

        // skip member variables
        if (is_def_member_variable(v1) || is_def_member_variable(v2))
        {
            continue;
        }

        // only the form "a := b" AND a and b are not neighbor
        if (inst->op == IROP_ASN &&
            inst->lvalue->type == IR_ASN_REF_DEFINITION &&
            inst->operand_1->type == IR_ASN_REF_DEFINITION &&
            !ig_cell_get(ig, IG_BIT_PLANE_FULL, v1->lid, v2->lid))
        {
            ig_connect(ig, IG_BIT_PLANE_MOVE_RELATED, v1->lid, v2->lid);
        }
    }

    allocator->state = ALLOCATOR_STATE_SIMPLIFY;
}

/**
 * Simplify Stage
 *
 * Keep scanning the graph and remove node that is:
 * 1. not move-related (deg_move == 0), AND
 * 2. deg_graph < num_registers
*/
static void optimizer_allocator_simplify(heuristic_allocator* allocator)
{
    bool run = true;

    while (run)
    {
        run = false;

        for (size_t i = 0; i < allocator->ig.dim; i++)
        {
            run = allocator->ig.mutable_nodes[i] && // in graph
                !allocator->ig.deg_move[i] && // not move-related
                allocator->ig.deg_graph[i] < allocator->num_registers;

            if (run)
            {
                // graph will change, so do the job and start over
                allocator_push_color_ig_node(allocator, i);
                break;
            }
        }
    }

    allocator->state = ALLOCATOR_STATE_COALESCE;
}

/**
 * Coalesce Stage (George's Heuristic)
 *
 * Safe to coalesce x and y if, for every neighbor t of x:
 * 1. t interferes y, OR
 * 2. deg(t) < K
 *
 * Briggs' Heuristic requires resulting node so it is harder
 * to implement, and since 2 strategies are distinguishable
 * in terms of the conservative effect, so either approach
 * is okay here
*/
static void optimizer_allocator_coalesce(heuristic_allocator* allocator)
{
    interference_graph* ig = &allocator->ig;

    allocator->state = ALLOCATOR_STATE_FREEZE;

    for (size_t x = 0; x < ig->dim; x++)
    {
        if (!allocator->ig.mutable_nodes[x] || !ig->deg_move[x]) { continue; }

        for (size_t y = 0; y < ig->dim; y++)
        {
            bool coalesce = true;

            if (x == y || !allocator->ig.mutable_nodes[y] || !ig_cell_get(ig, IG_BIT_PLANE_COALESCE, x, y))
            {
                continue;
            }

            // George's Heuristic
            for (size_t t = 0; coalesce && t < ig->dim; t++)
            {
                if (!ig_cell_get(ig, IG_BIT_PLANE_MUTABLE, x, t))
                {
                    continue;
                }

                coalesce = coalesce &&
                    (ig_cell_get(ig, IG_BIT_PLANE_MUTABLE, t, y) || ig->deg_graph[t] < allocator->num_registers);
            }

            /**
             * Coalesce One Move-Related Pair
             *
             * then we are done
             *
             * if coalesced node cannot be simplified, ALLOCATOR_STATE_SIMPLIFY
             * will be no-op and fall-through this state again, and coalesce
             * again.
             *
             * This process can be modified to avoid the jump, but it is
             * defying the meaning of state machine and more importantly, this
             * state must restart anyway due to a successful coalescing: because
             * the inerference graph will be modified so the graph must be
             * scanned from the beginning
             *
             * As a result, simply change the state and leave it to state machine
            */
            if (coalesce)
            {
                ig_node_coalesce(ig, x, y);
                allocator->state = ALLOCATOR_STATE_SIMPLIFY;
                return;
            }
        }
    }
}

/**
 * Freeze Stage
 *
 * Find a node that is :
 * 1. move related, AND
 * 2. degree < #registers
 *
 * mark it as not move-related, and go back to Simplify Stage
*/
static void optimizer_allocator_freeze(heuristic_allocator* allocator)
{
    // if this stage does nothing, goes to Spill Stage
    allocator->state = ALLOCATOR_STATE_SPILL;

    for (size_t i = 0; i < allocator->ig.dim; i++)
    {
        /**
         * Only freeze one, then we are done
         *
         * because we need Simplify Stage to clean it up first
        */
        if (allocator->ig.mutable_nodes[i] && // in graph
            allocator->ig.deg_move[i] && // move-related
            allocator->ig.deg_graph[i] < allocator->num_registers)
        {
            ig_node_remove(&allocator->ig, IG_BIT_PLANE_MOVE_RELATED, i);
            allocator->state = ALLOCATOR_STATE_SIMPLIFY;
            return;
        }
    }
}

/**
 * Spill Stage
 *
 * choose node with degree >=k to potentially spill
 * Then continue with simplify
*/
static void optimizer_allocator_spill(heuristic_allocator* allocator)
{
    interference_graph* ig = &allocator->ig;
    bool exist = false;
    float spill_min;
    size_t spill_node;

    // choose one node to spill
    for (size_t i = 0; i < ig->dim; i++)
    {
        if (!ig->mutable_nodes[i] || ig->deg_graph[i] < allocator->num_registers)
        {
            continue;
        }

        float p = allocator_node_spill_cost(allocator, i);

        if (exist)
        {
            if (p < spill_min)
            {
                spill_node = i;
                spill_min = p;
            }
        }
        else
        {
            exist = true;
            spill_node = i;
            spill_min = p;
        }
    }

    // spill
    if (exist)
    {
        allocator_push_spill_ig_node(allocator, spill_node);
        allocator->state = ALLOCATOR_STATE_SIMPLIFY;
    }
    else
    {
        allocator->state = ALLOCATOR_STATE_SELECT;
    }
}

/**
 * Select Stage
 *
 * when graph is empty, start restoring nodes in reverse order and color them
*/
static void optimizer_allocator_select(heuristic_allocator* allocator)
{
    interference_graph* ig = &allocator->ig;
    color_stack_frame* frame = allocator->color_stack;

    allocator->state = ALLOCATOR_STATE_DONE;

    // walk the stack
    while (frame)
    {
        size_t n = frame->node;

        // determine action to the node
        if (frame->spill)
        {
            // if to spill, we pick this node and stop here
            allocator->state = ALLOCATOR_STATE_BUILD;
            break;
        }
        else if (!allocator_ig_node_assign_color(allocator, n))
        {
            // non-spilled node must have at least 1 register available
            fprintf(stderr, "TODO error: register depleted for node %zd.\n", n);
        }

        frame = frame->next;
    }

    // spill the variable
    if (allocator->state == ALLOCATOR_STATE_BUILD)
    {
        size_t num_nodes = allocator->om->profile.num_nodes;
        variable_item* var_spilled = &allocator->om->variables[varmap_lid2idx(allocator->om, frame->node)];

        // assign stack index, later in backend this index will be translated into offset
        var_spilled->allocation.type = REG_ALLOC_STACK;
        var_spilled->allocation.location = allocator->num_spilled++;

        /**
         * TODO: is it possible that the spilled node also coalesced?
         *
         * if so, move everything behind this loop into this loop
        */
        for (size_t i = 0; i < ig->dim; i++)
        {
            if (!ig_cell_get(ig, IG_BIT_PLANE_COALESCE, frame->node, i)) { continue; }

            fprintf(stderr, "TODO error: spill node has coalesced member: %zd.\n", i);
        }

        // mutate CFG
        for (size_t i = 0; i < num_nodes; i++)
        {
            instruction* p = allocator->om->node_postorder[num_nodes - i - 1]->inst_first;
            instruction* pn;

            while (p)
            {
                // log next first so we can skip new inserted code
                pn = p->next;

                allocator_spill_code_read(allocator, p, var_spilled);
                allocator_spill_code_write(allocator, p, var_spilled);

                p = pn;
            }
        }
    }
}

/**
 * Optimistic Allocator Heuristic State Machine
 *
 * next_profile instance will be filled with info for next iteration if applicable
 *
 * it returns true if heuristics reaches optimal result;
 * ir returns false if heuristics needs to start over due to spill code
*/
static bool optimizer_allocator_heuristics_state_machine(
    optimizer* om,
    optimizer_profile* next_profile,
    size_t num_avail_registers
)
{
    heuristic_allocator allocator;

    init_heuristic_allocator(&allocator, om, next_profile, num_avail_registers);

    while (allocator.state != ALLOCATOR_STATE_DONE)
    {
        switch (allocator.state)
        {
            case ALLOCATOR_STATE_BUILD:
                optimizer_allocator_build(&allocator);
                __debug_print_heuristic_allocator(&allocator);
                break;
            case ALLOCATOR_STATE_SIMPLIFY:
                optimizer_allocator_simplify(&allocator);
                break;
            case ALLOCATOR_STATE_COALESCE:
                optimizer_allocator_coalesce(&allocator);
                break;
            case ALLOCATOR_STATE_FREEZE:
                optimizer_allocator_freeze(&allocator);
                break;
            case ALLOCATOR_STATE_SPILL:
                optimizer_allocator_spill(&allocator);
                break;
            case ALLOCATOR_STATE_SELECT:
                optimizer_allocator_select(&allocator);

                // need rebuild, so clean exit with false flag
                if (allocator.state == ALLOCATOR_STATE_BUILD)
                {
                    release_heuristic_allocator(&allocator);
                    return false;
                }
                break;
            default:
                // should never reach here, so force exit
                fprintf(stderr, "TODO error: broken allocator state detected: %d\n", allocator.state);
                allocator.state = ALLOCATOR_STATE_DONE;
                break;
        }
    }

    __debug_print_heuristic_allocator(&allocator);
    release_heuristic_allocator(&allocator);
    return true;
}

/**
 * Heuristic Register Allocator Entry Point
 *
 * NOTE: optimizer::variables and optimizer::instructions will be changed
*/
void optimizer_allocator_heuristic(optimizer* om, size_t num_avail_registers)
{
    optimizer_profile profile;

    optimizer_profile_copy(om, &profile);

    do
    {
        // repopulate, no need to make persistent though
        optimizer_profile_apply(om, &profile, false);

        // facts needed by allocator
        optimizer_defuse_analyze(om);
        optimizer_liveness_analyze(om);
    } while (!optimizer_allocator_heuristics_state_machine(om, &profile, num_avail_registers));

    // make data persistent
    optimizer_profile_apply(om, &profile, true);
}
