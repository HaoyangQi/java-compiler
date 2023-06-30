#include <stdio.h>

#include "file.h"

void debug_print_memory(byte* mem, long size, long line_break)
{
    for (long i = 0; i < size; i++)
    {
        if (i % line_break == 0)
        {
            printf("    ");
        }

        byte c = mem[i];
        bool should_break = (i + 1) % line_break == 0 && (i + 1) != size;

        printf("%02X%c%c%s", c,
            isprint(c) ? '/' : ' ',
            isprint(c) ? c : ' ',
            should_break ? "\n" : "    ");
    }
}

void debug_file_buffer(file_buffer* reader)
{
    printf("size: %ld byte(s)\n", reader->size);
    printf("buffer: 0x%p\n", (void*)(reader->buf));
    printf("cur: 0x%p", (void*)(reader->cur));

    if (reader->buf)
    {
        if (reader->size <= 0)
        {
            printf("\n    (buffer registered but size is invalid)");
        }

        printf("\n");

        debug_print_memory(reader->buf, reader->size, 10);

        printf("\n");
    }
    else
    {
        printf("\n    (NULL)\n");
    }
}
