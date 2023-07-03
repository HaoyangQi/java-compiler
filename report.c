#include "report.h"

debug_report compiler_debug_report;

void init_debug_report()
{
    // internal
    compiler_debug_report.internal.reserved_words_count = 0;
    compiler_debug_report.internal.reserved_words_lookup_table_size = 0;
    compiler_debug_report.internal.reserved_words_hash_collisions = 0;
    compiler_debug_report.internal.reserved_words_longest_probing = 0;

    // external
    compiler_debug_report.general.language_version = 1;
}
