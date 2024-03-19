#include "ir.h"
#include "jil.h"

/**
 * hierarchical name register routine
 *
 * NOTE: import scope and global scope will be enforced
 *       during lookup because they are global names
 *       that are reserved across entire compilation unit
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
    if (use(ir, *name, duc, JAVA_E_MAX) || shash_table_get(&ir->tbl_import, *name))
    {
        ir_error(ir, err_dup);
        return NULL;
    }
    else
    {
        /**
         * global condition is a bit tricky due to constructor
         *
         * method name can have same name as the class name that
         * is currently scoping. In this case this method is a
         * constructor
        */
        hash_pair* __p = shash_table_get(&ir->tbl_global, *name);

        if (__p && !(
            __p->value == ir->working_top_level &&
            ir->working_top_level->type == TOP_LEVEL_CLASS &&
            (duc & DU_CTL_METHOD_NAME)))
        {
            ir_error(ir, err_dup);
            return NULL;
        }
    }

    hash_table* table = lookup_working_scope(ir);
    bool copy_def = duc & DU_CTL_DATA_COPY;
    definition* tdef = NULL;

    // only assign when parameter is not NULL
    if (type_def)
    {
        tdef = copy_def ? definition_copy(*type_def) : *type_def;
    }

    // register
    shash_table_insert(table, *name, tdef);

    // we always move name, so simply detach
    *name = NULL;

    // only detach if the reference is moved successfully
    if (!copy_def && type_def)
    {
        *type_def = NULL;
    }

    /**
     * mark sid if it is a member
     *
     * this information is needed because the order of member variable declaration needs to be
     * preserved for object size calculation with struct padding
    */
    if (tdef && tdef->variable.is_class_member)
    {
        if (!ir->working_top_level || lookup_top_level_scope(ir) != table)
        {
            fprintf(stderr, "TODO ERROR: internal error: member variable detected inside local scope.\n");
        }

        tdef->sid = ir->working_top_level->num_member_variable;
        ir->working_top_level->num_member_variable++;
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
 * it uses a name, and when it is used, it means lookup in global
 * and import scope is unecessary, because they do not consist
 * valid name to "use"
 *
 * NOTE: on-demand imports are not checked here, meaning definitions
 * are prioritized as local definitions, and during linking, names
 * will be imported and check for ambiguity
*/
definition* use(java_ir* ir, const char* name, def_use_control duc, java_error_id err_undef)
{
    scope_frame* cur = ir->scope_stack_top;
    hash_table* top_level = lookup_top_level_scope(ir);
    hash_pair* p;

    // nothing to look for
    if (!cur && !top_level)
    {
        return NULL;
    }

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

    // if nothing we try top level
    if (duc & DU_CTL_LOOKUP_TOP_LEVEL)
    {
        p = shash_table_get(top_level, name);
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
 * if registration is not successful, content stays as-is;
 * otherwise it is detached and set to NULL
 *
*/
definition* def_li(
    java_ir* ir,
    char** content,
    java_lexeme_type token_type,
    java_number_type num_type,
    java_number_bit_length num_bits
)
{
    if (!content) { return NULL; }

    hash_table* literals = lookup_top_level_literal_scope(ir);
    definition* v = NULL;
    hash_pair* pair;
    binary_data bin;

    if (!literals)
    {
        fprintf(stderr, "TODO ERROR: internal error: literal detected outside of top level scope.\n");
    }

    // lookup
    pair = shash_table_get(literals, *content);

    // if key exists, we use; otherwise create
    if (pair)
    {
        return pair->value;
    }

    // otherwise we register
    switch (token_type)
    {
        case JLT_LTR_NUMBER:
            v = new_definition(DEFINITION_NUMBER);
            v->li_number.type = r2p(ir, *content, &bin, token_type, num_type, num_bits);
            v->li_number.imm = bin.number;
            break;
        case JLT_LTR_CHARACTER:
            v = new_definition(DEFINITION_CHARACTER);
            v->li_number.type = r2p(ir, *content, &bin, token_type, num_type, num_bits);
            v->li_number.imm = bin.number;
            break;
        case JLT_RWD_TRUE:
        case JLT_RWD_FALSE:
            v = new_definition(DEFINITION_BOOLEAN);
            v->li_number.type = r2p(ir, *content, &bin, token_type, num_type, num_bits);
            v->li_number.imm = bin.number;
            break;
        case JLT_RWD_NULL:
            // NULL has no aux data
            v = new_definition(DEFINITION_NULL);
            break;
        case JLT_LTR_STRING:
            v = new_definition(DEFINITION_STRING);
            r2p(ir, *content, &bin, token_type, num_type, num_bits);
            v->li_string.stream = bin.stream;
            v->li_string.length = bin.len;
            v->li_string.wide_char = bin.wide_char;
            break;
        default:
            break;
    }

    // register: content will be moved if successful
    if (v)
    {
        shash_table_insert(literals, *content, v);
        *content = NULL;
    }

    return v;
}

/**
 * generate definition for literal from raw data
 *
*/
definition* def_li_raw(
    java_ir* ir,
    const char* raw,
    java_lexeme_type token_type,
    java_number_type num_type,
    java_number_bit_length num_bits
)
{
    size_t sz_raw = sizeof(char*) * (strlen(raw) + 1);
    char* content = (char*)malloc_assert(sz_raw);
    definition* def;

    memcpy(content, raw, sz_raw);
    def = def_li(ir, &content, token_type, num_type, num_bits);
    free(content);

    return def;
}

/**
 * interpret "type"
 *
 * TODO: resolve type and import meta
 *
 * it returns a definition regarding the type
 * modifier is optional, pass JLT_UNDEFINED
 * as "no modifier specified"
 *
 * node: JNT_TYPE
*/
definition* type2def(
    tree_node* node,
    definition_type type,
    lbit_flag modifier,
    bool is_member
)
{
    definition* desc = new_definition(type);

    switch (type)
    {
        case DEFINITION_VARIABLE:
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
        case DEFINITION_METHOD:
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
 * get a string that identifies param names
 * it returns NULL if parameter list is empty or undefined
 *
 * node: JNT_FORMAL_PARAM_LIST | NULL
*/
static char* get_param_list_type_name(java_ir* ir, tree_node* node, size_t* counter)
{
    /**
     * type check provides robustness especially to contructor's AST
     *
     * contructor does not have header guard to guarantee first child
     * must be JNT_FORMAL_PARAM_LIST | NULL
     *
     * if list not detected, then no parameter, so counter = 0
    */
    if (!node || node->type != JNT_FORMAL_PARAM_LIST)
    {
        if (counter) { *counter = 0; }
        return NULL;
    }

    char* result;
    char c;
    size_t param_count = 0;
    string_list sl;

    init_string_list(&sl);

    /**
     * JNT_FORMAL_PARAM_LIST
     * |
     * +--- JNT_FORMAL_PARAM    <--- HERE
     * |    |
     * |    +--- Type
     * |         |
     * |         +--- Name
     * |              |
     * |              +--- Unit
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
        // parse dimensions
        for (size_t i = node->first_child->data->declarator.dimension; i > 0; i--)
        {
            string_list_append_char(&sl, JIL_TYPE_ARRAY_DIM);
        }

        // parse primitive type
        c = primitive_type_to_jil_type(node->first_child->data->declarator.id.simple);

        if (c)
        {
            string_list_append_char(&sl, c);
        }
        else
        {
            // parse reference type name
            string_list_append_char(&sl, JIL_TYPE_OBJECT);
            string_list_append(&sl, name_unit_concat(node->first_child->first_child->first_child, NULL), false);
            string_list_append_char(&sl, ';');
        }

        param_count++;
        node = node->next_sibling;
    }

    // prepare result
    result = string_list_concat(&sl, NULL);
    if (counter) { *counter = param_count; }

    // cleanup
    release_string_list(&sl);
    return result;
}

/**
 * Get mangled method name
 *
 * it is a string representation of a name includes method name and
 * all parameter code name concatenated in order
 *
 * this name uniquely defines a method reference
 *
 * an optional counter param_count is used for parameter count
 *
 * node: JNT_METHOD_HEADER | JNT_CTOR_DECL
*/
static char* get_full_method_name(java_ir* ir, tree_node* node, size_t* param_count)
{
    /**
     * JNT_METHOD_HEADER | JNT_CTOR_DECL    <--- HERE
     * |
     * +--- JNT_FORMAL_PARAM_LIST
     *      |
     *      +--- param 1
     *      +--- ...
     *
     * one thing to note here is that JNT_CTOR_DECL's
     * first child is either JNT_FORMAL_PARAM_LIST or
     * JNT_CTOR_BODY, unlike method which will have
     * JNT_METHOD_HEADER as a guard to make sure its
     * child is always JNT_FORMAL_PARAM_LIST.
     *
     * get_param_list_type_name will cover this case
     * for constructor, so no further adjustment
     * needs to be done here
    */
    char* name = t2s(node->data->declarator.id.complex);
    char* param_name = get_param_list_type_name(ir, node->first_child, param_count);
    size_t len_n, len_p;

    if (param_name)
    {
        len_n = strlen(name);
        len_p = strlen(param_name);
        name = (char*)realloc_assert(name, sizeof(char) * (len_n + len_p + 1));

        strcpy(name + len_n, param_name);
        free(param_name);

        name[len_n + len_p] = '\0';
    }

    return name;
}

/**
 * register a constructor in lookup table
 *
 * It looks similar to def_method, but the implementation
 * is separated for better readability and capability for
 * future development
 *
 * node: JNT_CTOR_DECL
*/
static void def_constructor(java_ir* ir, tree_node* node, lbit_flag modifier)
{
    definition* desc = new_definition(DEFINITION_METHOD);
    definition* method;
    char* name;
    size_t param_count;

    /**
     * Get Name
     *
     * JNT_CTOR_DECL                      <--- HERE
     * |
     * +--- JNT_FORMAL_PARAM_LIST
     * |    |
     * |    +--- param 1
     * |    +--- ...
     * |
     * +--- JNT_CTOR_BODY
    */
    name = get_full_method_name(ir, node, &param_count);

    // register
    method = def(
        ir, &name, &desc, 0,
        DU_CTL_LOOKUP_TOP_LEVEL | DU_CTL_METHOD_NAME,
        JAVA_E_METHOD_DUPLICATE,
        JAVA_E_METHOD_DIM_AMBIGUOUS,
        JAVA_E_METHOD_DIM_DUPLICATE
    );

    if (method)
    {
        method->method.is_constructor = true;
        method->method.modifier = modifier;
        method->root_code_walk = node;
        method->method.parameter_count = param_count;
        method->method.parameters = (definition**)malloc_assert(sizeof(definition*) * param_count);
    }

    // cleanup
    free(name);
    definition_delete(desc);
}

/**
 * register a variable in lookup table
 *
 * returns the definition data reference of the variable registered;
 * NULL otherwise
 *
 * node: JNT_VAR_DECL
*/
definition* def_var(java_ir* ir, tree_node* node, definition** type, def_use_control duc, bool is_member)
{
    /**
     * JNT_VAR_DECL          <--- HERE
     * |
     * +--- JNT_EXPRESSION   <--- root_code_walk if is_member=true
    */
    char* name = t2s(node->data->declarator.id.complex);

    definition* data = def(
        ir, &name, type,
        node->data->declarator.dimension,
        duc | DU_CTL_LOOKUP_TOP_LEVEL,
        is_member ? JAVA_E_MEMBER_VAR_DUPLICATE : JAVA_E_LOCAL_VAR_DUPLICATE,
        is_member ? JAVA_E_MEMBER_VAR_DIM_AMBIGUOUS : JAVA_E_LOCAL_VAR_DIM_AMBIGUOUS,
        is_member ? JAVA_E_MEMBER_VAR_DIM_DUPLICATE : JAVA_E_LOCAL_VAR_DIM_DUPLICATE
    );

    /**
     * initializer code walk for members are not immediately processed,
     * so the code tree is logged to speed up the re-access
    */
    if (is_member)
    {
        data->root_code_walk = node->first_child;
    }

    // if succeeds, name is NULL here so it is safe
    free(name);

    return data;
}

/**
 * register all declarators under one type in lookup table
 *
 * node: JNT_TYPE
*/
void def_vars(java_ir* ir, tree_node* node, lbit_flag modifier, bool is_member)
{
    definition* desc = type2def(node, DEFINITION_VARIABLE, modifier, is_member);

    /**
     * JNT_TYPE
     * |
     * JNT_VAR_DECLARATORS
     * |
     * +--- JNT_VAR_DECL          <--- HERE
     * |    |
     * |    +--- JNT_EXPRESSION
     * |
     * +--- JNT_VAR_DECL
     * |
     * +--- ...
    */
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
 * An optional container ordered_list is provided to maintain
 * a ordered parameter definition reference list
 *
 * ordered_list must have same length as children count of
 * JNT_FORMAL_PARAM_LIST
 *
 * node: JNT_FORMAL_PARAM_LIST
*/
void def_params(java_ir* ir, tree_node* node, definition** ordered_list)
{
    if (!node)
    {
        return;
    }

    definition* desc;
    definition* param;
    char* name;
    size_t idx = 0;

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
        desc = type2def(node->first_child, DEFINITION_VARIABLE, JLT_UNDEFINED, false);
        name = t2s(node->data->declarator.id.complex);

        /**
         * 1. move name and desc
         * 2. do NOT lookup global scope: parameter name is scoped so it can co-exist
         *    with class member with same name
        */
        param = def(
            ir, &name, &desc,
            node->data->declarator.dimension,
            DU_CTL_DEFAULT,
            JAVA_E_PARAM_DUPLICATE,
            JAVA_E_PARAM_DIM_AMBIGUOUS,
            JAVA_E_PARAM_DUPLICATE
        );

        // maintain order if applicable
        if (ordered_list)
        {
            ordered_list[idx] = param;
        }

        // cleanup
        free(name);
        definition_delete(desc);

        node = node->next_sibling;
        idx++;
    }
}

/**
 * register a method in lookup table
 *
 * node: JNT_TYPE
*/
static void def_method(java_ir* ir, tree_node* node, lbit_flag modifier)
{
    definition* desc = type2def(node, DEFINITION_METHOD, modifier, true);
    definition* method;
    tree_node* node_method_decl = node->next_sibling;
    char* name;
    size_t param_count;

    /**
     * JNT_TYPE
     * |
     * JNT_METHOD_DECL                    <--- root_code_walk
     * |
     * +--- JNT_METHOD_HEADER             <--- HERE
     * |    |
     * |    +--- JNT_FORMAL_PARAM_LIST
     * |         |
     * |         +--- param 1
     * |         +--- ...
     * |
     * +--- JNT_METHOD_BODY
    */
    node = node_method_decl->first_child;

    // get name
    name = get_full_method_name(ir, node, &param_count);

    // register
    method = def(
        ir, &name, &desc,
        node->data->declarator.dimension,
        DU_CTL_LOOKUP_TOP_LEVEL | DU_CTL_METHOD_NAME,
        JAVA_E_METHOD_DUPLICATE,
        JAVA_E_METHOD_DIM_AMBIGUOUS,
        JAVA_E_METHOD_DIM_DUPLICATE
    );

    if (method)
    {
        /**
         * body code walk for method is not immediately processed,
         * so the code tree is logged to speed up the re-access
         *
         * but... marking JNT_METHOD_BODY is incorrect because
         * parameters needs to be re-walks under scope of the
         * method, so the method walk root needs to be the root
         * of entire method: JNT_METHOD_DECL
        */
        method->root_code_walk = node_method_decl;
        method->method.parameter_count = param_count;
        method->method.parameters = (definition**)malloc_assert(sizeof(definition*) * param_count);
    }

    // cleanup
    free(name);
    definition_delete(desc);
}

/**
 * contextualize "import"
 *
 * node: JNT_IMPORT_DECL
*/
static void def_import(java_ir* ir, tree_node* node)
{
    global_import* desc;
    hash_pair* pair;
    tree_node* name = node->first_child;
    tree_node* last_unit = NULL;
    char* registered_name = NULL;
    char* pkg_name = NULL;

    /**
     * JNT_IMPORT_DECL
     * |
     * +--- Name
     *      |
     *      +--- Unit
     *      |
     *      +--- Unit
     *      |
     *      +--- ...
    */
    if (!node->data->import.on_demand)
    {
        // last name unit is the import target
        last_unit = name->last_child;
        registered_name = t2s(last_unit->data->id.complex);
    }

    // construct package name list
    pkg_name = name_unit_concat(name->first_child, last_unit);

    // register the class name if applicable
    if (registered_name)
    {
        desc = new_global_import();
        desc->package_name = pkg_name;
    }
    else
    {
        desc = NULL;
        registered_name = pkg_name;
    }

    // name resolution must be unique
    if (!lookup_register(ir, &ir->tbl_import, &registered_name, &desc, JAVA_E_MAX))
    {
        pair = shash_table_get(&ir->tbl_import, registered_name);

        /**
         * duplicate only happen when both are:
         * 1. on-demand, OR
         * 2. regular import
         *
         * AND
         *
         * the package name matches
        */
        if (((desc == NULL) == (pair->value == NULL)) &&
            strcmp(pair->value ? ((global_import*)pair->value)->package_name : pair->key, pkg_name) == 0)
        {
            ir_error(ir, JAVA_E_IMPORT_DUPLICATE);
        }
        else
        {
            ir_error(ir, JAVA_E_IMPORT_AMBIGUOUS);
        }
    }

    // cleanup: no need to free pkg_name as it was already moved
    free(registered_name);
    delete_global_import(desc);
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
static void def_class(java_ir* ir, tree_node* node)
{
    /**
     * JNT_TOP_LEVEL
     * |
     * +--- JNT_CLASS_DECL                       <--- HERE
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
     *           +--- JNT_CLASS_BODY_DECL        <--- node_first_body_decl
     *           |    |
     *           |    +--- Type
     *           |    |
     *           |    +--- JNT_VAR_DECLARATORS | JNT_METHOD_DECL
     *           |
     *           +--- JNT_CLASS_BODY_DECL
     *           |
     *           +--- ...
    */
    tree_node* part = node->first_child;
    tree_node* probe;

    global_top_level* desc = new_global_top_level(TOP_LEVEL_CLASS);
    char* registered_name = t2s(part->data->id.complex);
    string_list sl;

    // definition data
    desc->modifier = node->data->top_level_declaration.modifier;

    // [extends, implements, body]
    part = part->first_child;

    // extends, this name will be resolved later in linker
    if (part && part->type == JNT_CLASS_EXTENDS)
    {
        // extends->classtype->unit
        desc->extend = name_unit_concat(part->first_child->first_child, NULL);
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
            string_list_append(&sl, name_unit_concat(probe->first_child, NULL), false);
            probe = probe->next_sibling;
        }

        // concat all names
        desc->implement = string_list_to_string_array(&sl);
        desc->num_implement = sl.count;
        release_string_list(&sl);

        part = part->next_sibling;
    }

    // now we must have class body
    // otherwise it should not pass syntax parser

    // log first body declaration
    desc->node_first_body_decl = part->first_child;

    // setup lookup hierarchy
    lookup_top_level_begin(ir, desc);

    // class register with import name conflict check
    if (shash_table_get(&ir->tbl_import, registered_name) ||
        !lookup_register(ir, lookup_global_scope(ir), &registered_name, &desc, JAVA_E_MAX))
    {
        ir_error(ir, JAVA_E_CLASS_NAME_DUPLICATE);

        free(registered_name);
        delete_global_top_level(desc);
        lookup_top_level_end(ir);

        // no need to continue as a conflict is found
        return;
    }

    ////////// desc is NULL after this line //////////

    // each part is a class body declaration
    part = part->first_child;

    // register definitions
    while (part)
    {
        // class body declaration -> [static|ctor|type]
        probe = part->first_child;

        if (probe->type == JNT_CTOR_DECL)
        {
            def_constructor(ir, probe, part->data->top_level_declaration.modifier);
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

    // cleanup
    lookup_top_level_end(ir);
}

/**
 * TODO:contextualize "interface declaration"
 *
 * parse an interface and all definitions within
 *
 * node: JNT_TOP_LEVEL
*/
static void def_interface(java_ir* ir, tree_node* node)
{
    global_top_level* desc = new_global_top_level(TOP_LEVEL_INTERFACE);

    /**
     * TODO: locate first body decl and log it
    */
    // desc->node_first_body_decl = part;

    // setup lookup hierarchy
    lookup_top_level_begin(ir, desc);

    /**
     * TODO:
    */

    // cleanup
    lookup_top_level_end(ir);
}

/**
 * Register all global definition
 *
 * node: JNT_UNIT
*/
void def_global(java_ir* ir, tree_node* compilation_unit)
{
    /**
     * JNT_UNIT
     * |
     * +--- [JNT_PKG_DECL]
     * |
     * +--- {JNT_IMPORT_DECL}
     * |
     * +--- {JNT_CLASS_DECL|JNT_INTERFACE_DECL}
    */
    tree_node* node = compilation_unit->first_child;

    // package
    if (node && node->type == JNT_PKG_DECL)
    {
        /**
         * TODO: handle pkg decl
        */
        node = node->next_sibling;
    }

    // imports
    while (node && node->type == JNT_IMPORT_DECL)
    {
        def_import(ir, node);
        node = node->next_sibling;
    }

    // top-levels
    while (node && node->type == JNT_TOP_LEVEL)
    {
        // handle top level
        if (node->first_child->type == JNT_CLASS_DECL)
        {
            def_class(ir, node);
        }
        else
        {
            def_interface(ir, node);
        }

        node = node->next_sibling;
    }
}
