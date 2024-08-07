#include "hash-table.h"

/**
 * Initialize hash table with estimated input data size
 *
 * By default the bucket size is HASH_TABLE_DEFAULT_BUCKET_SIZE
*/
void init_hash_table(hash_table* table, size_t data_size)
{
    data_size = data_size > 0 ? data_size : HASH_TABLE_DEFAULT_BUCKET_SIZE;

    table->bucket = (hash_pair**)malloc_assert(sizeof(hash_pair*) * data_size);
    table->bucket_size = data_size;
    reset_hash_table(table);
}

/**
 * release hash table
*/
void release_hash_table(hash_table* table, pair_data_deleter deleter)
{
    // free every header
    for (int i = 0; i < table->bucket_size; i++)
    {
        hash_pair* b = table->bucket[i];

        while (b)
        {
            // free pair data
            if (deleter)
            {
                (*deleter)(b->key, b->value);
            }

            // free the pair container and move on to next
            b = b->next;
            free(table->bucket[i]);
            table->bucket[i] = b;
        }
    }

    free(table->bucket);
}

/**
 * Reset the table
 *
 * It will reset all buckets without affecting the size of it
 *
 * WARNING: this function does not handle pair deletion
*/
void reset_hash_table(hash_table* table)
{
    table->num_filled = 0;
    table->num_pairs = 0;

    // this is important because we need to make sure 
    // all bucket header are set to NULL
    memset(table->bucket, 0, sizeof(hash_pair*) * table->bucket_size);
}

/**
 * calculate length of longest collision chain
 *
 * NOTE: this is expensive, only use it in reporting
*/
size_t hash_table_longest_chain_length(hash_table* table)
{
    size_t len = 0;

    for (int i = 0; i < table->bucket_size; i++)
    {
        hash_pair* b = table->bucket[i];
        size_t l = 0;

        while (b)
        {
            b = b->next;
            l++;
        }

        len = max(len, l);
    }

    return len;
}

/**
 * get number of bucket header that is filled
*/
size_t hash_table_bucket_head_filled(hash_table* table)
{
    return table->num_filled;
}

/**
 * get number of pairs inserted
*/
size_t hash_table_pairs(const hash_table* table)
{
    return table->num_pairs;
}

/**
 * calculate load factor of the table
*/
float hash_table_load_factor(hash_table* table)
{
    return (float)(table->num_pairs) / table->bucket_size;
}

/**
 * calculate memory size (in bytes) of hash table
*/
size_t hash_table_memory_size(hash_table* table)
{
    return sizeof(hash_pair) * table->num_pairs + sizeof(hash_pair*) * table->bucket_size;
}

/**
 * pair data full deleter
*/
void pair_data_delete_full(void* k, void* v)
{
    free(k);
    free(v);
}

/**
 * pair data key-only deleter
*/
void pair_data_delete_key(void* k, void* v)
{
    free(k);
}

/**
 * pair data value-only deleter
*/
void pair_data_delete_value(void* k, void* v)
{
    free(v);
}

/**
 * create a key-value pair
*/
hash_pair* new_pair(void* k, void* v, bytes_length len)
{
    hash_pair* b = (hash_pair*)malloc_assert(sizeof(hash_pair));

    b->key = k;
    b->value = v;
    b->key_length = len;
    b->prev = NULL;
    b->next = NULL;

    return b;
}

/**
 * key compare
 *
 * key MUST match length as well as the content
 * otherwise a substring of a key will produce false-positive match result
*/
bool hash_pair_key_compare(const hash_pair* pair, const void* key, size_t len)
{
    return pair->key_length == len && memcmp(pair->key, key, len) == 0;
}

/**
 * rehash: test if table needs resizing with one more element
 *
 * rehash does not guarantee prime size,
 * it simply doubles the size
 *
 * if table is not exceeding threshold, function is no-op
*/
static void rehash_test(hash_table* table)
{
    // load factor threshold check
    // but we need to look ahead by one more pair
    if (table->num_pairs + 1 < HASH_TABLE_REHASH_THRESHOLD * table->bucket_size)
    {
        return;
    }

    // new table has twice the size
    hash_table temp_table;
    init_hash_table(&temp_table, table->bucket_size * 2);

    // rehash all entries
    for (int i = 0; i < table->bucket_size; i++)
    {
        hash_pair* b = table->bucket[i];

        while (b)
        {
            bhash_table_insert(&temp_table, b->key, b->key_length, b->value);
            b = b->next;
        }
    }

    // now we override table
    // but before that we need to release it
    release_hash_table(table, NULL);
    memcpy(table, &temp_table, sizeof(hash_table));

    // now detach the shared reference from temp table
    // no deletion required because this is the only 
    // memory allocated
    temp_table.bucket = NULL;
}

/**
 * insert a pair
 *
 * NOTE: insert does not assert duplicated keys, it keeps them all
 * use test function to check for existence
*/
void bhash_table_insert(hash_table* table, void* k, bytes_length len, void* v)
{
    // first attempt to resize
    rehash_test(table);

    size_t index = bhash(k, len) % table->bucket_size;
    hash_pair* b = new_pair(k, v, len);

    // collision check
    if (table->bucket[index])
    {
        // O(1) insert: use new pair as the header
        table->bucket[index]->prev = b;
        b->next = table->bucket[index];
    }
    else
    {
        // otherwise a new bucket will be occupied
        table->num_filled++;
    }

    // insert it
    table->bucket[index] = b;
    table->num_pairs++;
}

/**
 * update existing pair
*/
bool bhash_table_update(hash_table* table, const void* k, bytes_length len, void* v)
{
    size_t index = bhash(k, len) % table->bucket_size;
    hash_pair* b = table->bucket[index];

    // lookup with collision check
    while (b)
    {
        if (hash_pair_key_compare(b, k, len))
        {
            b->value = v;
            return true;
        }

        b = b->next;
    }

    return false;
}

/**
 * key test
*/
bool bhash_table_test(const hash_table* table, const void* k, bytes_length len)
{
    size_t index = bhash(k, len) % table->bucket_size;
    hash_pair* b = table->bucket[index];

    // lookup with collision check
    while (b)
    {
        if (hash_pair_key_compare(b, k, len))
        {
            return true;
        }

        b = b->next;
    }

    return false;
}

/**
 * lookup a key and return the value if existed
 *
 * NOTE: this function cannot test if key exists when data
 * corresponding to a key in stor is designed to be empty
 * use test function to do key test
*/
void* bhash_table_find(hash_table* table, const void* k, bytes_length len)
{
    size_t index = bhash(k, len) % table->bucket_size;
    hash_pair* b = table->bucket[index];

    // lookup with collision check
    while (b)
    {
        if (hash_pair_key_compare(b, k, len))
        {
            return b->value;
        }

        b = b->next;
    }

    return NULL;
}

/**
 * find a pair and return the reference
 *
 * NULL returned from this method is unambiguous: means key does not exist
*/
hash_pair* bhash_table_get(const hash_table* table, const void* k, bytes_length len)
{
    size_t index = bhash(k, len) % table->bucket_size;
    hash_pair* b = table->bucket[index];

    // lookup with collision check
    while (b)
    {
        if (hash_pair_key_compare(b, k, len))
        {
            return b;
        }

        b = b->next;
    }

    return NULL;
}

/**
 * detach a specific pair from hash table
*/
hash_pair* bhash_table_remove(hash_table* table, const void* k, bytes_length len)
{
    size_t index = bhash(k, len) % table->bucket_size;
    hash_pair* b = table->bucket[index];

    // lookup with collision check
    while (b)
    {
        if (hash_pair_key_compare(b, k, len))
        {
            if (!b->prev && !b->next)
            {
                table->bucket[index] = NULL;
                table->num_filled--;
            }
            else
            {
                if (b->prev)
                {
                    b->prev->next = b->next;
                }

                if (b->next)
                {
                    b->next->prev = b->prev;
                }
            }

            table->num_pairs--;
            return b;
        }

        b = b->next;
    }

    return NULL;
}

/**
 * detach any pair from hash table
*/
hash_pair* bhash_table_pop(hash_table* table)
{
    hash_pair* b;

    for (size_t index = 0; index < table->bucket_size; index++)
    {
        b = table->bucket[index];

        if (b)
        {
            if (!b->next)
            {
                table->num_filled--;
            }

            table->bucket[index] = b->next;
            table->num_pairs--;
            return b;
        }
    }

    return NULL;
}

/**
 * string key insert
*/
void shash_table_insert(hash_table* table, char* k, void* v)
{
    bhash_table_insert(table, k, strlen(k), v);
}

/**
 * string key pair update
*/
bool shash_table_update(hash_table* table, const char* k, void* v)
{
    return bhash_table_update(table, k, strlen(k), v);
}

/**
 * string key test
*/
bool shash_table_test(hash_table* table, const char* k)
{
    return bhash_table_test(table, k, strlen(k));
}

/**
 * string key find
*/
void* shash_table_find(hash_table* table, const char* k)
{
    return bhash_table_find(table, k, strlen(k));
}

/**
 * string key get
*/
hash_pair* shash_table_get(const hash_table* table, const char* k)
{
    return bhash_table_get(table, k, strlen(k));
}

/**
 * bit-length data & string key insert
 *
 * size_t is:
 * 1. 4 bytes on 32-bit machine
 * 2. 8 bytes on 64-bit machine
 *
 * length is same as pointer length, so we do not allocate
 * anything in pair, just use pointer itself for the data
*/
void shash_table_bl_insert(hash_table* table, char* k, size_t v)
{
    bhash_table_insert(table, k, strlen(k), (void*)v);
}

/**
 * bit-length data & string key pair update
*/
bool shash_table_bl_update(hash_table* table, const char* k, size_t v)
{
    return bhash_table_update(table, k, strlen(k), (void*)v);
}

/**
 * bit-length data & string key test
*/
bool shash_table_bl_test(hash_table* table, const char* k)
{
    return bhash_table_test(table, k, strlen(k));
}

/**
 * bit-length data & string key find
 *
 * since we use the pointer itself to store the data so
 * the cast is safe here
*/
size_t shash_table_bl_find(hash_table* table, const char* k)
{
    return (size_t)bhash_table_find(table, k, strlen(k));
}
