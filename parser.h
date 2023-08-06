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

#endif
