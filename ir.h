#pragma once
#ifndef __COMPILER_SEMANTICS_H__
#define __COMPILER_SEMANTICS_H__

#include "types.h"
#include "hash-table.h"
#include "token.h"
#include "expression.h"
#include "tree.h"
#include "error.h"
#include "architecture.h"

#include "string-list.h"

// hash table mapping string->definition wrapper
#define HT_STR2DEF(t, s) ((definition*)shash_table_find(t, s))

/**
 * Primitive Value Bit Size
*/

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
 * every scope has a counter for instrction numbering
*/
typedef struct _scope_frame
{
    // scope identifier
    lookup_scope_type type;

    /**
     * TYPE: hash_table<char*, definition*>
    */
    hash_table* table;

    struct _scope_frame* next;
} scope_frame;

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
 * Primitive Value Types
 *
 * TODO: string literal?
 * well... it is NOT primitive type, but it can be literal
*/
typedef enum
{
    // byte
    IRPV_INTEGER_BIT_8 = 0,
    // short
    IRPV_INTEGER_BIT_16,
    // int
    IRPV_INTEGER_BIT_32,
    // long
    IRPV_INTEGER_BIT_64,
    // char
    IRPV_INTEGER_BIT_U16,
    // float
    IRPV_PRECISION_SINGLE,
    // double
    IRPV_PRECISION_DOUBLE,
    // boolean
    IRPV_BOOLEAN,

    IRPV_MAX,
} primitive;

/**
 * scope lookup table value descriptor
 *
 * here we use node type for further classification
*/
typedef struct _definition
{
    java_node_query type;
    java_lexeme_type li_type;
    struct _definition* next;

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
            // a list of names, separated by ','
            char* implement;
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
            // if it is a member variable
            bool is_class_member;
            // modifier
            lbit_flag modifier;
            // type
            type_name type;
            // current version
            size_t version;
        } variable;

        struct
        {
            // modifier
            lbit_flag modifier;
            // return type
            type_name return_type;
        } method;

        struct
        {
            // primitive type
            primitive type;
            // literal value
            uint64_t imm;
        } li_number;
    };
} definition;

/**
 * reference type
 *
 * IR_ASN_REF_DEFINITION: definition from scope tables
 * IR_ASN_REF_INSTRUCTION: instruction
 * IR_ASN_REF_LITERAL: definition from tbl_literal
*/
typedef enum
{
    IR_ASN_REF_UNDEFINED = 0,

    IR_ASN_REF_DEFINITION,
    IR_ASN_REF_INSTRUCTION,
    IR_ASN_REF_LITERAL,
} reference_type;

/**
 * assignment reference
 *
 * NOTE: doi must stay as reference at all times
 * (hence: no copy/allocation for doi is allowed)
 * because same doi may be reference in multiple places
*/
typedef struct
{
    // type selector
    reference_type type;
    // Definition Or Instruction
    void* doi;
    // version of reference, used only for definition
    size_t ver;
} reference;

/**
 * Single Assignment Form
 *
 * It is generalized as a "single instruction"
 * a sequence of instructions defines a node
 *
 * max form: lvalue <- operand_1 op operand_2
*/
typedef struct _instruction
{
    // opcode
    operation op;
    // value reference
    reference* lvalue;
    reference* operand_1;
    reference* operand_2;

    // previous instruction
    struct _instruction* prev;
    // next instruction
    struct _instruction* next;
} instruction;

struct _basic_block;

/**
 * CFG Edge Info
*/
typedef struct _cfg_edge
{
    struct _basic_block* from;
    struct _basic_block* to;
} cfg_edge;

/**
 * Dynamic array of edges
*/
typedef struct
{
    cfg_edge** arr;
    size_t size;
    size_t num;
} edge_array;

/**
 * block edge type
*/
typedef enum
{
    BLOCK_EDGE_IN,
    BLOCK_EDGE_OUT,
} basic_block_edge_type;

/**
 * CFG Basic Block
 *
*/
typedef struct _basic_block
{
    instruction* inst_first;
    instruction* inst_last;

    edge_array in;
    edge_array out;
} basic_block;

/**
 * Dynamic array of nodes
*/
typedef struct
{
    basic_block** arr;
    size_t size;
    size_t num;
} node_array;

/**
 * CFG Node Type
*/
typedef enum
{
    CFG_BLOCK_ENTRY,
    CFG_BLOCK_EXIT,
    CFG_BLOCK_NORMAL,
} cfg_block_type;

/**
 * CFG Entry Point
 *
 * CFG is always one-way-in, one-way-out
 *
 * nodes and edges are managed here as the source of all references
 * and as the aid for deletion process
*/
typedef struct
{
    // nodes
    node_array nodes;
    // edges
    edge_array edges;
    // entry point
    basic_block* entry;
    // exit point
    basic_block* exit;
} cfg;

/**
 * Top Level of Semantics
 *
 * on top level, each method occupies a tree of blocks
 * additionally, member initializers will occupy a
 * tree;
 *
 * all code graphs should be NULL by default
*/
typedef struct
{
    // on-demand import package names
    hash_table tbl_on_demand_packages;
    // other global names
    hash_table tbl_global;
    // literal lookup table
    hash_table tbl_literal;
    // stack top symbol lookup
    scope_frame* scope_stack_top;
    // toplevel block count
    size_t num_methods;

    // architecture info
    architecture* arch;
    // expression-related info
    java_expression* expression;
    // error data
    java_error* error;

    // member declarator initialization code
    cfg code_member_init;
} java_ir;

char* t2s(java_token* token);
definition* t2d(hash_table* table, java_token* token);
primitive t2p(java_ir* ir, java_token* t, uint64_t* n);

void lookup_scope_deleter(char* k, definition* v);
hash_table* lookup_new_scope(java_ir* ir, lookup_scope_type type);
bool lookup_pop_scope(java_ir* ir, bool merge_global);
hash_table* lookup_global_scope(java_ir* ir);
hash_table* lookup_working_scope(java_ir* ir);
hash_table* lookup_top_scope(java_ir* ir);
bool lookup_register(
    java_ir* ir,
    hash_table* table,
    char* name,
    definition* desc,
    java_error_id err
);
bool def(
    java_ir* ir,
    definition* type_def,
    tree_node* declarator,
    java_error_id err_dup,
    java_error_id err_dim_amb,
    java_error_id err_dim_dup
);
definition* use(java_ir* ir, tree_node* declarator, java_error_id err_undef);
definition* def_li(java_ir* ir, java_token* token);

definition* new_definition(java_node_query type);
void definition_concat(definition* dest, definition* src);
void definition_delete(definition* v);
definition* definition_copy(definition* v);

void init_cfg(cfg* g);
void release_cfg(cfg* g);
basic_block* cfg_new_basic_block(cfg* g);
void cfg_new_edge(cfg* g, basic_block* from, basic_block* to);
cfg* cfg_connect(cfg* g, cfg* src_graph);

reference* new_reference();
reference* copy_reference(const reference* r);
void delete_reference(reference* ref);

instruction* new_instruction();
void delete_instruction(instruction* inst, bool destructive);
bool instruction_insert(basic_block* node, instruction* prev, instruction* inst);
bool instruction_push_back(basic_block* node, instruction* inst);
bool instruction_push_front(basic_block* node, instruction* inst);

void walk_expression(java_ir* ir, cfg* g, tree_node* expression);
void walk_block(java_ir* ir, cfg* g, tree_node* block);

void init_ir(java_ir* ir, java_expression* expression, java_error* error);
void release_ir(java_ir* ir);
void contextualize(java_ir* ir, architecture* arch, tree_node* compilation_unit);
void ir_error(java_ir* ir, java_error_id id);

#endif
