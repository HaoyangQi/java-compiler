#include "compiler.h"

#include "debug.h"
#include "report.h"

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
    init_error(&compiler->error);

    load_language_spec(&compiler->rw_lookup_table);
    init_parser(
        &compiler->context,
        &compiler->reader,
        &compiler->rw_lookup_table,
        &compiler->expression,
        &compiler->error
    );

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
    release_error(&compiler->error);
    release_parser(&compiler->context, false);
}

/**
 * Detach source-file-specific context from compiler
*/
bool detask_compiler(compiler* compiler)
{
    if (compiler->tasked)
    {
        release_file_buffer(&compiler->reader);
        clear_error(&compiler->error);
        release_parser(&compiler->context, false);
        compiler->tasked = false;

        return true;
    }

    return false;
}

/**
 * format print file loader error message
*/
static void format_file_loader_message(file_loader_status status)
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
        &compiler->expression,
        &compiler->error
    );

    compiler->tasked = true;
    return true;
}

/**
 * Print error stack
 *
 * TODO: format?
 * MSVC: <file path>(<ln>,<col>): <error level> <error code>: <error message>
 * GCC: <file name>:<ln>:<col>: <error level>: <error message> <snap shot>
 * JAVA: <file name>:<ln> <error level>: <error message> <snap shot>
*/
void compiler_error_format_print(compiler* compiler)
{
    java_error* error = &compiler->error;
    java_error_entry* cur = error->data;
    error_definiton def;

    while (cur)
    {
        def = error->definition[cur->id];

        // file path:
        fprintf(stderr, "%s:%zd:%zd: ", compiler->source_file_name, cur->ln, cur->col);

        // print error level
        switch (def & ERR_DEF_MASK_LEVEL)
        {
            case JEL_INFORMATION:
                fprintf(stderr, "info");
                break;
            case JEL_WARNING:
                fprintf(stderr, "warning");
                break;
            case JEL_ERROR:
                fprintf(stderr, "error");
                break;
            default:
                fprintf(stderr, "(unknown error level)");
                break;
        }

        fprintf(stderr, " ");

        // print error code header
        switch (def & ERR_DEF_MASK_SCOPE)
        {
            case JES_INTERNAL:
                fprintf(stderr, "IN");
                break;
            case JES_RUNTIME:
                fprintf(stderr, "RU");
                break;
            case JES_LEXICAL:
                fprintf(stderr, "LE");
                break;
            case JES_SYNTAX:
                fprintf(stderr, "SY");
                break;
            case JES_CONTEXT:
                fprintf(stderr, "CO");
                break;
            case JES_OPTIMIZATION:
                fprintf(stderr, "OP");
                break;
            case JES_LINKER:
                fprintf(stderr, "LI");
                break;
            case JES_BUILD:
                fprintf(stderr, "BU");
                break;
        }

        // print error code
        fprintf(stderr, "%04d: ", cur->id);

        // message
        fprintf(stderr, "%s\n", error->message[cur->id]);

        cur = cur->next;
    }
}
