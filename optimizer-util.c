#include "optimizer.h"

/**
 * extracts definition pointer from reference data
 * if it is not a definition, it returns NULL
*/
definition* ref2def(const reference* r)
{
    if (r && r->type == IR_ASN_REF_DEFINITION)
    {
        return r->def;
    }

    return NULL;
}

/**
 * same as ref2def, but it also validates if it is a variable
*/
definition* ref2vardef(const reference* r)
{
    if (r && r->type == IR_ASN_REF_DEFINITION && is_def_variable(r->def))
    {
        return r->def;
    }

    return NULL;
}

/**
 * Convert variable id to var_map index
 *
 * 1. members: definition::mid
 * 2. locals: num_members + definition::lid
*/
size_t varmap_varid2idx(optimizer* om, const definition* variable)
{
    return variable->variable->kind == VARIABLE_KIND_MEMBER ? variable->mid : variable->lid + om->profile.num_members;
}

/**
 * Convert var_map index to variable id
 *
 * 1. members: definition::mid
 * 2. locals: index - num_members
*/
size_t varmap_idx2varid(optimizer* om, size_t var_map_index)
{
    return var_map_index < om->profile.num_members ? var_map_index : var_map_index - om->profile.num_members;
}

/**
 * Convert lid to var_map index
 *
 * num_members + definition::lid
*/
size_t varmap_lid2idx(optimizer* om, size_t lid)
{
    return lid + om->profile.num_members;
}

/**
 * Convert var_map index to lid
 *
 * num_members + definition::lid
*/
size_t varmap_idx2lid(optimizer* om, size_t var_map_index)
{
    return var_map_index - om->profile.num_members;
}

/**
 * check if variable index is a member
*/
bool varmap_idx_is_member(optimizer* om, size_t var_map_index)
{
    return var_map_index < om->profile.num_members;
}

/**
 * locate PHI function of given variable
 *
 * it returns NULL if it does not exist
*/
instruction* optimizer_phi_locate(basic_block* node, const definition* variable)
{
    if (!variable) { return NULL; }

    instruction* phi = node->inst_first;

    // locate PHI function
    while (phi)
    {
        if (phi->op == IROP_PHI)
        {
            definition* var = ref2def(phi->lvalue);

            if (var == variable)
            {
                break;
            }
        }
        else
        {
            // PHI must stay at top
            return NULL;
        }

        phi = phi->next;
    }

    return phi;
}

/**
 * Place A PHI function and returns its reference
 *
 * it returns true if insertion occurs
 *
 * Arity of PHI = number of incoming edge of the node
*/
bool optimizer_phi_place(optimizer* om, basic_block* node, definition* variable)
{
    instruction* phi = optimizer_phi_locate(node, variable);

    if (phi) { return false; }

    // insert new one if does not exist
    phi = new_instruction();

    phi->id = om->profile.num_instructions;
    phi->op = IROP_PHI;
    phi->node = node;
    phi->lvalue = new_reference(IR_ASN_REF_DEFINITION, variable);
    phi->operand_phi.arr = (instruction**)malloc_assert(sizeof(instruction*) * node->in.num);
    phi->operand_phi.num = node->in.num;

    om->profile.num_instructions++;

    instruction_push_front(node, phi);
    return true;
}

static void __optimizer_populate_variable_item(optimizer* om, reference* ref, bool in_loop)
{
    if (!ref || ref->def->type != DEFINITION_VARIABLE) { return; }

    definition* d = ref->def;
    variable_item* vi = &om->variables[varmap_varid2idx(om, d)];

    vi->ref = d;

    if (in_loop)
    {
        vi->ud_loop_inside++;
    }
    else
    {
        vi->ud_loop_outside++;
    }
}

/**
 * Populate All Variables
 *
 * This includes all member variables on top level and all local variables
 * of current CFG
 *
 * Member: index = mid
 * Local: index = lid + num_members
*/
void optimizer_populate_variables(optimizer* om)
{
    size_t sz_variables = sizeof(variable_item) * om->profile.num_variables;

    om->variables = (variable_item*)malloc_assert(sz_variables);
    memset(om->variables, 0, sz_variables);

    for (size_t i = 0; i < om->graph->nodes.num; i++)
    {
        basic_block* bb = om->graph->nodes.arr[i];

        for (instruction* p = bb->inst_first; p != NULL; p = p->next)
        {
            __optimizer_populate_variable_item(om, p->lvalue, bb->in_loop);
            __optimizer_populate_variable_item(om, p->operand_1, bb->in_loop);
            __optimizer_populate_variable_item(om, p->operand_2, bb->in_loop);
        }
    }
}

/**
 * Populate All Instructions in IR order
 *
 * The order is reversed post order on node array,
 * forward order within the node
 *
 * This order will also override instruction ID
 * to make:
 *
 * instruction ID = index = IR order
*/
void optimizer_populate_instructions(optimizer* om)
{
    size_t num_nodes = om->graph->nodes.num;
    size_t sz_instructions = sizeof(instruction_item) * om->profile.num_instructions;

    om->instructions = (instruction_item*)malloc_assert(sz_instructions);
    memset(om->instructions, 0, sz_instructions);

    for (size_t i = 0, j = 0; i < num_nodes; i++)
    {
        basic_block* b = om->node_postorder[num_nodes - i - 1];

        for (instruction* p = b->inst_first; p != NULL; p = p->next, j++)
        {
            p->id = j;
            om->instructions[j].ref = p;
        }
    }
}

/**
 * Release variables array based on current optimizer profile
*/
void optimizer_invalidate_variables(optimizer* om)
{
    free(om->variables);
    om->variables = NULL;
}

/**
 * Release instructions array based on current optimizer profile
*/
void optimizer_invalidate_instructions(optimizer* om)
{
    if (!om->instructions) { return; }

    for (size_t i = 0; i < om->profile.num_instructions; i++)
    {
        release_index_set(&om->instructions[i].def);
        release_index_set(&om->instructions[i].use);
        release_index_set(&om->instructions[i].in);
        release_index_set(&om->instructions[i].out);
    }

    free(om->instructions);
    om->instructions = NULL;
}

void optimizer_profile_copy(optimizer* om, optimizer_profile* profile)
{
    memcpy(profile, &om->profile, sizeof(optimizer_profile));
}

/**
 * Repopulate Using Profile
*/
void optimizer_profile_apply(optimizer* om, optimizer_profile* profile)
{
    optimizer_invalidate_instructions(om);
    optimizer_invalidate_variables(om);

    // override profile
    memcpy(&om->profile, profile, sizeof(optimizer_profile));

    // re-populate
    optimizer_populate_variables(om);
    optimizer_populate_instructions(om);
}
