#include "vecbuf.h"

/**
 * initialize a vector with default size
*/
void init_vec(vec* v, size_t size)
{
    v->size = size;
    v->length = 0;
    v->buffer = malloc_assert(sizeof(void*) * size);

    memset(v->buffer, 0, sizeof(void*) * size);
}

/**
 * release vector buffer
 *
 * shallow release only, the procedure will not free element itself
*/
void release_vec(vec* v)
{
    free(v->buffer);
}

/**
 * add new element in the back
*/
void vec_push(vec* v, void* element)
{
    if (!element)
    {
        return;
    }

    if (v->length >= v->size)
    {
        v->size = v->length + 1;

        void** nb = (void**)realloc(v->buffer, sizeof(void*) * v->size);
        ASSERT_ALLOCATION(nb);

        v->buffer = nb;
    }

    v->buffer[v->length] = element;
    v->length++;
}

/**
 * pop new element from the back
*/
void* vec_pop(vec* v)
{
    if (vec_empty(v))
    {
        return NULL;
    }

    // we need last valid element so it is at length - 1
    // so decrement first so simplify access
    v->length--;
    void* elem = v->buffer[v->length];
    v->buffer[v->length] = NULL;

    return elem;
}

/**
 * get element at index
*/
void* vec_at(vec* v, size_t i)
{
    if (i >= v->length)
    {
        return NULL;
    }

    return v->buffer[i];
}

/**
 * check if empty
*/
bool vec_empty(vec* v)
{
    return v->length == 0;
}

/**
 * length
*/
size_t vec_length(vec* v)
{
    return v->length;
}
