#pragma once
#ifndef __COMPILER_SEMANTICS_H__
#define __COMPILER_SEMANTICS_H__

#include "types.h"
#include "hash-table.h"
#include "token.h"
#include "expression.h"
#include "tree.h"
#include "error.h"

#include "string-list.h"

/**
 * scope type
 *
 * LST_NONE: a generic scope with no header
 * we do not mark "switch" because its scope
 * does not allow symbol creation
*/
typedef enum
{
    LST_COMPILATION_UNIT,
    LST_CLASS,
    LST_INTERFACE,
    LST_NONE, // just a scope, no header
    LST_IF,
    LST_ELSE,
    LST_FOR,
    LST_WHILE,
    LST_DO,
    LST_TRY,
    LST_CATCH,
    LST_FINALLY,
} lookup_scope_type;

/**
 * Symbol Lookup Hierarchy
 *
 * It is a compile-time dynamic stack trace of
 * current scope
 *
 * key is a copy of name string
 * value is various based on scope type it serves
 *
 * TODO: need semantic_block* reference back to a block
 * if it is a function or a loop
*/
typedef struct _lookup_hierarchy
{
    lookup_scope_type type;

    // name -> lookup_value_descriptor
    hash_table table;

    struct _lookup_hierarchy* next;
} lookup_hierarchy;

/**
 * type info
*/
typedef struct
{
    java_lexeme_type primitive;
    char* reference;

    size_t dim;
} type_name;

/**
 * scope lookup table value descriptor
 *
 * here we use node type for further classification
*/
typedef struct _lookup_value_descriptor
{
    java_node_query type;

    union
    {
        struct
        {
            // package name string
            char* package_name;
        } import;

        struct
        {
            // modifier
            lbit_flag modifier;
            // max one super class allowed
            char* extend;
            /**
             * TODO: implement list
            */
        } class;

        struct
        {
            // max one super interface allowed
            char* extend;
        } interface;

        struct
        {
            // modifier
            lbit_flag modifier;
        } constructor;

        struct
        {
            // modifier
            lbit_flag modifier;
            // type
            type_name type;
        } member_variable;

        struct
        {
            // modifier
            lbit_flag modifier;
        } method;
    };
} lookup_value_descriptor;

/**
 * variable descriptor
 *
 * TODO: need to describe type
*/
typedef struct
{
    // use counter, 0 means definition
    size_t version;
} semantic_variable_descriptor;

/**
 * value type
*/
typedef enum
{
    SVT_NAME,
    SVT_LITERAL,
} semantic_value_type;

/**
 * value
*/
typedef struct
{
    semantic_value_type type;

    // value description
    union
    {
        // reference type
        struct
        {
            /**
             * TODO: do we need key here?
            */
            // char* s;
            semantic_variable_descriptor* definition;
            size_t version;
        } name;

        // primitive type
        struct
        {
            java_lexeme_type type;
            java_number_bit_length bits;
            void* data;
        } literal;
    } describe;
} semantic_value;

/**
 * single assignment form
 *
 * v <- aux_1 op aux_2
 *
 * multiple statements are chained in order
 * when they are in same block
 *
 * the simplist form is an assignment expression:
 * a ;
 * it takes one value, and that is it
 *
 * but there is allowed 3 values at most because
 * it is a single assignment form
 *
 * what about "? :"
 * it is considered as a syntatic suger instead of
 * a simple assignment, because it branches statements
 * and diverges the pathway; it needs to be broken
 * down into simpler version in SSA
 *
 * TODO:
*/
typedef struct _semantic_assignment
{
    // at least one value is mandatory
    semantic_value v;
    // another two values are optional
    semantic_value* aux_1;
    semantic_value* aux_2;
    // operator is also optional
    java_operator op;

    struct _semantic_assignment* next;
} semantic_assignment;

typedef enum
{
    SBT_SIMPLE,
    SBT_IF,
    SBT_LOOP,
} semantic_block_branch_type;

/**
 * A block of assignments
 *
 * Connecting together we have SSA
*/
typedef struct _semantic_block
{
    // lookupup table of current scope
    lookup_hierarchy* lookup_from;
    // assignments within the block
    semantic_assignment* assignment;
    // branch type
    semantic_block_branch_type branch_type;

    // branch form
    union
    {
        struct
        {
            struct _semantic_block* next;
        } single;

        struct
        {
            struct _semantic_block* yes;
            struct _semantic_block* no;
        } binary;

        struct
        {
            struct _semantic_block* condition_block;
        } loop;
    } branch_like;
} semantic_block;

/**
 * Top Level of Semantics
 *
 * on top level, each method occupies a tree of blocks
 * additionally, member initializers will occupy a
 * tree;
*/
typedef struct
{
    // on-demand import package names
    hash_table tbl_on_demand_packages;
    // other global names
    hash_table tbl_global;
    // symbol lookup of current scope
    lookup_hierarchy* lookup_current_scope;
    // member initializer block
    semantic_block* member_initialization;
    // method SSAs
    semantic_block** methods;
    // toplevel block count
    size_t num_methods;
    // error data
    java_error* error;
} java_ir;

void lookup_scope_deleter(char* k, lookup_value_descriptor* v);
hash_table* lookup_new_scope(java_ir* ir, lookup_scope_type type);
bool lookup_pop_scope(java_ir* ir);
hash_table* lookup_global_scope(java_ir* ir);
hash_table* lookup_current_scope(java_ir* ir);
lookup_value_descriptor* new_lookup_value_descriptor(java_node_query type);
void lookup_value_descriptor_delete(lookup_value_descriptor* v);
lookup_value_descriptor* lookup_value_descriptor_copy(lookup_value_descriptor* v);
bool lookup_register(
    java_ir* ir,
    hash_table* table,
    char* name,
    lookup_value_descriptor* desc,
    java_error_id err
);

void init_ir(java_ir* ir, java_error* error);
void release_ir(java_ir* ir);
void contextualize(java_ir* ir, tree_node* compilation_unit);
void ir_error(java_ir* ir, java_error_id id);

/**
 * TODO: more
*/

#endif
