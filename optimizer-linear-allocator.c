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

static void delete_intervals(live_interval* intervals)
{
    free(intervals);
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
 *
 * (default: INTERVAL_SORT_DECREASE | INTERVAL_SORT_END)
*/
static bool interval_compare(const live_interval* a, const live_interval* b, interval_sort program)
{
    bool sinc = a->start < b->start;
    bool einc = a->end < b->end;
    bool prog_inc = (program & INTERVAL_SORT_INCREASE) ? true : false;

    return (program & INTERVAL_SORT_START) ? (prog_inc == sinc) : (prog_inc == einc);
}

static void index_swap(size_t* a, size_t* b)
{
    size_t tmp = *b;
    *b = *a;
    *a = tmp;
}

/**
 * interval sort algorithm (merge sort)
 *
 * it sorts range [from, len - 1]
*/
static void sort_intervals_internal(size_t* arr, size_t* buf, size_t from, size_t len, const live_interval* interval, interval_sort program)
{
    size_t to = from + len - 1;
    size_t len_half = len / 2;
    size_t div_from[2] = { from, from + len_half };
    size_t div_len[2] = { len_half, len - len_half };
    size_t i = from;
    size_t v0, v1;

    switch (len - from)
    {
        case 0:
        case 1:
            // no need to sort
            break;
        case 2:
            // minimum case: swap once
            if (!interval_compare(&interval[arr[from]], &interval[arr[to]], program))
            {
                index_swap(&arr[from], &arr[to]);
            }
            break;
        default:
            sort_intervals_internal(arr, buf, div_from[0], div_len[0], interval, program);
            sort_intervals_internal(arr, buf, div_from[1], div_len[1], interval, program);

            // fill
            while (true)
            {
                if (div_len[0])
                {
                    v0 = arr[div_from[0]];

                    if (div_len[1])
                    {
                        v1 = arr[div_from[1]];

                        if (interval_compare(&interval[v0], &interval[v1], program))
                        {
                            buf[i++] = v0;
                            buf[i++] = v1;
                        }
                        else
                        {
                            buf[i++] = v1;
                            buf[i++] = v0;
                        }

                        div_from[1]++;
                        div_len[1]--;
                    }
                    else
                    {
                        buf[i++] = v0;
                    }

                    div_from[0]++;
                    div_len[0]--;
                }
                else if (div_len[1])
                {
                    buf[i++] = arr[div_from[1]];
                    div_from[1]++;
                    div_len[1]--;
                }
                else
                {
                    break;
                }
            }

            memcpy(arr + from, buf + from, sizeof(size_t) * len);
            free(buf);
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

            if (first)
            {
                first = false;
                allocator->intervals[n].start = inst_id;
                allocator->intervals[n].end = inst_id;
            }
            else
            {
                allocator->intervals[n].start = min(allocator->intervals[n].start, inst_id);
                allocator->intervals[n].end = max(allocator->intervals[n].end, inst_id);
            }
        }

        index_set_iterator_next(&it);
    }

    index_set_iterator_release(&it);
}

static void init_linear_scan_allocator(optimizer* om, linear_scan_allocator* allocator, size_t num_registers)
{
    size_t num_intervals = om->profile.num_locals;
    size_t sz_intervals = sizeof(live_interval) * num_intervals;

    allocator->om = om;
    allocator->num_intervals = num_intervals;
    allocator->num_registers = num_registers;
    allocator->num_on_stack = 0;
    allocator->register_occupied = (byte*)malloc_assert(sizeof(byte) * num_intervals);
    allocator->intervals = (live_interval*)malloc_assert(sz_intervals);
    allocator->active = (byte*)malloc_assert(sizeof(byte) * num_intervals);
    allocator->active_count = 0;

    // initialize intervals
    memset(allocator->intervals, 0, sz_intervals);

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
    delete_intervals(allocator->intervals);
    free(allocator->scan_order);
    free(allocator->active_order);
    free(&allocator->register_occupied);
    free(&allocator->active);
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

void optimizer_allocator_linear(optimizer* om, size_t num_avail_registers)
{
    linear_scan_allocator allocator;

    // init allocator
    init_linear_scan_allocator(om, &allocator, num_avail_registers);

    // main loop
    for (size_t i = 0; i < allocator.num_intervals; i++)
    {
        size_t idx_current_interval = allocator.scan_order[i];
        live_interval* in = &allocator.intervals[idx_current_interval];

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

        var->alloc_type = in->spilled ? VAR_ALLOC_STACK : VAR_ALLOC_REGISTER;
        var->alloc_loc = in->spilled ? in->alloc.stack : in->alloc.reg;
    }

    release_linear_scan_allocator(&allocator);
}
