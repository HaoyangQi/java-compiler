#pragma once
#ifndef __COMPILER_FILE_H__
#define __COMPILER_FILE_H__

#include "types.h"

/**
 * Java source file buffer
 *
 * file buffer reader will read file content into memory
 * in binary format
*/
typedef struct _file_buffer
{
    /* file size (in bytes) */
    long size;
    /* file content buffer */
    byte* buf;
    /* buffer cursor of current read position */
    byte* cur;
} file_buffer;

void init_file_buffer(file_buffer* buffer);
void release_file_buffer(file_buffer* buffer);
bool load_source_file(file_buffer* buffer, const char* name);

#endif
