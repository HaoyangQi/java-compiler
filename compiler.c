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

    // static data: init only once
    init_symbol_table(&compiler->rw_lookup_table);
    init_expression(&compiler->expression);

    // high-priority instance
    init_error_logger(&compiler->logger);

    // compiler framework
    init_file_buffer(&compiler->reader, &compiler->logger);
    init_lexer(
        &compiler->lexer,
        &compiler->reader,
        &compiler->rw_lookup_table,
        &compiler->logger
    );
    init_parser(
        &compiler->context,
        &compiler->lexer,
        &compiler->rw_lookup_table,
        &compiler->expression,
        &compiler->logger
    );
    init_ir(&compiler->ir, &compiler->expression, &compiler->logger);

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
    release_error_logger(&compiler->logger);
    release_parser(&compiler->context);
    release_ir(&compiler->ir);
}

/**
 * Detach source-file-specific context from compiler
*/
void detask_compiler(compiler* compiler)
{
    release_file_buffer(&compiler->reader);
    clear_error_logger(&compiler->logger);
    release_lexer(&compiler->lexer);
    release_parser(&compiler->context);
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
    init_file_buffer(&compiler->reader, &compiler->logger);
    init_lexer(
        &compiler->lexer,
        &compiler->reader,
        &compiler->rw_lookup_table,
        &compiler->logger
    );
    init_parser(
        &compiler->context,
        &compiler->lexer,
        &compiler->rw_lookup_table,
        &compiler->expression,
        &compiler->logger
    );
    init_ir(&compiler->ir, &compiler->expression, &compiler->logger);

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
bool compile(compiler* compiler, architecture* arch, char* source_path, compiler_stage stages)
{
    if (!retask_compiler(compiler, source_path))
    {
        return false;
    }
    else if (stages < COMPILER_STAGE_PARSE)
    {
        return true;
    }

    parse(&compiler->context);

    // check error from parser
    if (!error_logger_if_main_stack_no_error(&compiler->logger))
    {
        return false;
    }
    else if (stages < COMPILER_STAGE_CONTEXT)
    {
        return true;
    }

    contextualize(&compiler->ir, arch, compiler->context.ast_root);

    // check error from contextualizer
    if (!error_logger_if_main_stack_no_error(&compiler->logger))
    {
        return false;
    }
    else if (stages < COMPILER_STAGE_EMIT)
    {
        return true;
    }

    // emit IR
    jil_emit(&compiler->ir);

    /**
     * TODO:
     * also: before we return, we need to check if error stack
     * has ambiguity left. if so, then it is an error
    */

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

    java_error_logger* logger = &compiler->logger;
    java_error_entry* cur = logger->main_stream.first;
    java_error_id id;
    error_type def, level, scope;

    while (cur)
    {
        id = cur->id;
        def = logger->def[id].descriptor;
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
                    id
                );
                break;
            case JES_LEXICAL:
            case JES_SYNTAX:
            case JES_CONTEXT:
                // only parsing phase requires line info
                fprintf(stderr, msg_header_full,
                    compiler->source_file_name,
                    cur->begin.ln,
                    cur->begin.col,
                    error_level_map[JEL_TO_INDEX(level)],
                    error_scope_map[scope],
                    id
                );
                break;
            default:
                // otherwise we show everything except line info
                fprintf(stderr, msg_header_no_line_info,
                    compiler->source_file_name,
                    error_level_map[JEL_TO_INDEX(level)],
                    error_scope_map[scope],
                    id
                );
                break;
        }

        // now print message
        fprintf(stderr, cur->msg ? cur->msg : logger->def[id].message);

        /**
         * TODO: print snapshot content for parsing errors
        */
        fprintf(stderr, "\n");

        cur = cur->next;
    }
}
