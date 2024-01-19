#include "symtbl.h"
#include "utils.h"

/**
 * this value is calculated using:
 * java_symbol_lookup_table_no_collision_test(true)
*/
#define SYMBOL_TABLE_SIZE_PRIME (353)

/**
 * initialize reserved word lookup table
*/
void init_symbol_table(hash_table* table)
{
    init_hash_table(table, SYMBOL_TABLE_SIZE_PRIME);

    for (int i = 0; i < num_java_reserved_words; i++)
    {
        shash_table_insert(table, java_reserved_words[i].content, &java_reserved_words[i]);
    }

    if (table->num_filled != num_java_reserved_words)
    {
        fprintf(stderr, "TODO warning: collision detected: symbol table contains collision");
    }
}

/**
 * release table
 *
 * since we only reference data, so no deletion required
*/
void release_symbol_table(hash_table* table)
{
    release_hash_table(table, NULL);
}

/**
 * symbol lookup
*/
java_reserved_word* symbol_lookup(hash_table* table, char* string)
{
    return (java_reserved_word*)shash_table_find(table, string);
}
