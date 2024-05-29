/**
 * Index Set
 *
 * Index set uses bit-mask to implement set properties for any data
 * that is associated with an index identifier
 *
 * each cell in the set has 32(64) bits for corresponding number of indicies;
 * to locate the unique position of an index number:
 *
 * For any index number i:
 * cell index: i / INDEX_CELL_BITS
 * bit shift-right amount: i % INDEX_CELL_BITS
 * union: bit-OR for every cell
 * intersect: bit-AND for every cell
 *
*/

#pragma once
#ifndef __UTIL_INDEX_SET_H__
#define __UTIL_INDEX_SET_H__

#include "types.h"

#define INDEX_CELL_TYPE uint32_t
#define INDEX_CELL_BITS 32 // sizeof(INDEX_CELL_TYPE) * 8
#define INDEX_CELL_MASK_IDX0 ((INDEX_CELL_TYPE)(0x80000000))
#define INDEX_CELL_COUNT_DEFAULT 0

/**
 * 64-bit profile, not used
*/
// #define INDEX_CELL_TYPE uint64_t
// #define INDEX_CELL_BITS 64 // sizeof(INDEX_CELL_TYPE) * 8
// #define INDEX_CELL_MASK_IDX0 0x8000000000000000

typedef struct _index_set
{
    INDEX_CELL_TYPE* data;
    size_t n_cell;
} index_set;

typedef struct _index_set_iterator
{
    index_set* set;
    size_t cur_cell;
    size_t cur_offset;
} index_set_iterator;

void init_index_set_fill(index_set* ixs, size_t upper_bound, bool fill);
void init_index_set(index_set* ixs, size_t upper_bound);
void init_index_set_copy(index_set* dest, const index_set* src);
void release_index_set(index_set* ixs);

void index_set_add(index_set* ixs, size_t index);
void index_set_remove(index_set* ixs, size_t index);
bool index_set_pop(index_set* ixs, size_t* idx);
void index_set_clear(index_set* ixs);
bool index_set_contains(const index_set* ixs, size_t index);
bool index_set_empty(const index_set* ixs);
size_t index_set_count(const index_set* ixs);
bool index_set_equal(const index_set* ixs1, const index_set* ixs2);
void index_set_union(index_set* dest, const index_set* src);
void index_set_intersect(index_set* dest, const index_set* src);
void index_set_subtract(index_set* dest, const index_set* src);
size_t index_set_to_array(index_set* set, size_t* buf);

void index_set_iterator_init(index_set_iterator* itor, index_set* set);
void index_set_iterator_release(index_set_iterator* itor);
void index_set_iterator_next(index_set_iterator* itor);
size_t index_set_iterator_get(const index_set_iterator* itor);
bool index_set_iterator_end(const index_set_iterator* itor);

#endif
