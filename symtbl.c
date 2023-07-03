#include "symtbl.h"
#include "utils.h"
#include "vecbuf.h"
#include "report.h"

/**
 * clear table content
*/
void init_symbol_table(java_symbol_table* table)
{
    table->slots = NULL;
    table->num_slot = 0;
}

/**
 * free table array memory
*/
void release_symbol_table(java_symbol_table* table)
{
    for (int i = 0; i < table->num_slot; i++)
    {
        free(table->slots[i]);
    }

    free(table->slots);
}

/**
 * create a symbol slot
 *
 * creation must have external info to proceed
 * hence: no default ones
*/
java_symbol* create_symbol(java_reserved_word* word)
{
    java_symbol* symbol = (java_symbol*)malloc(sizeof(java_symbol));
    ASSERT_ALLOCATION(symbol);

    symbol->full_hash = hash_djb2(word->content);
    symbol->word = word;

    return symbol;
}

/**
 * lookup using linear probing
 *
 * it hash the string, then modulo table size to get a slot index
 * if index is null, return null
 * otherwise it compares the full hash, if missed, it probe next possible slot
 * if no probe remains, return null, otherwise returns symbol
*/
java_symbol* symbol_lookup(java_symbol_table* table, char* string)
{
    // start linear probing
    for (int i = 0; i <= table->num_slot; i++)
    {
        hash_bits hs = hash_djb2(string) +
            i * hash_sdbm(string);
        java_symbol* sym = table->slots[MOD_OF_HASH(hs, table->num_slot)];

        if (sym == NULL)
        {
            // if probing reaches NULL, meaning we are out of luck
            return NULL;
        }
        else if (sym->full_hash == hs)
        {
            // if full hash match, we accept it
            return sym;
        }

        // if full hash mismatch, we continue searching
    }

    // after all possible slots are probed and no luck, then no luck
    return NULL;
}

/**
 * populate table slots using linear probing
 *
 * idx = (h1(s) + i * h2(s)) mod |T|, i=[0, 1, ..., |T|]
 *
 * 2 passes are used:
 * 1. fill in using no probe
 * 2. resolve collision
*/
void load_language_spec(java_symbol_table* table)
{
    vec collisions;
    java_symbol* new_sym = NULL;
    size_t new_idx;

    init_vec(&collisions, 10);

    report_reserved_words_count(num_java_reserved_words);

    // create the table
    table->num_slot = find_next_prime(2 * num_java_reserved_words);
    table->slots = (java_symbol**)malloc(sizeof(java_symbol*) * table->num_slot);
    ASSERT_ALLOCATION(table->slots);
    memset(table->slots, 0, sizeof(java_symbol*) * table->num_slot);

    report_reserved_words_lookup_table_size(table->num_slot);

    // first pass, no probing
    for (int idx_word = 0; idx_word < num_java_reserved_words; idx_word++)
    {
        new_sym = create_symbol(&java_reserved_words[idx_word]);
        new_idx = MOD_OF_HASH(new_sym->full_hash, table->num_slot);

        // collision, buffer it first
        if (table->slots[new_idx] != NULL)
        {
            vec_push(&collisions, new_sym);
        }
        else
        {
            table->slots[new_idx] = new_sym;
        }
    }

    report_reserved_words_hash_collisions(vec_length(&collisions));

    // second pass, resolve hash collision using linear probing
    while (!vec_empty(&collisions))
    {
        new_sym = vec_pop(&collisions);

        // probe it
        for (int i = 1; i <= table->num_slot; i++)
        {
            new_sym->full_hash =
                hash_djb2(new_sym->word->content) +
                i * hash_sdbm(new_sym->word->content);
            new_idx = MOD_OF_HASH(new_sym->full_hash, table->num_slot);

            if (table->slots[new_idx] == NULL)
            {
                // if we find a spot, we add it
                table->slots[new_idx] = new_sym;
                report_reserved_words_longest_probing(i);
                break;
            }
        }
    }

    // sanity check: collision buffer should be empty
    if (!vec_empty(&collisions))
    {
        fprintf(stderr, "language spec failed to load due to hash collision(s).\n");
        exit(1);
    }

    release_vec(&collisions);
}
