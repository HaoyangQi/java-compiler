#include "file.h"

void init_file_buffer(file_buffer* buffer, java_error_logger* error_logger)
{
    buffer->size = 0;
    buffer->base = NULL;
    buffer->cur = NULL;
    buffer->limit = NULL;
    buffer->logger = error_logger;
}

void release_file_buffer(file_buffer* buffer)
{
    free(buffer->base);
}

bool load_source_file(file_buffer* buffer, const char* name)
{
    // rough test to see if path name is valid
    if (!name || name[0] == '\0')
    {
        buffer_error(buffer, JAVA_E_FILE_NO_PATH);
        return false;
    }

    FILE* fp = fopen(name, "rb");

    if (!fp)
    {
        buffer_error(buffer, JAVA_E_FILE_OPEN_FAILED, name);
        return false;
    }

    fseek(fp, 0, SEEK_END);
    buffer->size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (buffer->size > 0)
    {
        buffer->base = (byte*)malloc_assert(buffer->size + 1);

        // read 1 byte for *size* times in case of large files
        long read_size = fread(buffer->base, 1, buffer->size, fp);

        if (read_size != buffer->size)
        {
            free(buffer->base);
            buffer->base = NULL;
            buffer_error(buffer, JAVA_E_FILE_SIZE_NOT_MATCH, name);
            return false;
        }

        buffer->base[buffer->size] = 0x00;
    }

    buffer->cur = buffer->base;
    buffer->limit = buffer->base + buffer->size;

    fclose(fp);
    return true;
}

inline bool is_eof(file_buffer* buffer)
{
    return *buffer->cur == 0x00;
}

void buffer_error(file_buffer* buffer, java_error_id id, ...)
{
    va_list args;

    va_start(args, id);
    error_logger_vslog(buffer->logger, NULL, NULL, id, &args);
    va_end(args);
}

/**
 * move buffer cur pointer, returns state whether
 * there is still character left afterwards
 *
 * if, before AND after move, cur reaches \0
 * it returns false, true otherwise
*/
bool buffer_ptr_safe_move(file_buffer* buffer)
{
    if (is_eof(buffer))
    {
        return false;
    }

    // uncomment the following section for debug purpose
    // printf("reading index %lld, char %02x ", buffer->cur - buffer->base, *buffer->cur);
    // if (isprint(*buffer->cur))
    // {
    //     printf("char|_ %c _|", *buffer->cur);
    // }
    // printf("\n");

    buffer->cur++;
    return !is_eof(buffer);
}

/**
 * peek forward or backward
 *
 * if out of bound, it returns \0
*/
byte buffer_peek(file_buffer* buffer, int offset)
{
    byte* target = buffer->cur + offset;
    return (target >= buffer->base && target < buffer->limit) ? *target : 0x00;
}

/**
 * count of characters between 2 cursors
*/
size_t buffer_count(const byte* from, const byte* to)
{
    return to - from;
}

/**
 * substring of buffer stream
 *
 * no safety check to maintain efficiency
*/
void buffer_substring(byte* dest, const byte* from, size_t len)
{
    memcpy(dest, from, len);
    dest[len] = '\0';
}
