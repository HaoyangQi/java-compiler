#pragma once
#ifndef __COMPILER_SYMBOL_TABLE_H__
#define __COMPILER_SYMBOL_TABLE_H__

#include "types.h"
#include "langspec.h"
#include "hash.h"

/**
 * Java hash symbol for reserved words
 *
 * symbol is a hashable wrapper of reserved word for symbol table
*/
typedef struct _java_symbol
{
    /* raw hash of reserved word */
    hash_bits full_hash;
    /* word reference of the hash */
    java_reserved_word* word;
} java_symbol;

/**
 * Java reserved word lookup table
 *
 * a hash table using linear probing
 * the table is an array of java_symbol
*/
typedef struct _java_symbol_table
{
    /* the table, slot is null if available */
    java_symbol** slots;
    /* array length the table */
    size_t num_slot;
} java_symbol_table;

void init_symbol_table(java_symbol_table* table);
void release_symbol_table(java_symbol_table* table);
java_symbol* create_symbol(java_reserved_word* word);
java_symbol* symbol_lookup(java_symbol_table* table, char* string);
void load_language_spec(java_symbol_table* table);

#endif
