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

void init_index_set(index_set* ixs, size_t index_data_size);
void init_index_set_copy(index_set* dest, const index_set* src);
void release_index_set(index_set* ixs);

void index_set_add(index_set* ixs, size_t index);
void index_set_remove(index_set* ixs, size_t index);
void index_set_clear(index_set* ixs);
bool index_set_contains(const index_set* ixs, size_t index);
bool index_set_empty(const index_set* ixs);
size_t index_set_count(const index_set* ixs);
bool index_set_equal(const index_set* ixs1, const index_set* ixs2);
void index_set_union(index_set* dest, const index_set* src);
void index_set_intersect(index_set* dest, const index_set* src);

#endif
