#include "ir.h"

// hash table mapping string->lookup_value_descriptor wrapper
#define HT_STR2DESC(t, s) ((lookup_value_descriptor*)shash_table_find(t, s))

/**
 * return a copy of string that a token consists
 *
 * no validation here because we are beyond that
 * (thus not in job description :P)
*/
static char* token2string(java_token* token)
{
    size_t len = buffer_count(token->from, token->to);
    char* content = (char*)malloc_assert(sizeof(char) * (len + 1));

    buffer_substring(content, token->from, len);

    return content;
}

static void cxt_import(java_ir* ir, tree_node* node);

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
        cxt_import(ir, node);
        node = node->next_sibling;
    }

    // top-levels
    while (node && node->type == JNT_TOP_LEVEL)
    {
        /**
         * TODO: handle import decl
        */
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
 * import does not have scope, so it uses current one
*/
static void cxt_import(java_ir* ir, tree_node* node)
{
    lookup_value_descriptor* desc = new_lookup_value_descriptor(JNT_IMPORT_DECL);
    hash_table* table = lookup_current_scope(ir);
    // JNT_IMPORT_DECL -> Name -> Unit
    tree_node* name = node->first_child;
    tree_node* name_unit = name->first_child;
    tree_node* last_unit = NULL;
    char* registered_name = NULL;
    char* pkg_name = NULL;

    if (!node->data->import.on_demand)
    {
        // last name unit is the import target
        last_unit = name->last_child;
        registered_name = token2string(last_unit->data->id.complex);
    }

    // now we concat the package name
    while (name_unit != last_unit)
    {
        char* sn = token2string(name_unit->data->id.complex);

        if (!pkg_name)
        {
            pkg_name = sn;
        }
        else
        {
            // pkg_name + "." + sn + "\0"
            size_t cur_len = strlen(pkg_name);
            pkg_name = (char*)realloc_assert(pkg_name, sizeof(char) * (cur_len + 1 + strlen(sn) + 1));

            strcpy(pkg_name + cur_len, ".");
            strcpy(pkg_name + cur_len + 1, sn);
        }

        name_unit = name_unit->next_sibling;
    }

    // register the class name if applicable
    if (registered_name)
    {
        desc->import.package_name = pkg_name;

        // name resolution must be unique
        if (shash_table_test(table, registered_name))
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
        else
        {
            shash_table_insert(table, registered_name, desc);
        }
    }
    else
    {
        /**
         * on-demand import in ir is not needed to link to specifc symbol
         * unless there is an ambiguity in AST that require this info to
         * resolve
         *
         * so the table simply logs the package name so we can use it
         * whenever necessary in ir, and in linker
        */
        if (shash_table_test(&ir->tbl_on_demand_packages, pkg_name))
        {
            ir_error(ir, JAVA_E_IMPORT_DUPLICATE);
        }
        else
        {
            shash_table_insert(&ir->tbl_on_demand_packages, pkg_name, NULL);
        }

        // value not used anyway
        lookup_value_descriptor_delete(desc);
    }
}
