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
        }
        else if (token->class == JT_IDENTIFIER)
        {
            ref->type = IR_ASN_REF_DEFINITION;
            content = t2s(token);
            ref->doi = use(ir, content, DU_CTL_LOOKUP_GLOBAL, JAVA_E_REF_UNDEFINED);
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
 * it returns if the code requires logical precedence expansion
 *
 * NOTE: lvalue reference must be copied from an operand!
 * because the version number will be different
 *
 * op: JNT_OPERATOR
*/
static bool __execute_instruction(
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
    bool needs_logical_expansion = false;

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
        case OPID_LOGIC_AND:
        case OPID_LOGIC_OR:
        case OPID_TERNARY_1:
        case OPID_TERNARY_2:
            needs_logical_expansion = true;
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

    return needs_logical_expansion;
}

/**
 * grow worker to guarantee that next statement always
 * starts in a new block
 *
 * NOTE: use this when:
 * 1. a parser enforces in new block at the beginning, OR
 * 2. not sure what the upcoming statement is
 *
 * NOTE: keep this method up-to-date based on algorithm of
 *       corresponding statement parser
*/
static void __start_statement_in_new_block(java_ir* ir, java_node_query stmt_type)
{
    cfg_worker* w = get_scope_worker(ir);

    switch (stmt_type)
    {
        case JNT_BLOCK:
            /**
             * block merges worker graph
             * so by definition a block always enforces in new block
            */
            break;
        default:
            // only grow when current block is not empty or empty graph
            if (!cfg_worker_current_block_empty(w) || cfg_empty(w->graph))
            {
                cfg_worker_grow(w);
            }
            break;
    }
}

/**
 * Expression AST Walk
 *
 * This method generate a single block of code without any logical expansion
 *
 * It returns the flag that if the code needs logical precedence expansion
 *
 * node: JNT_EXPRESSION
*/
void walk_expression(java_ir* ir, tree_node* expression)
{
    // if start is OP or end with non-OP, expression is invalid
    if (!expression->first_child || expression->first_child->type == JNT_OPERATOR)
    {
        ir_error(ir, JAVA_E_EXPRESSION_NO_OPERAND);
        return;
    }

    // stack control
    tree_node* base1;
    tree_node* base2;
    tree_node* top = expression->first_child;
    instruction* first = TSW(ir)->cur_blk ? TSW(ir)->cur_blk->inst_last : NULL;
    bool needs_logical_expansion = false;

    if (!top->next_sibling)
    {
        // minimum case: constant expression (only one operand)
        reference* constant = __interpret_operand(ir, top);
        cfg_worker_execute(ir, TSW(ir), IROP_STORE, NULL, &constant, NULL);
        delete_reference(constant);

        return;
    }
    else if (expression->last_child->type != JNT_OPERATOR)
    {
        // if end with non-OP, expression is invalid
        ir_error(ir, JAVA_E_EXPRESSION_NO_OPERATOR);
        return;
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

        /**
         * Code Generation
         *
         * order matters here!
         * 1. operand order
         * 2. flag OR order: __execute_instruction needs to stay at
         * left to make sure it always executes, otherwise the
         * short circuit might block it
        */
        needs_logical_expansion = __execute_instruction(
            ir, TSW(ir), top,
            __interpret_operand(ir, base2),
            __interpret_operand(ir, base1)
        ) || needs_logical_expansion;

        // reduction of current operator completed, move on
        top = top->next_sibling;
    }

    // logical precedence
    if (needs_logical_expansion)
    {
        /**
         * if the block needs logical precedence expansion,
         * start expression code in new block
        */
        if (first)
        {
            cfg_worker_current_block_split(ir, TSW(ir), first, EDGE_ANY, false);
        }

        cfg_worker_expand_logical_precedence(ir, TSW(ir));
    }
}

/**
 * Recursive Statement Walk
 *
 * WARNING: big recursion ahead!
 *
 * NOTE: to optimize stack usage,
 * 1. do NOT add parameter unless there is a valid reason to do so
 *    otherwise, just use linked-list on heap to simulate data passed
 *    on stack
 * 2. allocate local variables wisely: always write all local variable
 *    at the beginning for readability
 *
 * scope worker usage
 * walk_block will start a new scope worker, and all statement will use it
 * to pass it around, a stack on heap is managed to get current worker
 * but, do not invoke the getter call, simply use the macro:
 *
 * TSW(ir)
 *
 * and with this no parameter is required to access current scope worker
 *
*/

void __execute_statement(java_ir* ir, tree_node* stmt);
cfg_worker* walk_block(java_ir* ir, tree_node* block, bool use_new_scope);

/**
 * walk if statement
 *
 * if ---+--- Expression
 * |     |
 * |     +--- Statement
 * |
 * else ---if ---+--- Expression
 *         |     |
 *         |     +--- Statement
 *         |
 *         else--- Statement
 *
 * node: JNT_STATEMENT_IF
*/
void __execute_statement_if(java_ir* ir, tree_node* stmt)
{
    basic_block* test;
    basic_block* phi;

    // Expression
    stmt = stmt->first_child;

    // parse condition
    walk_expression(ir, stmt);

    // mark block as a test block
    cfg_worker_execute(ir, TSW(ir), IROP_TEST, NULL, NULL, NULL);

    // mark test node
    test = cfg_worker_current_block(TSW(ir));

    // Statement (TRUE Branch)
    stmt = stmt->next_sibling;

    // branch into TRUE branch
    cfg_worker_next_outbound_strategy(TSW(ir), EDGE_TRUE);
    __start_statement_in_new_block(ir, stmt->type);

    // parse true branch
    if (stmt->type == JNT_STATEMENT_VAR_DECL)
    {
        ir_error(ir, JAVA_E_IF_LOCAL_VAR_DECL);
    }
    else
    {
        __execute_statement(ir, stmt);
    }

    // place a phi node
    phi = cfg_worker_grow(TSW(ir));

    // Statement (FALSE Branch, optional)
    stmt = stmt->next_sibling;

    // go back to test node and prepare for false branch
    cfg_worker_jump(TSW(ir), test, true, false);
    cfg_worker_next_outbound_strategy(TSW(ir), EDGE_FALSE);

    if (stmt)
    {
        __start_statement_in_new_block(ir, stmt->type);

        // parse true branch
        if (stmt->type == JNT_STATEMENT_VAR_DECL)
        {
            ir_error(ir, JAVA_E_IF_LOCAL_VAR_DECL);
        }
        else
        {
            __execute_statement(ir, stmt);
        }
    }

    // connect else branch
    cfg_worker_jump(TSW(ir), phi, true, true);
}

/**
 * walk variable declaration statement
 *
 * JNT_STATEMENT_VAR_DECL
 * |
 * +--- JNT_LOCAL_VAR_DECL
 *      |
 *      +--- JNT_TYPE
 *      |
 *      +--- JNT_VAR_DECLARATORS
 *           |
 *           +--- JNT_VAR_DECL
 *           |    |
 *           |    +--- JNT_EXPRESSION
 *           |
 *           +--- JNT_VAR_DECL
 *           |
 *           ...
 *
 * node: JNT_STATEMENT_VAR_DECL
*/
void __execute_statement_variable_declaration(java_ir* ir, tree_node* stmt)
{
    // JNT_TYPE
    stmt = stmt->first_child->first_child;

    // get type definition
    definition* type = type2def(stmt, JNT_VAR_DECL, JLT_UNDEFINED, false);
    definition* var;
    reference* lvalue;
    reference* operand;

    // register from first JNT_VAR_DECL, every id has same type
    for (stmt = stmt->next_sibling->first_child; stmt != NULL; stmt = stmt->next_sibling)
    {
        // only move the definition for the last variable
        var = def_var(ir, stmt, &type, stmt->next_sibling ? DU_CTL_DATA_COPY : DU_CTL_DEFAULT, false);

        // only generate code for successful registration
        if (!var) { continue; }

        // create variable data chunk reference
        lvalue = new_reference(IR_ASN_REF_DEFINITION, var);

        if (stmt->first_child)
        {
            // if there is an initializer, parse it
            walk_expression(ir, stmt->first_child);

            // assignment code
            operand = new_reference(IR_ASN_REF_INSTRUCTION, TSW(ir)->cur_blk->inst_last);
            cfg_worker_execute(ir, TSW(ir), IROP_ASN, &lvalue, &operand, NULL);

            // cleanup
            delete_reference(operand);
        }
        else
        {
            /**
             * otherwise we insert a dummy code, indicate that
             * the variable is defined here and some initialization required
            */
            cfg_worker_execute(ir, TSW(ir), IROP_INIT, &lvalue, NULL, NULL);
        }

        // cleanup
        delete_reference(lvalue);
    }

    // cleanup
    definition_delete(type);
}

/**
 * walk while statement
 *
 * JNT_STATEMENT_WHILE
 * |
 * +--- JNT_EXPRESSION
 * |
 * +--- statement
 *
 * node: JNT_STATEMENT_WHILE
*/
void __execute_statement_while(java_ir* ir, tree_node* stmt)
{
    statement_context* sc = push_statement_context(ir, SCQ_LOOP);

    /**
     * since condition block needs to be revisited
     * during iteration, so it must stay in its own block
     *
     * do not use cfg_worker_grow here: we do not know if
     * current node is empty
    */
    __start_statement_in_new_block(ir, JNT_STATEMENT_WHILE);

    // mark continue point: which is the start of expression
    sc->_continue = cfg_worker_current_block(TSW(ir));

    // parse condition
    stmt = stmt->first_child;
    walk_expression(ir, stmt);

    // mark test block
    cfg_worker_execute(ir, TSW(ir), IROP_TEST, NULL, NULL, NULL);

    /**
     * prepare break point first
     *
     * WARNING: order matters here!
     * we must prepare loop break-out node first to finalize
     * the statement context before we go into the loop
     * body, which is the recursion, and other statements
     * within the loop body need this context to work properly
     *
     * mark break point: which is the false branch block;
     * worker will also stop at this empty block for
     * future parsing
    */
    cfg_worker_next_outbound_strategy(TSW(ir), EDGE_FALSE);
    sc->_break = cfg_worker_grow(TSW(ir));

    // Statement (loop body)
    stmt = stmt->next_sibling;

    // go back to test node and branch into loop body
    cfg_worker_jump(TSW(ir), sc->_continue, true, false);
    cfg_worker_next_outbound_strategy(TSW(ir), EDGE_TRUE);
    __start_statement_in_new_block(ir, stmt->type);

    // parse loop body
    if (stmt->type == JNT_STATEMENT_VAR_DECL)
    {
        ir_error(ir, JAVA_E_WHILE_LOCAL_VAR_DECL);
    }
    else
    {
        __execute_statement(ir, stmt);
    }

    // add edge to loop back to test block
    cfg_worker_next_outbound_strategy(TSW(ir), EDGE_JUMP);
    cfg_worker_jump(TSW(ir), sc->_continue, false, true);

    // cleanup: need to stop at break point for future parsing
    cfg_worker_jump(TSW(ir), sc->_break, true, false);
    pop_statement_context(ir);
}

/**
 * walk do-while statement
 *
 * JNT_STATEMENT_DO
 * |
 * +--- statement
 * |
 * +--- JNT_EXPRESSION
 *
 * node: JNT_STATEMENT_DO
*/
void __execute_statement_do(java_ir* ir, tree_node* stmt)
{
    statement_context* sc = push_statement_context(ir, SCQ_LOOP);
    basic_block* body;

    /**
     * do not use cfg_worker_grow here: we do not know if
     * current node is empty
     *
     * NOTE: do NOT use body type stmt->first_child->type
     * to determine whether we need a new node. In fact:
     * we need new node no matter what; because otherwise
     * it is impossible for us to know which one is the
     * entry node of the statement if it is a Block. As
     * a compromise, we always start from new node, leaving
     * it empty when the body statement is Block statement
    */
    __start_statement_in_new_block(ir, JNT_STATEMENT_DO);

    // first block is body
    body = cfg_worker_current_block(TSW(ir));

    /**
     * mark continue point: which is the start of expression
     *
     * HACK: do not attach to graph, so body can grow properly
    */
    sc->_continue = cfg_new_basic_block(TSW(ir)->graph);

    /**
     * break node
     *
     * HACK: do not attach to graph, so body can grow properly
    */
    // cfg_worker_next_outbound_strategy(TSW(ir), EDGE_FALSE);
    sc->_break = cfg_new_basic_block(TSW(ir)->graph);

    // Statement (loop body)
    stmt = stmt->first_child;
    cfg_worker_jump(TSW(ir), body, true, false);

    // parse loop body
    if (stmt->type == JNT_STATEMENT_VAR_DECL)
    {
        ir_error(ir, JAVA_E_WHILE_LOCAL_VAR_DECL);
    }
    else
    {
        __execute_statement(ir, stmt);
    }

    // connect end of statement to continue
    cfg_worker_next_outbound_strategy(TSW(ir), EDGE_ANY);
    cfg_worker_jump(TSW(ir), sc->_continue, true, true);

    // loop edge
    cfg_worker_next_outbound_strategy(TSW(ir), EDGE_TRUE);
    cfg_worker_jump(TSW(ir), body, false, true);

    // parse condition
    walk_expression(ir, stmt->next_sibling);
    cfg_worker_execute(ir, TSW(ir), IROP_TEST, NULL, NULL, NULL);

    // connect end of expression to break point
    cfg_worker_next_outbound_strategy(TSW(ir), EDGE_FALSE);
    cfg_worker_jump(TSW(ir), sc->_break, false, true);

    // cleanup
    pop_statement_context(ir);
}

/**
 * walk return statement
 *
 * return ---+--- Expression
 *
 * node: JNT_STATEMENT_RETURN
*/
void __execute_statement_return(java_ir* ir, tree_node* stmt)
{
    reference* ref = NULL;

    // Expression
    stmt = stmt->first_child;

    if (stmt)
    {
        // parse return value
        walk_expression(ir, stmt);

        // prepare reference
        ref = new_reference(IR_ASN_REF_INSTRUCTION, TSW(ir)->cur_blk->inst_last);
    }

    // execute
    cfg_worker_execute(ir, TSW(ir), IROP_RET, NULL, &ref, NULL);

    // cleanup
    delete_reference(ref);

    /**
     * here we do NOT grow
     *
     * because: a valid code structure will make sure this statement
     * is the last one in current scope
     *
     * otherwise those statements are not valid anyway
    */
}

/**
 * walk break statement
 *
 * node->data->id.complex->class = JT_IDENTIFIER will contain the optional ID
 *
 * node: JNT_STATEMENT_BREAK
*/
void __execute_statement_break(java_ir* ir, tree_node* stmt)
{
    // break is bounded by switch and loop
    statement_context* sc = get_statement_context(ir, SCQ_LOOP | SCQ_SWITCH);

    // the statement must be bounded
    if (!sc)
    {
        // early return to avoid grow
        ir_error(ir, JAVA_E_BREAK_UNBOUND);
        return;
    }

    if (stmt->data->id.complex->class == JT_IDENTIFIER)
    {
        /**
         * TODO: branch to label (additional lookup)
         *
         * early return on failure
        */
    }
    else
    {
        // execute
        cfg_worker_execute(ir, TSW(ir), IROP_JMP, NULL, NULL, NULL);

        /**
         * here sc->_break must be valid
         * so no check here
        */
        cfg_worker_next_outbound_strategy(TSW(ir), EDGE_JUMP);
        cfg_worker_jump(TSW(ir), sc->_break, false, true);
    }

    // mark node (since IROP_JMP cannot distinguish break/continue)
    cfg_worker_set_current_block_type(TSW(ir), BLOCK_BREAK);

    /**
     * here we do NOT grow
     *
     * because: a valid code structure will make sure this statement
     * is the last one in current scope
     *
     * otherwise those statements are not valid anyway
    */
}

/**
 * walk continue statement
 *
 * node->data->id.complex->class = JT_IDENTIFIER will contain the optional ID
 *
 * node: JNT_STATEMENT_BREAK
*/
void __execute_statement_continue(java_ir* ir, tree_node* stmt)
{
    // break is bounded by loop
    statement_context* sc = get_statement_context(ir, SCQ_LOOP);

    // the statement must be bounded
    if (!sc)
    {
        // early return to avoid grow
        ir_error(ir, JAVA_E_CONTINUE_UNBOUND);
        return;
    }

    if (stmt->data->id.complex->class == JT_IDENTIFIER)
    {
        /**
         * TODO: branch to label (additional lookup)
         *
         * early return on failure
        */
    }
    else
    {
        // execute
        cfg_worker_execute(ir, TSW(ir), IROP_JMP, NULL, NULL, NULL);

        /**
         * here sc->_continue must be valid
         * so no check here
        */
        cfg_worker_next_outbound_strategy(TSW(ir), EDGE_JUMP);
        cfg_worker_jump(TSW(ir), sc->_continue, false, true);
    }

    // mark node (since IROP_JMP cannot distinguish break/continue)
    cfg_worker_set_current_block_type(TSW(ir), BLOCK_CONTINUE);

    /**
     * here we do NOT grow
     *
     * because: a valid code structure will make sure this statement
     * is the last one in current scope
     *
     * otherwise those statements are not valid anyway
    */
}

/**
 * TODO: Statemnt Parser Dispatch
 *
 * node: any statement (including JNT_BLOCK)
*/
void __execute_statement(java_ir* ir, tree_node* stmt)
{
    if (TSW(ir)->cur_blk)
    {
        switch (TSW(ir)->cur_blk->type)
        {
            case BLOCK_RETURN:
            case BLOCK_BREAK:
            case BLOCK_CONTINUE:
                /**
                 * TODO: issue warning here
                 *
                 * because code behind a return will never execute
                 * but... we probably need to issue only one warning
                */
                fprintf(stderr, "TODO ir error: warning: statement will never execute.\n");
                break;
            default:
                break;
        }
    }

    switch (stmt->type)
    {
        case JNT_STATEMENT_SWITCH:
            break;
        case JNT_STATEMENT_DO:
            __execute_statement_do(ir, stmt);
            break;
        case JNT_STATEMENT_BREAK:
            __execute_statement_break(ir, stmt);
            break;
        case JNT_STATEMENT_CONTINUE:
            __execute_statement_continue(ir, stmt);
            break;
        case JNT_STATEMENT_RETURN:
            __execute_statement_return(ir, stmt);
            break;
        case JNT_STATEMENT_SYNCHRONIZED:
            break;
        case JNT_STATEMENT_THROW:
            break;
        case JNT_STATEMENT_TRY:
            break;
        case JNT_STATEMENT_IF:
            __execute_statement_if(ir, stmt);
            break;
        case JNT_STATEMENT_WHILE:
            __execute_statement_while(ir, stmt);
            break;
        case JNT_STATEMENT_FOR:
            break;
        case JNT_STATEMENT_LABEL:
            break;
        case JNT_STATEMENT_EXPRESSION:
            walk_expression(ir, stmt->first_child);
            break;
        case JNT_STATEMENT_VAR_DECL:
            __execute_statement_variable_declaration(ir, stmt);
            break;
        case JNT_STATEMENT_CATCH:
            break;
        case JNT_STATEMENT_FINALLY:
            break;
        case JNT_SWITCH_LABEL:
            break;
        case JNT_BLOCK:
            // still need this because we do not know
            // who dispatched this
            cfg_worker_grow_with_graph(TSW(ir), walk_block(ir, stmt, true));
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
    // prepare scope lookup
    if (use_new_scope)
    {
        lookup_new_scope(ir, LST_NONE);
    }
    else
    {
        lookup_top_scope(ir);
    }

    // prepare scope worker
    push_scope_worker(ir);

    // every child is a statement
    block = block->first_child;
    while (block)
    {
        /**
         * optimization:
         *
         * if it is a block statement, just recursively call itself
         * so walk_block and __execute_statement will not alternate
         * on call stack
        */
        if (block->type == JNT_BLOCK)
        {
            cfg_worker_grow_with_graph(TSW(ir), walk_block(ir, block, true));
        }
        else
        {
            __execute_statement(ir, block);
        }

        block = block->next_sibling;
    }

    if (use_new_scope)
    {
        lookup_pop_scope(ir, true);
    }

    return pop_scope_worker(ir);
}
