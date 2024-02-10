#include "compiler.h"

// JEL_* translation
static const char* error_level_map[] = {
    "(undefine)",
    "note",
    "warning",
    "error",
};

// JES_* translation
static const char* error_scope_map[] = {
    "(UNDEFINED)",
    "IN",
    "RU",
    "LE",
    "SY",
    "CO",
    "OP",
    "LI",
    "BU",
};

/**
 * Full initialization of compiler instance
 *
 * initialization order matters here!
*/
bool init_compiler(compiler* compiler)
{
    compiler->version = 1;

    // special global instance
    init_error(&compiler->error);

    init_file_buffer(&compiler->reader, &compiler->error);
    init_symbol_table(&compiler->rw_lookup_table);
    init_expression(&compiler->expression);

    init_parser(
        &compiler->context,
        &compiler->reader,
        &compiler->rw_lookup_table,
        &compiler->expression,
        &compiler->error
    );

    init_ir(&compiler->ir, &compiler->expression, &compiler->error);

    return true;
}

/**
 * Release compiler instance
*/
void release_compiler(compiler* compiler)
{
    release_file_buffer(&compiler->reader);
    release_symbol_table(&compiler->rw_lookup_table);
    release_expression(&compiler->expression);
    release_error(&compiler->error);
    release_parser(&compiler->context, false);
    release_ir(&compiler->ir);
}

/**
 * Detach source-file-specific context from compiler
*/
void detask_compiler(compiler* compiler)
{
    release_file_buffer(&compiler->reader);
    clear_error(&compiler->error);
    release_parser(&compiler->context, false);
    release_ir(&compiler->ir);
}

/**
 * Retask compiler to a new source file
*/
bool retask_compiler(compiler* compiler, char* source_path)
{
    detask_compiler(compiler);

    // relink all references
    compiler->source_file_name = source_path;
    init_file_buffer(&compiler->reader, &compiler->error);
    init_parser(
        &compiler->context,
        &compiler->reader,
        &compiler->rw_lookup_table,
        &compiler->expression,
        &compiler->error
    );
    init_ir(&compiler->ir, &compiler->expression, &compiler->error);

    /**
     * load file last
     *
     * this is important because load_source_file may
     * generate error, and once error occured, retask
     * needs to exit immediately
    */
    return load_source_file(&compiler->reader, source_path);
}

/**
 * Compiler Entry Point
*/
bool compile(compiler* compiler, architecture* arch, char* source_path)
{
    if (!retask_compiler(compiler, source_path))
    {
        return false;
    }

    parse(&compiler->context);

    // check error from parser
    if (error_count(&compiler->error, JEL_ERROR) > 0)
    {
        return false;
    }

    contextualize(&compiler->ir, arch, compiler->context.ast_root);

    // check error from parser
    if (error_count(&compiler->error, JEL_ERROR) > 0)
    {
        return false;
    }

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
    // <error level> <error code>: 
    static char* msg_header_plain = "%s %s%04d: ";
    // <file name>: <error level> <error code>: 
    static char* msg_header_no_line_info = "%s: %s %s%04d: ";
    // <file name>:<ln>:<col>: <error level> <error code>: 
    static char* msg_header_full = "%s:%zd:%zd: %s %s%04d: ";

    java_error* error = &compiler->error;
    java_error_entry* cur = error->data;
    error_definiton def, level, scope;

    while (cur)
    {
        def = error->definition[cur->id];
        level = def & ERR_DEF_MASK_LEVEL;
        scope = def & ERR_DEF_MASK_SCOPE;

        // print header
        switch (scope)
        {
            case JES_INTERNAL:
            case JES_RUNTIME:
                // internal or runtime errors are too premature so 
                // file info will not be displayed by default
                fprintf(stderr, msg_header_plain,
                    error_level_map[JEL_TO_INDEX(level)],
                    error_scope_map[scope],
                    cur->id
                );
                break;
            case JES_LEXICAL:
            case JES_SYNTAX:
            case JES_CONTEXT:
                // only parsing phase requires line info
                fprintf(stderr, msg_header_full,
                    compiler->source_file_name,
                    cur->ln,
                    cur->col,
                    error_level_map[JEL_TO_INDEX(level)],
                    error_scope_map[scope],
                    cur->id
                );
                break;
            default:
                // otherwise we show everything except line info
                fprintf(stderr, msg_header_no_line_info,
                    compiler->source_file_name,
                    error_level_map[JEL_TO_INDEX(level)],
                    error_scope_map[scope],
                    cur->id
                );
                break;
        }

        // now print message
        // some of them may need format print
        switch (cur->id)
        {
            case JAVA_E_FILE_OPEN_FAILED:
            case JAVA_E_FILE_SIZE_NOT_MATCH:
                fprintf(stderr, error->message[cur->id], compiler->source_file_name);
                break;
            default:
                fprintf(stderr, error->message[cur->id]);
                break;
        }

        /**
         * TODO: print snapshot content for parsing errors
        */
        fprintf(stderr, "\n");

        cur = cur->next;
    }
}
