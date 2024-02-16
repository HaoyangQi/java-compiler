#include <stdio.h>

#include "compiler.h"
#include "hash-table.h"
#include "utils.h"
#include "debug.h"

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

    "./test/simple.txt",

    // "./test/general-no-block-and-statement.txt",

    // "./test/switch-1.txt",

    // "./test/ambiguity-1.txt",
    // "./test/ambiguity-2.txt",

    // "./test/general-1.txt",
    // "./test/general-3.txt",

    // "./test/recovery/pkg-decl-1.txt",
    // "./test/recovery/pkg-decl-2.txt",
    // "./test/recovery/pkg-decl-3.txt",
};

int main(int argc, char* argv[])
{
    architecture arch;
    compiler compiler;
    int num_source_files = ARRAY_SIZE(test_paths);

    /**
     * TODO: architecture info
     * need command line for this
    */
    arch.bits = ARCH_64_BIT;

    // this is heavy, initialize once and retask
    // compiler for every input file
    init_compiler(&compiler);

    debug_report(&compiler);
    // debug_print_reserved_words();
    // debug_print_symbol_table(&compiler.rw_lookup_table);

    for (int i = 0; i < num_source_files; i++)
    {
        printf("\nFile %d: %s\n", i + 1, test_paths[i]);

        if (compile(&compiler, &arch, test_paths[i]))
        {
            debug_ast(&compiler.context);
            debug_ir_on_demand_imports(&compiler.ir);
            debug_ir_global_names(&compiler.ir);
            debug_ir_lookup(&compiler.ir);
            debug_print_definition_pool(&compiler.ir);
        }

        // debug_file_buffer(&compiler.reader);
        // debug_tokenize(&compiler.reader, &compiler.rw_lookup_table);
        compiler_error_format_print(&compiler);
    }

    release_compiler(&compiler);
    return 0;
}
