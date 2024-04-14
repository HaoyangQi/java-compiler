/**
 * Expression Worker
 *
 * Expression is parsed using Shunting Yard Algorithm,
 * definitions here are aids for parser to apply the algorithm
*/

#pragma once
#ifndef __COMPILER_EXPRESSION_H__
#define __COMPILER_EXPRESSION_H__

#include "types.h"
#include "langspec.h"
#include "tree.h"
#include "lexer.h"

/**
 * Operator Info
 *
 * Operator consists 3 information:
 * 1. token id
 * 2. precedence
 * 3. associativity
 *
 * considering the value range is fairly narrow and bounded, we can
 * merge operator information using bit-wise representation:
 *
 * 0000 0000 0000 0000
 * _
 * associativity, 1 bit (0x8000)
 *  ___ __
 *  precedence, 5 bit (0x7C00)
 *        __ ____ ____
 *        token ID, 10 bit (0x03FF)
 *
 * precedence value ranges from 0 to 31, higher the value, higher the
 * precedence
 *
 * 10-bit token ID can handle up to 1024 types (0-1023)
 *
 * 0xFFFF is reserved for validation purposes
 *
 * one token might represent multiple operators
 *
 * method invocation, field access, object creation, and array access
 * operators are parsed in Primary, and they have highest precedence
 *
 * parenthesized precedence are implemented through Primary, also
 * implementing this way supports type casting
*/

typedef unsigned short java_operator;

// so far we do not have that many tokens, so 0xFFFF is a safe bound
#define OP_INVALID 0

#define OP_MASK_ASSOC ((java_operator)0x8000)
#define OP_MASK_PRECD ((java_operator)0x7C00)
#define OP_MASK_TOKEN ((java_operator)0x03FF)

#define OP_ASSOC_LR 0
#define OP_ASSOC_RL ((java_operator)0x8000)

#define OP_ASSOC(v) (((java_operator)(v)) & OP_MASK_ASSOC)
#define OP_PRECD(v) (((java_operator)(v)) & OP_MASK_PRECD)
#define OP_TOKEN(v) (((java_operator)(v)) & OP_MASK_TOKEN)
#define OP_LEVEL_TO_PRECD(v) (((java_operator)(v)) << 11)

#define OP(a, p, t) (OP_ASSOC(a) | OP_PRECD(p) | OP_TOKEN(t))

/**
 * IR OP CODE
 *
 * every operation has unique identifier, despite what form it has
 *
 * The following operator id does not have specific operation:
 * 1. composite assignment operators will not be modelled here because it is
 *    implicitly supported by the assignment form
 *
 * The following operator id looks unecessary but still modelled for consistency
 * 1. arithmetic unary sign "+" and "-" are modelled for consistency
 *
 * The following operator id is modelled but not allowed in final code:
 * 1. operator "? :", because it is an syntatic sugar which should be
 *    expanded in "if-else" form in code graph
 *    (see walk_expression)
 * 2. logical connectors "||" and "&&": they will be transformed into logical
 *    control flow during post-processing in walk_expression
 *    (see walk_expression)
 * 2. increment/decrement operator "++" and "--": they will be transformed into
 *    IROP_ADD and IROP_SUB during post-processing in walk_expression
 *
 * IROP_POS     sign +
 * IROP_NEG     sign -
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
 * IROP_LAND    logical and
 * IROP_LOR     logical or
 * IROP_BNEG    bit-wise negation
 * IROP_BAND    bit-wise and
 * IROP_BOR     bit-wise or
 * IROP_XOR     bit-wise exclusive or
 * IROP_ASN     assignment
 * IROP_TC      ternary: condition part (a ? TB)
 * IROP_TB,     ternary: branch part (c : d)
 * IROP_LMD     lambda
 * IROP_STORE   the instruction only stores an operand
 * IROP_INIT    default initialization of a variable
 * IROP_JMP     jump (triggers EDGE_JUMP)
 * IROP_RET     return
 * IROP_TEST    test-and-jump
 * IROP_PHI     branch convergence selector
 * IROP_NOOP    no-op, but occupies an instruction
 * IROP_BOOL    cast a value to boolean data
*/
typedef enum
{
    // undefined value
    IROP_UNDEFINED = 0,

    /* Sign */

    IROP_POS,
    IROP_NEG,

    /* Arithmetic */

    IROP_ADD,
    IROP_SUB,
    IROP_MUL,
    IROP_DIV,
    IROP_MOD,
    IROP_BINC, // internal-use ONLY
    IROP_AINC, // internal-use ONLY
    IROP_BDEC, // internal-use ONLY
    IROP_ADEC, // internal-use ONLY
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
    IROP_LAND, // internal-use ONLY
    IROP_LOR,  // internal-use ONLY

    /* Bit-wise */

    IROP_BNEG,
    IROP_BAND,
    IROP_BOR,
    IROP_XOR,

    /* Assignment */

    IROP_ASN,

    /* Ternary (internal-use ONLY) */

    IROP_TC,
    IROP_TB,

    /* Lambda */

    IROP_LMD,

    /* IR-specific */

    IROP_STORE,
    IROP_INIT,
    IROP_JMP,
    IROP_RET,
    IROP_TEST,
    IROP_PHI,
    IROP_NOOP,
    IROP_BOOL,

    IROP_MAX,
} operation;

/**
 * Java Expression Definition
 *
 * static data used for expression parsing
 *
 * ONLYSTATIC: do NOT include any run-time instance-specific data
 * in this struct, use java_expression_worker instead; because
 * this struct is designed to stay active for all compile tasks
*/
typedef struct
{
    /* array of operator definitions */
    java_operator* definition;
    /* token-to-operator map */
    operator_id* op_map;
    /* static map from OPID to IROP */
    operation* ir_map;
    /* #operand needed for each IROP */
    size_t* opid_operand_count;
} java_expression;

void init_expression(java_expression* expression);
void release_expression(java_expression* expression);

tree_node* expr_opid2node(const operator_id opid);
java_operator expr_opid2def(const java_expression* expression, operator_id opid);
operator_id expr_tid2opid(const java_expression* expression, java_lexeme_type tid);
operation expr_opid2irop(const java_expression* expression, operator_id opid);
size_t expr_opid_operand_count(const java_expression* expression, operator_id opid);

/**
 * Expression Element
 *
 * operator: JNT_EXPRESSION
 * operand: JNT_PRIMARY
*/
typedef struct _expression_element
{
    tree_node* node;
    struct _expression_element* next;
} expression_element;

/**
 * Expression Worker
 *
 * Shunting Yard Algorithm State Machine
*/
typedef struct
{
    // expression static data reference
    java_expression* definition;

    // last error
    java_error_id last_error;

    // operator stack used during parsing
    expression_element* operator_stack;
    // operand stack used during parsing
    expression_element* operand_stack;

    // operator stack length
    size_t num_operator;
    // operand stack length
    size_t num_operand;

    // operand push state: true if last push is operand
    bool last_push_operand;
} java_expression_worker;

void init_expression_worker(java_expression_worker* worker, java_expression* definition);
void release_expression_worker(java_expression_worker* worker);

void expression_worker_push(java_expression_worker* worker, tree_node* element);
bool expression_worker_complete(java_expression_worker* worker);
tree_node* expression_worker_export(java_expression_worker* worker);
bool expression_worker_is_complete(java_expression_worker* worker);

#endif
