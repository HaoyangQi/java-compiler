/**
 * Unordered Map
 *
 * This table does NOT copy key or value to the store,
 * only references them, but user can specify a deleter
 * to do extra deletion work during deconstruction
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
#define HASH_TABLE_REHASH_THRESHOLD (0.75)

/**
 * default table size
*/
#define HASH_TABLE_DEFAULT_BUCKET_SIZE (11)

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

// pair data deleter interface
typedef void (*pair_data_deleter)(void* key, void* value);

void init_hash_table(hash_table* table, size_t data_size);
void release_hash_table(hash_table* table, pair_data_deleter deleter);

size_t hash_table_longest_chain_length(hash_table* table);
size_t hash_table_bucket_head_filled(hash_table* table);
size_t hash_table_pairs(hash_table* table);
float hash_table_load_factor(hash_table* table);
size_t hash_table_memory_size(hash_table* table);

void pair_data_delete_full(void* k, void* v);
void pair_data_delete_key(void* k, void* v);
void pair_data_delete_value(void* k, void* v);

void bhash_table_insert(hash_table* table, void* k, bytes_length len, void* v);
bool bhash_table_update(hash_table* table, void* k, bytes_length len, void* v);
bool bhash_table_test(hash_table* table, void* k, bytes_length len);
void* bhash_table_find(hash_table* table, void* k, bytes_length len);
hash_pair* bhash_table_get(hash_table* table, void* k, bytes_length len);

void shash_table_insert(hash_table* table, char* k, void* v);
bool shash_table_update(hash_table* table, char* k, void* v);
bool shash_table_test(hash_table* table, char* k);
void* shash_table_find(hash_table* table, char* k);
hash_pair* shash_table_get(hash_table* table, char* k);

void shash_table_bl_insert(hash_table* table, char* k, size_t v);
bool shash_table_bl_update(hash_table* table, char* k, size_t v);
bool shash_table_bl_test(hash_table* table, char* k);
size_t shash_table_bl_find(hash_table* table, char* k);

#endif
