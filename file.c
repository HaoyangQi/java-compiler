#include "file.h"

void init_file_buffer(file_buffer* buffer)
{
    buffer->size = 0;
    buffer->buf = NULL;
    buffer->cur = NULL;
}

void release_file_buffer(file_buffer* buffer)
{
    free(buffer->buf);
}

bool load_source_file(file_buffer* buffer, const char* name)
{
    if (!name)
    {
        return false;
    }

    FILE* fp = fopen(name, "rb");

    if (!fp)
    {
        return false;
    }

    fseek(fp, 0, SEEK_END);
    buffer->size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (buffer->size > 0)
    {
        buffer->buf = (byte*)malloc(buffer->size + 1);
        ASSERT_ALLOCATION(buffer->buf);

        // read 1 byte for *size* times in case of large files
        long read_size = fread(buffer->buf, 1, buffer->size, fp);

        if (read_size != buffer->size)
        {
            free(buffer->buf);
            buffer->buf = NULL;
            return false;
        }

        buffer->buf[buffer->size] = 0x00;
    }

    buffer->cur = buffer->buf;

    fclose(fp);
    return true;
}
