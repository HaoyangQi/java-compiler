#include "ir.h"

// hash table mapping string->lookup_value_descriptor wrapper
#define HT_STR2DESC(t, s) ((lookup_value_descriptor*)shash_table_find(t, s))

/* reserved constant context */

// reserved entry point name
static const char* reserved_method_name_entry_point = "main";

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
    lookup_new_scope(ir, LST_COMPILATION_UNIT);

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
 * contextualize "import"
 *
 * import goes into global scope
 * on-demand import goes into separate global table
*/
static void ctx_import(java_ir* ir, tree_node* node)
{
    lookup_value_descriptor* desc = NULL;
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
        desc = new_lookup_value_descriptor(JNT_IMPORT_DECL);
        desc->import.package_name = pkg_name;

        // name resolution must be unique
        if (!lookup_register(ir, table, registered_name, desc, JAVA_E_MAX))
        {
            if (strcmp(pkg_name, HT_STR2DESC(table, registered_name)->import.package_name) == 0)
            {
                ir_error(ir, JAVA_E_IMPORT_DUPLICATE);
            }
            else
            {
                ir_error(ir, JAVA_E_IMPORT_AMBIGUOUS);
            }

            // discard descriptor candidate
            lookup_value_descriptor_delete(desc);
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
 * node: top level
*/
static void ctx_class(java_ir* ir, tree_node* node)
{
    hash_table* table = NULL;
    lookup_value_descriptor* desc = new_lookup_value_descriptor(JNT_CLASS_DECL);
    tree_node* part = node->first_child; // class declaration
    char* registered_name = t2s(part->data->id.complex);

    // modifier data
    desc->class.modifier = node->data->top_level_declaration.modifier;

    // class name register
    table = lookup_global_scope(ir);
    lookup_register(ir, table, registered_name, desc, JAVA_E_CLASS_NAME_DUPLICATE);

    // [extends, implements, body]
    part = part->first_child;

    // extends
    if (part && part->type == JNT_CLASS_EXTENDS)
    {
        /**
         * TODO:
         * register name in lookup, because it is a type name
         * value is NULL, resolve later
        */
        part = part->next_sibling;
    }

    // implements
    if (part && part->type == JNT_CLASS_IMPLEMENTS)
    {
        /**
         * TODO:
         * register name in lookup, because it is a type name
         * value is NULL, resolve later
        */
        part = part->next_sibling;
    }

    // now we must have class body
    // otherwise it should not pass syntax parser

    // class body scope
    table = lookup_new_scope(ir, LST_CLASS);

    // each part is a class body declaration
    part = part->first_child;

    while (part)
    {
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
        else
        {
            /**
             * Type as starter, it can be method/variable declarator
            */

            if (declaration->next_sibling->type == JNT_VAR_DECLARATORS)
            {
                desc = new_lookup_value_descriptor(JNT_VAR_DECL);

                desc->member_variable.modifier = part->data->top_level_declaration.modifier;
                desc->member_variable.type.primitive = declaration->data->declarator.id.simple;
                desc->member_variable.type.dim = declaration->data->declarator.dimension;

                // if not primitive type, then it must be a reference type
                if (desc->member_variable.type.primitive == JLT_MAX)
                {
                    // type->class_type->unit
                    desc->member_variable.type.reference = __name_unit_concat(declaration->first_child->first_child, NULL);
                }

                // variable declarators -> [var, var, ...]
                declaration = declaration->next_sibling->first_child;

                // register, every id has same type
                while (declaration)
                {
                    lookup_register(ir, table,
                        t2s(declaration->data->declarator.id.complex),
                        lookup_value_descriptor_copy(desc),
                        JAVA_E_MEMBER_VAR_DUPLICATE
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
                    if (declaration->data->declarator.dimension > 0)
                    {
                        if (desc->member_variable.type.dim != declaration->data->declarator.dimension)
                        {
                            ir_error(ir, JAVA_E_MEMBER_VAR_DIM_AMBIGUOUS);
                        }
                        else
                        {
                            ir_error(ir, JAVA_E_MEMBER_VAR_DIM_DUPLICATE);
                        }
                    }

                    /**
                     * TODO: generate code for initializer
                    */

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

    // pop context
    /**
     * TODO: pop it!
    */
    // lookup_pop_scope(ir);
}

/**
 * TODO:contextualize "interface declaration"
*/
static void ctx_interface(java_ir* ir, tree_node* node)
{
    hash_table* table = lookup_new_scope(ir, LST_INTERFACE);

    lookup_pop_scope(ir);
}
