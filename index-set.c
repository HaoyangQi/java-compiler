#include "index-set.h"

/**
 * get cell index of given index
*/
inline static size_t idx2cidx(size_t idx)
{
    return idx / INDEX_CELL_BITS;
}

/**
 * get index mask in cell
*/
inline static INDEX_CELL_TYPE idx2mask(size_t idx)
{
    return INDEX_CELL_MASK_IDX0 >> (idx % INDEX_CELL_BITS);
}

/**
 * recover original index
*/
inline static size_t cidx2idx(size_t cell_idx, size_t in_cell_idx)
{
    return cell_idx * INDEX_CELL_BITS + in_cell_idx;
}

/**
 * match set cell count with the longest one
*/
static void index_set_match_cell_count(index_set* ixs1, index_set* ixs2)
{
    if (ixs1->n_cell == ixs2->n_cell) { return; }

    index_set* shorter = ixs1->n_cell < ixs2->n_cell ? ixs1 : ixs2;
    index_set* longer = ixs1 == shorter ? ixs2 : ixs1;

    shorter->data = (INDEX_CELL_TYPE*)realloc_assert(shorter->data, sizeof(INDEX_CELL_TYPE) * longer->n_cell);
    memset(shorter->data + shorter->n_cell, 0, sizeof(INDEX_CELL_TYPE) * (longer->n_cell - shorter->n_cell));
    shorter->n_cell = longer->n_cell;
}

/**
 * match set cell count with the source, if it is longer than dest
*/
static void index_set_match_source_cell_count(index_set* dest, const index_set* src)
{
    if (dest->n_cell >= src->n_cell) { return; }

    dest->data = (INDEX_CELL_TYPE*)realloc_assert(dest->data, sizeof(INDEX_CELL_TYPE) * src->n_cell);
    memset(dest->data + dest->n_cell, 0, sizeof(INDEX_CELL_TYPE) * (src->n_cell - dest->n_cell));
    dest->n_cell = src->n_cell;
}

/**
 * initialize set with fill option
 *
 * it will initialize set size that can hold range [0, upper_bound)
 *
 * by default, it reserves 1 cell, hence [0, INDEX_CELL_BITS)
*/
void init_index_set_fill(index_set* ixs, size_t upper_bound, bool fill)
{
    ixs->n_cell = idx2cidx(upper_bound) + 1;
    ixs->data = (INDEX_CELL_TYPE*)malloc_assert(sizeof(INDEX_CELL_TYPE) * ixs->n_cell);

    // this is important because set is empty by default
    memset(ixs->data, fill ? 0xFF : 0, sizeof(INDEX_CELL_TYPE) * ixs->n_cell);
}

/**
 * initialize set
 *
 * it will initialize set size that can hold range [0, upper_bound)
 *
 * by default, it reserves 1 cell, hence [0, INDEX_CELL_BITS)
*/
void init_index_set(index_set* ixs, size_t upper_bound)
{
    init_index_set_fill(ixs, upper_bound, false);
}

/**
 * initialize a set with an existing set
 *
 * all content of the source will also be copied and preserved
*/
void init_index_set_copy(index_set* dest, const index_set* src)
{
    size_t sz = sizeof(INDEX_CELL_TYPE) * src->n_cell;

    dest->n_cell = src->n_cell;
    dest->data = (INDEX_CELL_TYPE*)malloc_assert(sz);

    memcpy(dest->data, src->data, sz);
}

/**
 * release set
*/
void release_index_set(index_set* ixs)
{
    if (ixs) { free(ixs->data); }
}

/**
 * add element to set
*/
void index_set_add(index_set* ixs, size_t index)
{
    size_t cidx = idx2cidx(index);
    size_t n = cidx + 1;

    // resize
    if (cidx >= ixs->n_cell)
    {
        ixs->data = (INDEX_CELL_TYPE*)realloc_assert(ixs->data, sizeof(INDEX_CELL_TYPE) * n);
        memset(ixs->data + ixs->n_cell, 0, sizeof(INDEX_CELL_TYPE) * (n - ixs->n_cell));
        ixs->n_cell = n;
    }

    ixs->data[cidx] |= idx2mask(index);
}

/**
 * remove element from set
*/
void index_set_remove(index_set* ixs, size_t index)
{
    size_t cidx = idx2cidx(index);

    if (cidx >= ixs->n_cell) { return; }

    ixs->data[cidx] &= ~idx2mask(index);
}

/**
 * pop one index value
 *
 * if output argument is not set, it behaves exactly as: !index_set_empty()
 * it returns true if an element is popped, false if set is empty
*/
bool index_set_pop(index_set* ixs, size_t* idx)
{
    for (size_t i = 0; i < ixs->n_cell; i++)
    {
        if (ixs->data[i] != 0)
        {
            size_t out;

            for (size_t j = 0; j < INDEX_CELL_BITS; j++)
            {
                INDEX_CELL_TYPE mask = idx2mask(j);

                if (ixs->data[i] & mask)
                {
                    out = cidx2idx(i, j);

                    if (idx) { *idx = out; }

                    // fast index_set_remove, since we already know cell index and mask
                    ixs->data[i] &= ~mask;

                    return true;
                }
            }
        }
    }

    return false;
}

/**
 * clear set to make it a null set
*/
void index_set_clear(index_set* ixs)
{
    for (size_t i = 0; i < ixs->n_cell; i++)
    {
        ixs->data[i] = 0;
    }
}

/**
 * test if element exists in set
*/
bool index_set_contains(const index_set* ixs, size_t index)
{
    size_t cidx = idx2cidx(index);
    return cidx < ixs->n_cell && (bool)(ixs->data[cidx] & idx2mask(index));
}

/**
 * test if empty set
*/
bool index_set_empty(const index_set* ixs)
{
    for (size_t i = 0; i < ixs->n_cell; i++)
    {
        if (ixs->data[i] != 0) { return false; }
    }

    return true;
}

/**
 * count elements in set
*/
size_t index_set_count(const index_set* ixs)
{
    size_t count = 0;

    for (size_t i = 0; i < ixs->n_cell; i++)
    {
        INDEX_CELL_TYPE c = ixs->data[i];

        for (size_t j = 0; j < INDEX_CELL_BITS; j++)
        {
            if ((c >> j) & 1)
            {
                count++;
            }
        }
    }

    return count;
}

/**
 * determine if sets are equal
*/
bool index_set_equal(const index_set* ixs1, const index_set* ixs2)
{
    const index_set* longer = ixs1->n_cell >= ixs2->n_cell ? ixs1 : ixs2;
    const index_set* shorter = longer == ixs1 ? ixs2 : ixs1;

    // traverse as many as possible
    for (size_t i = 0; i < longer->n_cell; i++)
    {
        if ((i < shorter->n_cell && longer->data[i] != shorter->data[i]) ||
            (i >= shorter->n_cell && longer->data[i] != 0))
        {
            return false;
        }
    }

    return true;
}

/**
 * union sets
*/
void index_set_union(index_set* dest, const index_set* src)
{
    index_set_match_source_cell_count(dest, src);

    // source cell count is always <= dest cell count here
    for (size_t i = 0; i < src->n_cell; i++)
    {
        dest->data[i] |= src->data[i];
    }
}

/**
 * intersect sets
*/
void index_set_intersect(index_set* dest, const index_set* src)
{
    index_set_match_source_cell_count(dest, src);

    // source cell count is always <= dest cell count here
    for (size_t i = 0; i < src->n_cell; i++)
    {
        dest->data[i] &= src->data[i];
    }
}

/**
 * subtract sets: remove all dest's elements that exist in both sets
*/
void index_set_subtract(index_set* dest, const index_set* src)
{
    index_set_match_source_cell_count(dest, src);

    // source cell count is always <= dest cell count here
    for (size_t i = 0; i < src->n_cell; i++)
    {
        dest->data[i] &= ~src->data[i];
    }
}

/**
 * generate an array of index
*/
size_t index_set_to_array(index_set* set, size_t* buf)
{
    if (!set)
    {
        buf = NULL;
        return 0;
    }

    index_set_iterator it;
    size_t i = 0;
    size_t count = index_set_count(set);

    if (!count)
    {
        buf = NULL;
        return 0;
    }

    index_set_iterator_init(&it, set);

    while (!index_set_iterator_end(&it))
    {
        buf[i++] = index_set_iterator_get(&it);
        index_set_iterator_next(&it);
    }

    index_set_iterator_release(&it);

    return count;
}

/**
 * locate first available element (including current one)
*/
static void index_set_iterator_locate_next(index_set_iterator* itor)
{
    for (; itor->cur_cell < itor->set->n_cell; itor->cur_cell++)
    {
        if (itor->set->data[itor->cur_cell] != 0)
        {
            for (; itor->cur_offset < INDEX_CELL_BITS; itor->cur_offset++)
            {
                if (itor->set->data[itor->cur_cell] & idx2mask(itor->cur_offset))
                {
                    // break everything
                    return;
                }
            }
        }
    }
}

/**
 * initialize iterator
*/
void index_set_iterator_init(index_set_iterator* itor, index_set* set)
{
    itor->set = set;
    itor->cur_cell = 0;
    itor->cur_offset = 0;

    index_set_iterator_locate_next(itor);
}

/**
 * release iterator instance
*/
void index_set_iterator_release(index_set_iterator* itor)
{
    // so far, no-op
}

/**
 * move iterator to next available element
*/
void index_set_iterator_next(index_set_iterator* itor)
{
    // progress
    itor->cur_offset = (itor->cur_offset + 1) % INDEX_CELL_BITS;

    // progress cell carry
    if (itor->cur_offset == 0)
    {
        itor->cur_cell++;
    }

    index_set_iterator_locate_next(itor);
}

/**
 * get current element in set using iterator
*/
size_t index_set_iterator_get(const index_set_iterator* itor)
{
    return cidx2idx(itor->cur_cell, itor->cur_offset);
}

/**
 * check if iterator has reached the end
*/
bool index_set_iterator_end(const index_set_iterator* itor)
{
    return itor->cur_cell >= itor->set->n_cell;
}
