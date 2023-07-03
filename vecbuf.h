#pragma once
#ifndef __COMPILER_VECTOR_BUFFER_H__
#define __COMPILER_VECTOR_BUFFER_H__

#include "types.h"

/**
 * A continuous memory buffer that is accessed like an array,
 * controlled like stack
*/
typedef struct _vec
{
    void** buffer;
    size_t size;
    size_t length;
} vec;

void init_vec(vec* v, size_t size);
void release_vec(vec* v);
void vec_push(vec* v, void* element);
void* vec_pop(vec* v);
void* vec_at(vec* v, size_t i);
bool vec_empty(vec* v);
size_t vec_length(vec* v);

#endif
