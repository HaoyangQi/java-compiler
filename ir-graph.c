#include "ir.h"

instruction* new_instruction()
{
    instruction* inst = (instruction*)malloc_assert(sizeof(instruction));
    memset(inst, 0, sizeof(instruction));
    return inst;
}

instruction* delete_instruction(instruction* inst)
{
    instruction* next = inst->next;

    free(inst);
    return next;
}

code_graph_node* new_graph_node()
{
    code_graph_node* n = (code_graph_node*)malloc_assert(sizeof(code_graph_node));
    memset(n, 0, sizeof(code_graph_node));
    return n;
}

void instruction_append(code_graph_node* node, instruction* inst)
{
    if (node->inst_first)
    {
        node->inst_last->next = inst;
    }
    else
    {
        node->inst_first = inst;
    }

    node->inst_last = inst;
}
