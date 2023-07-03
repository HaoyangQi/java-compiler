#include <stdio.h>

#include "file.h"
#include "symtbl.h"
#include "langspec.h"

#include "report.h"

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
    printf("===== FILE BUFFER =====\n");
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

void debug_format_report(byte report_type)
{
    printf("===== COMPILER RUNTIME REPORT =====\n");

    if (report_type & REPORT_INTERNAL)
    {
        printf("Internal:\n");
        printf("    Number of reserved words of compiled Java: %zd\n",
            compiler_debug_report.internal.reserved_words_count);
        printf("    Lookup table size of reserved words: %zd\n",
            compiler_debug_report.internal.reserved_words_lookup_table_size);
        printf("    Number of collisions during table creation: %zd\n",
            compiler_debug_report.internal.reserved_words_hash_collisions);
        printf("    Longest probing of reserved word lookup: %zd\n",
            compiler_debug_report.internal.reserved_words_longest_probing);
    }

    if (report_type & REPORT_GENERAL)
    {
        printf("General:\n");
        printf("    Compiled Java language version: %zd\n",
            compiler_debug_report.general.language_version);
    }

    printf("===== END OF REPORT =====\n");
}

void debug_print_reserved_words()
{
    printf("===== JAVA RESERVED WORDS =====\n");

    for (int i = 0; i < num_java_reserved_words; i++)
    {
        printf("%s %d\n", java_reserved_words[i].content, java_reserved_words[i].id);
    }
}

void debug_print_symbol_table(java_symbol_table* table)
{
    printf("===== RESERVED WORDS LOOKUP =====\n");

    bool in_skip = false;
    int c = 0;

    for (int i = 0; i < table->num_slot; i++)
    {
        if (table->slots[i])
        {
            if (table->slots[i]->word)
            {
                printf("    %lx \"%s\", id=%d\n",
                    table->slots[i]->full_hash,
                    table->slots[i]->word->content,
                    table->slots[i]->word->id);
            }
            else
            {
                printf("    ERROR: word not attached.\n");
            }
            in_skip = false;
            c++;
        }
        else if (!in_skip)
        {
            printf("    ...\n");
            in_skip = true;
        }
    }

    printf("count: %d, should match reserved word count (%d)\n",
        c, num_java_reserved_words);
}
