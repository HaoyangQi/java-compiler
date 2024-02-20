#include "ir.h"

/**
 * hierarchical name register routine
 *
 * if name has conflict, funtions ignores def_data_control
 * and keeps type_def as-is;
 * otherwise type_def will be set to NULL if data control is
 * DEF_DATA_MOVE
 *
 * name will be set to NULL if successful; stay as-is otherwise
 *
 * it returns the definition reference of the variable after
 * successful registration; NULL otherwise
 *
 * node: variable declarator
*/
definition* def(
    java_ir* ir,
    char** name,
    definition** type_def,
    size_t name_dims,
    def_use_control duc,
    java_error_id err_dup,
    java_error_id err_dim_amb,
    java_error_id err_dim_dup
)
{
    // test if declarator can be registered
    if (use(ir, *name, duc, JAVA_E_MAX) != NULL)
    {
        ir_error(ir, err_dup);
        return NULL;
    }

    hash_table* table = lookup_working_scope(ir);
    bool copy_def = duc & DU_CTL_DATA_COPY;
    definition* tdef = copy_def ? definition_copy(*type_def) : *type_def;

    // register
    shash_table_insert(table, *name, tdef);

    // we always move name, so simply detach
    *name = NULL;

    // only detach if the reference is moved successfully
    if (!copy_def)
    {
        *type_def = NULL;
    }

    /**
     * dimension check
     *
     * Java allows any of the array declaration form:
     * 1. Type[] Name;
     * 2. Type Name[];
     *
     * but not both, so if dimension matches, warning will be issued;
     * otherwise it is an error
    */
    if (name_dims > 0)
    {
        if (tdef->variable.type.dim != name_dims)
        {
            ir_error(ir, err_dim_amb);
        }
        else
        {
            ir_error(ir, err_dim_dup);
        }
    }

    return tdef;
}

/**
 * hierarchical name lookup routine
 *
 * NOTE: on-demand imports are not checked here, meaning definitions
 * are prioritized local definitions, and during linking, names
 * will be imported and check for ambiguity
 *
 * node: variable declarator
*/
definition* use(java_ir* ir, const char* name, def_use_control duc, java_error_id err_undef)
{
    scope_frame* cur = ir->scope_stack_top;
    hash_pair* p;

    // first we go through hierarchy
    while (cur)
    {
        p = shash_table_get(cur->table, name);

        if (p)
        {
            return p->value;
        }

        cur = cur->next;
    }

    // if nothing we try global
    if (duc & DU_CTL_LOOKUP_GLOBAL)
    {
        p = shash_table_get(&ir->tbl_global, name);
    }

    // error check
    if (!p)
    {
        ir_error(ir, err_undef);
        return NULL;
    }

    return p->value;
}

/**
 * generate definition for literal
 *
 * if token is not literal, funtion is no-op and NULL is returned
 *
 * TODO: other literals
 * for string literals, we probably need string->definition map
 * and definition reference back to the key string,
 * because we need definition object in instruction
*/
definition* def_li(java_ir* ir, java_token* token)
{
    definition* v = NULL;
    char* content;
    hash_pair* pair;
    binary_data bin;

    // lookup
    content = t2s(token);
    pair = shash_table_get(&ir->tbl_literal, content);

    // if key exists, we use; otherwise create
    if (pair)
    {
        free(content);
        return pair->value;
    }

    // otherwise we register
    switch (token->type)
    {
        case JLT_LTR_NUMBER:
            v = new_definition(JLT_LTR_NUMBER);
            v->li_number.type = t2p(ir, token, &bin);
            v->li_number.imm = bin.number;
            break;
        case JLT_LTR_CHARACTER:
            v = new_definition(JLT_LTR_CHARACTER);
            v->li_number.type = t2p(ir, token, &bin);
            v->li_number.imm = bin.number;
            break;
        case JLT_RWD_TRUE:
        case JLT_RWD_FALSE:
            v = new_definition(JLT_RWD_BOOLEAN);
            v->li_number.type = t2p(ir, token, &bin);
            v->li_number.imm = bin.number;
            break;
        case JLT_RWD_NULL:
            // NULL has no aux data
            v = new_definition(JLT_RWD_NULL);
            break;
        case JLT_LTR_STRING:
            v = new_definition(JLT_LTR_STRING);
            t2p(ir, token, &bin);
            v->li_string.stream = bin.stream;
            v->li_string.length = bin.len;
            v->li_string.wide_char = bin.wide_char;
            break;
        default:
            break;
    }

    // register: content cannot be freed if successful
    if (v)
    {
        shash_table_insert(&ir->tbl_literal, content, v);
    }
    else
    {
        free(content);
    }

    return v;
}

/**
 * interpret "type"
 *
 * it returns a definition regarding the type
 * modifier is optional, pass JLT_UNDEFINED
 * as "no modifier specified"
 *
 * node: JNT_TYPE
*/
definition* type2def(
    tree_node* node,
    java_node_query type,
    lbit_flag modifier,
    bool is_member
)
{
    definition* desc = new_definition(type);

    switch (type)
    {
        case JNT_VAR_DECL:
            desc->variable.is_class_member = is_member;
            desc->variable.modifier = modifier;
            desc->variable.type.primitive = node->data->declarator.id.simple;
            desc->variable.type.dim = node->data->declarator.dimension;

            // if not primitive type, then it must be a reference type
            if (desc->variable.type.primitive == JLT_MAX)
            {
                // type->class_type->unit
                desc->variable.type.reference = name_unit_concat(node->first_child->first_child, NULL);
            }
            break;
        case JNT_METHOD_DECL:
            desc->method.modifier = modifier;
            desc->method.return_type.primitive = node->data->declarator.id.simple;
            desc->method.return_type.dim = node->data->declarator.dimension;

            // if not primitive type, then it must be a reference type
            if (desc->method.return_type.primitive == JLT_MAX)
            {
                // type->class_type->unit
                desc->method.return_type.reference = name_unit_concat(node->first_child->first_child, NULL);
            }
            break;
        default:
            break;
    }

    return desc;
}

/**
 * register a variable in lookup table
 *
 * returns the definition data reference of the variable registered;
 * NULL otherwise
 *
 * JNT_VAR_DECL
 * |
 * +--- JNT_EXPRESSION
 *
 * node: JNT_VAR_DECL
*/
definition* def_var(java_ir* ir, tree_node* node, definition** type, def_use_control duc, bool is_member)
{
    char* name = t2s(node->data->declarator.id.complex);

    definition* data = def(
        ir, &name, type,
        node->data->declarator.dimension,
        duc | DU_CTL_LOOKUP_GLOBAL,
        is_member ? JAVA_E_MEMBER_VAR_DUPLICATE : JAVA_E_LOCAL_VAR_DUPLICATE,
        is_member ? JAVA_E_MEMBER_VAR_DIM_AMBIGUOUS : JAVA_E_LOCAL_VAR_DIM_AMBIGUOUS,
        is_member ? JAVA_E_MEMBER_VAR_DIM_DUPLICATE : JAVA_E_LOCAL_VAR_DIM_DUPLICATE
    );

    // if succeeds, name is NULL here so it is safe
    free(name);

    return data;
}

/**
 * register all declarators under one type in lookup table
 *
 * NOTE: this method will skip initializers
 *
 * JNT_TYPE
 * |
 * JNT_VAR_DECLARATORS
 * |
 * +--- JNT_VAR_DECL
 * |    |
 * |    +--- JNT_EXPRESSION
 * |
 * +--- JNT_VAR_DECL
 * |
 * +--- ...
 *
 * node: JNT_TYPE
*/
void def_vars(java_ir* ir, tree_node* node, lbit_flag modifier, bool is_member)
{
    definition* desc = type2def(node, JNT_VAR_DECL, modifier, is_member);

    // first JNT_VAR_DECL
    node = node->next_sibling->first_child;

    // register, every id has same type
    while (node)
    {
        // only move the definition for the last variable
        def_var(ir, node, &desc, node->next_sibling ? DU_CTL_DATA_COPY : DU_CTL_DEFAULT, is_member);
        node = node->next_sibling;
    }

    // cleanup
    definition_delete(desc);
}

/**
 * register a variable in lookup table
 *
 * node: JNT_FORMAL_PARAM_LIST
*/
void def_params(java_ir* ir, tree_node* node)
{
    if (!node)
    {
        return;
    }

    /**
     * JNT_FORMAL_PARAM_LIST
     * |
     * +--- JNT_FORMAL_PARAM
     * |    |
     * |    +--- Type
     * |
     * +--- JNT_FORMAL_PARAM
     * |    |
     * |    +--- Type
     * |
     * +--- ...
    */
    node = node->first_child;

    while (node)
    {
        definition* desc = type2def(node->first_child, JNT_VAR_DECL, JLT_UNDEFINED, false);
        char* name = t2s(node->data->declarator.id.complex);

        /**
         * 1. move name and desc
         * 2. do NOT lookup global scope: parameter name is scoped so it can co-exist
         *    with class member with same name
        */
        def(
            ir, &name, &desc,
            node->data->declarator.dimension,
            DU_CTL_DEFAULT,
            JAVA_E_PARAM_DUPLICATE,
            JAVA_E_PARAM_DIM_AMBIGUOUS,
            JAVA_E_PARAM_DUPLICATE
        );

        // cleanup
        free(name);
        definition_delete(desc);

        node = node->next_sibling;
    }
}

/**
 * register a method in lookup table
 *
 * TODO: method overloadiing
 * we need name mangling algorithm to encode parameter types into name string
 *
 * node: JNT_TYPE---JNT_METHOD_DECL
*/
void def_method(java_ir* ir, tree_node* node, lbit_flag modifier)
{
    definition* desc = type2def(
        node,
        JNT_METHOD_DECL,
        modifier,
        true
    );

    /**
     * type
     * |
     * method declaration
     * |
     * +--- header
     * |    |
     * |    +--- param list
     * |         |
     * |         +--- param 1
     * |         +--- ...
     * |
     * +--- body
    */
    node = node->next_sibling->first_child;

    // get name
    char* name = t2s(node->data->declarator.id.complex);

    // register
    def(
        ir, &name, &desc,
        node->data->declarator.dimension,
        DU_CTL_LOOKUP_GLOBAL,
        JAVA_E_METHOD_DUPLICATE,
        JAVA_E_METHOD_DIM_AMBIGUOUS,
        JAVA_E_METHOD_DIM_DUPLICATE
    );

    // cleanup
    free(name);
    definition_delete(desc);
}

/**
 * contextualize "class declaration"
 *
 * parse a class and all definitions within
 *
 * although it is scoped by class, we do not use specific scope on stack
 * because we flush top-level one by one and class/interface will serve
 * as global by default
 *
 * this is 1st-pass parsing logic, because use of top level members does
 * not obey "def before use" rule; and since all definitions (including
 * local variables) will be flushed into global scope, so it is not safe
 * to use a dummy definition first then fill when reaching the def
 *
 * node: JNT_TOP_LEVEL
*/
void def_class(java_ir* ir, tree_node* node)
{
    hash_table* table = lookup_global_scope(ir);
    definition* desc = NULL;
    tree_node* part = NULL;
    tree_node* probe = NULL;
    string_list sl;
    char* registered_name = NULL;

    /**
     * JNT_TOP_LEVEL
     * |
     * +--- JNT_CLASS_DECL      <--- HERE
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
     *           +--- JNT_CLASS_BODY_DECL
     *           |    |
     *           |    +--- Type
     *           |    |
     *           |    +--- JNT_VAR_DECLARATORS | JNT_METHOD_DECL
     *           |
     *           +--- JNT_CLASS_BODY_DECL
     *           |
     *           +--- ...
    */
    part = node->first_child;

    // get class name
    registered_name = t2s(part->data->id.complex);

    // definition data
    desc = new_definition(JNT_CLASS_DECL);
    desc->class.modifier = node->data->top_level_declaration.modifier;

    // [extends, implements, body]
    part = part->first_child;

    // extends, this name will be resolved later in linker
    if (part && part->type == JNT_CLASS_EXTENDS)
    {
        // extends->classtype->unit
        desc->class.extend = name_unit_concat(part->first_child->first_child, NULL);
        part = part->next_sibling;
    }

    // implements, names will be resolved later in linker
    if (part && part->type == JNT_CLASS_IMPLEMENTS)
    {
        // implements->list->interfacetype
        probe = part->first_child->first_child;

        // extract all names
        init_string_list(&sl);
        while (probe)
        {
            string_list_append(&sl, name_unit_concat(probe->first_child, NULL));
            probe = probe->next_sibling;
        }

        // concat all names
        desc->class.implement = string_list_concat(&sl, ",");
        release_string_list(&sl);

        part = part->next_sibling;
    }

    // class register & cleanup
    lookup_register(ir, table, &registered_name, &desc, JAVA_E_CLASS_NAME_DUPLICATE);
    free(registered_name);
    definition_delete(desc);

    // now we must have class body
    // otherwise it should not pass syntax parser

    // each part is a class body declaration
    part = part->first_child;

    // register definitions
    while (part)
    {
        // class body declaration -> [static|ctor|type]
        probe = part->first_child;

        if (probe->type == JNT_CTOR_DECL)
        {
            /**
             * TODO: constructor
            */
        }
        else if (probe->type == JNT_TYPE)
        {
            /**
             * Type as starter, it can be method/variable declarator
            */
            switch (probe->next_sibling->type)
            {
                case JNT_VAR_DECLARATORS:
                    def_vars(ir, probe, part->data->top_level_declaration.modifier, true);
                    break;
                case JNT_METHOD_DECL:
                    def_method(ir, probe, part->data->top_level_declaration.modifier);
                    break;
                default:
                    break;
            }
        }

        part = part->next_sibling;
    }
}
