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
    // frame instruction counter
    size_t fic;

    /**
     * TYPE: hash_table<char*, definition>
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
        } literal;
    };
} definition;

/**
 * IR OP CODE
 *
 * every operation has unique identifier, despite what
 * form it has
 *
 * 1. assignment operator will not be modelled here because it is implicitly
 *    supported by the assignment form
 * 2. arithmetic unary sign "+" and "-" are not modelled because they can be
 *    done at compile-time
 * 3. operator "? :" is not modelled because it is an syntatic sugar which
 *    should be expanded in "if-else" form in code graph
 *
 * IROP_ADD     addition
 * IROP_SUB     subtraction
 * IROP_MUL     multiplication
 * IROP_DIV     division
 * IROP_MOD     modulo
 * IROP_BINC    pre-increment
 * IROP_AINC    post-increment
 * IROP_BDEC    pre-decrement
 * IROP_ADEC    post-decrement
 * IROP_SLS     left shift
 * IROP_SRS     right shift
 * IROP_URS     unsigned right shift
 * IROP_LT      less than
 * IROP_GT      greater than
 * IROP_LE      less or equal
 * IROP_GE      greater or equal
 * IROP_EQ      equal
 * IROP_NE      not equal
 * IROP_IO      instance of
 * IROP_LNEG    logical negation
 * IROP_LAND    logicl and
 * IROP_LOR     logical or
 * IROP_BNEG    bit-wise negation
 * IROP_BAND    bit-wise and
 * IROP_BOR     bit-wise or
 * IROP_XOR     bit-wise exclusive or
 * IROP_GOTO    jump
 * IROP_RETURN  return
 * IROP_TEST    test-and-jump
*/
typedef enum
{
    // undefined value
    IROP_UNDEFINED = 0,

    /* arithmetic */

    IROP_ADD,
    IROP_SUB,
    IROP_MUL,
    IROP_DIV,
    IROP_MOD,
    IROP_BINC,
    IROP_AINC,
    IROP_BDEC,
    IROP_ADEC,
    IROP_SLS,
    IROP_SRS,
    IROP_URS,

    /* Relational */

    IROP_LT,
    IROP_GT,
    IROP_LE,
    IROP_GE,
    IROP_EQ,
    IROP_NE,
    IROP_IO,

    /* Logical */

    IROP_LNEG,
    IROP_LAND,
    IROP_LOR,

    /* Bit-wise */

    IROP_BNEG,
    IROP_BAND,
    IROP_BOR,
    IROP_XOR,

    /* IR-specific */

    IROP_GOTO,
    IROP_RETURN,
    IROP_TEST,
    IROP_PHI,

    IROP_MAX,
} operation;

/**
 * reference type
 *
 * IR_ASN_REF_DEFINITION: definition
 * IR_ASN_REF_INSTRUCTION: instruction
 * IR_ASN_REF_LITERAL: definition copy
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
*/
typedef struct
{
    // type selector
    reference_type type;
    // definition OR instruction
    void* ref;
    // version of reference, used only for definition
    size_t ver;
} reference;

/**
 * Single Assignment Form
 *
 * It is generalized as a "single instruction"
 * a sequence of instructions defines a node
 *
 * max form: ref[0] = ref[1] op ref[2]
 * only ref[0] can be lvalue
*/
typedef struct _instruction
{
    // opcode
    operation op;
    // value reference
    reference ref[3];

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
    // stack top symbol lookup
    scope_frame* scope_stack_top;
    // toplevel block count
    size_t num_methods;
    // error data
    java_error* error;

    // member declarator initialization code
    cfg* code_member_init;
} java_ir;

void lookup_scope_deleter(char* k, definition* v);
hash_table* lookup_new_scope(java_ir* ir, lookup_scope_type type);
bool lookup_pop_scope(java_ir* ir, bool merge_global);
hash_table* lookup_global_scope(java_ir* ir);
hash_table* lookup_current_scope(java_ir* ir);
bool lookup_register(
    java_ir* ir,
    hash_table* table,
    char* name,
    definition* desc,
    java_error_id err
);

definition* new_definition(java_node_query type);
void definition_concat(definition* dest, definition* src);
void definition_delete(definition* v);
definition* definition_copy(definition* v);

void init_cfg(cfg* g);
void release_cfg(cfg* g);
basic_block* cfg_new_basic_block(cfg* g);
void cfg_new_edge(cfg* g, basic_block* from, basic_block* to);
cfg* cfg_connect(cfg* g, cfg* src_graph);

instruction* new_instruction();
void delete_instruction(instruction* inst);
void instruction_insert(basic_block* node, instruction* prev, instruction* inst);
void instruction_push_back(basic_block* node, instruction* inst);
void instruction_push_front(basic_block* node, instruction* inst);

cfg* walk_expression(tree_node* expression);
cfg* walk_block(tree_node* block);

void init_ir(java_ir* ir, java_error* error);
void release_ir(java_ir* ir);
void contextualize(java_ir* ir, tree_node* compilation_unit);
void ir_error(java_ir* ir, java_error_id id);

/**
 * TODO: more
*/

#endif
