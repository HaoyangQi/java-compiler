#pragma once
#ifndef __COMPILER_DEBUG_H__
#define __COMPILER_DEBUG_H__

#include <stdio.h>

#include "types.h"
#include "file.h"
#include "symtbl.h"
#include "tree.h"

void debug_print_reserved_words();
void debug_file_buffer(file_buffer* reader);
void debug_print_symbol_table(java_symbol_table* table);
void debug_format_report(byte report_type);
void debug_tokenize(file_buffer* buffer, java_symbol_table* table);
void debug_ast(tree_node* root);

#endif
