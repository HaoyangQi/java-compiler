#include "il.h"
#include "utils.h"

#define IL_BYTE_STREAM_DEFAULT_SIZE (64)

#define u1(stream, data_ref) il_byte_stream_add(stream, data_ref, 1)
#define u2(stream, data_ref) il_byte_stream_add(stream, data_ref, 2)
#define u4(stream, data_ref) il_byte_stream_add(stream, data_ref, 4)
#define u8(stream, data_ref) il_byte_stream_add(stream, data_ref, 8)

static inline int64_t __struct_padding_bytes(int64_t offset, int64_t align)
{
    return (-offset) & (align - 1);
}

typedef struct
{
    uint8_t* data;
    size_t length;
    size_t size;
} il_byte_stream;

static void init_il_byte_stream(il_byte_stream* stream)
{
    stream->data = (uint8_t*)malloc_assert(sizeof(uint8_t) * IL_BYTE_STREAM_DEFAULT_SIZE);
    stream->length = 0;
    stream->size = IL_BYTE_STREAM_DEFAULT_SIZE;
}

static void release_il_byte_stream(il_byte_stream* stream)
{
    free(stream->data);
}

static void il_byte_stream_resize(il_byte_stream* stream, size_t by)
{
    size_t new_size = stream->length + by;

    if (new_size > stream->size)
    {
        stream->size = find_next_pow2_size(new_size);
        stream->data = (uint8_t*)realloc_assert(stream->data, stream->size);
    }
}

static il_byte_stream_add(il_byte_stream* stream, void* data, size_t size)
{
    il_byte_stream_resize(stream, size);
    memcpy(stream->data + stream->length, data, size);
    stream->length += size;
}

/**
 * IR Emitter
 *
*/
void jil_emit(java_ir* ir)
{
    hash_table* table = lookup_global_scope(ir);

    // every top-level has one JIL
    for (size_t i = 0; i < table->bucket_size; i++)
    {
        for (hash_pair* p = table->bucket[i]; p != NULL; p = p->next)
        {
        }
    }
}
