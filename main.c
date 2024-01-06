#include <stdio.h>

#include "file.h"
#include "symtbl.h"
#include "parser.h"
#include "expression.h"

#include "debug.h"
#include "report.h"

typedef struct
{
    bool tasked;

    char* source_file_name;
    file_buffer reader;
    java_symbol_table rw_lookup_table;
    java_expression expression;
    java_parser context;
} compiler;

/**
 * Full initialization of compiler instance
*/
bool init_compiler(compiler* compiler)
{
    compiler->tasked = false;

    init_debug_report();
    init_file_buffer(&compiler->reader);
    init_symbol_table(&compiler->rw_lookup_table);
    init_expression(&compiler->expression);

    load_language_spec(&compiler->rw_lookup_table);
    init_parser(
        &compiler->context,
        &compiler->reader,
        &compiler->rw_lookup_table,
        &compiler->expression
    );

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

void format_file_loader_message(file_loader_status status)
{
    switch (status)
    {
        case FILE_OK:
            fprintf(stderr, "File loading completed without errors.\n");
            break;
        case FILE_PATH_REQUIRED:
            fprintf(stderr, "Required file path name.\n");
            break;
        case FILE_OPEN_FAILED:
            fprintf(stderr, "File failed to open as it does not exist.\n");
            break;
        case FILE_SIZE_MISMATCHED:
            fprintf(stderr, "File mapping results in incorrect size.\n");
            break;
        default:
            fprintf(stderr, "(Unrecognized error code: %d)\n", status);
            break;
    }
}

/**
 * Retask compiler to a new source file
*/
bool retask_compiler(compiler* compiler, char* source_path)
{
    detask_compiler(compiler);

    file_loader_status loader_status;

    compiler->source_file_name = source_path;
    init_file_buffer(&compiler->reader);
    loader_status = load_source_file(&compiler->reader, source_path);

    if (loader_status != FILE_OK)
    {
        fprintf(stderr, "ERROR: File failed to load.\n");
        format_file_loader_message(loader_status);
        return false;
    }

    init_parser(
        &compiler->context,
        &compiler->reader,
        &compiler->rw_lookup_table,
        &compiler->expression
    );

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
    release_expression(&compiler->expression);
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

    // "./test/class-decl-1.txt",
    // "./test/interface-decl-1.txt",

    "./test/general-no-block-and-statement.txt",
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
    }

    release_compiler(&compiler);
    return 0;
}
