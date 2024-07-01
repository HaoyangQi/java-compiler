#pragma once
#ifndef __COMPILER_OPTIMIZATION_CONTEXT_H__
#define __COMPILER_OPTIMIZATION_CONTEXT_H__

#include "types.h"
#include "ir.h"
#include "optimizer.h"

typedef struct _code_context
{
    char* name_top_level;
    char* name_method;
    definition* def;
    optimizer om;
} code_context;

typedef struct _top_level_optimizer
{
    size_t num_methods;
    code_context* contexts;
} top_level_optimizer;

typedef struct _optimization_context
{
    size_t num_top_level;
    java_ir* ir;
    top_level_optimizer* top_levels;
} optimization_context;

void init_optimization_context(optimization_context* oc, java_ir* ir);
void release_optimization_context(optimization_context* oc);
void optimization_context_build(optimization_context* oc);

#endif