#pragma once
#ifndef __COMPILER_H__
#define __COMPILER_H__

#include "architecture.h"
#include "file.h"
#include "hash-table.h"
#include "parser.h"
#include "expression.h"
#include "ir.h"
#include "il.h"
#include "error.h"

typedef struct
{
    unsigned int version;

    char* source_file_name;
    file_buffer reader;
    hash_table rw_lookup_table;
    java_expression expression;
    java_error_definition err_def;
    java_parser context;
    java_ir ir;
    java_error_stack error;
} compiler;

bool init_compiler(compiler* compiler);
void release_compiler(compiler* compiler);

void detask_compiler(compiler* compiler);
bool retask_compiler(compiler* compiler, char* source_path);

bool compile(compiler* compiler, architecture* arch, char* source_path);
void compiler_error_format_print(compiler* compiler);

#endif
