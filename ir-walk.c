/**
 * Control Flow Graph Generation Functions
 *
 * Most of them are recursive, and mainly divided into 2 types:
 * 1. expression walk
 * 2. statement walk
*/

#include "ir.h"

static tree_node* __previous_available_operand(tree_node* from)
{
    while (from && from->type != JNT_OPERATOR)
    {
        from = from->prev_sibling;
    }

    return from;
}

/**
 * parse an operand and fill the provided reference object
*/
static reference* __interpret_operand(java_ir* ir, basic_block* block, tree_node* base)
{
    // this is not a guard: an operand can be marked as not-needed (thus NULL)
    // if so, the function is no-op
    if (!base)
    {
        return NULL;
    }

    reference* ref = new_reference();

    // if an operand is refernecing an old OP, it means
    // it referes to that instruction
    if (base->type == JNT_OPERATOR)
    {
        /**
         * TODO: should we include JNT_EXPRESSION here for ( Expression ) ?
        */
        printf("OP[%d] ", base->data->operator.id);
        ref->type = IR_ASN_REF_INSTRUCTION;
        ref->doi = base->data->operator.instruction;

        return ref;
    }

    tree_node* primary = base->first_child;
    java_token* token;

    /**
     * TODO: other primary types
    */
    if (primary->type == JNT_PRIMARY_COMPLEX)
    {
        token = primary->data->id.complex;

        // try get literal definition
        // if token is not literal, funtion is no-op and NULL is returned
        definition* __def = def_li(ir, token);

        /**
         * TODO: interpret all token types
        */
        if (token->type == JLT_LTR_NUMBER)
        {
            ref->type = IR_ASN_REF_LITERAL;
            ref->doi = __def;

            char* content = t2s(token);
            printf("%llu(%s) ", __def->li_number.imm, content);
            free(content);
        }
        else if (token->class == JT_IDENTIFIER)
        {
            char* content = t2s(token);
            printf("%s ", content);
            free(content);

            // def(__def, move)
            // definition_delete(__def)
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

    return ref;
}

/**
 * finalize instruction
 *
 * lvalue <- operand_1 op operand_2
 *
 * NOTE: lvalue reference must be copied from an operand!
 * because the version number will be different
 *
 * op: JNT_OPERATOR
*/
static bool __execute_instruction(
    java_ir* ir,
    basic_block* block,
    tree_node* op,
    reference* operand_1,
    reference* operand_2
)
{
    instruction* inst = new_instruction();
    operator_id id = op->data->operator.id;
    bool validate_lvalue = false;

    /**
     * reduce composite assignment operators
     *
     * for composite assignment operators, lvalue needs to be set
    */
    switch (id)
    {
        case OPID_ASN:
            // simply move
            inst->lvalue = operand_1;
            operand_1 = operand_2;
            operand_2 = NULL;
            validate_lvalue = true;
            break;
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
            // need topy because use version will differ
            inst->lvalue = copy_reference(operand_1);
            validate_lvalue = true;
            break;
        default:
            inst->lvalue = NULL;
            break;
    }

    // fill
    inst->op = expr_opid2irop(ir->expression, id);
    inst->operand_1 = operand_1;
    inst->operand_2 = operand_2;

    /**
     * validate lvalue
     *
     * NOTE: other 2 types of reference is hard to validate here
     * because we do not know if it infers a valid lvalue
     * so we do it somewhere else
    */
    if (validate_lvalue)
    {
        if (!inst->lvalue)
        {
            ir_error(ir, JAVA_E_EXPRESSION_NO_LVALUE);
        }
        else if (inst->lvalue->type == IR_ASN_REF_LITERAL)
        {
            ir_error(ir, JAVA_E_EXPRESSION_NO_LVALUE);
        }
        else
        {
            validate_lvalue = false;
        }
    }

    // append instruction & validation
    if (validate_lvalue || !instruction_push_back(block, inst))
    {
        delete_instruction(inst, false);
        return false;
    }
    else
    {
        // link the instruction to this op
        op->data->operator.instruction = inst;
        return true;
    }
}

/**
 * TODO: Expression AST Walk
 *
 * It walks the expression and convert it into a series of instructions
 * all instructions will be in single block
 *
 * Putting everything in single block reduces complexity of the recursion
 * and also can sever the ties with AST completely, so that second pass
 * does not have to worry about AST anymore
 *
 * node: JNT_EXPRESSION
*/
bool __expression_to_block(java_ir* ir, basic_block* block, tree_node* expression)
{
    bool ret = true;

    // if start is OP, expression is invalid
    if (!expression->first_child || expression->first_child->type == JNT_OPERATOR)
    {
        ir_error(ir, JAVA_E_EXPRESSION_NO_OPERAND);
        return false;
    }

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
    while (top)
    {
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
            ret = false;
            break;
        }

        // adjust base2 if needed
        if (expr_opid_operand_count(ir->expression, top->data->operator.id) == 2)
        {
            // see the NOTE below for reasoning
            base2 = base1->prev_sibling;

            // operand must be available
            if (!base2)
            {
                ir_error(ir, JAVA_E_EXPRESSION_NO_OPERAND);
                ret = false;
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

        printf("instruction: OP[%d] ", top->data->operator.id);

        /**
         * Code Generation
         *
         * order matters here!
        */
        __execute_instruction(
            ir, block, top,
            __interpret_operand(ir, block, base2),
            __interpret_operand(ir, block, base1)
        );

        printf("\n");

        // reduction of current operator completed, move on
        top = top->next_sibling;
    }

    return ret;
}

/**
 * TODO: Expression AST Walk
 *
 * It walks an expression, and sanitize the expression block
 * so it does not contain and syntatic sugar
 *
 * so far: we only have ternary operator (? :) to be converted
 *
 * node: JNT_EXPRESSION
*/
void walk_expression(java_ir* ir, cfg* g, tree_node* expression)
{
    basic_block* b = cfg_new_basic_block(g);

    /**
     * TODO: shall we handle return value being false?
    */
    __expression_to_block(ir, b, expression);

    /**
     * sanitize the block
     *
     * this may cause the block be transformed into a graph
     *
     * TODO: transform ternary operators (? :)
     * TODO: in future, we probably need to do something for lambda
    */
}

/**
 * TODO: Block AST Walk
 *
 * node: JNT_BLOCK
*/
void walk_block(java_ir* ir, cfg* g, tree_node* block, bool use_new_scope)
{
    hash_table* scope = use_new_scope ? lookup_new_scope(ir, LST_NONE) : lookup_top_scope(ir);

    /**
     * TODO:
    */

    if (use_new_scope)
    {
        lookup_pop_scope(ir, true);
    }
}
