#pragma once
#ifndef __COMPILER_REPORT_H__
#define __COMPILER_REPORT_H__

#include "types.h"

#define REPORT_INTERNAL 0x1
#define REPORT_GENERAL 0x2

#ifdef DEBUG

#define report_reserved_words_count(n) \
    compiler_debug_report.internal.reserved_words_count = (n)
#define report_reserved_words_lookup_table_size(n) \
    compiler_debug_report.internal.reserved_words_lookup_table_size = (n)
#define report_reserved_words_hash_collisions(n) \
    compiler_debug_report.internal.reserved_words_hash_collisions = (n)
#define report_reserved_words_longest_probing(n) \
    compiler_debug_report.internal.reserved_words_longest_probing = \
    max(compiler_debug_report.internal.reserved_words_longest_probing, (n))
#define report_expression_static_data_size(n) \
    compiler_debug_report.internal.expression_static_data_size = (n)
#define report_error_definition_size(n) \
    compiler_debug_report.internal.error_definition_size = (n)
#define report_error_message_size(n) \
    compiler_debug_report.internal.error_message_size = (n)

#else

#define report_reserved_words_count(n)
#define report_reserved_words_lookup_table_size(n)
#define report_reserved_words_hash_collisions(n)
#define report_reserved_words_longest_probing(n)
#define report_expression_static_data_size(n)
#define report_error_definition_size(n)
#define report_error_message_size(n)

#endif

/**
 * internal runtime info of compiler
 *
 * * initialization
 * * TODO: more
*/
typedef struct _debug_report_internal
{
    /* #words reserved */
    size_t reserved_words_count;
    /* size of hash table of reserved words */
    size_t reserved_words_lookup_table_size;
    /* #collisions during direct hash of reserved words */
    size_t reserved_words_hash_collisions;
    /* max #probings needed for reserved word lookup */
    size_t reserved_words_longest_probing;
    /* expression static data size */
    size_t expression_static_data_size;
    /* error definition static data size */
    size_t error_definition_size;
    /* error message static data size */
    size_t error_message_size;
} debug_report_internal;

/**
 * general runtime info of compiler
 *
 * * timing
 * * TODO: more
*/
typedef struct _debug_report_general
{
    size_t language_version;
} debug_report_general;

/**
 * runtime summary info of compiler
*/
typedef struct _debug_report
{
    debug_report_general general;
    debug_report_internal internal;
} debug_report;

extern debug_report compiler_debug_report;

void init_debug_report();

#endif
