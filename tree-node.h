#pragma once
#ifndef __COMPILER_TREE_NODE_H__
#define __COMPILER_TREE_NODE_H__

#include "types.h"
#include "token.h"
#include "vecbuf.h"

typedef struct
{
    /* ID list */
    vec ids;
} java_tree_node_pkg_decl;

typedef struct
{
    /* ID list */
    vec ids;
    /* on demand import */
    bool on_demand;
} java_tree_node_import_decl;

#endif
