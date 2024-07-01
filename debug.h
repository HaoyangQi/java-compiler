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
#include "optimizer.h"
#include "index-set.h"

void debug_print_indentation(size_t depth);
void debug_print_binary_stream(const char* stream, size_t bin_len, size_t depth);
void debug_print_memory(byte* mem, long size, long line_break);
void debug_print_lexeme_type(java_lexeme_type id);
void debug_print_operator(java_parser* parser, operator_id id);
void debug_print_number_bit_length(java_number_bit_length l);
void debug_print_number_type(java_number_type number);
void debug_print_token_content(java_token* token);
void debug_print_modifier_bit_flag(lbit_flag modifiers);
void debug_print_ast_node(java_parser* parser, tree_node* node);
void debug_print_irop(irop op);
void debug_print_cfg_node_type(block_type type);
void debug_print_reference(reference* r);
void debug_print_instructions(instruction* inst, size_t* cnt, size_t depth);
void debug_print_cfg(cfg* g, size_t depth);
void debug_print_definition(definition* v, size_t depth);
void debug_print_definition_pool(definition_pool* pool, size_t depth);
void debug_print_name_definition_table(hash_table* table, size_t depth);
void debug_print_error_stack(java_error_stack* stack, size_t depth);
void debug_print_index_set(index_set* ixs);
void debug_print_register_allocation_type(register_allocation_type type);
void debug_print_register_allocation_info(register_allocation_info* info);
void debug_print_variable_item(variable_item* item, size_t index, size_t depth);
void debug_print_reference_with_allocation_info(reference* ref, bool lvalue, register_allocation_info* info);
void debug_print_instruction_item(instruction_item* item, size_t index, size_t depth);

void debug_report(compiler* compiler);
void debug_reserved_words();
void debug_file_buffer(file_buffer* reader);
void debug_symbol_table(hash_table* table);
void debug_tokenize(file_buffer* buffer, hash_table* table, java_error_logger* logger);
void debug_ast(java_parser* parser);
void debug_java_symbol_lookup_table_no_collision_test(bool use_prime_size);
void debug_global_import(java_ir* ir);
void debug_ir_global_names(java_ir* ir);
void debug_error_logger(java_error_logger* logger);
void debug_optimization_context(optimization_context* oc);

void debug_test_number_library();
void debug_test_dominance();

#endif
