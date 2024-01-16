#include <stdio.h>

#include "compiler.h"
#include "report.h"

#include "hash-table.h"
#include "utils.h"

static char* test_paths[] = {
    // "./test/pkg-decl-1.txt",
    // "./test/pkg-decl-2.txt",
    // "./test/pkg-decl-3.txt",
    // "./test/pkg-decl-4.txt",
    // "./test/pkg-decl-5.txt",

    // "./test/import-decl-1.txt",
    // "./test/import-decl-2.txt",

    // "./test/top-level-1.txt",

    // "./test/class-decl-1.txt",
    // "./test/interface-decl-1.txt",

    // "./test/general-no-block-and-statement.txt",

    // "./test/ambiguity-1.txt",
    // "./test/ambiguity-2.txt",

    // "./test/general-1.txt",

    "./test/recovery/pkg-decl-1.txt",
    "./test/recovery/pkg-decl-2.txt",
    "./test/recovery/pkg-decl-3.txt",
};

void java_symbol_lookup_table_no_collision_test(bool use_prime_size)
{
    printf("=====symbol table hash test (enforce prime size: %s)=====\n", use_prime_size ? "true" : "false");

    hash_table t;
    size_t bucket_size = use_prime_size ? find_next_prime(num_java_reserved_words) : num_java_reserved_words;

    while (true)
    {
        printf("testing collision with bucket size %zd...", bucket_size);

        init_hash_table(&t, bucket_size);
        for (int idx_word = 0; idx_word < num_java_reserved_words; idx_word++)
        {
            shash_table_insert(&t, java_reserved_words[idx_word].content, &java_reserved_words[idx_word]);

            if (t.num_filled != idx_word + 1)
            {
                printf("failed\n");
                break;
            }
        }

        if (t.num_filled == num_java_reserved_words)
        {
            printf("success (load factor: %f = %f)\n", hash_table_load_factor(&t), (float)num_java_reserved_words / bucket_size);
            break;
        }
        else
        {
            release_hash_table(&t);
            bucket_size = use_prime_size ? find_next_prime(bucket_size + 1) : (bucket_size + 1);
        }
    }

    printf(">>>Lookup Test<<<\n");
    const char* not_found = "(null)";
    for (int idx_word = 0; idx_word < num_java_reserved_words; idx_word++)
    {
        java_reserved_word* w = shash_table_find(&t, java_reserved_words[idx_word].content);
        printf("key: %s -> %s\n", java_reserved_words[idx_word].content, w ? w->content : not_found);
    }
    debug_shash_table(&t);

    release_hash_table(&t);

    printf("================================\n");
}

int main(int argc, char* argv[])
{
    java_symbol_lookup_table_no_collision_test(true);
    // java_symbol_lookup_table_no_collision_test(false);

    compiler compiler;
    int num_source_files = ARRAY_SIZE(test_paths);

    // this is heavy, initialize once and retask
    // compiler for every input file
    init_compiler(&compiler);

    debug_format_report(REPORT_INTERNAL | REPORT_GENERAL);
    // debug_print_reserved_words();
    // debug_print_symbol_table(&compiler.rw_lookup_table);

    for (int i = 0; i < num_source_files; i++)
    {
        printf("\nFile %d: %s\n", i + 1, test_paths[i]);

        if (!retask_compiler(&compiler, test_paths[i]))
        {
            fprintf(stderr, "WARNING: File \"%s\" skipped.\n", test_paths[i]);
            continue;
        }

        parse(&compiler.context);

        // debug_file_buffer(&compiler.reader);
        // debug_tokenize(&compiler.reader, &compiler.rw_lookup_table);
        debug_ast(compiler.context.ast_root);
        compiler_error_format_print(&compiler);
    }

    release_compiler(&compiler);
    return 0;
}
