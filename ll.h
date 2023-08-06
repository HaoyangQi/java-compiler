#pragma once
#ifndef __COMPILER_UTIL_LINKED_LIST_H__
#define __COMPILER_UTIL_LINKED_LIST_H__

#include "types.h"

/**
 * linked-list item type
*/
typedef struct _ll_item
{
    void* data;
    struct _ll_item* next;
} ll_item;

/**
 * Simple linked-list manager
*/
typedef struct
{
    ll_item* first;
    ll_item* last;
    size_t num_item;
} linked_list;

typedef bool (*ll_iterator)(size_t idx, void* data);

void init_linked_list(linked_list* llm);
void linked_list_append(linked_list* ll, void* data);
void* linked_list_pop_front(linked_list* ll);
void* linked_list_empty(linked_list* ll);
void linked_list_iterate(linked_list* ll, ll_iterator iter);
void* linked_list_at(linked_list* ll, size_t idx);
void* linked_list_front(linked_list* ll);

#endif
