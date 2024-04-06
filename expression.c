#include "expression.h"
#include "node.h"

/**
 * Expression Worker Initialization
 *
 * use heap to save stack space
*/
void init_expression(java_expression* expression)
{
    size_t map_size = sizeof(operator_id) * JLT_MAX;

    expression->definition = (java_operator*)malloc_assert(sizeof(java_operator) * OPID_MAX);
    expression->op_map = (operator_id*)malloc_assert(map_size);
    expression->ir_map = (operation*)malloc_assert(sizeof(operation) * OPID_MAX);
    expression->opid_operand_count = (size_t*)malloc_assert(sizeof(size_t) * OPID_MAX);

    // set default because OPID_UNDEFINED = 0
    memset(expression->op_map, 0, map_size);

    /**
     * operator definition initialization
    */

    expression->definition[OPID_UNDEFINED] = OP_INVALID;

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

    expression->op_map[JLT_SYM_INCREMENT] = OPID_POST_INC;
    expression->op_map[JLT_SYM_DECREMENT] = OPID_POST_DEC;
    // OPID_SIGN_POS has conflict lexeme JLT_SYM_PLUS
    // OPID_SIGN_NEG has conflict lexeme JLT_SYM_MINUS
    expression->op_map[JLT_SYM_EXCALMATION] = OPID_LOGIC_NOT;
    expression->op_map[JLT_SYM_TILDE] = OPID_BIT_NOT;
    // OPID_PRE_INC has conflict lexeme JLT_SYM_INCREMENT
    // OPID_PRE_DEC has conflict lexeme JLT_SYM_DECREMENT
    expression->op_map[JLT_SYM_ASTERISK] = OPID_MUL;
    expression->op_map[JLT_SYM_FORWARD_SLASH] = OPID_DIV;
    expression->op_map[JLT_SYM_PERCENT] = OPID_MOD;
    expression->op_map[JLT_SYM_PLUS] = OPID_ADD;
    expression->op_map[JLT_SYM_MINUS] = OPID_SUB;
    expression->op_map[JLT_SYM_LEFT_SHIFT] = OPID_SHIFT_L;
    expression->op_map[JLT_SYM_RIGHT_SHIFT] = OPID_SHIFT_R;
    expression->op_map[JLT_SYM_RIGHT_SHIFT_UNSIGNED] = OPID_SHIFT_UR;
    expression->op_map[JLT_SYM_ANGLE_BRACKET_OPEN] = OPID_LESS;
    expression->op_map[JLT_SYM_LESS_EQUAL] = OPID_LESS_EQ;
    expression->op_map[JLT_SYM_ANGLE_BRACKET_CLOSE] = OPID_GREAT;
    expression->op_map[JLT_SYM_GREATER_EQUAL] = OPID_GREAT_EQ;
    expression->op_map[JLT_RWD_INSTANCEOF] = OPID_INSTANCE_OF;
    expression->op_map[JLT_SYM_RELATIONAL_EQUAL] = OPID_EQ;
    expression->op_map[JLT_SYM_NOT_EQUAL] = OPID_NOT_EQ;
    expression->op_map[JLT_SYM_AMPERSAND] = OPID_BIT_AND;
    expression->op_map[JLT_SYM_CARET] = OPID_BIT_XOR;
    expression->op_map[JLT_SYM_PIPE] = OPID_BIT_OR;
    expression->op_map[JLT_SYM_LOGIC_AND] = OPID_LOGIC_AND;
    expression->op_map[JLT_SYM_LOGIC_OR] = OPID_LOGIC_OR;
    expression->op_map[JLT_SYM_QUESTION] = OPID_TERNARY_1;
    expression->op_map[JLT_SYM_COLON] = OPID_TERNARY_2;
    expression->op_map[JLT_SYM_EQUAL] = OPID_ASN;
    expression->op_map[JLT_SYM_ADD_ASSIGNMENT] = OPID_ADD_ASN;
    expression->op_map[JLT_SYM_SUBTRACT_ASSIGNMENT] = OPID_SUB_ASN;
    expression->op_map[JLT_SYM_MULTIPLY_ASSIGNMENT] = OPID_MUL_ASN;
    expression->op_map[JLT_SYM_DIVIDE_ASSIGNMENT] = OPID_DIV_ASN;
    expression->op_map[JLT_SYM_MODULO_ASSIGNMENT] = OPID_MOD_ASN;
    expression->op_map[JLT_SYM_BIT_AND_ASSIGNMENT] = OPID_AND_ASN;
    expression->op_map[JLT_SYM_BIT_XOR_ASSIGNMENT] = OPID_XOR_ASN;
    expression->op_map[JLT_SYM_BIT_OR_ASSIGNMENT] = OPID_OR_ASN;
    expression->op_map[JLT_SYM_LEFT_SHIFT_ASSIGNMENT] = OPID_SHIFT_L_ASN;
    expression->op_map[JLT_SYM_RIGHT_SHIFT_ASSIGNMENT] = OPID_SHIFT_R_ASN;
    expression->op_map[JLT_SYM_RIGHT_SHIFT_UNSIGNED_ASSIGNMENT] = OPID_SHIFT_UR_ASN;
    expression->op_map[JLT_SYM_ARROW] = OPID_LAMBDA;

    /**
     * OPID to IROP map
     *
     * NOTE: composite assignment operators are reduced by stripping the
     * "assignment" part
    */

    expression->ir_map[0] = IROP_UNDEFINED;
    expression->ir_map[OPID_POST_INC] = IROP_AINC;
    expression->ir_map[OPID_POST_DEC] = IROP_ADEC;
    expression->ir_map[OPID_SIGN_POS] = IROP_POS;
    expression->ir_map[OPID_SIGN_NEG] = IROP_NEG;
    expression->ir_map[OPID_LOGIC_NOT] = IROP_LNEG;
    expression->ir_map[OPID_BIT_NOT] = IROP_BNEG;
    expression->ir_map[OPID_PRE_INC] = IROP_BINC;
    expression->ir_map[OPID_PRE_DEC] = IROP_BDEC;
    expression->ir_map[OPID_MUL] = IROP_MUL;
    expression->ir_map[OPID_DIV] = IROP_DIV;
    expression->ir_map[OPID_MOD] = IROP_MOD;
    expression->ir_map[OPID_ADD] = IROP_ADD;
    expression->ir_map[OPID_SUB] = IROP_SUB;
    expression->ir_map[OPID_SHIFT_L] = IROP_SLS;
    expression->ir_map[OPID_SHIFT_R] = IROP_SRS;
    expression->ir_map[OPID_SHIFT_UR] = IROP_URS;
    expression->ir_map[OPID_LESS] = IROP_LT;
    expression->ir_map[OPID_LESS_EQ] = IROP_LE;
    expression->ir_map[OPID_GREAT] = IROP_GT;
    expression->ir_map[OPID_GREAT_EQ] = IROP_GE;
    expression->ir_map[OPID_INSTANCE_OF] = IROP_IO;
    expression->ir_map[OPID_EQ] = IROP_EQ;
    expression->ir_map[OPID_NOT_EQ] = IROP_NE;
    expression->ir_map[OPID_BIT_AND] = IROP_BAND;
    expression->ir_map[OPID_BIT_XOR] = IROP_XOR;
    expression->ir_map[OPID_BIT_OR] = IROP_BOR;
    expression->ir_map[OPID_LOGIC_AND] = IROP_LAND;
    expression->ir_map[OPID_LOGIC_OR] = IROP_LOR;
    expression->ir_map[OPID_TERNARY_1] = IROP_TC;
    expression->ir_map[OPID_TERNARY_2] = IROP_TB;
    expression->ir_map[OPID_ASN] = IROP_ASN;
    expression->ir_map[OPID_ADD_ASN] = IROP_ADD;
    expression->ir_map[OPID_SUB_ASN] = IROP_SUB;
    expression->ir_map[OPID_MUL_ASN] = IROP_MUL;
    expression->ir_map[OPID_DIV_ASN] = IROP_DIV;
    expression->ir_map[OPID_MOD_ASN] = IROP_MOD;
    expression->ir_map[OPID_AND_ASN] = IROP_LAND;
    expression->ir_map[OPID_XOR_ASN] = IROP_XOR;
    expression->ir_map[OPID_OR_ASN] = IROP_LOR;
    expression->ir_map[OPID_SHIFT_L_ASN] = IROP_SLS;
    expression->ir_map[OPID_SHIFT_R_ASN] = IROP_SRS;
    expression->ir_map[OPID_SHIFT_UR_ASN] = IROP_URS;
    expression->ir_map[OPID_LAMBDA] = IROP_LMD;

    /**
     * OPID operand count
     *
     * this info can definitely be merged into ir_map using bit mask,
     * but to maintain faster access, let's just waste some memory
     *
     * for ternary operators: see expression_worker_push, the value
     * is chosen carefully here because in AST, ':' does not appear;
     * while '?' will contain all 3 parts of the conditional expression;
     * values are configured this way to make expression_worker_push work
    */

    expression->opid_operand_count[0] = 0;
    expression->opid_operand_count[OPID_POST_INC] = 1;
    expression->opid_operand_count[OPID_POST_DEC] = 1;
    expression->opid_operand_count[OPID_SIGN_POS] = 1;
    expression->opid_operand_count[OPID_SIGN_NEG] = 1;
    expression->opid_operand_count[OPID_LOGIC_NOT] = 1;
    expression->opid_operand_count[OPID_BIT_NOT] = 1;
    expression->opid_operand_count[OPID_PRE_INC] = 1;
    expression->opid_operand_count[OPID_PRE_DEC] = 1;
    expression->opid_operand_count[OPID_MUL] = 2;
    expression->opid_operand_count[OPID_DIV] = 2;
    expression->opid_operand_count[OPID_MOD] = 2;
    expression->opid_operand_count[OPID_ADD] = 2;
    expression->opid_operand_count[OPID_SUB] = 2;
    expression->opid_operand_count[OPID_SHIFT_L] = 2;
    expression->opid_operand_count[OPID_SHIFT_R] = 2;
    expression->opid_operand_count[OPID_SHIFT_UR] = 2;
    expression->opid_operand_count[OPID_LESS] = 2;
    expression->opid_operand_count[OPID_LESS_EQ] = 2;
    expression->opid_operand_count[OPID_GREAT] = 2;
    expression->opid_operand_count[OPID_GREAT_EQ] = 2;
    expression->opid_operand_count[OPID_INSTANCE_OF] = 2;
    expression->opid_operand_count[OPID_EQ] = 2;
    expression->opid_operand_count[OPID_NOT_EQ] = 2;
    expression->opid_operand_count[OPID_BIT_AND] = 2;
    expression->opid_operand_count[OPID_BIT_XOR] = 2;
    expression->opid_operand_count[OPID_BIT_OR] = 2;
    expression->opid_operand_count[OPID_LOGIC_AND] = 2;
    expression->opid_operand_count[OPID_LOGIC_OR] = 2;
    expression->opid_operand_count[OPID_TERNARY_1] = 3; // special: control entire conditional expression
    expression->opid_operand_count[OPID_TERNARY_2] = 0; // special: not appear in AST
    expression->opid_operand_count[OPID_ASN] = 2;
    expression->opid_operand_count[OPID_ADD_ASN] = 2;
    expression->opid_operand_count[OPID_SUB_ASN] = 2;
    expression->opid_operand_count[OPID_MUL_ASN] = 2;
    expression->opid_operand_count[OPID_DIV_ASN] = 2;
    expression->opid_operand_count[OPID_MOD_ASN] = 2;
    expression->opid_operand_count[OPID_AND_ASN] = 2;
    expression->opid_operand_count[OPID_XOR_ASN] = 2;
    expression->opid_operand_count[OPID_OR_ASN] = 2;
    expression->opid_operand_count[OPID_SHIFT_L_ASN] = 2;
    expression->opid_operand_count[OPID_SHIFT_R_ASN] = 2;
    expression->opid_operand_count[OPID_SHIFT_UR_ASN] = 2;
    expression->opid_operand_count[OPID_LAMBDA] = 2;
}

/**
 * Release Expression Worker
*/
void release_expression(java_expression* expression)
{
    free(expression->op_map);
    free(expression->definition);
    free(expression->ir_map);
    free(expression->opid_operand_count);
}

/**
 * map OPID to AST node
*/
tree_node* expr_opid2node(const operator_id opid)
{
    tree_node* node = ast_node_new(JNT_EXPRESSION);
    node->data.expression->op = opid;

    return node;
}

/**
 * map OPID to op definition
*/
java_operator expr_opid2def(const java_expression* expression, operator_id opid)
{
    return expression->definition[opid];
}

/**
 * map token id to OPID
*/
operator_id expr_tid2opid(const java_expression* expression, java_lexeme_type tid)
{
    return expression->op_map[tid];
}

/**
 * map OPID to IROP
*/
operation expr_opid2irop(const java_expression* expression, operator_id opid)
{
    return expression->ir_map[opid];
}

/**
 * map OPID to operand count
*/
size_t expr_opid_operand_count(const java_expression* expression, operator_id opid)
{
    return expression->opid_operand_count[opid];
}

static void expression_worker_push_operator(java_expression_worker* worker, tree_node* element)
{
    expression_element* new_stack_top = (expression_element*)malloc_assert(sizeof(expression_element));
    new_stack_top->node = element;
    new_stack_top->next = worker->operator_stack;

    worker->num_operator++;
    worker->operator_stack = new_stack_top;
}

static tree_node* expression_worker_pop_operator(java_expression_worker* worker)
{
    if (!worker->operator_stack)
    {
        return NULL;
    }

    tree_node* n = worker->operator_stack->node;
    expression_element* e = worker->operator_stack;

    worker->operator_stack = e->next;
    worker->num_operator--;
    free(e);

    return n;
}

static void expression_worker_push_operand(java_expression_worker* worker, tree_node* element)
{
    expression_element* new_stack_top = (expression_element*)malloc_assert(sizeof(expression_element));
    new_stack_top->node = element;
    new_stack_top->next = worker->operand_stack;

    worker->num_operand++;
    worker->operand_stack = new_stack_top;
}

static tree_node* expression_worker_pop_operand(java_expression_worker* worker)
{
    if (!worker->operand_stack)
    {
        return NULL;
    }

    tree_node* n = worker->operand_stack->node;
    expression_element* e = worker->operand_stack;

    worker->operand_stack = e->next;
    worker->num_operand--;
    free(e);

    return n;
}

static operator_id expression_worker_top_operator(java_expression_worker* worker)
{
    return worker->operator_stack ? worker->operator_stack->node->data.expression->op : OPID_UNDEFINED;
}

/**
 * check if operator on stack pop needs to be popped before pushing the new one
 *
 * if operator=NULL, means there is no more incoming operator, as if the incoming
 * operator have infinitely low precedence, hence enforces any operator on stack
 * top to pop
*/
static bool expression_worker_should_pop_top_operator(const java_expression_worker* worker, const tree_node* operator)
{
    if (!worker->operator_stack)
    {
        return false;
    }

    // this semantic is useful in expression_worker_complete
    if (!operator)
    {
        return true;
    }

    operator_id opid_top = worker->operator_stack->node->data.expression->op;
    operator_id opid_in = operator->data.expression->op;
    java_operator op_top = expr_opid2def(worker->definition, opid_top);
    java_operator op_in = expr_opid2def(worker->definition, opid_in);

    /**
     * Precedence Check
     *
     * regular logic:
     *
     * 1. stack-top operator has higher precedence than incoming one, OR
     * 2. same precedence, but associativity is from left to right
     *
     * now if the new top operator and incoming operator are both ':',
     * it means the follwing condition occurs:
     *
     * a ? b ? c : d : e
     *           ~~~~~
     *
     * and since we what above to be evaluated as:
     *
     * a ? (b ? c : d) : e
     *
     * so when both are ':', the one on stack top still needs to be popped
    */
    return OP_PRECD(op_in) < OP_PRECD(op_top) ||
        (OP_PRECD(op_in) == OP_PRECD(op_top) && OP_ASSOC(op_in) == OP_ASSOC_LR) ||
        (opid_in == OPID_TERNARY_2 && opid_top == OPID_TERNARY_2);
}

/**
 * Operator Stack Pre-Push Evaluation
 *
 * it evaluates incoming operator, to see if stack is ready for pushing,
 * by enforcing precedence of operators
 *
 * if not ready, it starts popping operator off of stack, and for each,
 * it will finalize this operation, and transform it into an "operand"
 *
 * if incoming operator is not provided, the function collapses entire
 * operator stack at its best efforts
 * (see expression_worker_should_pop_top_operator)
 *
 * it returns false if stack evaluation finishes with error(s)
*/
static bool expression_worker_evaluate(java_expression_worker* worker, tree_node* incoming_op)
{
    tree_node* top_op;
    operator_id top_opid;
    size_t operand_count;
    bool collapse_ternary;
    bool ret = true;

    // Precedence Check
    while (expression_worker_should_pop_top_operator(worker, incoming_op))
    {
        collapse_ternary = false;
        top_op = expression_worker_pop_operator(worker);
        top_opid = top_op->data.expression->op;
        operand_count = expr_opid_operand_count(worker->definition, top_opid);

        /**
         * Skip Condition (Ternary Operator)
         *
         * For ternary operator ":", it is transparent to AST and contribute
         * no meaning but only allows next pop to be, AND MUST BE, the "?"
         * operator
         *
         * re-fetch will come later because stack needs to be validated first;
         * but we do need other info of stack top during halt condition check
        */
        if (top_opid == OPID_TERNARY_2)
        {
            // discard this operator, and read stack top info without pop
            collapse_ternary = true;
            top_opid = expression_worker_top_operator(worker);
            operand_count = expr_opid_operand_count(worker->definition, top_opid);
            tree_node_delete(top_op);
        }

        /**
         * Halt Condition & Recovery
         *
         * if expression is reduced at an inappropriate state, the expression
         * has syntax error and error flag will be set, and recovery strategy
         * is chosen so that the routine can continue at its best efforts
         *
         * multiple errors might occur, but only the last one will be logged,
         * as it is unecessary to log every step of error during one stack
         * evaluation
         *
         * 1. incorrectly pop '?': as a ternary operator, '?' is NOT the bound
         *    of the operation, ':' is. trying to pop '?' means the operation
         *    is incomplete
         * 2. try to pop '?' but not reached: when ':' is popped, '?' needs to
         *    be the actual one to be popped; if it is not there, then it is an
         *    error
         * 3. insufficient operand on stack (range check)
         *
         * Order Matters Here, sort of... and it is not mandatory:
         *
         * insufficient operand check has lowest priority for better error
         * message: e.g. if ternary-related has error, then operand count
         * is not trustworthy, error message needs to fall under condition
         * 1 or 2 (especially for 1: where count can be any operator, but
         * the range check should be ignored in this case as it is not
         * meaningful)
         *
        */
        if (collapse_ternary && top_opid != OPID_TERNARY_1)
        {
            /**
             * if collapsing ":" but cannot find "?" on stack top
             *
             * do nothing: this will result in incorrect expression,
             * but at least there is an error
            */
            worker->last_error = JAVA_E_EXPRESSION_INCOMPLETE_TERNARY;
            ret = false;
            continue;
        }
        else if (!collapse_ternary && top_opid == OPID_TERNARY_1)
        {
            /**
             * if not collapsing but popping "?"
             *
             * means there is one operand missing, so only pop:
             * (total required by "?") - 1
             *
             * extra guard against operand stack length is applied
             * just to be safe
            */
            worker->last_error = JAVA_E_EXPRESSION_INCOMPLETE_TERNARY;
            operand_count = min(worker->num_operand, operand_count - 1);
            ret = false;
        }
        else if (worker->num_operand < operand_count)
        {
            worker->last_error = JAVA_E_EXPRESSION_NO_OPERAND;
            operand_count = worker->num_operand;
            ret = false;
        }

        /**
         * Ternary Re-Fetch
         *
         * collapse_ternary is not used in condition, because 2 cases are covered here:
         *
         * 1. successful collapsing: need to collapse a ternary, and top is "?"
         * 2. halt condition: not collapsing ternary but stack top is "?", see above
         *
         * both conditions have stack top OPID_TERNARY_1
        */
        if (top_opid == OPID_TERNARY_1)
        {
            top_op = expression_worker_pop_operator(worker);
        }

        // pop operands to complete this operator
        for (size_t i = 0; i < operand_count; i++)
        {
            tree_node_add_first_child(top_op, expression_worker_pop_operand(worker));
        }

        // now the operator is complete, it becomes a "value" of other operators
        // hence: an operand
        expression_worker_push_operand(worker, top_op);
    }

    return ret;
}

/**
 * Expression Worker Initialization
 *
 * use heap to save stack space
*/
void init_expression_worker(java_expression_worker* worker, java_expression* definition)
{
    worker->definition = definition;
    worker->last_error = JAVA_E_MAX;
    worker->operator_stack = NULL;
    worker->operand_stack = NULL;
    worker->num_operator = 0;
    worker->num_operand = 0;
    worker->last_push_operand = false;
}

/**
 * Release Expression Worker
 *
*/
void release_expression_worker(java_expression_worker* worker)
{
    tree_node* n;

    // release operator stack
    while (true)
    {
        n = expression_worker_pop_operator(worker);

        if (!n) { break; }

        tree_node_delete(n);
    }

    // release operand stack
    while (true)
    {
        n = expression_worker_pop_operand(worker);

        if (!n) { break; }

        tree_node_delete(n);
    }

    // worker->definition is a reference so no need to free
}

/**
 * Feed new expression element to expression worker
 *
 * expression element can be:
 * JNT_PRIMARY: operand
 * JNT_EXPRESSION: operator
 *
 * when JNT_EXPRESSION has children attached, it is an operand
 * instead of operator
 *
 * for any tree_node that is not any type mentioned above,
 * function is no-op
 *
 * NOTE: this functions clears last error information
*/
void expression_worker_push(java_expression_worker* worker, tree_node* element)
{
    // clear error
    worker->last_error = JAVA_E_MAX;

    // guard: invalid node will not be processed
    if (element->type != JNT_EXPRESSION && element->type != JNT_PRIMARY)
    {
        return;
    }

    /**
     * incoming element is strictly classified
     *
     * for parenthesized expression, it is already evaluated by parse_primary
     * so JNT_EXPRESSION is considered as an operator only when it has no
     * children attached;
     *
     * otherwise it is an operand: this is consistent with semantics used in
     * expression_worker_evaluate
    */
    if (element->type == JNT_EXPRESSION && !element->first_child)
    {
        /**
         * TODO: should we return after expression_worker_evaluate?
         * if so, need to delete element node here to make memory safe
        */
        expression_worker_evaluate(worker, element);
        expression_worker_push_operator(worker, element);
        worker->last_push_operand = false;
    }
    else // JNT_PRIMARY or JNT_EXPRESSION with children
    {
        expression_worker_push_operand(worker, element);
        worker->last_push_operand = true;
    }
}

/**
 * Try collapsing current operator stack
 *
*/
bool expression_worker_complete(java_expression_worker* worker)
{
    return expression_worker_evaluate(worker, NULL);
}

/**
 * Get the completed expression
 *
 * after expression_worker_complete, the final expression will locate at top
 * of operand stack
*/
tree_node* expression_worker_export(java_expression_worker* worker)
{
    return expression_worker_pop_operand(worker);
}

/**
 * Check if expression worker is in complete state after flush
*/
bool expression_worker_is_complete(java_expression_worker* worker)
{
    // no expression, or no dangling operands/operators left
    return !worker->operator_stack && (!worker->operand_stack || !worker->operand_stack->next);
}
