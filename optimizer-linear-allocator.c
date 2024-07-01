/**
 * Greedy Linear Scan Allocator
 *
 * Allocator will not operate on member variables
 *
 * Real-time complexity allocator (it is fast)
*/

#include "optimizer.h"

typedef enum _interval_sort
{
    INTERVAL_SORT_INCREASE = 1,
    INTERVAL_SORT_DECREASE = 2,
    INTERVAL_SORT_START = 4,
    INTERVAL_SORT_END = 8,
} interval_sort;

typedef struct _live_interval
{
    size_t id;
    size_t start;
    size_t end;
    bool spilled;
    bool cached;
    size_t active_order;

    struct
    {
        size_t reg;
        size_t stack;
    } alloc;
} live_interval;

typedef struct _variable_live_range_item
{
    live_interval range;
    struct _variable_live_range_item* next;
} variable_live_range_item;

typedef struct _variable_live_range_header
{
    variable_live_range_item* first;
    size_t num;
} variable_live_range_header;

typedef struct _variable_live_range_data
{
    // range process program
    linear_allocator_range_process program;
    // array of range data, indexed by variable id
    variable_live_range_header* variables;
    // total number of ranges
    size_t num_ranges;
    // number of variables;
    size_t num_locals;

    // scan order (inc start) reference array
    variable_live_range_item** scan_order;

    // active order (inc end) info
    struct
    {
        // reference array
        variable_live_range_item** array;
        // element in-array flag
        byte* active;
        // number of element count
        size_t active_count;
    } active_order;
} variable_live_range_data;

typedef struct _linear_scan_allocator
{
    // optimizer instance
    optimizer* om;
    // live range data
    variable_live_range_data range_data;
    // profile info tracker
    optimizer_profile profile;

    // register tracker
    struct
    {
        // register pool
        byte* occupied;
        // last range info of each variable that was in a register
        variable_live_range_item** last_range;
        // total number of registers available
        size_t num;
    } registers;

    // stack memory tracker
    struct
    {
        // a tracker flags if corresponding variable has a stack space allocated
        byte* allocated;
        // stack index id
        size_t* location;
        // number of variables that requires stack space for allocation
        size_t count;
    } stack;
} linear_scan_allocator;

static void __debug_print_interval(const live_interval* interval)
{
    printf("(%zd, %zd), id: %zd, active order: %zd, alloc: %c%zd\n",
        interval->start,
        interval->end,
        interval->id,
        interval->active_order,
        interval->spilled ? 's' : 'r',
        interval->spilled ? interval->alloc.stack : interval->alloc.reg
    );
}

static void __debug_print_linear_allocator(const linear_scan_allocator* allocator)
{
    printf("\n===== LINEAR ALLOCATOR INFO =====\n");

    printf("Registers: ");
    for (size_t i = 0; i < allocator->registers.num; i++)
    {
        printf("%d ", allocator->registers.occupied[i]);
    }
    printf("\n");

    printf("Liveness:\n");
    for (size_t i = 0; i < allocator->om->profile.num_instructions; i++)
    {
        size_t* buf = (size_t*)malloc_assert(sizeof(size_t) * allocator->om->profile.num_variables);
        size_t n;

        printf("[%zd]: live-in: ", i);
        n = index_set_to_array(&allocator->om->instructions[i].in, buf);
        printf("{");
        for (size_t j = 0; j < n; j++)
        {
            if (j > 0) { printf(", "); }

            printf("%zd", buf[j]);
        }
        printf("}");

        printf(", live-out: ");
        n = index_set_to_array(&allocator->om->instructions[i].out, buf);
        printf("{");
        for (size_t j = 0; j < n; j++)
        {
            if (j > 0) { printf(", "); }

            printf("%zd", buf[j]);
        }
        printf("}\n");

        free(buf);
    }

    printf("Variables require stack (count: %zd): ", allocator->stack.count);
    for (size_t i = 0; i < allocator->om->profile.num_locals; i++)
    {
        if (allocator->stack.allocated[i])
        {
            printf("%zd ", i);
        }
    }
    printf("\n");

    printf("Ranges:\n");
    for (size_t i = 0; i < allocator->range_data.num_locals; i++)
    {
        variable_live_range_header* h = &allocator->range_data.variables[i];

        printf("Local ID %zd (count: %zd):\n", i, h->num);

        for (variable_live_range_item* it = h->first; it; it = it->next)
        {
            printf("  ");
            __debug_print_interval(&it->range);
        }
    }

    printf("Ranges In Scan Order (Inc, Start): \n");
    for (size_t i = 0; i < allocator->range_data.num_ranges; i++)
    {
        __debug_print_interval(&allocator->range_data.scan_order[i]->range);
    }

    printf("Ranges In Active Order (Inc, End): \n");
    for (size_t i = 0; i < allocator->range_data.num_ranges; i++)
    {
        printf("%s: ", allocator->range_data.active_order.active[i] ? "[.]" : "[ ]");
        __debug_print_interval(&allocator->range_data.active_order.array[i]->range);
    }

    printf("\n");
}

/**
 * compare intervals
 *
 * it returns true if order a->b matches program
 * it uses equality bound "<=" to make original order stable
 *
 * (default: INTERVAL_SORT_DECREASE | INTERVAL_SORT_END)
*/
static bool interval_compare(const live_interval* a, const live_interval* b, interval_sort program)
{
    bool sinc = a->start <= b->start;
    bool einc = a->end <= b->end;
    bool prog_inc = (program & INTERVAL_SORT_INCREASE) ? true : false;

    return (program & INTERVAL_SORT_START) ? (prog_inc == sinc) : (prog_inc == einc);
}

static variable_live_range_item* new_variable_live_range_item(size_t lid, size_t inst_id)
{
    variable_live_range_item* r = (variable_live_range_item*)malloc_assert(sizeof(variable_live_range_item));

    memset(r, 0, sizeof(variable_live_range_item));

    r->range.id = lid;
    r->range.start = inst_id;
    r->range.end = inst_id;

    return r;
}

static void delete_variable_live_range_item(variable_live_range_item* r)
{
    free(r);
}

static void init_live_range_data(variable_live_range_data* rd, size_t num_locals, linear_allocator_range_process program)
{
    size_t sz_headers = sizeof(variable_live_range_header) * num_locals;

    rd->program = program;
    rd->variables = (variable_live_range_header*)malloc_assert(sz_headers);
    rd->num_ranges = 0;
    rd->num_locals = num_locals;

    memset(rd->variables, 0, sz_headers);
}

static void release_live_range_data(variable_live_range_data* rd)
{
    for (size_t i = 0; i < rd->num_locals; i++)
    {
        variable_live_range_item* item = rd->variables[i].first;

        while (item)
        {
            variable_live_range_item* next = item->next;
            delete_variable_live_range_item(item);
            item = next;
        }
    }

    free(rd->scan_order);
    free(rd->active_order.array);
    free(rd->active_order.active);
}

/**
 * Allocate a range to a range
 *
 * Source of register selection has following priority, from highest to lowest:
 * 1. transfer source (second parameter)
 * 2. cache
 * 3. register pool
 */
static void live_range_data_allocate_register(variable_live_range_item* item, variable_live_range_item* src, linear_scan_allocator* allocator)
{
    size_t reg;
    variable_live_range_item* cache = allocator->registers.last_range[item->range.id];

    // register selection
    if (src)
    {
        reg = src->range.alloc.reg;
        item->range.cached = false;
    }
    else if (cache)
    {
        reg = cache->range.alloc.reg;
        item->range.cached = true;
    }
    else
    {
        for (reg = 0; reg < allocator->registers.num && allocator->registers.occupied[reg]; reg++);
        item->range.cached = false;
    }

    // exception check
    if (reg >= allocator->registers.num)
    {
        fprintf(stderr, "TODO error: invalid register selection, register pool may have been depleted.\n");
    }

    allocator->registers.occupied[reg] = 1;
    item->range.alloc.reg = reg;
    item->range.spilled = false;

    // invalidate the cache: remove this register from cache completely
    for (size_t i = 0; i < allocator->range_data.num_locals; i++)
    {
        cache = allocator->registers.last_range[i];

        if (cache && cache->range.alloc.reg == reg)
        {
            // invalidate the cache because now a new range is taking this register
            allocator->registers.last_range[i] = NULL;
        }
    }

    // align the cache to current setting
    allocator->registers.last_range[item->range.id] = item;
}

/**
 * Release the register occupied by a range
 *
 * Register used by the given range will be cached, and will be used if cache hits upon next range allocation
 */
static void live_range_data_release_register(variable_live_range_item* item, linear_scan_allocator* allocator)
{
    allocator->registers.occupied[item->range.alloc.reg] = 0;
    allocator->registers.last_range[item->range.id] = item;
}

/**
 * Stack Space Allocation
 *
 * Every variable will only allocate once for all of its ranges
 */
static void live_range_data_allocate_stack_location(variable_live_range_item* item, linear_scan_allocator* allocator)
{
    size_t id = item->range.id;
    size_t s = allocator->stack.allocated[id] ? allocator->stack.location[id] : (allocator->stack.count++);

    allocator->stack.allocated[id] = 1;
    allocator->stack.location[id] = s;
    item->range.alloc.stack = s;
    item->range.spilled = true;
}

/**
 * Use given item as an anchor, check and merge next neighbor
 *
 * Once merge occurs, its next neighbor will be deleted
 */
static void live_range_data_merge_next(variable_live_range_data* rd, size_t lid, variable_live_range_item* cur)
{
    if (!cur) { return; }

    variable_live_range_item* next = cur->next;

    if (cur && next && next->range.start == cur->range.end)
    {
        cur->range.end = next->range.end;
        cur->next = next->next;
        delete_variable_live_range_item(next);

        rd->variables[lid].num--;
        rd->num_ranges--;
    }
}

static void live_range_data_insert_after(
    variable_live_range_data* rd,
    size_t lid,
    variable_live_range_item* cur,
    variable_live_range_item* item
)
{
    variable_live_range_header* header = &rd->variables[lid];

    if (cur)
    {
        item->next = cur->next;
        cur->next = item;
    }
    else
    {
        item->next = header->first;
        header->first = item;
    }

    header->num++;
    rd->num_ranges++;
}

/**
 * Update Live Range Data (Split)
 *
 * It will maintain a sorted range list
 */
static void live_range_data_update_split(variable_live_range_data* rd, size_t lid, size_t inst_id)
{
    variable_live_range_item* cur = rd->variables[lid].first;
    variable_live_range_item* prev = NULL;
    variable_live_range_item* next = cur ? cur->next : NULL;
    size_t inst_next = inst_id + 1;

    // header insertion check
    if (!cur || inst_next < cur->range.start)
    {
        live_range_data_insert_after(rd, lid, NULL, new_variable_live_range_item(lid, inst_id));
        return;
    }

    // sliding-window insertion
    while (true)
    {
        if (inst_next == cur->range.start)
        {
            // quick merge
            cur->range.start = inst_id;
            break;
        }
        else if (inst_id >= cur->range.start && inst_id <= cur->range.end)
        {
            // already included, no-op
            return;
        }
        else if (cur->range.end + 1 == inst_id)
        {
            // quick merge
            cur->range.end = inst_id;
            break;
        }
        else if (!next || (inst_next > cur->range.end && inst_next < next->range.start))
        {
            // insertion, then we are done
            live_range_data_insert_after(rd, lid, cur, new_variable_live_range_item(lid, inst_id));
            return;
        }

        prev = cur;
        cur = cur->next;
        next = cur->next;
    }

    /**
     * Merge Range (Quick Merge Only)
     *
     * Order Matters Here!
     *
     * Because merge might delete item, so we merge it backwards
     */
    live_range_data_merge_next(rd, lid, cur);
    live_range_data_merge_next(rd, lid, prev);
}

/**
 * Update Live Range Data (Merge)
 *
 * Only one range for each variable is maintained
 */
static void live_range_data_update_merge(variable_live_range_data* rd, size_t lid, size_t inst_id)
{
    variable_live_range_item* it = rd->variables[lid].first;

    if (!it)
    {
        live_range_data_insert_after(rd, lid, NULL, new_variable_live_range_item(lid, inst_id));
    }
    else if (inst_id < it->range.start)
    {
        it->range.start = inst_id;
    }
    else if (inst_id > it->range.end)
    {
        it->range.end = inst_id;
    }
}

/**
 * Update Live Range Data
 *
 * It will maintain a sorted range list
 */
static void live_range_data_update(variable_live_range_data* rd, size_t lid, size_t inst_id)
{
    switch (rd->program)
    {
        case LINEAR_ALLOCATOR_RANGE_MERGE:
            live_range_data_update_merge(rd, lid, inst_id);
            break;
        case LINEAR_ALLOCATOR_RANGE_SPLIT:
            live_range_data_update_split(rd, lid, inst_id);
            break;
        default:
            fprintf(stderr,
                "TODO error: invalid allocator configuration (%d), fallback to default process (merge all ranges).\n",
                rd->program
            );
            live_range_data_update_merge(rd, lid, inst_id);
            break;
    }

    return;
}

/**
 * Live Range Array Sort Algorithm (Merge Sort)
 *
 * it sorts range [from, len - 1]
 *
 * arr and buf should have same length:
 * 1. arr needs to be populated data
 * 2. buf is used as a container so it does not need initialization
*/
static void sort_live_ranges(
    variable_live_range_item** arr,
    variable_live_range_item** buf,
    size_t from,
    size_t len,
    interval_sort program
)
{
    size_t to = len == 0 ? from : (from + len - 1);
    size_t len_half = len / 2;
    size_t div_from[2] = { from, from + len_half };
    size_t div_len[2] = { len_half, len - len_half };
    size_t i = from;

    switch (len)
    {
        case 0:
        case 1:
            // no need to sort
            break;
        case 2:
            // minimum case: swap once
            if (!interval_compare(&arr[from]->range, &arr[to]->range, program))
            {
                variable_live_range_item* tmp = arr[from];

                arr[from] = arr[to];
                arr[to] = tmp;
            }
            break;
        default:
            sort_live_ranges(arr, buf, div_from[0], div_len[0], program);
            sort_live_ranges(arr, buf, div_from[1], div_len[1], program);

            // fill
            while (div_len[0] || div_len[1])
            {
                if (div_len[0] && div_len[1])
                {
                    variable_live_range_item* v0 = arr[div_from[0]];
                    variable_live_range_item* v1 = arr[div_from[1]];

                    if (interval_compare(&v0->range, &v1->range, program))
                    {
                        buf[i++] = v0;
                        div_from[0]++;
                        div_len[0]--;
                    }
                    else
                    {
                        buf[i++] = v1;
                        div_from[1]++;
                        div_len[1]--;
                    }
                }
                else if (div_len[0])
                {
                    buf[i++] = arr[div_from[0]];
                    div_from[0]++;
                    div_len[0]--;
                }
                else if (div_len[1])
                {
                    buf[i++] = arr[div_from[1]];
                    div_from[1]++;
                    div_len[1]--;
                }
            }

            memcpy(arr + from, buf + from, sizeof(variable_live_range_item*) * len);
            break;
    }
}

/**
 * Populate Sorted Orders Of Live Range Data
 *
 */
static void live_range_data_sort(variable_live_range_data* rd)
{
    size_t sz_ranges = sizeof(variable_live_range_item*) * rd->num_ranges;
    variable_live_range_item** buf = (variable_live_range_item**)malloc_assert(sz_ranges);

    // allocation
    rd->scan_order = (variable_live_range_item**)malloc_assert(sz_ranges);
    rd->active_order.array = (variable_live_range_item**)malloc_assert(sz_ranges);
    rd->active_order.active = (byte*)malloc_assert(sizeof(byte) * rd->num_ranges);

    // initialize active array tracker
    rd->active_order.active_count = 0;
    memset(rd->active_order.active, 0, sizeof(byte) * rd->num_ranges);

    // populate data
    for (size_t i = 0, idx = 0; i < rd->num_locals; i++)
    {
        for (variable_live_range_item* it = rd->variables[i].first; it; it = it->next)
        {
            rd->scan_order[idx] = it;
            rd->active_order.array[idx] = it;
            idx++;
        }
    }

    // sort
    sort_live_ranges(rd->scan_order, buf, 0, rd->num_ranges, INTERVAL_SORT_START | INTERVAL_SORT_INCREASE);
    sort_live_ranges(rd->active_order.array, buf, 0, rd->num_ranges, INTERVAL_SORT_END | INTERVAL_SORT_INCREASE);

    // clean up
    free(buf);
}

static void live_range_data_active_add(variable_live_range_data* rd, size_t idx_active_order)
{
    rd->active_order.active[idx_active_order] = 1;
    rd->active_order.active_count++;
}

static void live_range_data_active_remove(variable_live_range_data* rd, size_t idx_active_order)
{
    rd->active_order.active[idx_active_order] = 0;
    rd->active_order.active_count--;
}

/**
 * Check Errors In Live Ranges
 *
 * Error messages will be logged accordingly
 */
static void allocator_range_data_validate(linear_scan_allocator* allocator)
{
    variable_live_range_data* rd = &allocator->range_data;
    optimizer* om = allocator->om;

    for (size_t i = 0; i < rd->num_locals; i++)
    {
        variable_item* var = &om->variables[varmap_lid2idx(om, i)];

        if (!rd->variables[i].first)
        {
            // issue warnings for variables without valid range
            if (is_def_user_defined_variable(var->ref))
            {
                if (is_def_register_optimizable_variable(var->ref))
                {
                    fprintf(stderr, "TODO warning: unused variable: lid=%zd.\n", i);
                }
                else
                {
                    fprintf(stderr, "TODO info: variable %zd with kind %d is ignored by allocator.\n",
                        get_variable_id(var->ref),
                        var->ref->variable->kind
                    );
                }
            }
            else
            {
                fprintf(stderr, "TODO info: variable lid=%zd has uninitialized interval.\n", i);
            }

            var->allocation.type = REG_ALLOC_UNDEFINED;
        }
    }
}

static void allocator_fill_optimizer_register_allocation_info(
    linear_scan_allocator* allocator,
    variable_live_range_item* item
)
{
    optimizer* om = allocator->om;
    live_interval* interval = &item->range;
    bool spilled = interval->spilled;
    size_t lid = interval->id;
    size_t var_idx = varmap_lid2idx(om, lid);
    size_t loc = spilled ? interval->alloc.stack : interval->alloc.reg;
    variable_item* var_item = &om->variables[var_idx];
    register_allocation_type type = spilled ? REG_ALLOC_STACK : REG_ALLOC_REGISTER;
    reference* operands[3];

    /**
     * Mutate Variable Item Allocation Info
     *
     * Once type becomes REG_ALLOC_HYBRID, data set will be no longer necessary,
     * and instruction item will contain allocation information in further details
     */
    var_item->allocation.stack_loc_allocated = spilled || var_item->allocation.stack_loc_allocated;
    switch (var_item->allocation.type)
    {
        case REG_ALLOC_STACK:
            if (type != var_item->allocation.type)
            {
                var_item->allocation.type = REG_ALLOC_HYBRID;
            }
            break;
        case REG_ALLOC_REGISTER:
            // stack-to-reg or reg_1-to-reg_2 are both "hybrid condition"
            if (type != var_item->allocation.type || (!spilled && var_item->allocation.location != interval->alloc.reg))
            {
                var_item->allocation.type = REG_ALLOC_HYBRID;
            }
            break;
        case REG_ALLOC_HYBRID:
            // data set is no longer necessary, so skip
            break;
        default:
            var_item->allocation.type = type;
            var_item->allocation.location = loc;
            break;
    }

    /**
     * Fill Instruction Item Allocation Info
     */
    for (size_t i = interval->start; i <= interval->end; i++)
    {
        instruction_item* inst_item = &om->instructions[i];

        operands[0] = inst_item->ref->lvalue;
        operands[1] = inst_item->ref->operand_1;
        operands[2] = inst_item->ref->operand_2;

        for (size_t j = 0; j < 3; j++)
        {
            definition* var_def = ref2vardef(operands[j]);
            register_allocation_info* info = &inst_item->allocation[j];

            if (var_def && varmap_varid2idx(om, var_def) == var_idx)
            {
                info->type = type;
                info->stack_loc_allocated = spilled;
                info->location = loc;
            }
        }
    }
}

/**
 * Load variable at the beginning of the range
 */
static void allocator_variable_range_load(linear_scan_allocator* allocator, size_t lid, variable_live_range_item* item)
{
    optimizer* om = allocator->om;
    variable_item* var = &om->variables[varmap_lid2idx(om, lid)];
    instruction* start = om->instructions[item->range.start].ref;
    instruction* inst = new_instruction();

    inst->op = IROP_READ;
    inst->node = start->node;
    inst->lvalue = new_reference(IR_ASN_REF_DEFINITION, var->ref);
    inst->operand_rw_stack_loc = allocator->stack.location[lid];
    inst->allocation[0].type = REG_ALLOC_REGISTER;
    inst->allocation[0].location = item->range.alloc.reg;
    inst->allocation[0].stack_loc_allocated = true;
    instruction_insert(start->node, start->prev, inst);

    allocator->profile.num_instructions++;
}

/**
 * Evict variable at the end of the range
 */
static void allocator_variable_range_evict(linear_scan_allocator* allocator, size_t lid, variable_live_range_item* item)
{
    optimizer* om = allocator->om;
    variable_item* var = &om->variables[varmap_lid2idx(om, lid)];
    instruction* end = om->instructions[item->range.end].ref;
    instruction* inst = new_instruction();

    inst->op = IROP_WRITE;
    inst->node = end->node;
    inst->operand_1 = new_reference(IR_ASN_REF_DEFINITION, var->ref);
    inst->operand_rw_stack_loc = allocator->stack.location[lid];
    inst->allocation[1].type = REG_ALLOC_REGISTER;
    inst->allocation[1].location = item->range.alloc.reg;
    inst->allocation[1].stack_loc_allocated = true;
    instruction_insert(end->node, end, inst);

    allocator->profile.num_instructions++;
}

static void allocator_project_data_to_optimizer_merge(linear_scan_allocator* allocator)
{
    variable_live_range_data* rd = &allocator->range_data;

    for (size_t i = 0; i < rd->num_locals; i++)
    {
        variable_live_range_item* it = rd->variables[i].first;

        if (it)
        {
            if (it->next)
            {
                // merge mode does not allow > 1 range for any variable
                fprintf(stderr, "TODO error: lid=%zd has more than one ranges in merge mode.\n", i);
            }
            else
            {
                allocator_fill_optimizer_register_allocation_info(allocator, it);
            }
        }
    }
}

static void allocator_project_data_to_optimizer_split(linear_scan_allocator* allocator)
{
    variable_live_range_data* rd = &allocator->range_data;

    for (size_t lid = 0; lid < rd->num_locals; lid++)
    {
        variable_live_range_item* first = rd->variables[lid].first;
        variable_live_range_item* cur = first;

        // if (cur) printf("Variable lid=%zd:\n", cur->range.id);

        while (cur)
        {
            variable_live_range_item* next = cur->next;

            // printf("  (%zd, %zd): ", cur->range.start, cur->range.end);

            if (cur->range.spilled)
            {
                // spill the range
                live_range_data_allocate_stack_location(cur, allocator);
                // printf("use stack location: %zd\n", allocator->stack.location[lid]);
            }
            else
            {
                /**
                 * Data Load-Back Stage
                 *
                 * In here, current range should use a register
                 *
                 * If storage is not consistent, then loading is required, except...
                 * first range because it has no previous range context, hence nothing to load
                 */
                if (cur != first && !cur->range.cached)
                {
                    allocator_variable_range_load(allocator, lid, cur);
                    // printf("load from stack location: %zd, ", allocator->stack.location[lid]);
                }

                // printf("use register: %zd", cur->range.alloc.reg);

                /**
                 * Eviction Stage
                 *
                 * If next range has inconsistent allocation, it implies that the register
                 * value needs to be moved to stack location, and load it back upon next range
                 *
                 * The algorithm is designed to *consecutively* use same register for as many
                 * ranges as possible per variable, see live_range_data_allocate_register()
                 */
                if (next && (next->range.spilled || !next->range.cached))
                {
                    live_range_data_allocate_stack_location(cur, allocator);
                    allocator_variable_range_evict(allocator, lid, cur);
                    // printf(", and evict to stack location %zd", cur->range.alloc.stack);
                }

                // printf("\n");
            }

            // now move all information
            allocator_fill_optimizer_register_allocation_info(allocator, cur);
            cur = next;
        }
    }
}

static void allocator_project_data_to_optimizer(linear_scan_allocator* allocator)
{
    switch (allocator->range_data.program)
    {
        case LINEAR_ALLOCATOR_RANGE_MERGE:
            allocator_project_data_to_optimizer_merge(allocator);
            break;
        case LINEAR_ALLOCATOR_RANGE_SPLIT:
            allocator_project_data_to_optimizer_split(allocator);
            break;
        default:
            fprintf(stderr, "TODO error: invalid linear allocator process %d, fallback to default (merge).\n", allocator->range_data.program);
            allocator_project_data_to_optimizer_merge(allocator);
            break;
    }

    // make data persistent
    optimizer_profile_apply(allocator->om, &allocator->profile, true);

    // do this last
    allocator->om->profile.num_registers = allocator->registers.num;
    allocator->om->profile.num_var_on_stack = allocator->stack.count;
}

static void allocator_populate_live_set(linear_scan_allocator* allocator, index_set* live_set, size_t inst_id)
{
    index_set_iterator it;
    size_t n;
    bool first = false;

    index_set_iterator_init(&it, live_set);

    while (!index_set_iterator_end(&it))
    {
        n = index_set_iterator_get(&it);

        // only process local variables
        if (is_def_register_optimizable_variable(allocator->om->variables[n].ref))
        {
            live_range_data_update(&allocator->range_data, varmap_idx2lid(allocator->om, n), inst_id);
        }

        index_set_iterator_next(&it);
    }

    index_set_iterator_release(&it);
}

static void init_linear_scan_allocator(
    optimizer* om,
    linear_scan_allocator* allocator,
    size_t num_registers,
    linear_allocator_range_process program
)
{
    // nothing to work on, then work nothing
    if (om->profile.num_instructions == 0) { return; }

    /**
     * Build Optimizer Data
     *
     * It uses a simple NULL check since this allocator does not
     * change CFG, so it simply relies on whatever optimizer holds
     *
     * If there is no data, allocator will create it
    */
    if (!om->variables) { optimizer_populate_variables(om); }
    if (!om->instructions) { optimizer_populate_instructions(om); }

    // facts needed
    optimizer_defuse_analyze(om);
    optimizer_liveness_analyze(om);

    size_t num_locals = om->profile.num_locals;
    size_t sz_bytes = sizeof(byte) * num_locals;
    size_t sz_items = sizeof(variable_live_range_item*) * num_locals;

    allocator->om = om;
    allocator->registers.occupied = (byte*)malloc_assert(sz_bytes);
    allocator->registers.last_range = (variable_live_range_item**)malloc_assert(sz_items);
    allocator->registers.num = num_registers;
    allocator->stack.allocated = (byte*)malloc_assert(sz_bytes);
    allocator->stack.location = (size_t*)malloc_assert(sizeof(size_t) * num_locals);
    allocator->stack.count = 0;

    // initizlize data
    memset(allocator->registers.occupied, 0, sz_bytes);
    memset(allocator->registers.last_range, 0, sz_items);
    memset(allocator->stack.allocated, 0, sz_bytes);
    optimizer_profile_copy(om, &allocator->profile);
    init_live_range_data(&allocator->range_data, om->profile.num_locals, program);

    // populate ranges
    for (size_t i = 0; i < om->profile.num_instructions; i++)
    {
        allocator_populate_live_set(allocator, &om->instructions[i].in, i);
        allocator_populate_live_set(allocator, &om->instructions[i].out, i);
    }

    // populate orders
    live_range_data_sort(&allocator->range_data);

    // finalize interval info
    for (size_t i = 0; i < allocator->range_data.num_ranges; i++)
    {
        allocator->range_data.active_order.array[i]->range.active_order = i;
    }
}

static void release_linear_scan_allocator(linear_scan_allocator* allocator)
{
    free(allocator->registers.occupied);
    free(allocator->registers.last_range);
    free(allocator->stack.allocated);
    free(allocator->stack.location);
    release_live_range_data(&allocator->range_data);
}

/**
 * Expire Old Intervals
*/
static void linear_scan_expire(linear_scan_allocator* allocator, variable_live_range_item* range_object)
{
    variable_live_range_data* rd = &allocator->range_data;

    for (size_t i = 0; i < rd->num_ranges; i++)
    {
        variable_live_range_item* item = rd->active_order.array[i];

        if (!rd->active_order.active[i]) { continue; }

        if (item->range.end >= range_object->range.start)
        {
            return;
        }

        live_range_data_active_remove(rd, i);
        live_range_data_release_register(item, allocator);
    }
}

/**
 * Spill Range
*/
static void linear_scan_spill(linear_scan_allocator* allocator, variable_live_range_item* item)
{
    variable_live_range_data* rd = &allocator->range_data;
    variable_live_range_item* active_last = NULL;

    // locate last one in active order
    for (size_t i = 0, idx_last_active; i < rd->num_ranges; i++)
    {
        idx_last_active = rd->num_ranges - i - 1;

        if (rd->active_order.active[idx_last_active])
        {
            active_last = rd->active_order.array[idx_last_active];
            break;
        }
    }

    // spill
    if (active_last && active_last->range.end > item->range.end)
    {
        live_range_data_allocate_register(item, active_last, allocator);
        live_range_data_allocate_stack_location(active_last, allocator);
        live_range_data_active_remove(rd, active_last->range.active_order);
        live_range_data_active_add(rd, item->range.active_order);
    }
    else
    {
        live_range_data_allocate_stack_location(item, allocator);
    }
}

/**
 * Linear Scan Register Allocator
*/
void optimizer_allocator_linear(optimizer* om, size_t num_avail_registers, linear_allocator_range_process program)
{
    linear_scan_allocator allocator;
    optimizer_profile profile;
    variable_live_range_data* rd;

    // init allocator
    init_linear_scan_allocator(om, &allocator, num_avail_registers, program);

    rd = &allocator.range_data;

    // main loop
    for (size_t i = 0; i < rd->num_ranges; i++)
    {
        variable_live_range_item* cur = rd->scan_order[i];

        linear_scan_expire(&allocator, cur);

        if (rd->active_order.active_count == num_avail_registers)
        {
            linear_scan_spill(&allocator, cur);
        }
        else
        {
            live_range_data_allocate_register(cur, NULL, &allocator);
            live_range_data_active_add(rd, cur->range.active_order);
        }
    }

    allocator_range_data_validate(&allocator);

    // flush result into optimizer instance
    allocator_project_data_to_optimizer(&allocator);
    release_linear_scan_allocator(&allocator);
}
