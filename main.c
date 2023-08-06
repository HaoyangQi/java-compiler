#include <stdio.h>

#include "file.h"
#include "symtbl.h"

#include "debug.h"
#include "report.h"

typedef struct _parser_info
{
    char* source_file_name;
    file_buffer reader;
    java_symbol_table rw_lookup_table;
} parser_info;

bool init_compiler(parser_info* parser, char* source_path)
{
    parser->source_file_name = source_path;

    init_file_buffer(&parser->reader);
    init_symbol_table(&parser->rw_lookup_table);
    init_debug_report();

    if (!load_source_file(&parser->reader, source_path))
    {
        fprintf(stderr, "File failed to load.");
        return false;
    }

    load_language_spec(&parser->rw_lookup_table);

    return true;
}

int main(int argc, char* argv[])
{
    parser_info parser;

    init_compiler(&parser, "./test/number.txt");

    // debug_print_reserved_words();
    // debug_file_buffer(&parser.reader);
    // debug_print_symbol_table(&parser.rw_lookup_table);
    debug_format_report(REPORT_INTERNAL | REPORT_GENERAL);
    debug_tokenize(&parser.reader, &parser.rw_lookup_table);

    release_file_buffer(&parser.reader);
    return 0;
}
