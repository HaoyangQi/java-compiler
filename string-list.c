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
    sl->count = 0;
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
}

/**
 * append element
 *
*/
void string_list_append(string_list* sl, char* str_data, bool copy)
{
    if (!str_data)
    {
        return;
    }

    string_list_item* e = new_string_list_item(copy ? strmcpy_assert(str_data) : str_data);

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
    sl->count++;
}

/**
 * append element by passing a character
 *
*/
void string_list_append_char(string_list* sl, char c)
{
    if (!c)
    {
        return;
    }

    char* s = (char*)malloc_assert(sizeof(char) * 2);
    s[0] = c;
    s[1] = '\0';

    string_list_append(sl, s, false);
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
    sl->count--;

    return s;
}

/**
 * concat all strings with delimiter
 *
 * when dlim = NULL, atring are concatenation without any separator
 * when dlim[0] = '\0', strings are separated by '\0'
 *
 * if list is empty, it returns NULL
*/
char* string_list_concat(string_list* sl, const char* dlim)
{
    if (!sl || sl->count == 0) { return NULL; }

    string_list_item* e;
    char* s;
    char* p;
    bool has_dlim = dlim && dlim[0] != '\0';
    size_t dlen = dlim == NULL ? 0 : (dlim[0] == '\0' ? 1 : strlen(dlim));
    size_t total_length = (sl->count - 1) * dlen;
    size_t total_size;

    /**
     * first pass: calculate total length
    */
    for (e = sl->first; e != NULL; e = e->next)
    {
        total_length += strlen(e->s);
    }

    // allocate and initialize content
    total_size = sizeof(char) * (total_length + 1);
    s = (char*)malloc_assert(total_size);
    memset(s, 0, total_size);

    /**
     * second pass: fill the content
    */
    for (e = sl->first, p = s; e != NULL; e = e->next)
    {
        // copy string content
        strcpy(p, e->s);
        p += strlen(e->s);

        // copy dlimiter
        if (e->next)
        {
            if (has_dlim)
            {
                strcpy(p, dlim);
            }

            // if dlim = '\0', simply move pointer by 1 char leaving a \0 in-between
            p += dlen;
        }
    }

    return s;
}

// flatten into simple string array
char** string_list_to_string_array(string_list* sl)
{
    string_list_item* e = sl->first;
    char** arr = (char**)malloc_assert(sizeof(char*) * sl->count);

    for (size_t i = 0; e != NULL; i++)
    {
        arr[i] = strmcpy_assert(e->s);
        e = e->next;
    }

    return arr;
}
