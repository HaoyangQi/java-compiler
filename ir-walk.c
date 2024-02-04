/**
 * Control Flow Graph Generation Functions
 *
 * Most of them are recursive, and mainly divided into 2 types:
 * 1. expression walk
 * 2. statement walk
*/

#include "ir.h"

static size_t get_required_operand_count(java_operator type)
{
    switch (type)
    {
        default:
            break;
    }

    return 0;
}

/**
 * TODO: Expression AST Walk
 *
 * node: JNT_EXPRESSION
*/
cfg* walk_expression(java_ir* ir, tree_node* expression)
{
    // if start is OP, expression is invalid
    if (!expression->first_child || expression->first_child->type == JNT_OPERATOR)
    {
        ir_error(ir, JAVA_E_EXPRESSION_NO_OPERAND);
        return NULL;
    }

    // cfg* g = (cfg*)malloc_assert(sizeof(cfg));
    // init_cfg(g);

    tree_node* base = expression->first_child;
    tree_node* top = expression->first_child;

    /**
     * expression traversal
     *
     * base case is: we find pattern PO or PPO
     * where P is Primary and O is Operator
     *
     * reduce from left to right, until all are accepted
     *
     * if there are remainders, expression is invalid
     *
    */
    // while (true)
    // {
    //     // locate next operator
    //     while (top->type != JNT_OPERATOR)
    //     {
    //         top = top->next_sibling;
    //     }

    //     // if no op, break
    //     if (!top)
    //     {
    //         break;
    //     }
    // }

    return NULL;
}

/**
 * TODO: Block AST Walk
 *
 * node: JNT_BLOCK
*/
cfg* walk_block(java_ir* ir, tree_node* block)
{
    return NULL;
}
