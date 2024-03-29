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
static void __execute_instruction(
    java_ir* ir,
    cfg_worker* worker,
    tree_node* op,
    reference* operand_1,
    reference* operand_2
)
{
    operator_id id = op->data.operator->id;
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
            // need copy because use version will differ
            lvalue = copy_reference(operand_1);
            validate_lvalue = true;
            break;
        case OPID_PRE_INC:
        case OPID_PRE_DEC:
            /**
             * pre-inc/dec is simple
             *
             * do NOT convert op code here because we do not know
             * where we use this part
             *
             * e.g.
             * b = 1;
             * ++b + b;
             *
             * becomes IR code:
             * L0: b0 <- 1
             * L1: b1 <- b0 + 1
             * L2: b1 + b1
             *
             * for operand that references pre-inc/dec instruction,
             * simply reference the newest version of the def
            */
            lvalue = copy_reference(operand_2);
            operand_1 = new_reference(IR_ASN_REF_LITERAL, def_li_dec32(ir, "1"));
            validate_lvalue = true;
            break;
        case OPID_POST_INC:
        case OPID_POST_DEC:
            /**
             * post-inc/dec is a bit tricky
             *
             * do NOT convert op code here because we do not know
             * where we use this part
             *
             * e.g.
             * b = 1;
             * b++ + b;
             *
             * the answer is 3
             * "b++" yields 1, hence the init value;
             * but "b" is now 2 because it is after "b++"
             *
             * becomes IR code:
             * L0: b0 <- 1
             * L1: STORE b0
             * L2: b1 <- b0 + 1
             * L3: L1 + b1
             *
             * so, for operand that references the post-inc/dec instruction,
             * use the STORE value
             * if directly references to the def, just use the newest one,
             * which is the default behavior
             *
             * so op->data.operator->instruction references to STORE instruction
            */

            // first, prepare STORE instruction, and override instruction
            // attached to this operator
            lvalue = copy_reference(operand_2);
            op->data.operator->instruction = cfg_worker_execute(
                ir, worker, IROP_STORE, NULL, &lvalue, NULL);
            delete_reference(lvalue);

            // then, prepare what happens in fall-through
            lvalue = copy_reference(operand_2);
            operand_1 = new_reference(IR_ASN_REF_LITERAL, def_li_dec32(ir, "1"));
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
    // if there was an override occurred, do not override here
    if (op->data.operator->instruction)
    {
        cfg_worker_execute(ir, worker, expr_opid2irop(ir->expression, id), &lvalue, &operand_1, &operand_2);
    }
    else
    {
        op->data.operator->instruction = cfg_worker_execute(
            ir, worker, expr_opid2irop(ir->expression, id), &lvalue, &operand_1, &operand_2);
    }


    // cleanup
    delete_reference(lvalue);
    delete_reference(operand_1);
    delete_reference(operand_2);
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
 * BIG RECURSION AHEAD
 *
 * walk_expression and walk_operand are recursively correlated
 * to handle parenthesis
*/

/**
 * Operand Bound State
 *
 * It flags the context of next primary node
 *
 * OPERAND_WALK_FIRST:   first node
 * OPERAND_BOUND_LAST:   no node is acceptable
 * OPERAND_WALK_THIS:    next name should be from current top-level scope only
 * OPERAND_WALK_SUPER:   next name should be from parent's top-level scope only
 * OPERAND_BOUND_FIELD:  next is a member (field/method) access of previous one's scope
 * OPERAND_BOUND_METHOD: next name is a method reference of previous one's scope
*/
typedef enum
{
    OPERAND_BOUND_FIRST,
    OPERAND_BOUND_LAST,
    OPERAND_BOUND_THIS,
    OPERAND_BOUND_SUPER,
    OPERAND_BOUND_FIELD,
    OPERAND_BOUND_METHOD,
} operand_bound_state;

static void walk_expression(java_ir* ir, tree_node* expression);

/**
 * Walk An Operand
 *
 * TODO: we need a way to control current lookup scope for this function
 * so field access operator (.) can be interpreted as a sequence of
 * "load-field" instruction
 * basically: we need to control a global_top_level* reference to track
 * which scope we are looking at right now
 *
 * this is a state machine so that the context becomes the constraint on
 * top of syntax and interprets entire operand
 *
 * if failed or no-op, it returns NULL
 *
 * node: JNT_PRIMARY | JNT_OPERATOR
*/
static reference* walk_operand(java_ir* ir, tree_node* base)
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
        ref->doi = base->data.operator->instruction;

        return ref;
    }

    java_token* token;
    char* content;
    definition* __def = NULL;
    operand_bound_state bound_state = OPERAND_BOUND_FIRST;

    // locate first primary item
    base = base->first_child;

    /**
     * TODO: other primary types
     * TODO: validate primary sequence
    */
    while (base)
    {
        if (bound_state == OPERAND_BOUND_LAST)
        {
            /**
             * TODO: error and break and return NULL
            */
        }

        switch (base->type)
        {
            case JNT_PRIMARY_COMPLEX:
                token = base->data.id->complex;
                content = t2s(token);

                // try get literal definition
                // if token is not literal, funtion is no-op and NULL is returned
                __def = def_li(ir, &content, token->type, token->number.type, token->number.bits);

                /**
                 * TODO: interpret all token types
                 * TODO: this includes sequence of JT_IDENTIFIER
                 *       as field access
                */
                if (__def)
                {
                    ref->type = IR_ASN_REF_LITERAL;
                    ref->doi = __def;
                }
                else if (token->class == JT_IDENTIFIER)
                {
                    /**
                     * TODO: now we need to way to enforce lookup-from scope
                     * to be top-level
                     *
                     * TODO: the use scope here needs to be the scope of previous
                     * field access point, if ther is none, it has to be current
                     * top-level scope (only)
                    */
                    __def = use(ir, content, DU_CTL_LOOKUP_TOP_LEVEL, JAVA_E_REF_UNDEFINED);

                    ref->type = IR_ASN_REF_DEFINITION;
                    ref->doi = __def;
                    ref->ver = __def ? __def->def_count : 0;

                    /**
                     * TODO: how to handle field access?
                    */
                }

                free(content);
                break;
            case JNT_PRIMARY_SIMPLE:
                /**
                 * TODO:
                 * JLT_SYM_METHOD_REFERENCE
                 * JLT_RWD_TRUE
                 * JLT_RWD_FALSE
                 * JLT_RWD_NULL
                 * JLT_RWD_THIS
                 * JLT_RWD_SUPER
                 *
                 * this and super MUST start at
                 * the beginning, and cannot
                 * repeat
                 *
                 * "this" directs next ID lookup
                 * scope to be top-level, instead of
                 * current local scope
                 * "super" is parent class
                */
                switch (base->data.id->simple)
                {
                    case JLT_RWD_TRUE:
                        def_li_raw(ir, "true", JLT_RWD_TRUE, JT_NUM_MAX, JT_NUM_BIT_LENGTH_NORMAL);
                        break;
                    case JLT_RWD_FALSE:
                        def_li_raw(ir, "false", JLT_RWD_FALSE, JT_NUM_MAX, JT_NUM_BIT_LENGTH_NORMAL);
                        break;
                    case JLT_RWD_NULL:
                        def_li_raw(ir, "null", JLT_RWD_NULL, JT_NUM_MAX, JT_NUM_BIT_LENGTH_NORMAL);
                        break;
                    case JLT_RWD_THIS:
                        if (bound_state != OPERAND_BOUND_FIRST)
                        {
                            /**
                             * TODO: error and return
                            */
                        }
                        /**
                         * TODO: this cannot repeat
                        */
                        bound_state = OPERAND_BOUND_THIS;
                        break;
                    case JLT_RWD_SUPER:
                        if (bound_state != OPERAND_BOUND_FIRST)
                        {
                            /**
                             * TODO: error and return
                            */
                        }
                        /**
                         * TODO: validate if there is any extends, if not then it is an error
                         *
                         * this cannot repeate
                        */
                        break;
                    case JLT_SYM_METHOD_REFERENCE:
                        /**
                         * TODO: validate: must be static
                        */
                        break;
                    default:
                        break;
                }
                break;
            case JNT_PRIMARY_CREATION:
                /**
                 * TODO: Object creation ("new")
                */
                break;
            case JNT_PRIMARY_CLS_LITERAL:
                /**
                 * TODO: Class literal
                */
                bound_state = OPERAND_BOUND_LAST;
                break;
            case JNT_PRIMARY_METHOD_INVOKE:
                /**
                 * TODO: Method Invocation
                 *
                 * previous walked node must be ID
                 *
                 * must walk expression list and generate name
                 * like how the method name is registered in
                 * lookup table
                 *
                 * for parameters, needs new instruction: PUSH
                */
                break;
            case JNT_PRIMARY_ARR_ACCESS:
                /**
                 * TODO: Array access
                 * previous walked node must be ID
                */
                break;
            case JNT_TYPE:
                /**
                 * TODO: Type Case
                */
                break;
            case JNT_EXPRESSION:
                /**
                 * TODO: parenthesized expression
                */
                walk_expression(ir, base);
                break;
            case JNT_AMBIGUOUS:
                /**
                 * TODO: ambiguous JNT_TYPE and JNT_EXPRESSION
                 *
                 * now we need to resolve this by using context information
                */
                break;
            default:
                /**
                 * TODO: error
                */
                // if reached here, must be an error
                break;
        }

        base = base->next_sibling;
    }

    return ref;
}

/**
 * Expression AST Walk
 *
 * This method generate a single block of code without any logical expansion
 *
 * node: JNT_EXPRESSION
*/
static void walk_expression(java_ir* ir, tree_node* expression)
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
    operator_id opid;
    bool needs_logical_expansion = false;

    if (!top->next_sibling)
    {
        // minimum case: constant expression (only one operand)
        reference* constant = walk_operand(ir, top);
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
        opid = top->data.operator->id;

        // base1 must be available
        if (!base1)
        {
            ir_error(ir, JAVA_E_EXPRESSION_NO_OPERAND);
            break;
        }

        // adjust base2 if needed
        if (expr_opid_operand_count(ir->expression, opid) == 2)
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
        __execute_instruction(
            ir, TSW(ir), top,
            walk_operand(ir, base2),
            walk_operand(ir, base1)
        );

        switch (opid)
        {
            case OPID_LOGIC_AND:
            case OPID_LOGIC_OR:
            case OPID_TERNARY_1:
            case OPID_TERNARY_2:
                needs_logical_expansion = true;
                break;
            default:
                break;
        }

        // reduction of current operator completed, move on
        top = top->next_sibling;
    }

    /**
     * post-processing: transform pre-/post- inc/dec op code
     *
     * code itself is transformed in __execute_instruction, this routine
     * simply changes opcode
    */
    for (instruction* probe = first ? first : TSW(ir)->cur_blk->inst_first;
        probe != NULL;
        probe = probe->next)
    {
        switch (probe->op)
        {
            case IROP_AINC:
            case IROP_BINC:
                probe->op = IROP_ADD;
                break;
            case IROP_ADEC:
            case IROP_BDEC:
                probe->op = IROP_SUB;
                break;
            default:
                break;
        }
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
 * walk expression list statement
 *
 * JNT_EXPRESSION_LIST
 * |
 * +--- JNT_EXPRESSION
 * |
 * +--- JNT_EXPRESSION
 * |
 * +--- ...
 *
 * node: JNT_EXPRESSION_LIST
*/
static void __execute_expression_list(java_ir* ir, tree_node* stmt)
{
    stmt = stmt->first_child;

    while (stmt)
    {
        /**
         * since iniit code runs only once, so
         * it stays out side of loop body, thus
         * it can stay in current block instead
         * of a new one
        */
        walk_expression(ir, stmt);
        stmt = stmt->next_sibling;
    }
}

/**
 * walk variable declaration statement
 *
 * JNT_LOCAL_VAR_DECL
 * |
 * +--- JNT_TYPE
 * |
 * +--- JNT_VAR_DECLARATORS
 *      |
 *      +--- JNT_VAR_DECL
 *      |    |
 *      |    +--- JNT_EXPRESSION
 *      |
 *      +--- JNT_VAR_DECL
 *      |
 *      ...
 *
 * node: JNT_LOCAL_VAR_DECL
*/
static void __execute_variable_declaration(java_ir* ir, tree_node* stmt)
{
    // JNT_TYPE
    stmt = stmt->first_child;

    // get type definition
    definition* type = type2def(stmt, DEFINITION_VARIABLE, JLT_UNDEFINED, false);
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
            cfg_worker_next_asn_strategy(TSW(ir), true);
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

static void __execute_statement(java_ir* ir, tree_node* stmt);
static cfg_worker* walk_block(java_ir* ir, tree_node* block, bool use_new_scope);

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
static void __execute_statement_if(java_ir* ir, tree_node* stmt)
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
 *
 * node: JNT_STATEMENT_VAR_DECL
*/
static void __execute_statement_variable_declaration(java_ir* ir, tree_node* stmt)
{
    __execute_variable_declaration(ir, stmt->first_child);
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
static void __execute_statement_while(java_ir* ir, tree_node* stmt)
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
    sc->_test = cfg_worker_current_block(TSW(ir));

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
    cfg_worker_jump(TSW(ir), sc->_test, true, false);
    cfg_worker_next_outbound_strategy(TSW(ir), EDGE_TRUE);

    // while body can be empty
    if (stmt)
    {
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
    }
    else
    {
        // leave body empty
        cfg_worker_grow(TSW(ir));
    }

    // add edge to loop back to continue block
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
static void __execute_statement_do(java_ir* ir, tree_node* stmt)
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

    // connect end of expression to break point then stop there
    cfg_worker_next_outbound_strategy(TSW(ir), EDGE_FALSE);
    cfg_worker_jump(TSW(ir), sc->_break, true, true);

    // cleanup
    pop_statement_context(ir);
}

/**
 * walk for statement
 *
 * JNT_STATEMENT_FOR
 * |
 * +--- [JNT_FOR_INIT]
 * |    |
 * |    +--- JNT_LOCAL_VAR_DECL|JNT_EXPRESSION_LIST
 * |
 * +--- [JNT_EXPRESSION]
 * |
 * +--- [JNT_FOR_UPDATE]
 * |    |
 * |    +--- JNT_EXPRESSION_LIST
 * |
 * +--- statement (body)
 *
 * node: JNT_STATEMENT_FOR
*/
static void __execute_statement_for(java_ir* ir, tree_node* stmt)
{
    statement_context* sc = push_statement_context(ir, SCQ_LOOP);
    basic_block* test_expr_start; // loop-back node (NOT continue point!)

    // we need a scope for inits
    lookup_new_scope(ir);

    // not sure what it is yet
    stmt = stmt->first_child;

    // for init
    if (stmt->type == JNT_FOR_INIT)
    {
        switch (stmt->first_child->type)
        {
            case JNT_EXPRESSION_LIST:
                __execute_expression_list(ir, stmt->first_child);
                break;
            case JNT_LOCAL_VAR_DECL:
                __execute_variable_declaration(ir, stmt->first_child);
                break;
            default:
                break;
        }

        stmt = stmt->next_sibling;
    }

    // enforce new block, which is condition block
    __start_statement_in_new_block(ir, JNT_STATEMENT_FOR);
    test_expr_start = cfg_worker_current_block(TSW(ir));

    // for condition
    if (stmt->type == JNT_EXPRESSION)
    {
        walk_expression(ir, stmt);
        stmt = stmt->next_sibling;
    }

    // mark, also makes sure that this node is not empty
    // so that body can stays isolated
    cfg_worker_execute(ir, TSW(ir), IROP_TEST, NULL, NULL, NULL);

    // must get this after condition because 
    // expression may be expanded
    sc->_test = cfg_worker_current_block(TSW(ir));

    /**
     * continue block: which is for update
     *
     * this is continue point, and where we loop back
     *
     * HACK: do not attach to graph, so body can grow properly
    */
    sc->_continue = cfg_new_basic_block(TSW(ir)->graph);

    // break block
    cfg_worker_next_outbound_strategy(TSW(ir), EDGE_FALSE);
    sc->_break = cfg_worker_grow(TSW(ir));

    // for update
    cfg_worker_jump(TSW(ir), sc->_continue, true, false);
    if (stmt->type == JNT_FOR_UPDATE)
    {
        __execute_expression_list(ir, stmt->first_child);
        stmt = stmt->next_sibling;
    }

    // go back to test node and branch into loop body
    cfg_worker_jump(TSW(ir), sc->_test, true, false);
    cfg_worker_next_outbound_strategy(TSW(ir), EDGE_TRUE);

    // body can be empty in for loop
    if (stmt)
    {
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
    }

    // add edge from body to update
    cfg_worker_jump(TSW(ir), sc->_continue, true, true);

    // loop back: which is NOT back to TEST node
    // but back to the start of the test expression
    cfg_worker_next_outbound_strategy(TSW(ir), EDGE_JUMP);
    cfg_worker_jump(TSW(ir), test_expr_start, false, true);

    // cleanup
    cfg_worker_jump(TSW(ir), sc->_break, true, false);
    pop_statement_context(ir);
    lookup_pop_scope(ir, &TSW(ir)->variables);
}

/**
 * walk return statement
 *
 * return ---+--- Expression
 *
 * node: JNT_STATEMENT_RETURN
*/
static void __execute_statement_return(java_ir* ir, tree_node* stmt)
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
 * node->data.id->complex->class = JT_IDENTIFIER will contain the optional ID
 *
 * node: JNT_STATEMENT_BREAK
*/
static void __execute_statement_break(java_ir* ir, tree_node* stmt)
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

    if (stmt->data.id->complex->class == JT_IDENTIFIER)
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
 * node->data.id->complex->class = JT_IDENTIFIER will contain the optional ID
 *
 * node: JNT_STATEMENT_BREAK
*/
static void __execute_statement_continue(java_ir* ir, tree_node* stmt)
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

    if (stmt->data.id->complex->class == JT_IDENTIFIER)
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
static void __execute_statement(java_ir* ir, tree_node* stmt)
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
            __execute_statement_for(ir, stmt);
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
 * Block AST Walk
 *
 * merge_pool: if set, the pool will be merged to new stack top
 *
 * node: JNT_BLOCK
*/
static cfg_worker* walk_block(java_ir* ir, tree_node* block, bool use_new_scope)
{
    // prepare scope lookup
    if (use_new_scope)
    {
        lookup_new_scope(ir);
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

    /**
     * make sure block is never empty
     *
     * this condition is required by statement with branches:
     * if, while, for, do
    */
    if (cfg_empty(TSW(ir)->graph))
    {
        cfg_worker_grow(TSW(ir));
    }

    if (use_new_scope)
    {
        lookup_pop_scope(ir, &TSW(ir)->variables);
    }

    return pop_scope_worker(ir);
}

/**
 * Constructor Code Walk
 *
 * It looks similar to walk_method, but the implementation
 * is separated for better readability and capability for
 * future development
 *
 * node: JNT_METHOD_DECL
*/
static void walk_constructor(java_ir* ir, definition* ctor_def)
{
    cfg_worker* worker;

    /**
     * JNT_CTOR_DECL                      <--- HERE
     * |
     * +--- JNT_FORMAL_PARAM_LIST         <--- root_code_walk
     * |    |
     * |    +--- param 1
     * |    +--- ...
     * |
     * +--- JNT_CTOR_BODY
    */
    tree_node* node = ctor_def->root_code_walk->first_child;

    // begin scope
    lookup_new_scope(ir);

    // fill all parameter declarations
    if (node->type == JNT_FORMAL_PARAM_LIST)
    {
        def_params(ir, node, ctor_def->method->parameters);
        node = node->next_sibling;
    }

    // parse body (use current scope)
    worker = walk_block(ir, node, false);

    // we need to keep all definitions active
    lookup_pop_scope(ir, &worker->variables);

    // convert CFG to SSA form
    cfg_worker_ssa_build(worker);

    // release worker
    release_cfg_worker(worker, &ctor_def->method->code, &ctor_def->method->local_variables);
    free(worker);
}

/**
 * Field Member Initializer Code Walk
 *
 * An initialized worker must be provided to contain the code
 * because all initializers must be maintained within same CFG
 *
 * node: JNT_EXPRESSION | JNT_ARRAY_INIT | NULL
*/
static void walk_field(java_ir* ir, definition* field_def, cfg_worker* field_init_worker)
{
    tree_node* declaration = field_def->root_code_walk;
    cfg_worker* worker;
    reference* lvalue;
    reference* operand;

    // create variable data chunk ref
    lvalue = new_reference(IR_ASN_REF_DEFINITION, field_def);

    if (declaration)
    {
        switch (declaration->type)
        {
            case JNT_EXPRESSION:
                // parse right side
                push_scope_worker(ir);
                walk_expression(ir, declaration);

                // prepare assignment code
                worker = get_scope_worker(ir);
                operand = new_reference(IR_ASN_REF_INSTRUCTION, worker->cur_blk->inst_last);

                // add assignment code
                cfg_worker_next_asn_strategy(TSW(ir), true);
                cfg_worker_execute(ir, worker, IROP_ASN, &lvalue, &operand, NULL);

                // cleanup
                delete_reference(operand);

                // merge code
                worker = pop_scope_worker(ir);
                cfg_worker_grow_with_graph(field_init_worker, worker);
                break;
            case JNT_ARRAY_INIT:
                /**
                 * TODO: array (of expression) init code
                */
                break;
            default:
                break;
        }
    }
    else
    {
        /**
         * otherwise we insert a dummy code, indicate that
         * the variable is defined here and some initialization required
        */
        cfg_worker_execute(ir, field_init_worker, IROP_INIT, &lvalue, NULL, NULL);
    }

    // cleanup
    delete_reference(lvalue);
}

/**
 * Method Code Walk
 *
 * node: JNT_METHOD_DECL
*/
static void walk_method(java_ir* ir, definition* method_def)
{
    cfg_worker* worker;

    /**
     * go to the header
     *
     * JNT_METHOD_DECL                   <--- root_code_walk
     * |
     * +--- JNT_METHOD_HEADER            <--- HERE
     * |    |
     * |    +--- JNT_FORMAL_PARAM_LIST
     * |         |
     * |         +--- param 1
     * |         +--- ...
     * |
     * +--- JNT_METHOD_BODY
    */
    tree_node* node = method_def->root_code_walk->first_child;

    // begin scope
    lookup_new_scope(ir);

    // fill all parameter declarations
    def_params(ir, node->first_child, method_def->method->parameters);

    // parse body (use current scope)
    worker = walk_block(ir, node->next_sibling, false);

    // we need to keep all definitions active
    lookup_pop_scope(ir, &worker->variables);

    // convert CFG to SSA form
    cfg_worker_ssa_build(worker);

    // release worker
    release_cfg_worker(worker, &method_def->method->code, &method_def->method->local_variables);
    free(worker);
}

/**
 * walk "class declaration"
 *
 * since first pass is already performed in def_global,
 * so in here we can know the tree of this top-level
 * from global_top_level
 *
 * node: JNT_TOP_LEVEL
*/
void walk_class(java_ir* ir, global_top_level* class)
{
    // if ill-formed, no-op
    if (!class) { return; }

    tree_node* part = NULL;
    tree_node* declaration = NULL;
    definition* desc = NULL;
    hash_pair* p;
    cfg_worker member_init_worker;

    init_cfg_worker(&member_init_worker);
    lookup_top_level_begin(ir, class);

    /**
     * Walk field initializer and method body
     *
     * fields and methods are already defined
     * so no need to walk trees from top-level
     * for them, simply walk the table
    */
    for (size_t i = 0; i < class->tbl_member.bucket_size; i++)
    {
        p = class->tbl_member.bucket[i];

        while (p)
        {
            desc = p->value;

            switch (desc->type)
            {
                case DEFINITION_VARIABLE:
                    walk_field(ir, desc, &member_init_worker);
                    break;
                case DEFINITION_METHOD:
                    if (desc->method->is_constructor)
                    {
                        walk_constructor(ir, desc);
                    }
                    else
                    {
                        walk_method(ir, desc);
                    }
                    break;
                default:
                    break;
            }

            p = p->next;
        }
    }

    /**
     * static initializer does not have a proper definition,
     * so walk here from tree level
     *
     * JNT_TOP_LEVEL
     * |
     * +--- JNT_CLASS_DECL
     *      |
     *      +--- JNT_CLASS_EXTENDS
     *      |    |
     *      |    +--- Type
     *      |
     *      +--- JNT_CLASS_IMPLEMENTS
     *      |    |
     *      |    +--- JNT_INTERFACE_TYPE_LIST
     *      |         |
     *      |         +--- Type
     *      |         |
     *      |         +--- ...
     *      |
     *      +--- JNT_CLASS_BODY
     *           |
     *           +--- JNT_CLASS_BODY_DECL         <--- node_first_body_decl
     *           |    |
     *           |    +--- Type | JNT_STATIC_INIT | JNT_CTOR_DECL
     *           |    |
     *           |    +--- JNT_VAR_DECLARATORS | JNT_METHOD_DECL
     *           |
     *           +--- JNT_CLASS_BODY_DECL
     *           |
     *           +--- ...
    */
    part = class->node_first_body_decl;

    // code generation for static initializer
    while (part)
    {
        /**
         * TODO: IR code generation for:
         * 1. static initializer
        */

        // reach content
        declaration = part->first_child;

        if (declaration->type == JNT_STATIC_INIT)
        {
            /**
             * TODO: static initializer
            */
        }

        part = part->next_sibling;
    }

    // cleanup

    if (cfg_empty(member_init_worker.graph))
    {
        release_cfg_worker(&member_init_worker, NULL, NULL);
    }
    else
    {
        // no init, just need a memory chunk here
        class->code_member_init = new_cfg_container();
        release_cfg_worker(&member_init_worker, class->code_member_init, &class->member_init_variables);
    }

    lookup_top_level_end(ir);
}

/**
 * TODO:contextualize "interface declaration"
*/
void walk_interface(java_ir* ir, global_top_level* interface)
{
    // if ill-formed, no-op
    if (!interface) { return; }

    lookup_top_level_begin(ir, interface);

    /**
     * TODO:
    */

    lookup_top_level_end(ir);
}
