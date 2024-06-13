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
    size_t start;
    size_t end;
    bool spilled;
    size_t active_order;

    struct
    {
        size_t reg;
        size_t stack;
    } alloc;
} live_interval;

typedef struct _linear_scan_allocator
{
    // optimizer instance
    optimizer* om;

    /**
     * Register Pool & Stack Spill Trace
     *
     * index of register_occupied is actual register ID
    */
    byte* register_occupied;
    size_t num_registers;
    size_t num_on_stack;

    /**
     * Variable Liveness Interval (Local Only)
     *
     * array is indexed by lid
     *
     * Member variable will not be processed,
     * so num_intervals = num_locals
    */
    live_interval* intervals;
    size_t num_intervals;

    /**
     * The interval traserve order of intervals (inc start)
     *
     * it holds index order of "intervals"
    */
    size_t* scan_order;

    /**
     * The active traverse order of intervals (inc end)
     *
     * and the active set, which holds num_intervals
     * elements max, and holds bool of index of element in
     * active_order to show if it exists
     *
     * idx_last_active: index of active_order that is the last
     * element in active array
    */
    size_t* active_order;
    byte* active;
    size_t active_count;
} linear_scan_allocator;

static void __debug_print_interval(const live_interval* interval)
{
    printf("(%zd, %zd), active order: %zd, alloc: %c%zd\n",
        interval->start,
        interval->end,
        interval->active_order,
        interval->spilled ? 's' : 'r',
        interval->spilled ? interval->alloc.stack : interval->alloc.reg
    );
}

static void __debug_print_linear_allocator(const linear_scan_allocator* allocator)
{
    printf("\n===== LINEAR ALLOCATOR INFO =====\n");

    printf("Registers: ");
    for (size_t i = 0; i < allocator->num_registers; i++)
    {
        printf("%d ", allocator->register_occupied[i]);
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

    printf("Number of variables using stack: %zd\n", allocator->num_on_stack);

    printf("Invervals: \n");
    for (size_t i = 0; i < allocator->num_intervals; i++)
    {
        printf("[%zd]: ", i);
        __debug_print_interval(&allocator->intervals[i]);
    }

    printf("Invervals In Scan Order (Inc, Start): \n");
    for (size_t i = 0; i < allocator->num_intervals; i++)
    {
        printf("--- Local Var %zd: ", allocator->scan_order[i]);
        __debug_print_interval(&allocator->intervals[allocator->scan_order[i]]);
    }

    printf("Invervals In Active Order (Inc, End): \n");
    for (size_t i = 0; i < allocator->num_intervals; i++)
    {
        printf("%s Local Var %zd: ", allocator->active[i] ? "[.]" : "[ ]", allocator->active_order[i]);
        __debug_print_interval(&allocator->intervals[allocator->active_order[i]]);
    }

    printf("\n");
}

/**
 * allocate a register from the pool
 *
 * NOTE: this is NOT a generic purpose method, based on the algorithm,
 * this method should never throw, if it does, it means there is an
 * error in algorithm
*/
static size_t register_allocate(linear_scan_allocator* allocator)
{
    size_t i = 0;

    for (i = 0; i < allocator->num_registers && allocator->register_occupied[i]; i++);

    allocator->register_occupied[i] = 1;
    return i;
}

/**
 * free a register and return it to the pool
 *
 * NOTE: this is NOT a generic purpose method, based on the algorithm,
 * this method should never throw, if it does, it means there is an
 * error in algorithm
*/
static void register_release(linear_scan_allocator* allocator, size_t reg)
{
    allocator->register_occupied[reg] = 0;
}

/**
 * allocate a stack location
*/
static size_t stack_allocate(linear_scan_allocator* allocator)
{
    size_t s = allocator->num_on_stack++;
    return s;
}

static void active_interval_add(linear_scan_allocator* allocator, size_t idx_active_order)
{
    allocator->active[idx_active_order] = 1;
    allocator->active_count++;
}

static void active_interval_remove(linear_scan_allocator* allocator, size_t idx_active_order)
{
    allocator->active[idx_active_order] = 0;
    allocator->active_count--;
}

/**
 * Initialize Range data of interval object to mark it as "uninitialized"
 *
 * It sets start location to a max value that it can never reach
*/
static void interval_range_data_init(linear_scan_allocator* allocator, live_interval* interval)
{
    interval->start = allocator->om->profile.num_instructions;
}

/**
 * Check if an interval is valid
 *
 * Based on interval_init, this condition is sufficient
*/
static bool interval_valid(const linear_scan_allocator* allocator, const live_interval* interval)
{
    return interval->start < allocator->om->profile.num_instructions;
}

static void interval_assign_register(live_interval* interval, size_t reg)
{
    interval->alloc.reg = reg;
    interval->spilled = false;
}

static void interval_assign_stack_location(live_interval* interval, size_t loc)
{
    interval->alloc.stack = loc;
    interval->spilled = true;
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

/**
 * interval sort algorithm (merge sort)
 *
 * it sorts range [from, len - 1]
*/
static void sort_intervals_internal(size_t* arr, size_t* buf, size_t from, size_t len, const live_interval* interval, interval_sort program)
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
            if (!interval_compare(&interval[arr[from]], &interval[arr[to]], program))
            {
                size_t tmp = arr[from];

                arr[from] = arr[to];
                arr[to] = tmp;
            }
            break;
        default:
            sort_intervals_internal(arr, buf, div_from[0], div_len[0], interval, program);
            sort_intervals_internal(arr, buf, div_from[1], div_len[1], interval, program);

            // fill
            while (div_len[0] || div_len[1])
            {
                if (div_len[0] && div_len[1])
                {
                    size_t v0 = arr[div_from[0]];
                    size_t v1 = arr[div_from[1]];

                    if (interval_compare(&interval[v0], &interval[v1], program))
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

            memcpy(arr + from, buf + from, sizeof(size_t) * len);
            break;
    }
}

/**
 * Return a copy of sorted interval array
*/
static size_t* sort_intervals(live_interval* interval, size_t count, interval_sort program)
{
    size_t* r = (size_t*)malloc_assert(sizeof(size_t) * count);
    size_t* buf = (size_t*)malloc_assert(sizeof(size_t) * count);

    // initialize
    for (size_t i = 0; i < count; i++)
    {
        r[i] = i;
    }

    sort_intervals_internal(r, buf, 0, count, interval, program);
    free(buf);

    return r;
}

static void live_set_to_interval(linear_scan_allocator* allocator, index_set* live_set, size_t inst_id)
{
    index_set_iterator it;
    size_t n;
    bool first = false;

    index_set_iterator_init(&it, live_set);

    while (!index_set_iterator_end(&it))
    {
        n = index_set_iterator_get(&it);

        // only process local variables
        if (!varmap_idx_is_member(allocator->om, n))
        {
            n = varmap_idx2lid(allocator->om, n);

            if (!interval_valid(allocator, &allocator->intervals[n]))
            {
                // if first set, fill both
                allocator->intervals[n].start = inst_id;
                allocator->intervals[n].end = inst_id;
            }
            else if (inst_id < allocator->intervals[n].start)
            {
                allocator->intervals[n].start = inst_id;
            }
            else if (inst_id > allocator->intervals[n].end)
            {
                allocator->intervals[n].end = inst_id;
            }
        }

        index_set_iterator_next(&it);
    }

    index_set_iterator_release(&it);
}

static void init_linear_scan_allocator(optimizer* om, linear_scan_allocator* allocator, size_t num_registers)
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

    size_t num_intervals = om->profile.num_locals;
    size_t sz_intervals = sizeof(live_interval) * num_intervals;
    size_t sz_byte_intervals = sizeof(byte) * num_intervals;

    allocator->om = om;
    allocator->num_intervals = num_intervals;
    allocator->num_registers = num_registers;
    allocator->num_on_stack = 0;
    allocator->register_occupied = (byte*)malloc_assert(sz_byte_intervals);
    allocator->intervals = (live_interval*)malloc_assert(sz_intervals);
    allocator->active = (byte*)malloc_assert(sz_byte_intervals);
    allocator->active_count = 0;

    // initizlize data
    memset(allocator->register_occupied, 0, sz_byte_intervals);
    memset(allocator->intervals, 0, sz_intervals);
    memset(allocator->active, 0, sz_byte_intervals);

    // fill initial interval range data 
    // start point need to be max value by default, end point needs to be 0 by default
    for (size_t i = 0; i < num_intervals; i++)
    {
        interval_range_data_init(allocator, &allocator->intervals[i]);
    }

    // fill intervals
    for (size_t i = 0; i < om->profile.num_instructions; i++)
    {
        live_set_to_interval(allocator, &om->instructions[i].in, i);
        live_set_to_interval(allocator, &om->instructions[i].out, i);
    }

    // initialize useful orders
    allocator->scan_order = sort_intervals(allocator->intervals, num_intervals, INTERVAL_SORT_START | INTERVAL_SORT_INCREASE);
    allocator->active_order = sort_intervals(allocator->intervals, num_intervals, INTERVAL_SORT_END | INTERVAL_SORT_INCREASE);

    // finalize interval info
    for (size_t i = 0; i < num_intervals; i++)
    {
        allocator->intervals[allocator->active_order[i]].active_order = i;
    }
}

static void release_linear_scan_allocator(linear_scan_allocator* allocator)
{
    free(allocator->intervals);
    free(allocator->scan_order);
    free(allocator->active_order);
    free(allocator->register_occupied);
    free(allocator->active);
}

/**
 * Expire Old Intervals
*/
static void linear_scan_expire(linear_scan_allocator* allocator, size_t idx_interval)
{
    for (size_t i = 0; i < allocator->num_intervals; i++)
    {
        live_interval* cur = &allocator->intervals[allocator->active_order[i]];

        if (!allocator->active[i]) { continue; }

        if (cur->end >= allocator->intervals[idx_interval].start)
        {
            return;
        }

        active_interval_remove(allocator, i);
        register_release(allocator, cur->alloc.reg);
    }
}

/**
 * Spill At Interval
*/
static void linear_scan_spill(linear_scan_allocator* allocator, size_t idx_interval)
{
    if (allocator->active_count == 0) { return; }

    // locate last one in active order
    size_t idx_last_active;
    for (size_t i = 0; i < allocator->num_intervals; i++)
    {
        idx_last_active = allocator->num_intervals - i - 1;
        if (allocator->active[idx_last_active]) { break; }
    }

    live_interval* spill = &allocator->intervals[allocator->active_order[idx_last_active]];
    live_interval* in = &allocator->intervals[idx_interval];

    if (spill->end > in->end)
    {
        interval_assign_register(in, spill->alloc.reg);
        interval_assign_stack_location(spill, stack_allocate(allocator));
        active_interval_remove(allocator, idx_last_active);
        active_interval_add(allocator, in->active_order);
    }
    else
    {
        interval_assign_stack_location(in, stack_allocate(allocator));
    }
}

/**
 * Linear Scan Register Allocator
*/
void optimizer_allocator_linear(optimizer* om, size_t num_avail_registers)
{
    linear_scan_allocator allocator;

    // init allocator
    init_linear_scan_allocator(om, &allocator, num_avail_registers);
    __debug_print_linear_allocator(&allocator);

    // main loop
    for (size_t i = 0; i < allocator.num_intervals; i++)
    {
        size_t idx_current_interval = allocator.scan_order[i];
        live_interval* in = &allocator.intervals[idx_current_interval];

        // ignore invalid interval
        if (!interval_valid(&allocator, in))
        {
            definition* var = om->variables[varmap_lid2idx(om, idx_current_interval)].ref;

            if (is_def_user_defined_variable(var))
            {
                fprintf(stderr, "TODO warning: unused variable: lid=%zd\n", var->lid);
            }
            else
            {
                fprintf(stderr, "TODO info: variable lid=%zd has uninitialized interval.\n", var->lid);
            }

            continue;
        }

        linear_scan_expire(&allocator, idx_current_interval);

        if (allocator.active_count == num_avail_registers)
        {
            linear_scan_spill(&allocator, idx_current_interval);
        }
        else
        {
            interval_assign_register(in, register_allocate(&allocator));
            active_interval_add(&allocator, in->active_order);
        }
    }

    // flush result into optimizer instance
    om->profile.reg_count = num_avail_registers;
    om->profile.num_var_on_stack = allocator.num_on_stack;
    for (size_t i = 0; i < allocator.num_intervals; i++)
    {
        variable_item* var = &om->variables[varmap_lid2idx(om, i)];
        live_interval* in = &allocator.intervals[i];

        if (interval_valid(&allocator, in))
        {
            var->alloc_type = in->spilled ? VAR_ALLOC_STACK : VAR_ALLOC_REGISTER;
            var->alloc_loc = in->spilled ? in->alloc.stack : in->alloc.reg;
        }
        else
        {
            var->alloc_type = VAR_ALLOC_UNDEFINED;
        }
    }

    __debug_print_linear_allocator(&allocator);
    release_linear_scan_allocator(&allocator);
}
