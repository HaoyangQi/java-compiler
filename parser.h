#pragma once
#ifndef __COMPILER_PARSER_H__
#define __COMPILER_PARSER_H__

#include "types.h"
#include "token.h"
#include "file.h"
#include "symtbl.h"
#include "ast.h"

#define TOKEN_PEEK_1st 0
#define TOKEN_PEEK_2nd 1
#define TOKEN_PEEK_3rd 2
#define TOKEN_PEEK_4th 3

typedef struct _java_parser
{
    /* 4 look-ahead tokens */
    java_token tokens[4];
    /* num look-ahead available */
    size_t num_token_available;
    /* file buffer */
    file_buffer* buffer;
    /* language spec symbol table*/
    java_symbol_table* reserved_words;
    /* AST */
    tree_node* ast_root;
    /* token buffer for acceptance */
    java_token* token_buffer;
} java_parser;

void init_parser(java_parser* parser, file_buffer* buffer, java_symbol_table* rw);
void release_parser(java_parser* parser);

void parse(java_parser* parser);

java_token* token_peek(java_parser* parser, size_t idx);
void consume_token(java_parser* parser, java_token* dest);
bool peek_token_class_is(java_parser* parser, size_t idx, java_token_class class);
bool peek_token_type_is(java_parser* parser, size_t idx, java_lexeme_type type);
java_token* new_token_buffer(java_parser* parser);
void parser_ast_node_data_deleter(int metadata, void* data);

bool parser_trigger_name(java_parser* parser);

#endif
