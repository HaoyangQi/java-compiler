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
static reference* __interpret_operand(java_ir* ir, tree_node* base)
{
    // this is not a guard: an operand can be marked as not-needed (thus NULL)
    // if so, the function is no-op
    if (!base)
    {
        return NULL;
    }

    reference* ref = new_reference(IR_ASN_REF_UNDEFINED, NULL);

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
    char* content;

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
         * TODO: this includes sequence of JT_IDENTIFIER
         *       as field access
        */
        if (token->type == JLT_LTR_NUMBER)
        {
            ref->type = IR_ASN_REF_LITERAL;
            ref->doi = __def;

            content = t2s(token);
            printf("%llu(%s) ", __def->li_number.imm, content);
            free(content);
        }
        else if (token->class == JT_IDENTIFIER)
        {
            ref->type = IR_ASN_REF_DEFINITION;
            content = t2s(token);
            ref->doi = use(ir, content, DU_CTL_LOOKUP_GLOBAL, JAVA_E_REF_UNDEFINED);
            printf("%s ", content);
            free(content);

            /**
             * TODO: how to handle field access?
            */
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
static void __execute_instruction(
    java_ir* ir,
    cfg_worker* worker,
    tree_node* op,
    reference* operand_1,
    reference* operand_2
)
{
    operator_id id = op->data->operator.id;
    reference* lvalue = NULL;
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
            lvalue = operand_1;
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
            lvalue = copy_reference(operand_1);
            validate_lvalue = true;
            break;
        default:
            break;
    }

    /**
     * validate lvalue
     *
     * NOTE: only cover the case that has to be donw with OPID
     * the rest is covered somewhere else
    */
    if (validate_lvalue && !lvalue)
    {
        ir_error(ir, JAVA_E_EXPRESSION_NO_LVALUE);
    }

    // link the instruction to this op
    op->data->operator.instruction = cfg_worker_execute(
        ir, worker, expr_opid2irop(ir->expression, id), &lvalue, &operand_1, &operand_2);

    // cleanup
    delete_reference(lvalue);
    delete_reference(operand_1);
    delete_reference(operand_2);
}

/**
 * Expression AST Walk
 *
 * do not use initialized container, as the data region will be wiped
 *
 * node: JNT_EXPRESSION
*/
cfg_worker* walk_expression(java_ir* ir, tree_node* expression)
{
    cfg_worker* worker = (cfg_worker*)malloc_assert(sizeof(cfg_worker));
    init_cfg_worker(worker);

    // if start is OP or end with non-OP, expression is invalid
    if (!expression->first_child || expression->first_child->type == JNT_OPERATOR)
    {
        ir_error(ir, JAVA_E_EXPRESSION_NO_OPERAND);
        return worker;
    }

    // stack control
    tree_node* base1;
    tree_node* base2;
    tree_node* top = expression->first_child;

    if (!top->next_sibling)
    {
        // minimum case: constant expression (only one operand)
        reference* constant = __interpret_operand(ir, top);
        cfg_worker_execute(ir, worker, IROP_NONE, NULL, &constant, NULL);
        delete_reference(constant);

        return worker;
    }
    else if (expression->last_child->type != JNT_OPERATOR)
    {
        // if end with non-OP, expression is invalid
        ir_error(ir, JAVA_E_EXPRESSION_NO_OPERATOR);
        return worker;
    }

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
            ir, worker, top,
            __interpret_operand(ir, base2),
            __interpret_operand(ir, base1)
        );

        printf("\n");

        // reduction of current operator completed, move on
        top = top->next_sibling;
    }

    return worker;
}

void __execute_statement(java_ir* ir, cfg_worker* worker, tree_node* stmt, bool execute_in_new_block);
cfg_worker* walk_block(java_ir* ir, tree_node* block, bool use_new_scope);

/**
 * walk return statement
 *
 * return ---+--- Expression
 *
 * node: JNT_STATEMENT_RETURN
*/
void __execute_statement_return(java_ir* ir, cfg_worker* worker, tree_node* stmt)
{
    cfg_worker* w = NULL;
    reference* ref = NULL;

    // Expression
    stmt = stmt->first_child;

    if (stmt)
    {
        // parse return value
        w = walk_expression(ir, stmt);
        cfg_worker_grow_with_graph(worker, &w);

        // prepare reference
        ref = new_reference(IR_ASN_REF_INSTRUCTION, worker->cur_blk->inst_last);
    }

    // execute
    cfg_worker_execute(ir, worker, IROP_RET, NULL, &ref, NULL);

    // cleanup
    delete_reference(ref);
}

/**
 * walk if statement
 *
 * if ---+--- Expression
 *       |
 *       +--- Statement
 *
 * node: JNT_STATEMENT_IF
*/
void __execute_statement_if(java_ir* ir, cfg_worker* worker, tree_node* stmt)
{
    cfg_worker* w = NULL;
    basic_block* test;
    basic_block* phi;

    // Expression
    stmt = stmt->first_child;

    // parse condition
    w = walk_expression(ir, stmt);
    cfg_worker_grow_with_graph(worker, &w);

    // mark block as a test block
    cfg_worker_execute(ir, worker, IROP_TEST, NULL, NULL, NULL);
    cfg_worker_next_outbound_strategy(worker, EDGE_TRUE);

    // mark test node
    test = cfg_worker_current_block(worker);

    // Statement (If Branch)
    stmt = stmt->next_sibling;

    // parse true branch
    if (stmt->type == JNT_STATEMENT_VAR_DECL)
    {
        ir_error(ir, JAVA_E_IF_LOCAL_VAR_DECL);
    }
    else
    {
        __execute_statement(ir, worker, stmt, true);
    }

    // place a phi node
    phi = cfg_worker_grow(worker);

    // Statement (Else Branch)
    stmt = stmt->next_sibling;

    // go back to test node and prepare for false branch
    cfg_worker_jump(worker, test, true, false);
    cfg_worker_next_outbound_strategy(worker, EDGE_FALSE);

    if (stmt)
    {
        // parse true branch
        if (stmt->type == JNT_STATEMENT_VAR_DECL)
        {
            ir_error(ir, JAVA_E_IF_LOCAL_VAR_DECL);
        }
        else
        {
            __execute_statement(ir, worker, stmt, true);
        }
    }

    // connect else branch
    cfg_worker_jump(worker, phi, true, true);
}

/**
 * TODO: Statemnt AST Walk
 *
 * node: JNT_BLOCK
*/
void __execute_statement(java_ir* ir, cfg_worker* worker, tree_node* stmt, bool execute_in_new_block)
{
    cfg_worker* w;

    if (worker->cur_blk && worker->cur_blk->type == BLOCK_EXIT)
    {
        /**
         * TODO: issue warning here
         *
         * because code behind a return will never execute
         * but... we probably need to issue only one warning
        */
    }

    if (execute_in_new_block)
    {
        switch (stmt->type)
        {
            case JNT_STATEMENT_SWITCH:
            case JNT_STATEMENT_IF:
            case JNT_STATEMENT_WHILE:
            case JNT_STATEMENT_FOR:
            case JNT_STATEMENT_CATCH:
            case JNT_BLOCK:
                /**
                 * things with a leading expression will
                 * start from new block by definition
                */
                break;
            default:
                cfg_worker_grow(worker);
                break;
        }
    }

    switch (stmt->type)
    {
        case JNT_STATEMENT_SWITCH:
            break;
        case JNT_STATEMENT_DO:
            break;
        case JNT_STATEMENT_BREAK:
            break;
        case JNT_STATEMENT_CONTINUE:
            break;
        case JNT_STATEMENT_RETURN:
            __execute_statement_return(ir, worker, stmt);
            break;
        case JNT_STATEMENT_SYNCHRONIZED:
            break;
        case JNT_STATEMENT_THROW:
            break;
        case JNT_STATEMENT_TRY:
            break;
        case JNT_STATEMENT_IF:
            __execute_statement_if(ir, worker, stmt);
            break;
        case JNT_STATEMENT_WHILE:
            break;
        case JNT_STATEMENT_FOR:
            break;
        case JNT_STATEMENT_LABEL:
            break;
        case JNT_STATEMENT_EXPRESSION:
            break;
        case JNT_STATEMENT_VAR_DECL:
            break;
        case JNT_STATEMENT_CATCH:
            break;
        case JNT_STATEMENT_FINALLY:
            break;
        case JNT_SWITCH_LABEL:
            break;
        case JNT_BLOCK:
            w = walk_block(ir, stmt, true);
            cfg_worker_grow_with_graph(worker, &w);
            break;
        default:
            break;
    }
}

/**
 * TODO: Block AST Walk
 *
 * node: JNT_BLOCK
*/
cfg_worker* walk_block(java_ir* ir, tree_node* block, bool use_new_scope)
{
    hash_table* scope = use_new_scope ? lookup_new_scope(ir, LST_NONE) : lookup_top_scope(ir);
    cfg_worker* worker = (cfg_worker*)malloc_assert(sizeof(cfg_worker));

    init_cfg_worker(worker);

    // every child is a statement
    block = block->first_child;
    while (block)
    {
        __execute_statement(ir, worker, block, false);
        block = block->next_sibling;
    }

    if (use_new_scope)
    {
        lookup_pop_scope(ir, true);
    }

    return worker;
}
