#include "expression.h"
#include "node.h"
#include "ast.h"
#include "report.h"

/**
 * Expression Worker Initialization
 *
 * use heap to save stack space
*/
void init_expression(java_expression* expression)
{
    size_t definition_size = sizeof(java_operator) * OPID_MAX;
    size_t map_size = sizeof(java_operator) * JLT_MAX;

    expression->definition = (java_operator*)malloc_assert(definition_size);
    expression->op_map = (java_operator*)malloc_assert(map_size);

    // set default because OPID_UNDEFINED = 0
    memset(expression->definition, 0, definition_size);
    memset(expression->op_map, 0, map_size);

    report_expression_static_data_size(definition_size + map_size);

    /**
     * operator definition initialization
    */

    expression->definition[OPID_POST_INC] = OP(OP_ASSOC_LR, OP_LEVEL_TO_PRECD(14), JLT_SYM_INCREMENT);
    expression->definition[OPID_POST_DEC] = OP(OP_ASSOC_LR, OP_LEVEL_TO_PRECD(14), JLT_SYM_DECREMENT);

    expression->definition[OPID_SIGN_POS] = OP(OP_ASSOC_RL, OP_LEVEL_TO_PRECD(13), JLT_SYM_PLUS);
    expression->definition[OPID_SIGN_NEG] = OP(OP_ASSOC_RL, OP_LEVEL_TO_PRECD(13), JLT_SYM_MINUS);
    expression->definition[OPID_LOGIC_NOT] = OP(OP_ASSOC_RL, OP_LEVEL_TO_PRECD(13), JLT_SYM_EXCALMATION);
    expression->definition[OPID_BIT_NOT] = OP(OP_ASSOC_RL, OP_LEVEL_TO_PRECD(13), JLT_SYM_TILDE);
    expression->definition[OPID_PRE_INC] = OP(OP_ASSOC_RL, OP_LEVEL_TO_PRECD(13), JLT_SYM_INCREMENT);
    expression->definition[OPID_PRE_DEC] = OP(OP_ASSOC_RL, OP_LEVEL_TO_PRECD(13), JLT_SYM_DECREMENT);

    expression->definition[OPID_MUL] = OP(OP_ASSOC_LR, OP_LEVEL_TO_PRECD(12), JLT_SYM_ASTERISK);
    expression->definition[OPID_DIV] = OP(OP_ASSOC_LR, OP_LEVEL_TO_PRECD(12), JLT_SYM_FORWARD_SLASH);
    expression->definition[OPID_MOD] = OP(OP_ASSOC_LR, OP_LEVEL_TO_PRECD(12), JLT_SYM_PERCENT);

    expression->definition[OPID_ADD] = OP(OP_ASSOC_LR, OP_LEVEL_TO_PRECD(11), JLT_SYM_PLUS);
    expression->definition[OPID_SUB] = OP(OP_ASSOC_LR, OP_LEVEL_TO_PRECD(11), JLT_SYM_MINUS);

    expression->definition[OPID_SHIFT_L] = OP(OP_ASSOC_LR, OP_LEVEL_TO_PRECD(10), JLT_SYM_LEFT_SHIFT);
    expression->definition[OPID_SHIFT_R] = OP(OP_ASSOC_LR, OP_LEVEL_TO_PRECD(10), JLT_SYM_RIGHT_SHIFT);
    expression->definition[OPID_SHIFT_UR] = OP(OP_ASSOC_LR, OP_LEVEL_TO_PRECD(10), JLT_SYM_RIGHT_SHIFT_UNSIGNED);

    expression->definition[OPID_LESS] = OP(OP_ASSOC_LR, OP_LEVEL_TO_PRECD(9), JLT_SYM_ANGLE_BRACKET_OPEN);
    expression->definition[OPID_LESS_EQ] = OP(OP_ASSOC_LR, OP_LEVEL_TO_PRECD(9), JLT_SYM_LESS_EQUAL);
    expression->definition[OPID_GREAT] = OP(OP_ASSOC_LR, OP_LEVEL_TO_PRECD(9), JLT_SYM_ANGLE_BRACKET_CLOSE);
    expression->definition[OPID_GREAT_EQ] = OP(OP_ASSOC_LR, OP_LEVEL_TO_PRECD(9), JLT_SYM_GREATER_EQUAL);
    expression->definition[OPID_INSTANCE_OF] = OP(OP_ASSOC_LR, OP_LEVEL_TO_PRECD(9), JLT_RWD_INSTANCEOF);

    expression->definition[OPID_EQ] = OP(OP_ASSOC_LR, OP_LEVEL_TO_PRECD(8), JLT_SYM_RELATIONAL_EQUAL);
    expression->definition[OPID_NOT_EQ] = OP(OP_ASSOC_LR, OP_LEVEL_TO_PRECD(8), JLT_SYM_NOT_EQUAL);

    expression->definition[OPID_BIT_AND] = OP(OP_ASSOC_LR, OP_LEVEL_TO_PRECD(7), JLT_SYM_AMPERSAND);

    expression->definition[OPID_BIT_XOR] = OP(OP_ASSOC_LR, OP_LEVEL_TO_PRECD(6), JLT_SYM_CARET);

    expression->definition[OPID_BIT_OR] = OP(OP_ASSOC_LR, OP_LEVEL_TO_PRECD(5), JLT_SYM_PIPE);

    expression->definition[OPID_LOGIC_AND] = OP(OP_ASSOC_LR, OP_LEVEL_TO_PRECD(4), JLT_SYM_LOGIC_AND);

    expression->definition[OPID_LOGIC_OR] = OP(OP_ASSOC_LR, OP_LEVEL_TO_PRECD(3), JLT_SYM_LOGIC_OR);

    expression->definition[OPID_TERNARY_1] = OP(OP_ASSOC_RL, OP_LEVEL_TO_PRECD(2), JLT_SYM_QUESTION);
    expression->definition[OPID_TERNARY_2] = OP(OP_ASSOC_RL, OP_LEVEL_TO_PRECD(2), JLT_SYM_COLON);

    expression->definition[OPID_ASN] = OP(OP_ASSOC_RL, OP_LEVEL_TO_PRECD(1), JLT_SYM_EQUAL);
    expression->definition[OPID_ADD_ASN] = OP(OP_ASSOC_RL, OP_LEVEL_TO_PRECD(1), JLT_SYM_ADD_ASSIGNMENT);
    expression->definition[OPID_SUB_ASN] = OP(OP_ASSOC_RL, OP_LEVEL_TO_PRECD(1), JLT_SYM_SUBTRACT_ASSIGNMENT);
    expression->definition[OPID_MUL_ASN] = OP(OP_ASSOC_RL, OP_LEVEL_TO_PRECD(1), JLT_SYM_MULTIPLY_ASSIGNMENT);
    expression->definition[OPID_DIV_ASN] = OP(OP_ASSOC_RL, OP_LEVEL_TO_PRECD(1), JLT_SYM_DIVIDE_ASSIGNMENT);
    expression->definition[OPID_MOD_ASN] = OP(OP_ASSOC_RL, OP_LEVEL_TO_PRECD(1), JLT_SYM_MODULO_ASSIGNMENT);
    expression->definition[OPID_AND_ASN] = OP(OP_ASSOC_RL, OP_LEVEL_TO_PRECD(1), JLT_SYM_BIT_AND_ASSIGNMENT);
    expression->definition[OPID_XOR_ASN] = OP(OP_ASSOC_RL, OP_LEVEL_TO_PRECD(1), JLT_SYM_BIT_XOR_ASSIGNMENT);
    expression->definition[OPID_OR_ASN] = OP(OP_ASSOC_RL, OP_LEVEL_TO_PRECD(1), JLT_SYM_BIT_OR_ASSIGNMENT);
    expression->definition[OPID_SHIFT_L_ASN] = OP(OP_ASSOC_RL, OP_LEVEL_TO_PRECD(1), JLT_SYM_LEFT_SHIFT_ASSIGNMENT);
    expression->definition[OPID_SHIFT_R_ASN] = OP(OP_ASSOC_RL, OP_LEVEL_TO_PRECD(1), JLT_SYM_RIGHT_SHIFT_ASSIGNMENT);
    expression->definition[OPID_SHIFT_UR_ASN] = OP(OP_ASSOC_RL, OP_LEVEL_TO_PRECD(1), JLT_SYM_RIGHT_SHIFT_UNSIGNED_ASSIGNMENT);

    expression->definition[OPID_LAMBDA] = OP(OP_ASSOC_RL, OP_LEVEL_TO_PRECD(0), JLT_SYM_ARROW);

    /**
     * lexeme-to-operator map initialization
     * unused fields will be mapped to 0
     *
     * NOTE: lexeme and operator is NOT 1-1 relationship
     * so technically, this table is insufficient to determine correct OP
     * but for most cases, this works, so we cache this table to speed things up
     *
     * NOTE: rule for selecting conflict OPID is:
     * 1. it is unary, AND
     * 2. it can trigger an expression
     * so that we can use last_push_operand to test it
    */

    expression->op_map[JLT_SYM_INCREMENT] = expression->definition[OPID_POST_INC];
    expression->op_map[JLT_SYM_DECREMENT] = expression->definition[OPID_POST_DEC];
    // OPID_SIGN_POS has conflict lexeme JLT_SYM_PLUS
    // OPID_SIGN_NEG has conflict lexeme JLT_SYM_MINUS
    expression->op_map[JLT_SYM_EXCALMATION] = expression->definition[OPID_LOGIC_NOT];
    expression->op_map[JLT_SYM_TILDE] = expression->definition[OPID_BIT_NOT];
    // OPID_PRE_INC has conflict lexeme JLT_SYM_INCREMENT
    // OPID_PRE_DEC has conflict lexeme JLT_SYM_DECREMENT
    expression->op_map[JLT_SYM_ASTERISK] = expression->definition[OPID_MUL];
    expression->op_map[JLT_SYM_FORWARD_SLASH] = expression->definition[OPID_DIV];
    expression->op_map[JLT_SYM_PERCENT] = expression->definition[OPID_MOD];
    expression->op_map[JLT_SYM_PLUS] = expression->definition[OPID_ADD];
    expression->op_map[JLT_SYM_MINUS] = expression->definition[OPID_SUB];
    expression->op_map[JLT_SYM_LEFT_SHIFT] = expression->definition[OPID_SHIFT_L];
    expression->op_map[JLT_SYM_RIGHT_SHIFT] = expression->definition[OPID_SHIFT_R];
    expression->op_map[JLT_SYM_RIGHT_SHIFT_UNSIGNED] = expression->definition[OPID_SHIFT_UR];
    expression->op_map[JLT_SYM_ANGLE_BRACKET_OPEN] = expression->definition[OPID_LESS];
    expression->op_map[JLT_SYM_LESS_EQUAL] = expression->definition[OPID_LESS_EQ];
    expression->op_map[JLT_SYM_ANGLE_BRACKET_CLOSE] = expression->definition[OPID_GREAT];
    expression->op_map[JLT_SYM_GREATER_EQUAL] = expression->definition[OPID_GREAT_EQ];
    expression->op_map[JLT_RWD_INSTANCEOF] = expression->definition[OPID_INSTANCE_OF];
    expression->op_map[JLT_SYM_RELATIONAL_EQUAL] = expression->definition[OPID_EQ];
    expression->op_map[JLT_SYM_NOT_EQUAL] = expression->definition[OPID_NOT_EQ];
    expression->op_map[JLT_SYM_AMPERSAND] = expression->definition[OPID_BIT_AND];
    expression->op_map[JLT_SYM_CARET] = expression->definition[OPID_BIT_XOR];
    expression->op_map[JLT_SYM_PIPE] = expression->definition[OPID_BIT_OR];
    expression->op_map[JLT_SYM_LOGIC_AND] = expression->definition[OPID_LOGIC_AND];
    expression->op_map[JLT_SYM_LOGIC_OR] = expression->definition[OPID_LOGIC_OR];
    expression->op_map[JLT_SYM_QUESTION] = expression->definition[OPID_TERNARY_1];
    expression->op_map[JLT_SYM_COLON] = expression->definition[OPID_TERNARY_2];
    expression->op_map[JLT_SYM_EQUAL] = expression->definition[OPID_ASN];
    expression->op_map[JLT_SYM_ADD_ASSIGNMENT] = expression->definition[OPID_ADD_ASN];
    expression->op_map[JLT_SYM_SUBTRACT_ASSIGNMENT] = expression->definition[OPID_SUB_ASN];
    expression->op_map[JLT_SYM_MULTIPLY_ASSIGNMENT] = expression->definition[OPID_MUL_ASN];
    expression->op_map[JLT_SYM_DIVIDE_ASSIGNMENT] = expression->definition[OPID_DIV_ASN];
    expression->op_map[JLT_SYM_MODULO_ASSIGNMENT] = expression->definition[OPID_MOD_ASN];
    expression->op_map[JLT_SYM_BIT_AND_ASSIGNMENT] = expression->definition[OPID_AND_ASN];
    expression->op_map[JLT_SYM_BIT_XOR_ASSIGNMENT] = expression->definition[OPID_XOR_ASN];
    expression->op_map[JLT_SYM_BIT_OR_ASSIGNMENT] = expression->definition[OPID_OR_ASN];
    expression->op_map[JLT_SYM_LEFT_SHIFT_ASSIGNMENT] = expression->definition[OPID_SHIFT_L_ASN];
    expression->op_map[JLT_SYM_RIGHT_SHIFT_ASSIGNMENT] = expression->definition[OPID_SHIFT_R_ASN];
    expression->op_map[JLT_SYM_RIGHT_SHIFT_UNSIGNED_ASSIGNMENT] = expression->definition[OPID_SHIFT_UR_ASN];
    expression->op_map[JLT_SYM_ARROW] = expression->definition[OPID_LAMBDA];
}

/**
 * Release Expression Worker
*/
void release_expression(java_expression* expression)
{
    free(expression->op_map);
    free(expression->definition);
}

/**
 * Expression Worker Initialization
 *
 * use heap to save stack space
*/
void init_expression_worker(java_expression_worker* worker)
{
    worker->operator_stack = NULL;
    worker->last_push_operand = false;
}

/**
 * Release Expression Worker
*/
void release_expression_worker(java_expression_worker* worker)
{
    while (expression_stack_pop(worker));
}

/**
 * push operator to stack
 *
 * if push OPID_UNDEFINED, then it means it pushes an operand
 * on stack it is no-op, but state flag will change accordingly
*/
void expression_stack_push(java_expression_worker* worker, java_operator op)
{
    if (op == OPID_UNDEFINED)
    {
        worker->last_push_operand = true;
        return;
    }

    expr_operator* top = (expr_operator*)malloc_assert(sizeof(expr_operator));

    top->op = op;
    top->next = worker->operator_stack;
    worker->last_push_operand = true;
    worker->operator_stack = top;
}

/**
 * pop operator from top of stack
*/
bool expression_stack_pop(java_expression_worker* worker)
{
    if (!worker->operator_stack)
    {
        return false;
    }

    expr_operator* top = worker->operator_stack;
    worker->operator_stack = top->next;

    free(top);
    return true;
}

/**
 * peek the top operator
*/
java_operator expression_stack_top(java_expression_worker* worker)
{
    return worker->operator_stack ? worker->operator_stack->op : OP_INVALID;
}

/**
 * check if stack empty
*/
bool expression_stack_empty(java_expression_worker* worker)
{
    return worker->operator_stack == NULL;
}

/**
 * check if stack requires pop before pushing the op
 *
 * if we keep op consistent, for every op before allowing the push,
 * there are 3 conditions we need to pop before push
*/
bool expression_stack_pop_required(java_expression_worker* worker, java_operator op)
{
    if (!worker->operator_stack)
    {
        return false;
    }

    java_operator op_top = worker->operator_stack->op;

    return OP_PRECD(op) < OP_PRECD(op_top) ||
        (OP_PRECD(op) == OP_PRECD(op_top) && OP_ASSOC(op) == OP_ASSOC_LR);
}

/**
 * use top operator to generate an operator node
*/
tree_node* expression_stack_parse_top(java_expression_worker* worker)
{
    if (!worker->operator_stack)
    {
        return NULL;
    }

    tree_node* node = ast_node_operator();
    node_data_operator* data = (node_data_operator*)(node->data);

    data->op = worker->operator_stack->op;
    expression_stack_pop(worker);

    return node;
}
