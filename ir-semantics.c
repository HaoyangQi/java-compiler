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
    pkg_name = name_unit_concat(name->first_child, last_unit);

    // register the class name if applicable
    if (registered_name)
    {
        table = lookup_global_scope(ir);
        desc = new_definition(JNT_IMPORT_DECL);

        // move the data and detach
        desc->import.package_name = pkg_name;
        pkg_name = NULL;

        // name resolution must be unique
        if (!lookup_register(ir, table, &registered_name, &desc, JAVA_E_MAX))
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
         *
         * here: desc = NULL
        */
        lookup_register(ir, table, &pkg_name, &desc, JAVA_E_IMPORT_DUPLICATE);
    }

    // cleanup
    free(registered_name);
    free(pkg_name);
    definition_delete(desc);
}

/**
 * contextualize "class declaration"
 *
 * this is a 2-pass parsing logic, because use of top level members does
 * not obey "def before use" rule; and since all definitions (including
 * local variables) will be flushed into global scope, so it is not safe
 * to use a dummy definition first then fill when reaching the def
 *
 * node: JNT_TOP_LEVEL
*/
static void ctx_class(java_ir* ir, tree_node* node)
{
    // register all definitions first
    def_class(ir, node);

    hash_table* table = lookup_global_scope(ir);
    tree_node* part = NULL;
    tree_node* declaration = NULL;
    definition* desc = NULL;

    /**
     * JNT_TOP_LEVEL
     * |
     * +--- JNT_CLASS_DECL
     *      |
     *      +--- JNT_CLASS_EXTENDS      <--- HERE
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
    part = node->first_child->first_child;

    // locate JNT_CLASS_BODY
    while (part && part->type != JNT_CLASS_BODY)
    {
        part = part->next_sibling;
    }

    // reach first JNT_CLASS_BODY_DECL
    part = part->first_child;

    // code generation
    while (part)
    {
        /**
         * TODO: IR code generation for:
         * 1. static initializer
         * 2. constructor
         * 3. member variable initializer
         * 4. method block
        */

        // reach content
        declaration = part->first_child;

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
                // desc = t2d(table, declaration->data->declarator.id.complex);

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
                lookup_new_scope(ir, LST_METHOD);

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
