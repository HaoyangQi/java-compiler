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
#include "optimizer.h"
#include "optimization-context.h"

typedef struct _compiler
{
    unsigned int version;

    char* source_file_name;
    file_buffer reader;
    hash_table rw_lookup_table;
    java_expression expression;
    java_error_definition err_def;
    java_lexer lexer;
    java_parser context;
    java_ir ir;
    optimization_context optimizers;
    java_error_logger logger;
} compiler;

/**
 * compiler stage bit mask
 * and order matters here
*/
typedef enum
{
    COMPILER_STAGE_PARSE = 0x1,
    COMPILER_STAGE_CONTEXT = 0x2,
    COMPILER_STAGE_OPTIMIZE = 0x4,
    COMPILER_STAGE_EMIT = 0x8,
} compiler_stage;

bool init_compiler(compiler* compiler);
void release_compiler(compiler* compiler);

void detask_compiler(compiler* compiler);
bool retask_compiler(compiler* compiler, char* source_path);

bool compile(compiler* compiler, architecture* arch, char* source_path, compiler_stage stages);
void compiler_error_format_print(compiler* compiler);

#endif
