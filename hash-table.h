/**
 * Unordered Map
 *
 * This table does NOT copy key or value to the store,
 * only references them
 *
 * Table uses separate chaining approach to resolve hash collision
 *
*/

#pragma once
#ifndef __COMPILER_HASH_TABLE_H__
#define __COMPILER_HASH_TABLE_H__

#include "hash.h"
#include "types.h"

/**
 * rehash load factor threshold
 *
 * separate chaining can handle potentially infinite number of
 * collisions, but the load factor (average how many entries
 * are in one bucket) needs to be controlled to keep it fast
*/
#define HASH_TABLE_REHASH_THRESHOLD (0.8)

typedef struct _hash_bucket
{
    void* key;
    void* value;
    bytes_length key_length;
    struct _hash_bucket* next;
} hash_pair;

typedef struct
{
    /* store */
    hash_pair** bucket;
    /* number of buckets */
    size_t bucket_size;
    /* number of bucket head occupied */
    size_t num_filled;
    /* number of total inserted elements */
    size_t num_pairs;
} hash_table;

void init_hash_table(hash_table* table, size_t data_size);
void release_hash_table(hash_table* table);

size_t hash_table_longest_chain_length(hash_table* table);
size_t hash_table_bucket_head_filled(hash_table* table);
size_t hash_table_pairs(hash_table* table);
float hash_table_load_factor(hash_table* table);
size_t hash_table_memory_size(hash_table* table);

void shash_table_insert(hash_table* table, char* k, void* v);
void* shash_table_find(hash_table* table, char* k);

void bhash_table_insert(hash_table* table, void* k, bytes_length len, void* v);
void* bhash_table_find(hash_table* table, void* k, bytes_length len);

#endif
