#include <stdio.h>

#include "file.h"
#include "symtbl.h"
#include "parser.h"

#include "debug.h"
#include "report.h"

typedef struct
{
    bool tasked;

    char* source_file_name;
    file_buffer reader;
    java_symbol_table rw_lookup_table;
    java_parser context;
} compiler;

/**
 * Full initialization of compiler instance
*/
bool init_compiler(compiler* compiler)
{
    compiler->tasked = false;

    init_file_buffer(&compiler->reader);
    init_symbol_table(&compiler->rw_lookup_table);
    init_debug_report();

    load_language_spec(&compiler->rw_lookup_table);
    init_parser(&compiler->context, &compiler->reader, &compiler->rw_lookup_table);

    return true;
}

/**
 * Detach source-file-specific context from compiler
*/
bool detask_compiler(compiler* compiler)
{
    if (compiler->tasked)
    {
        release_file_buffer(&compiler->reader);
        release_parser(&compiler->context);
        compiler->tasked = false;

        return true;
    }

    return false;
}

/**
 * Retask compiler to a new source file
*/
bool retask_compiler(compiler* compiler, char* source_path)
{
    detask_compiler(compiler);

    compiler->source_file_name = source_path;

    init_file_buffer(&compiler->reader);
    if (!load_source_file(&compiler->reader, source_path))
    {
        fprintf(stderr, "File failed to load.");
        return false;
    }

    init_parser(&compiler->context, &compiler->reader, &compiler->rw_lookup_table);

    compiler->tasked = true;
    return true;
}

/**
 * Release compiler instance
*/
void release_compiler(compiler* compiler)
{
    compiler->tasked = false;

    release_file_buffer(&compiler->reader);
    release_symbol_table(&compiler->rw_lookup_table);
    release_parser(&compiler->context);
}

static char* test_paths[] = {
    // "./test/pkg-decl-1.txt",
    // "./test/pkg-decl-2.txt",
    // "./test/pkg-decl-3.txt",
    // "./test/pkg-decl-4.txt",
    // "./test/pkg-decl-5.txt",

    // "./test/import-decl-1.txt",
    // "./test/import-decl-2.txt",

    // "./test/top-level-1.txt",

    "./test/class-decl-1.txt",
    "./test/interface-decl-1.txt",
};

int main(int argc, char* argv[])
{
    compiler compiler;
    int num_source_files = ARRAY_SIZE(test_paths);

    init_compiler(&compiler);

    debug_format_report(REPORT_INTERNAL | REPORT_GENERAL);
    // debug_print_reserved_words();
    // debug_print_symbol_table(&compiler.rw_lookup_table);

    for (int i = 0; i < num_source_files; i++)
    {
        printf("\nFile %d: %s\n", i + 1, test_paths[i]);

        retask_compiler(&compiler, test_paths[i]);
        parse(&compiler.context);

        // debug_file_buffer(&compiler.reader);
        // debug_tokenize(&compiler.reader, &compiler.rw_lookup_table);
        debug_ast(compiler.context.ast_root);
    }

    release_compiler(&compiler);
    return 0;
}
