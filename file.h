#pragma once
#ifndef __COMPILER_FILE_H__
#define __COMPILER_FILE_H__

#include "types.h"
#include "error.h"

/**
 * Java source file buffer
 *
 * file buffer reader will read file content into memory
 * in binary format
 *
 * error logger is external reference, no release required
*/
typedef struct _file_buffer
{
    /* file size (in bytes) */
    long size;
    /* file content buffer */
    byte* base;
    /* buffer boundary */
    byte* limit;
    /* buffer cursor of current read position */
    byte* cur;
    /* error logger */
    java_error_logger* logger;
} file_buffer;

void init_file_buffer(file_buffer* buffer, java_error_logger* error_logger);
void release_file_buffer(file_buffer* buffer);
bool load_source_file(file_buffer* buffer, const char* name);

bool is_eof(file_buffer* buffer);
void buffer_error(file_buffer* buffer, java_error_id id, ...);
bool buffer_ptr_safe_move(file_buffer* buffer);
byte buffer_peek(file_buffer* buffer, int offset);
size_t buffer_count(const byte* from, const byte* to);
void buffer_substring(byte* dest, const byte* from, size_t len);

#endif
