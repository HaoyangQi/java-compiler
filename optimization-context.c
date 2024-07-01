#include "optimization-context.h"
#include "hash-table.h"

static void init_code_context(code_context* code, char* name_ref_top_level)
{
    code->name_top_level = name_ref_top_level;
    code->name_method = NULL; // fill later
    code->def = NULL; // fill later

    init_optimizer(&code->om);
}

/**
 * Release code context
 *
 * Fields that reference data externally do not need deletion
 */
static void release_code_context(code_context* code)
{
    if (!code) { return; }

    release_optimizer(&code->om);
}

/**
 * Initialize optimization context
 *
 * This function does not do actual job, it only resets the object
 */
void init_optimization_context(optimization_context* oc, java_ir* ir)
{
    oc->num_top_level = 0;
    oc->ir = ir;
    oc->top_levels = NULL;
}

void release_optimization_context(optimization_context* oc)
{
    if (!oc) { return; }

    for (size_t i = 0; i < oc->num_top_level; i++)
    {
        top_level_optimizer* tlo = &oc->top_levels[i];

        for (size_t j = 0; j < tlo->num_methods; j++)
        {
            release_code_context(&tlo->contexts[j]);
        }

        free(tlo->contexts);
    }

    free(oc->top_levels);

    // guard: detach the memory info
    oc->num_top_level = 0;
    oc->top_levels = NULL;
}

void optimization_context_build(optimization_context* oc)
{
    hash_table* table = lookup_global_scope(oc->ir);
    size_t sz_top_levels = sizeof(top_level_optimizer) * table->num_pairs;

    // initialize first dimension
    oc->num_top_level = table->num_pairs;
    oc->top_levels = (top_level_optimizer*)malloc_assert(sz_top_levels);
    memset(oc->top_levels, 0, sz_top_levels);

    // iterate all top levels
    for (size_t i = 0, d1 = 0, d2 = 0; i < table->bucket_size; i++)
    {
        for (hash_pair* p = table->bucket[i]; p != NULL; p = p->next, d1++, d2 = 0)
        {
            global_top_level* top_level = p->value;

            // so far only class contains code
            if (!top_level || top_level->type != TOP_LEVEL_CLASS)
            {
                continue;
            }

            // initialize second dimension
            oc->top_levels[d1].num_methods = top_level->num_methods;
            oc->top_levels[d1].contexts = (code_context*)malloc_assert(sizeof(code_context) * top_level->num_methods);

            // now construct optimizer instances
            for (size_t dim = 0; dim < top_level->num_methods; dim++)
            {
                init_code_context(&oc->top_levels[d1].contexts[dim], p->key);
            }

            // iterate all members
            for (size_t j = 0; j < top_level->tbl_member.bucket_size; j++)
            {
                for (hash_pair* pm = top_level->tbl_member.bucket[j]; pm != NULL; pm = pm->next)
                {
                    code_context* code = &oc->top_levels[d1].contexts[d2];

                    if (optimizer_attach(&code->om, top_level, pm->value))
                    {
                        optimizer_execute(&code->om);

                        code->name_method = pm->key;
                        code->def = pm->value;
                        d2++;
                    }
                }
            }
        }
    }
}
