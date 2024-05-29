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

/**
 * Context Analysis Entry Point
 *
*/
void contextualize(java_ir* ir, architecture* arch, tree_node* compilation_unit)
{
    // register architecture of current target
    ir->arch = arch ? arch : &default_arch;

    tree_node* node = compilation_unit->first_child;
    global_top_level* top;

    /**
     * state init
     *
     * instruction counter of member initialization code state must only init once
     * because the code are not guaranteed to stay together
    */
    ir_walk_state_init(ir);
    ir_walk_state_mutate(ir, IR_WALK_DEF_MEMBER_VAR);

    // prepare definitions
    def_global(ir, compilation_unit);

    /**
     * now we have all defs in place, we can start generating code
     *
    */
    for (size_t i = 0; i < ir->tbl_global.bucket_size; i++)
    {
        for (hash_pair* p = ir->tbl_global.bucket[i]; p != NULL; p = p->next)
        {
            top = p->value;

            if (!top) { continue; }

            switch (top->type)
            {
                case TOP_LEVEL_CLASS:
                    walk_class(ir, top);
                    break;
                case TOP_LEVEL_INTERFACE:
                    walk_interface(ir, top);
                    break;
                default:
                    break;
            }
        }
    }
}
