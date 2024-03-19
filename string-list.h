#pragma once
#ifndef __COMPILER_STRING_LIST_H__
#define __COMPILER_STRING_LIST_H__

#include "types.h"

/**
 * FIFO list of strings
*/
typedef struct _string_list_item
{
    char* s;

    struct _string_list_item* prev;
    struct _string_list_item* next;
} string_list_item;

typedef struct _string_list
{
    string_list_item* first;
    string_list_item* last;
    size_t count;
} string_list;

void init_string_list(string_list* sl);
void release_string_list(string_list* sl);
void string_list_append(string_list* sl, char* str_data, bool copy);
char* string_list_pop_front(string_list* sl);
char* string_list_concat(string_list* sl, const char* dlim);
char** string_list_to_string_array(string_list* sl);

#endif
