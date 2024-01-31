/**
 * IR Front-End: Semantic Analysis & CFG generation
 *
*/

#include "ir.h"

// hash table mapping string->definition wrapper
#define HT_STR2DEF(t, s) ((definition*)shash_table_find(t, s))

/* reserved constant context */

// reserved entry point name
static const char* reserved_method_name_entry_point = "main";

// primitive data bit-length
static const int primitive_bit_length[IRPV_MAX] = { 8, 16, 32, 64, 16, 32, 64, 32 };

/**
 * Token-To-String Helper
 *
 * return a copy of string that a token consists
 *
 * no validation here because we are beyond that
 * (thus not in job description :P)
*/
static char* t2s(java_token* token)
{
    size_t len = buffer_count(token->from, token->to);
    char* content = (char*)malloc_assert(sizeof(char) * (len + 1));

    buffer_substring(content, token->from, len);

    return content;
}

/**
 * Token-To-Definition Helper
 *
 * a definition associated to a name cannot be NULL, so if
 * undefined it returns NULL
 *
 * returned definition is a reference, not a copy
*/
static definition* t2d(hash_table* table, java_token* token)
{
    char* registered_name = t2s(token);
    definition* def = HT_STR2DEF(table, registered_name);
    free(registered_name);

    return def;
}

static void ctx_import(java_ir* ir, tree_node* node);
static void ctx_class(java_ir* ir, tree_node* node);
static void ctx_interface(java_ir* ir, tree_node* node);

/**
 * Context Analysis Entry Point
 *
 * It will generate a SSA form of AST
*/
void contextualize(java_ir* ir, tree_node* compilation_unit)
{
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
 * contexualize variable declaration
 *
 * node: variable declarator
*/
static bool def(
    java_ir* ir,
    hash_table* table,
    definition* type_def,
    tree_node* declarator,
    java_error_id err_dup,
    java_error_id err_dim_amb,
    java_error_id err_dim_dup
)
{
    bool success = lookup_register(
        ir,
        table,
        t2s(declarator->data->declarator.id.complex),
        definition_copy(type_def),
        err_dup
    );

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
    if (declarator->data->declarator.dimension > 0)
    {
        if (type_def->variable.type.dim != declarator->data->declarator.dimension)
        {
            ir_error(ir, err_dim_amb);
        }
        else
        {
            ir_error(ir, err_dim_dup);
        }
    }

    return success;
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
 * node: top level
*/
static void ctx_class(java_ir* ir, tree_node* node)
{
    hash_table* table = lookup_global_scope(ir);
    definition* desc = new_definition(JNT_CLASS_DECL);
    tree_node* part = node->first_child; // class declaration
    char* registered_name = t2s(part->data->id.complex);
    bool need_member_init = false;

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

            if (declaration->next_sibling->type == JNT_VAR_DECLARATORS)
            {
                desc = new_definition(JNT_VAR_DECL);

                desc->variable.is_class_member = true;
                desc->variable.modifier = part->data->top_level_declaration.modifier;
                desc->variable.type.primitive = declaration->data->declarator.id.simple;
                desc->variable.type.dim = declaration->data->declarator.dimension;

                // if not primitive type, then it must be a reference type
                if (desc->variable.type.primitive == JLT_MAX)
                {
                    // type->class_type->unit
                    desc->variable.type.reference = __name_unit_concat(declaration->first_child->first_child, NULL);
                }

                // variable declarators -> [var, var, ...]
                declaration = declaration->next_sibling->first_child;

                // register, every id has same type
                while (declaration)
                {
                    def(
                        ir, table, desc, declaration,
                        JAVA_E_MEMBER_VAR_DUPLICATE,
                        JAVA_E_MEMBER_VAR_DIM_AMBIGUOUS,
                        JAVA_E_MEMBER_VAR_DIM_DUPLICATE
                    );
                    need_member_init = need_member_init || declaration->first_child != NULL;

                    declaration = declaration->next_sibling;
                }
            }
            else if (declaration->next_sibling->type == JNT_METHOD_DECL)
            {
                /**
                 * TODO: method declarator
                */
            }
        }

        part = part->next_sibling;
    }

    // now go back to first declaration for code generation
    part = first_decl;

    // member code graph initialization
    if (need_member_init)
    {
        ir->code_member_init = (cfg*)malloc_assert(sizeof(cfg));
        init_cfg(ir->code_member_init);
    }

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
                        /**
                         * TODO: expression code
                        */
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
                 * TODO: method
                */
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
