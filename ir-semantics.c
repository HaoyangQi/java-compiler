/**
 * IR Front-End: Semantic Analysis & CFG generation
 *
*/

#include "ir.h"

/* reserved constant context */

// reserved entry point name
static const char* reserved_method_name_entry_point = "main";
// reserved default architecture
static const architecture default_arch = { .bits = ARCH_64_BIT };

static void ctx_import(java_ir* ir, tree_node* node);
static void ctx_class(java_ir* ir, tree_node* node);
static void ctx_interface(java_ir* ir, tree_node* node);

/**
 * Context Analysis Entry Point
 *
 * It will generate a SSA form of AST
*/
void contextualize(java_ir* ir, architecture* arch, tree_node* compilation_unit)
{
    // register architecture of current target
    ir->arch = arch ? arch : &default_arch;

    // compilation unit does not have siblings
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
        ctx_import(ir, node);
        node = node->next_sibling;
    }

    // top-levels
    while (node && node->type == JNT_TOP_LEVEL)
    {
        // handle top level
        if (node->first_child->type == JNT_CLASS_DECL)
        {
            ctx_class(ir, node);
        }
        else
        {
            ctx_interface(ir, node);
        }

        node = node->next_sibling;
    }

    /**
     * TODO:
     *
     * resolve all external types
    */
}

/**
 * name unit concatenation routine
*/
static char* __name_unit_concat(tree_node* from, tree_node* stop_before)
{
    string_list sl;
    char* s;

    // construct package name list
    init_string_list(&sl);
    while (from != stop_before)
    {
        string_list_append(&sl, t2s(from->data->id.complex));
        from = from->next_sibling;
    }

    // now we concat the package name
    s = string_list_concat(&sl, ".");
    release_string_list(&sl);

    return s;
}

/**
 * interpret "type"
 *
 * it returns a definition regarding the type
 * modifier is optional, pass JLT_UNDEFINED
 *
 * node: JNT_TYPE
*/
static definition* type2def(
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
                desc->variable.type.reference = __name_unit_concat(node->first_child->first_child, NULL);
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
                desc->method.return_type.reference = __name_unit_concat(node->first_child->first_child, NULL);
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
 * node: JNT_TYPE---JNT_VAR_DECLARATORS
*/
static void def_var(java_ir* ir, tree_node* node, lbit_flag modifier, bool is_member)
{
    definition* desc = type2def(node, JNT_VAR_DECL, modifier, is_member);

    /**
     * type
     * |
     * variable declarators
     * |
     * +--- declarator 1
     * |    |
     * |    +--- expr
     * |
     * +--- declarator 2
     * |    |
     * |    +--- expr
     * |
     * +--- ...
    */
    node = node->next_sibling->first_child;

    // register, every id has same type
    while (node)
    {
        char* name = t2s(node->data->declarator.id.complex);

        def(
            ir, &name, &desc,
            node->data->declarator.dimension,
            DU_CTL_DATA_COPY | DU_CTL_LOOKUP_GLOBAL,
            is_member ? JAVA_E_MEMBER_VAR_DUPLICATE : JAVA_E_LOCAL_VAR_DUPLICATE,
            is_member ? JAVA_E_MEMBER_VAR_DIM_AMBIGUOUS : JAVA_E_LOCAL_VAR_DIM_AMBIGUOUS,
            is_member ? JAVA_E_MEMBER_VAR_DIM_DUPLICATE : JAVA_E_LOCAL_VAR_DIM_DUPLICATE
        );

        // if succeeds, name is NULL here so it is safe
        free(name);

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
static void def_params(java_ir* ir, tree_node* node)
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
static void def_method(java_ir* ir, tree_node* node, lbit_flag modifier)
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
 * contextualize "import"
 *
 * import goes into global scope
 * on-demand import goes into separate global table
*/
static void ctx_import(java_ir* ir, tree_node* node)
{
    definition* desc = NULL;
    hash_table* table = NULL;
    // JNT_IMPORT_DECL -> Name -> Unit
    tree_node* name = node->first_child;
    tree_node* last_unit = NULL;
    char* registered_name = NULL;
    char* pkg_name = NULL;

    if (!node->data->import.on_demand)
    {
        // last name unit is the import target
        last_unit = name->last_child;
        registered_name = t2s(last_unit->data->id.complex);
    }

    // construct package name list
    pkg_name = __name_unit_concat(name->first_child, last_unit);

    // register the class name if applicable
    if (registered_name)
    {
        table = lookup_global_scope(ir);
        desc = new_definition(JNT_IMPORT_DECL);
        desc->import.package_name = pkg_name;

        // name resolution must be unique
        if (!lookup_register(ir, table, registered_name, desc, JAVA_E_MAX))
        {
            if (strcmp(pkg_name, HT_STR2DEF(table, registered_name)->import.package_name) == 0)
            {
                ir_error(ir, JAVA_E_IMPORT_DUPLICATE);
            }
            else
            {
                ir_error(ir, JAVA_E_IMPORT_AMBIGUOUS);
            }
        }
    }
    else
    {
        table = &ir->tbl_on_demand_packages;

        /**
         * on-demand import in ir is not needed to link to specifc symbol
         * unless there is an ambiguity in AST that require this info to
         * resolve
         *
         * so the table simply logs the package name so we can use it
         * whenever necessary in ir, and in linker
        */
        lookup_register(ir, table, pkg_name, NULL, JAVA_E_IMPORT_DUPLICATE);
    }
}

/**
 * contextualize "class declaration"
 *
 * although it is scoped by class, we do not use specific scope on stack
 * because we flush top-level one by one and class/interface will serve
 * as global by default
 *
 * this is a 2-pass parsing logic, because use of top level members does
 * not obey "def before use" rule; and since all definitions (including
 * local variables) will be flushed into global scope, so it is not safe
 * to use a dummy definition first then fill when reaching the def
 *
 * node: top level
*/
static void ctx_class(java_ir* ir, tree_node* node)
{
    hash_table* table = lookup_global_scope(ir);
    definition* desc = new_definition(JNT_CLASS_DECL);
    tree_node* part = node->first_child; // class declaration
    char* registered_name = t2s(part->data->id.complex);

    // modifier data
    desc->class.modifier = node->data->top_level_declaration.modifier;

    // class name register
    lookup_register(ir, table, registered_name, desc, JAVA_E_CLASS_NAME_DUPLICATE);

    // [extends, implements, body]
    part = part->first_child;

    // extends, this name will be resolved later in linker
    if (part && part->type == JNT_CLASS_EXTENDS)
    {
        // extends->classtype->unit
        desc->class.extend = __name_unit_concat(part->first_child->first_child, NULL);
        part = part->next_sibling;
    }

    // implements, names will be resolved later in linker
    if (part && part->type == JNT_CLASS_IMPLEMENTS)
    {
        // implements->list->interfacetype
        tree_node* interface_type = part->first_child->first_child;
        string_list sl;

        // extract all names
        init_string_list(&sl);
        while (interface_type)
        {
            string_list_append(&sl, __name_unit_concat(interface_type->first_child, NULL));
            interface_type = interface_type->next_sibling;
        }

        // concat all names
        desc->class.implement = string_list_concat(&sl, ",");
        release_string_list(&sl);

        part = part->next_sibling;
    }

    // now we must have class body
    // otherwise it should not pass syntax parser

    // each part is a class body declaration
    tree_node* first_decl = part->first_child;

    // first pass: register definition
    part = first_decl;
    while (part)
    {
        // class body declaration -> [static|ctor|type]
        tree_node* declaration = part->first_child;

        if (declaration->type == JNT_CTOR_DECL)
        {
            /**
             * TODO: constructor
            */
        }
        else if (declaration->type == JNT_TYPE)
        {
            /**
             * Type as starter, it can be method/variable declarator
            */
            switch (declaration->next_sibling->type)
            {
                case JNT_VAR_DECLARATORS:
                    def_var(ir, declaration, part->data->top_level_declaration.modifier, true);
                    break;
                case JNT_METHOD_DECL:
                    def_method(ir, declaration, part->data->top_level_declaration.modifier);
                    break;
                default:
                    break;
            }
        }

        part = part->next_sibling;
    }

    // now go back to first declaration for code generation
    part = first_decl;

    // second pass: code generation
    while (part)
    {
        /**
         * TODO: IR code generation for:
         * 1. static initializer
         * 2. constructor
         * 3. member variable initializer
         * 4. method block
        */

        // class body declaration -> [static|ctor|type]
        tree_node* declaration = part->first_child;

        if (declaration->type == JNT_STATIC_INIT)
        {
            /**
             * TODO: static initializer
            */
        }
        else if (declaration->type == JNT_CTOR_DECL)
        {
            /**
             * TODO: constructor
            */
        }
        else if (declaration->type == JNT_TYPE)
        {
            if (declaration->next_sibling->type == JNT_VAR_DECLARATORS)
            {
                declaration = declaration->next_sibling->first_child;
                desc = t2d(table, declaration->data->declarator.id.complex);

                // if has a child, then it is the initializer
                declaration = declaration->first_child;
                if (declaration)
                {
                    if (declaration->type == JNT_EXPRESSION)
                    {
                        // top-level defs do not have order
                        walk_expression(ir, &ir->code_member_init, declaration);
                    }
                    else if (declaration->type == JNT_ARRAY_INIT)
                    {
                        /**
                         * TODO: array (of expression) init code
                        */
                    }
                }
            }
            else if (declaration->next_sibling->type == JNT_METHOD_DECL)
            {
                /**
                 * type
                 * |
                 * method declaration
                 * |
                 * +--- header              <--- HERE
                 * |    |
                 * |    +--- param list
                 * |         |
                 * |         +--- param 1
                 * |         +--- ...
                 * |
                 * +--- body
                */
                declaration = declaration->next_sibling->first_child;

                // begin scope
                hash_table* method_scope = lookup_new_scope(ir, LST_METHOD);

                // on method, we only have global to lookup so no need to call use()
                desc = t2d(table, declaration->data->declarator.id.complex);

                // fill all parameter declarations
                def_params(ir, declaration->first_child);

                // parse body
                walk_block(ir, &desc->method.code, declaration->next_sibling, false);

                // we need to keep all definitions active
                lookup_pop_scope(ir, true);
            }
        }

        part = part->next_sibling;
    }
}

/**
 * TODO:contextualize "interface declaration"
*/
static void ctx_interface(java_ir* ir, tree_node* node)
{
    hash_table* table = lookup_new_scope(ir, LST_INTERFACE);

    lookup_pop_scope(ir, false);
}
