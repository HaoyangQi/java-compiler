#pragma once
#ifndef __COMPILER_PARSER_H__
#define __COMPILER_PARSER_H__

#include "types.h"
#include "hash-table.h"
#include "token.h"
#include "file.h"
#include "symtbl.h"
#include "ast.h"
#include "expression.h"
#include "error.h"

/**
 * Token Peek Index
 *
 * 4 look-aheads maximum
*/

#define TOKEN_PEEK_1st 0
#define TOKEN_PEEK_2nd 1
#define TOKEN_PEEK_3rd 2
#define TOKEN_PEEK_4th 3

/**
 * Parser Info
 *
 * All data structures that contain static data are referenced
 * from compiler level
*/
typedef struct _java_parser
{
    /* 4 look-ahead tokens */
    java_token tokens[4];
    /* num look-ahead available */
    size_t num_token_available;
    /* file buffer */
    file_buffer* buffer;
    /* language spec symbol table*/
    hash_table* reserved_words;
    /* AST */
    tree_node* ast_root;
    /* expression definition */
    java_expression* expression;
    /* error data */
    java_error_stack* error;
} java_parser;

void init_parser(
    java_parser* parser,
    file_buffer* buffer,
    hash_table* rw,
    java_expression* expr,
    java_error_stack* err
);
void copy_parser(java_parser* from, java_parser* to);
void mutate_parser(java_parser* parser, java_parser* copy);
void release_parser(java_parser* parser, bool is_copy);

void parse(java_parser* parser);

java_token* token_peek(java_parser* parser, size_t idx);
void consume_token(java_parser* parser, java_token* dest);
java_token_class peek_token_class(java_parser* parser, size_t idx);
java_lexeme_type peek_token_type(java_parser* parser, size_t idx);
bool peek_token_class_is(java_parser* parser, size_t idx, java_token_class class);
bool peek_token_type_is(java_parser* parser, size_t idx, java_lexeme_type type);
bool peek_token_is_primitive_type(java_parser* parser, size_t idx);
bool peek_token_is_literal(java_parser* parser, size_t idx);

bool is_lexeme_primitive_type(java_lexeme_type type);
bool is_lexeme_literal(java_lexeme_type type);

bool parser_trigger_name(java_parser* parser, size_t peek_from);
bool parser_trigger_class_type(java_parser* parser, size_t peek_from);
bool parser_trigger_interface_type(java_parser* parser, size_t peek_from);
bool parser_trigger_type(java_parser* parser, size_t peek_from);
bool parser_trigger_expression(java_parser* parser, size_t peek_from);
bool parser_trigger_primary(java_parser* parser, size_t peek_from);
bool parser_trigger_statement(java_parser* parser, size_t peek_from);

void parser_error(java_parser* parser, java_error_id id);
void parser_recovery_dispatch(java_parser* parser, java_error_id id);

#endif
