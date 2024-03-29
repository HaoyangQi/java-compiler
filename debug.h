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

void debug_number_library();
void debug_report(compiler* compiler);
void debug_print_reserved_words();
void debug_file_buffer(file_buffer* reader);
void debug_print_symbol_table(hash_table* table);
void debug_tokenize(file_buffer* buffer, hash_table* table);
void debug_ast(java_parser* parser);
void debug_java_symbol_lookup_table_no_collision_test(bool use_prime_size);
void debug_print_global_import(java_ir* ir);
void debug_ir_global_names(java_ir* ir);

#endif
