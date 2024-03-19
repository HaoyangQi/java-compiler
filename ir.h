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
#include "number.h"

// fast top scope worker getter (no NULL check)
#define TSW(ir) ((ir)->scope_workers->worker)

// some wrapper of def() variants
#define def_li_dec32(ir, content) def_li_raw(ir, content, JLT_LTR_NUMBER, JT_NUM_DEC, JT_NUM_BIT_LENGTH_NORMAL)

/**
 * def/use control bit flags
 *
 * from LSB:
 * -----+
 *  bit |
 * -----+
 *   1  | copy data (default move)
 *   2  | lookup top-level scope
 *   3  | method name: allow class name
 *   4  | reserved
 *   5  | reserved
 *   6  | reserved
 *   7  | reserved
 *   8  | reserved
*/
typedef unsigned char def_use_control;

#define DU_CTL_DEFAULT (0)
#define DU_CTL_DATA_COPY (0x01)
#define DU_CTL_LOOKUP_TOP_LEVEL (0x02)
#define DU_CTL_METHOD_NAME (0x04)

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
    // reference to the node this instruction belongs to
    struct _basic_block* node;

    // previous instruction
    struct _instruction* prev;
    // next instruction
    struct _instruction* next;
} instruction;

/**
 * block edge type
 *
 * EDGE_ANY: no additional info on this edge
 * EDGE_TRUE: boolean true branch
 * EDGE_FALSE: boolean false branch
 * EDGE_JUMP: jump edge, always directs to a previous node
 *            and this id is used for code generation
*/
typedef enum
{
    EDGE_ANY,
    EDGE_TRUE,
    EDGE_FALSE,
    EDGE_JUMP,
} edge_type;

/**
 * CFG Edge Info
*/
typedef struct _cfg_edge
{
    edge_type type;

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
 * Basic Block Hash Set
 *
 * it uses the hash of node id as key to
 * map to the basic block object
*/
typedef struct
{
    // type: hash_table<size_t, basic_block*>
    hash_table tbl;
} node_set;

/**
 * Node type
 *
 * special IDs for graph manipulation
 * BLOCK_RETURN: a node that (should) ends with "return"
 * BLOCK_BREAK: a node that (should) ends with "break"
 * BLOCK_CONTINUE: a node that (should) ends with "continue"
 * BLOCK_TEST: a node that ends with a logical test
 *             which will trigger 2 outbound branches
*/
typedef enum
{
    BLOCK_ANY,
    BLOCK_RETURN,
    BLOCK_BREAK,
    BLOCK_CONTINUE,
    BLOCK_TEST,
} block_type;

/**
 * CFG Basic Block
 *
*/
typedef struct _basic_block
{
    size_t id;
    block_type type;

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
 * CFG Entry Point
 *
 * CFG is always one-way-in, but multi-way-out
 *
 * why multi-way-out? think about how we use "return" statement
 * how to determine one-way-in? what about traverse and find the
 * one without inbound edges?
 *
 * well... it will not work: e.g. a loop
 *        +--->A
 *        |   / \
 *        +--B   C
 *
 * so, we do that manually, and chronologically
 * that is: the very first node we created during parsing, IS the
 * entry node we want to have
 *
 * nodes and edges are managed here as the source of all references
 * and as the aid for deletion process
*/
typedef struct _cfg
{
    // nodes
    node_array nodes;
    // edges
    edge_array edges;
    // entry point
    basic_block* entry;
} cfg;

typedef struct _definition definition;

/**
 * Definition Pool
*/
typedef struct
{
    definition** arr;
    size_t size;
    size_t num;
} definition_pool;

/**
 * definition type
 *
*/
typedef enum
{
    DEFINITION_VARIABLE,
    DEFINITION_METHOD,
    DEFINITION_NUMBER,
    DEFINITION_CHARACTER,
    DEFINITION_BOOLEAN,
    DEFINITION_NULL,
    DEFINITION_STRING,
} definition_type;

/**
 * scope lookup table value descriptor
 *
 * here we use node type for further classification
*/
typedef struct _definition
{
    // definition type
    definition_type type;
    // used by dataflow analysis
    size_t def_count;
    // serialization id, used during IR emit process
    size_t sid;
    // internal-only: the code walk root
    tree_node* root_code_walk;

    union
    {
        struct
        {
            // if it is a member variable
            bool is_class_member;
            // modifier
            lbit_flag modifier;
            // type
            type_name type;
        } variable;

        struct
        {
            // modifier
            lbit_flag modifier;
            // if method is a constructor
            bool is_constructor;
            // parameter count
            size_t parameter_count;
            // ordered parameter definition
            struct _definition** parameters;
            // return type
            type_name return_type;
            // code
            cfg code;
            definition_pool local_variables;
        } method;

        struct
        {
            // primitive type
            primitive type;
            // literal value
            uint64_t imm;
        } li_number;

        struct
        {
            // 16-bit character binary stream
            char* stream;
            // number of bytes of stream
            size_t length;
            // if stream contains 16-bit wide character
            bool wide_char;
        } li_string;
    };
} definition;

/**
 * Optimizer Data
 *
 * It contains data for SSA and other useful info
 * from every node for code analysis and optization
*/
typedef struct
{
    basic_block* node;

    // SSA: dominator set
    node_set dominators;
    // SSA: dominance frontier
    node_set df;
} ssa;

/**
 * IR CFG Worker
 *
 * when a CFG is finalized, worker will contain following information:
 * 1. the code graph
 * 2. entry node: specified by graph->entry
 * 3. implicit exit node: which is the node specified by cur_blk
 * 4. outbound strategy: indicates how implicit exit node is connected
 *                       to it successor
 *
 * following control flags are defined:
 * 1. execute_inverse: push instruction in front of start of current instruction list
 * 2. grow_insert: allow next block grow to handle insertion
 * 3. is_next_asn_init: mark next IROP_ASN instruction as part of initialization,
 *                      so it will not increament version number
 *
 * a CFG can have multiple exit node because every "return" statemnt
 * will behave as an exit node;
 * but a CFG can only have one implict exit node
*/
typedef struct
{
    cfg* graph;
    // all variable definitions in this CFG
    definition_pool variables;

    basic_block* cur_blk;
    edge_type next_outbound_strategy;
    bool execute_inverse;
    bool grow_insert;
    bool is_next_asn_init;

    // code analysis data
    ssa* optimizer;
} cfg_worker;

/**
 * scope worker context
 *
*/
typedef struct __cfg_worker_context
{
    cfg_worker* worker;
    struct __cfg_worker_context* next;
} cfg_worker_context;

/**
 * bit mask of statement context type
 *
 * 0000 0001: loop
 * 0000 0010: switch
*/
typedef enum
{
    SCQ_LOOP = 1,
    SCQ_SWITCH = 2,
} statement_context_query;

/**
 * statement context info
 *
 * this info is used by statements that allow following:
 * 1. break: while/do/for/switch
 * 2. continue: while/do/for
 *
 * test: an internal context that always points to the
 * tail block of condition expression, since expression
 * may be expanded, so the start block of expression is
 * not necessarily the end block of it
*/
typedef struct __statement_context
{
    // context type
    statement_context_query type;
    // the block in statement that "continue" jumps to
    basic_block* _continue;
    // the block in statement that "break" jumps to
    basic_block* _break;
    // the tail block of condition expression
    basic_block* _test;

    struct __statement_context* next;
} statement_context;

/**
 * Global Definition: Import Name
 *
 * for on-demand imports, key is package name, so
 * this object is redundant
*/
typedef struct
{
    char* package_name;
} global_import;

/**
 * Top Level Type
*/
typedef enum
{
    TOP_LEVEL_CLASS = 0,
    TOP_LEVEL_INTERFACE = 1,
} top_level_type;

/**
 * Global Definition: Top Level
 *
 * we need to preserve all names because a
 * top level mey be referenced by others so
 * we need to export them
 *
*/
typedef struct
{
    // top level type
    top_level_type type;
    // modifier
    lbit_flag modifier;
    // super (only one super allowed)
    char* extend;
    // a list of implement names
    char** implement;
    // number of implement names
    size_t num_implement;
    // variable pool for member initialization code
    definition_pool member_init_variables;
    // member initialization code
    cfg* code_member_init;
    // member definitions: map<string, definition*>
    hash_table tbl_member;
    // literal: map<string, definition*>
    hash_table tbl_literal;

    /* following fields are internal-use only */

    // internal-only: first JNT_CLASS_BODY_DECL node reference
    tree_node* node_first_body_decl;
    // member definition order tracker
    size_t num_member_variable;
} global_top_level;

/**
 * Compilation Unit Abstraction
 *
 * tbl_import: all import names, it maps from class name to package name
 *             for on-demand: it maps from package name to NULL
 *
 * tbl_implicit_import: all implicit import names that requires on-demand
 *                      import to resolve, those names show up in code,
 *                      but not listed in specific import statements
 *
 * tbl_global: top level implementation, it maps from the name to the
 *             descriptor global_top_level
*/
typedef struct
{
    // imports: map<string, global_import*>
    hash_table tbl_import;
    // implicit imports: map<string, NULL>
    hash_table tbl_implicit_import;
    // top level: map<string, global_top_level*>
    hash_table tbl_global;

    // current top level
    global_top_level* working_top_level;
    // stack top symbol lookup
    scope_frame* scope_stack_top;
    // scope worker context stack
    cfg_worker_context* scope_workers;
    // statement context stack
    statement_context* statement_contexts;

    // architecture info
    architecture* arch;
    // expression-related info
    java_expression* expression;
    // error data
    java_error_stack* error;
} java_ir;

char* t2s(java_token* token);
definition* t2d(hash_table* table, java_token* token);
primitive r2p(
    java_ir* ir,
    const char* content,
    binary_data* data,
    java_lexeme_type token_type,
    java_number_type num_type,
    java_number_bit_length num_bits
);
primitive t2p(java_ir* ir, java_token* t, binary_data* data);
char* name_unit_concat(tree_node* from, tree_node* stop_before);

void init_definition_pool(definition_pool* pool);
void release_definition_pool(definition_pool* pool);
void definition_pool_grow(definition_pool* pool, size_t by);
void definition_pool_add(definition_pool* pool, definition* def);
void definition_pool_merge(definition_pool* dest, definition_pool* src);

global_import* new_global_import();
void delete_global_import(global_import* i);

global_top_level* new_global_top_level(top_level_type type);
void delete_global_top_level(global_top_level* top);

void push_scope_worker(java_ir* ir);
cfg_worker* get_scope_worker(java_ir* ir);
cfg_worker* pop_scope_worker(java_ir* ir);

statement_context* push_statement_context(java_ir* ir, statement_context_query type);
statement_context* get_statement_context(java_ir* ir, statement_context_query query);
void pop_statement_context(java_ir* ir);

void definition_lookup_deleter(char* k, definition* v);
hash_table* lookup_new_scope(java_ir* ir);
bool lookup_pop_scope(java_ir* ir, definition_pool* pool);
hash_table* lookup_global_scope(java_ir* ir);
hash_table* lookup_top_level_scope(java_ir* ir);
hash_table* lookup_top_level_literal_scope(java_ir* ir);
hash_table* lookup_working_scope(java_ir* ir);
void lookup_top_level_begin(java_ir* ir, global_top_level* desc);
void lookup_top_level_end(java_ir* ir);
bool lookup_register(
    java_ir* ir,
    hash_table* table,
    char** name,
    void** desc,
    java_error_id err
);

definition* def(
    java_ir* ir,
    char** name,
    definition** type_def,
    size_t name_dims,
    def_use_control duc,
    java_error_id err_dup,
    java_error_id err_dim_amb,
    java_error_id err_dim_dup
);
definition* use(
    java_ir* ir,
    const char* name,
    def_use_control duc,
    java_error_id err_undef
);
definition* def_li(
    java_ir* ir,
    char** content,
    java_lexeme_type token_type,
    java_number_type num_type,
    java_number_bit_length num_bits
);
definition* def_li_raw(
    java_ir* ir,
    const char* raw,
    java_lexeme_type token_type,
    java_number_type num_type,
    java_number_bit_length num_bits
);
definition* type2def(
    tree_node* node,
    definition_type type,
    lbit_flag modifier,
    bool is_member
);
definition* def_var(java_ir* ir, tree_node* node, definition** type, def_use_control duc, bool is_member);
void def_vars(java_ir* ir, tree_node* node, lbit_flag modifier, bool is_member);
void def_params(java_ir* ir, tree_node* node, definition** ordered_list);
void def_global(java_ir* ir, tree_node* compilation_unit);

definition* new_definition(definition_type type);
void definition_delete(definition* v);
definition* definition_copy(definition* v);

void edge_array_resize(edge_array* edges, size_t by);
void node_array_resize(node_array* nodes, size_t by);

cfg* new_cfg_container();
void init_cfg(cfg* g);
void release_cfg(cfg* g);
basic_block* cfg_new_basic_block(cfg* g);
void cfg_new_edge(cfg* g, basic_block* from, basic_block* to, edge_type type);
bool cfg_empty(const cfg* g);
void cfg_detach(cfg* g);

void init_cfg_worker(cfg_worker* worker);
void release_cfg_worker(cfg_worker* worker, cfg* move_to, definition_pool* pool);
basic_block* cfg_worker_current_block(cfg_worker* worker);
basic_block* cfg_worker_grow(cfg_worker* worker);
void cfg_worker_next_outbound_strategy(cfg_worker* worker, edge_type type);
void cfg_worker_execution_strategy(cfg_worker* worker, bool inverse);
void cfg_worker_next_grow_strategy(cfg_worker* worker, bool insert);
void cfg_worker_next_asn_strategy(cfg_worker* worker, bool is_init);
void cfg_worker_set_current_block_type(cfg_worker* worker, block_type t);
void cfg_worker_jump(cfg_worker* worker, basic_block* to, bool change_cur, bool edge);
void cfg_worker_grow_with_graph(cfg_worker* dest, cfg_worker* src);
instruction* cfg_worker_execute(
    java_ir* ir,
    cfg_worker* worker,
    operation irop,
    reference** lvalue,
    reference** operand_1,
    reference** operand_2
);
bool cfg_worker_current_block_empty(const cfg_worker* worker);
basic_block* cfg_worker_current_block_split(
    java_ir* ir,
    cfg_worker* worker,
    instruction* at,
    edge_type to_remainder,
    bool split_before
);
void cfg_worker_expand_logical_precedence(java_ir* ir, cfg_worker* worker);

void cfg_worker_ssa_release(cfg_worker* worker);
void cfg_worker_ssa_build(cfg_worker* worker);

void init_node_set(node_set* s);
void init_node_set_with_copy(node_set* dest, const node_set* src);
void release_node_set(node_set* s);
bool node_set_contains(const node_set* s, const basic_block* entry);
void node_set_add(node_set* s, basic_block* entry);
void node_set_remove(node_set* s, basic_block* entry);
basic_block* node_set_pop(node_set* s);
bool node_set_empty(const node_set* s);
bool node_set_equal(const node_set* s1, const node_set* s2);
void node_set_union(node_set* dest, const node_set* src);
void node_set_intersect(node_set* dest, node_set* src);

reference* new_reference(reference_type t, void* doi);
reference* copy_reference(const reference* r);
void delete_reference(reference* ref);

instruction* new_instruction();
void delete_instruction(instruction* inst, bool destructive);
bool instruction_insert(basic_block* node, instruction* prev, instruction* inst);
bool instruction_push_back(basic_block* node, instruction* inst);
instruction* instruction_pop_back(basic_block* node);
bool instruction_push_front(basic_block* node, instruction* inst);
instruction* instruction_locate_enclosure_start(instruction* inst);

void walk_class(java_ir* ir, global_top_level* class);
void walk_interface(java_ir* ir, global_top_level* interface);

char primitive_type_to_jil_type(java_lexeme_type p);

void init_ir(java_ir* ir, java_expression* expression, java_error_stack* error);
void release_ir(java_ir* ir);
void contextualize(java_ir* ir, architecture* arch, tree_node* compilation_unit);
void ir_error(java_ir* ir, java_error_id id);

#endif
