#include "string-list.h"

static string_list_item* new_string_list_item(char* s)
{
    string_list_item* l = (string_list_item*)malloc_assert(sizeof(string_list_item));

    l->s = s;
    l->prev = NULL;
    l->next = NULL;

    return l;
}

// init list
void init_string_list(string_list* sl)
{
    sl->first = NULL;
    sl->last = NULL;
}

// delete entire list
void release_string_list(string_list* sl)
{
    string_list_item* e = sl->first;

    while (e)
    {
        free(e->s);
        sl->first = e->next;
        free(e);

        e = sl->first;
    }

    sl->first = NULL;
    sl->last = NULL;
}

/**
 * append element
 *
 * str_data is void* becuase the list enforces string data
 * to be a copy, so literal is not allowed
*/
void string_list_append(string_list* sl, void* str_data)
{
    if (!str_data)
    {
        return;
    }

    string_list_item* e = new_string_list_item((char*)str_data);

    if (sl->first)
    {
        e->prev = sl->last;
        sl->last->next = e;
    }
    else
    {
        sl->first = e;
    }

    sl->last = e;
}

// pop front
char* string_list_pop_front(string_list* sl)
{
    if (!sl->first)
    {
        return NULL;
    }

    string_list_item* e = sl->first;
    char* s = e->s;

    sl->first = e->next;
    free(e);
    sl->first->prev = NULL;

    return s;
}

// concat
char* string_list_concat(string_list* sl, const char* dlim)
{
    string_list_item* e = sl->first;
    char* s = (char*)malloc_assert(sizeof(char));

    s[0] = '\0';
    while (e)
    {
        size_t dlim_len = e->next ? strlen(dlim) : 0;

        s = (char*)realloc_assert(s, sizeof(char) * (strlen(s) + strlen(e->s) + dlim_len + 1));
        strcpy(s + strlen(s), e->s);

        if (dlim_len)
        {
            strcpy(s + strlen(s), dlim);
        }

        e = e->next;
    }

    return s;
}
