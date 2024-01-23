#pragma once
#ifndef __COMPILER_DEBUG_H__
#define __COMPILER_DEBUG_H__

#include <stdio.h>

#include "types.h"
#include "file.h"
#include "symtbl.h"
#include "tree.h"
#include "hash-table.h"
#include "compiler.h"
#include "ir.h"

void debug_report(compiler* compiler);
void debug_print_reserved_words();
void debug_file_buffer(file_buffer* reader);
void debug_print_symbol_table(hash_table* table);
void debug_tokenize(file_buffer* buffer, hash_table* table);
void debug_ast(tree_node* root);
void debug_shash_table(hash_table* table);
void debug_java_symbol_lookup_table_no_collision_test(bool use_prime_size);
void debug_ir_on_demand_imports(java_ir* ir);
void debug_ir_type_imports(java_ir* ir);

#endif
