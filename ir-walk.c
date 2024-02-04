/**
 * Control Flow Graph Generation Functions
 *
 * Most of them are recursive, and mainly divided into 2 types:
 * 1. expression walk
 * 2. statement walk
*/

#include "ir.h"

static size_t get_required_operand_count(operator_id opid)
{
    switch (opid)
    {
        case OPID_POST_INC:
        case OPID_POST_DEC:
        case OPID_SIGN_POS:
        case OPID_SIGN_NEG:
        case OPID_LOGIC_NOT:
        case OPID_BIT_NOT:
        case OPID_PRE_INC:
        case OPID_PRE_DEC:
            return 1;
        case OPID_MUL:
        case OPID_DIV:
        case OPID_MOD:
        case OPID_ADD:
        case OPID_SUB:
        case OPID_SHIFT_L:
        case OPID_SHIFT_R:
        case OPID_SHIFT_UR:
        case OPID_LESS:
        case OPID_LESS_EQ:
        case OPID_GREAT:
        case OPID_GREAT_EQ:
        case OPID_INSTANCE_OF:
        case OPID_EQ:
        case OPID_NOT_EQ:
        case OPID_BIT_AND:
        case OPID_BIT_XOR:
        case OPID_BIT_OR:
        case OPID_LOGIC_AND:
        case OPID_LOGIC_OR:
        case OPID_TERNARY_1:
        case OPID_TERNARY_2:
        case OPID_ASN:
        case OPID_ADD_ASN:
        case OPID_SUB_ASN:
        case OPID_MUL_ASN:
        case OPID_DIV_ASN:
        case OPID_MOD_ASN:
        case OPID_AND_ASN:
        case OPID_XOR_ASN:
        case OPID_OR_ASN:
        case OPID_SHIFT_L_ASN:
        case OPID_SHIFT_R_ASN:
        case OPID_SHIFT_UR_ASN:
        case OPID_LAMBDA:
            return 2;
        default:
            break;
    }

    return 0;
}

static tree_node* __previous_available_operand(tree_node* from)
{
    while (from && from->type != JNT_OPERATOR)
    {
        from = from->prev_sibling;
    }

    return from;
}

static void __interpret_operand(java_ir* ir, tree_node* base)
{
    // if an operand is refernecing an old OP, it means
    // it referes to that instruction
    if (base->type == JNT_OPERATOR)
    {
        /**
         * TODO: get that instruction
        */
        printf("OP[%d] ", base->data->operator.id);
        return;
    }

    tree_node* primary = base->first_child;
    java_token* token;

    /**
     * TODO: other primary types
    */
    if (primary->type == JNT_PRIMARY_COMPLEX)
    {
        token = primary->data->id.complex;

        /**
         * TODO: interpret all token types
        */
        if (token->type == JLT_LTR_NUMBER)
        {
            uint64_t __n;
            primitive __p = t2p(ir, token, &__n);
            char* content = t2s(token);
            printf("%llu(%s) ", __n, content);
            free(content);
        }
        else if (token->class == JT_IDENTIFIER)
        {
            char* content = t2s(token);
            printf("%s ", content);
            free(content);
        }
        else
        {
            printf("TODO ");
        }
    }
    else
    {
        printf("TODO ");
    }
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

    // stack control
    tree_node* base1;
    tree_node* base2;
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
     * the in aft order, 3 nodes are:
     * base2 base1 op
     *
    */
    size_t debug_loop_count = 0;
    while (top)
    {
        debug_loop_count++;
        // locate next operator
        // do NOT set base1 here: because if top is already an OP
        // loop will not run and base1 will not be set
        while (top && top->type != JNT_OPERATOR)
        {
            top = top->next_sibling;
        }

        // if no op, break
        if (!top)
        {
            break;
        }

        // initialize operand refernces
        // first operand is always the immediate previous one
        base1 = top->prev_sibling;
        base2 = NULL;

        // base1 must be available
        if (!base1)
        {
            ir_error(ir, JAVA_E_EXPRESSION_NO_OPERAND);
            break;
        }

        // adjust base2 if needed
        if (get_required_operand_count(top->data->operator.id) == 2)
        {
            // see the NOTE below for reasoning
            base2 = base1->prev_sibling;

            // operand must be available
            if (!base2)
            {
                ir_error(ir, JAVA_E_EXPRESSION_NO_OPERAND);
                break;
            }
        }

        /**
         * NOTE: now this is interesting...
         *
         * algorithm depends on this.
         *
         * for every step, we adjust prev sibling to enclose it
         * so next step will be able to jump over the entire
         * instruction instead of single OP node
         *
         * when jumping backwards from an OP, it will jump to
         * the Primary that is not parsed yet
         *
         * if there is none, it yields NULL
         *
         * WARNING: do NOT modify "next" pointer in AST as it is
         * important to maintain integrity of the tree
        */
        top->prev_sibling = base2 ? base2->prev_sibling : base1->prev_sibling;

        /**
         * TODO:Operator Code Generation
         * TODO:in AST node level or data level (better be data level), add a void*
         * reference to the instruction
        */
        printf("instruction: OP[%d] ", top->data->operator.id);

        /**
         * TODO:Operand Code Generation
         *
         * order matters here!
        */
        __interpret_operand(ir, base2);
        __interpret_operand(ir, base1);
        printf("\n");

        // reduction of current operator completed, move on
        top = top->next_sibling;
    }
    printf("expression walked %zd times.\n", debug_loop_count);

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
