#pragma once
#ifndef __COMPILER_DEBUG_H__
#define __COMPILER_DEBUG_H__

#include <stdio.h>

#include "types.h"
#include "file.h"
#include "symtbl.h"
#include "tree.h"
#include "hash-table.h"

void debug_print_reserved_words();
void debug_file_buffer(file_buffer* reader);
void debug_print_symbol_table(hash_table* table);
void debug_format_report(byte report_type);
void debug_tokenize(file_buffer* buffer, hash_table* table);
void debug_ast(tree_node* root);
void debug_shash_table(hash_table* table);

#endif
