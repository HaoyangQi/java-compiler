#include "ll.h"

/**
 * INTERNAL
 *
 * item initializer
*/
static void init_linked_list_item(ll_item* item, void* data)
{
    item->data = data;
    item->next = NULL;
}

/**
 * link list initializer
*/
void init_linked_list(linked_list* ll)
{
    ll->first = NULL;
    ll->last = NULL;
    ll->num_item = 0;
}

/**
 * append element to the end of the linked list
*/
void linked_list_append(linked_list* ll, void* data)
{
    ll_item* item = (ll_item*)malloc_assert(sizeof(ll_item));

    init_linked_list_item(item, data);

    if (ll->first)
    {
        ll->last->next = item;
        ll->last = item;
    }
    else
    {
        ll->first = item;
        ll->last = item;
    }

    ll->num_item++;
}

/**
 * pop first item and return the data reference
*/
void* linked_list_pop_front(linked_list* ll)
{
    ll_item* item = ll->first;
    void* data = NULL;

    if (!item)
    {
        return NULL;
    }

    data = item->data;
    ll->first = item->next;
    ll->num_item--;

    if (ll->num_item == 0)
    {
        ll->last = NULL;
    }

    free(item);
    return data;
}

/**
 * test if linked list is empty
*/
bool linked_list_empty(linked_list* ll)
{
    return ll->num_item == 0;
}

/**
 * iterate through list, O(n) complexity
 * stops when callback returns false
*/
void linked_list_iterate(linked_list* ll, ll_iterator iter)
{
    ll_item* item = ll->first;
    size_t idx = 0;

    while (item && (*iter)(idx, item->data))
    {
        item = item->next;
        idx++;
    }
}

/**
 * find element by index, O(n) complexity
*/
void* linked_list_at(linked_list* ll, size_t idx)
{
    ll_item* item = ll->first;

    while (item && idx)
    {
        item = item->next;
        idx--;
    }

    return item->data;
}

/**
 * peek the front element data without manipulating the list
*/
void* linked_list_front(linked_list* ll)
{
    if (ll->first)
    {
        return ll->first->data;
    }

    return NULL;
}
