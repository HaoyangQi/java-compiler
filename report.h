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

#else

#define report_reserved_words_count(n)
#define report_reserved_words_lookup_table_size(n)
#define report_reserved_words_hash_collisions(n)
#define report_reserved_words_longest_probing(n)

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
