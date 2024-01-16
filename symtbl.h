/**
 * Java Reserved Words Lookup Table
 *
 * The size is carefully selected to make sure there is no collision:
 *
 * prime bucket size: true
 * total symbol: 50
 * load factor: 14.16%
 * memory: 4024 bytes
 *
 * when bucket size is not enforced to be prime, the difference is
 * not significant, so prime size is selected to maintain a more
 * uniform distribution of filled buckets across table
 *
 * This is a wrapper of hash_table so there is no data structure
 * for this module
 *
*/

#pragma once
#ifndef __COMPILER_SYMBOL_TABLE_H__
#define __COMPILER_SYMBOL_TABLE_H__

#include "types.h"
#include "langspec.h"
#include "hash.h"
#include "hash-table.h"

void init_symbol_table(hash_table* table);
void release_symbol_table(hash_table* table);
java_reserved_word* symbol_lookup(hash_table* table, char* string);

#endif
