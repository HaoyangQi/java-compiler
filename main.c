#include <stdio.h>

#include "compiler.h"
#include "report.h"

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

int main(int argc, char* argv[])
{
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
