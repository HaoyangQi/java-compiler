#include "ir.h"

void init_node_set(node_set* s)
{
    init_hash_table(&s->tbl, HASH_TABLE_DEFAULT_BUCKET_SIZE);
}

void init_node_set_with_copy(node_set* dest, const node_set* src)
{
    if (dest == src)
    {
        return;
    }

    hash_pair* p;
    hash_pair* b;

    init_hash_table(&dest->tbl, src->tbl.bucket_size);
    dest->tbl.num_filled = src->tbl.num_filled;
    dest->tbl.num_pairs = src->tbl.num_pairs;
    dest->tbl.bucket_size = src->tbl.bucket_size;

    // since we are copying, so no needs to insert
    for (int i = 0; i < src->tbl.bucket_size; i++)
    {
        b = src->tbl.bucket[i];

        while (b)
        {
            p = new_pair(b->key, b->value, sizeof(basic_block));
            p->next = dest->tbl.bucket[i];
            dest->tbl.bucket[i] = p;

            b = b->next;
        }
    }
}

void release_node_set(node_set* s)
{
    if (s)
    {
        release_hash_table(&s->tbl, NULL);
    }
}

bool node_set_contains(const node_set* s, const basic_block* entry)
{
    return bhash_table_get(&s->tbl, &entry->id, sizeof(size_t)) != NULL;
}

void node_set_add(node_set* s, basic_block* entry)
{
    if (!node_set_contains(s, entry))
    {
        bhash_table_insert(&s->tbl, &entry->id, sizeof(size_t), entry);
    }
}

void node_set_remove(node_set* s, basic_block* entry)
{
    hash_pair* p = bhash_table_remove(&s->tbl, &entry->id, sizeof(size_t));

    // value in node_set is reference, so pair deletion is sufficient
    free(p);
}

basic_block* node_set_pop(node_set* s)
{
    hash_pair* p = bhash_table_pop(&s->tbl);
    basic_block* d = p->value;

    free(p);
    return d;
}

bool node_set_empty(const node_set* s)
{
    return hash_table_pairs(&s->tbl) == 0;
}

bool node_set_equal(const node_set* s1, const node_set* s2)
{
    if (hash_table_pairs(&s1->tbl) != hash_table_pairs(&s2->tbl))
    {
        return false;
    }

    for (int i = 0; i < s1->tbl.bucket_size; i++)
    {
        hash_pair* b = s1->tbl.bucket[i];

        while (b)
        {
            if (!bhash_table_test(&s2->tbl, b->key, sizeof(size_t)))
            {
                return false;
            }

            b = b->next;
        }
    }

    return true;
}

void node_set_union(node_set* dest, const node_set* src)
{
    if (dest == src)
    {
        return;
    }

    for (int i = 0; i < src->tbl.bucket_size; i++)
    {
        hash_pair* b = src->tbl.bucket[i];

        while (b)
        {
            node_set_add(dest, b->key);
            b = b->next;
        }
    }
}

void node_set_intersect(node_set* dest, node_set* src)
{
    if (dest == src || node_set_empty(dest))
    {
        return;
    }

    hash_pair* b;
    basic_block* blk;
    bool contains;

    for (int i = 0; i < dest->tbl.bucket_size; i++)
    {
        b = dest->tbl.bucket[i];

        while (b)
        {
            blk = b->value;

            if (!node_set_contains(src, blk))
            {
                // move away first in case current one is erased
                b = b->next;
                bhash_table_remove(&dest->tbl, &blk->id, sizeof(size_t));
            }
            else
            {
                b = b->next;
            }
        }
    }
}
