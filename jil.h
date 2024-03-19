/**
 * This is the public data structures used by il.h
 *
*/

#pragma once
#ifndef __COMPILER_JIL_H__
#define __COMPILER_JIL_H__

#include "types.h"

#define JIL_FILE_SIGNATURE ((uint32_t)0x4A494C00) // file signature "JIL\0"
#define JIL_FILE_VERSION_MAJOR (1)
#define JIL_FILE_VERSION_MINOR (0)

#define JIL_TYPE_BYTE 'B'
#define JIL_TYPE_BOOL 'Z'
#define JIL_TYPE_CHAR 'C'
#define JIL_TYPE_SHORT 'S'
#define JIL_TYPE_INT 'I'
#define JIL_TYPE_FLOAT 'F'
#define JIL_TYPE_LONG 'J'
#define JIL_TYPE_DOUBLE 'D'
#define JIL_TYPE_OBJECT 'L' // {L ClassName ;} an instance of ClassName
#define JIL_TYPE_ARRAY_DIM '[' // one array dimension
#define JIL_TYPE_VOID 'V'

#define JIL_CONSTANT_METADATA 1
#define JIL_CONSTANT_CLASS 2
#define JIL_CONSTANT_FIELD 3
#define JIL_CONSTANT_METHOD 4
#define JIL_CONSTANT_INTERFACE_METHOD 5
#define JIL_CONSTANT_STRING 6
#define JIL_CONSTANT_INTEGER 7
#define JIL_CONSTANT_FLOAT 8
#define JIL_CONSTANT_LONG 9
#define JIL_CONSTANT_DOUBLE 10
#define JIL_CONSTANT_TYPE_AND_NAME 11
#define JIL_CONSTANT_METHOD_HANDLE 12
#define JIL_CONSTANT_METHOD_TYPE 13
#define JIL_CONSTANT_INVOKE_DYNAMIC 14

// typedef struct
// {
//     // signature
//     uint32_t signature;
//     // major version
//     uint16_t major_version;
//     // minor version
//     uint16_t minor_version;
// } jil_file_header;

// typedef struct
// {
//     // access flag
//     uint32_t access_flags;
//     // [FO] constant info of "this" class
//     uint32_t this_class;
//     // [FO] constant info of super class
//     uint32_t super_class;
//     // number of implemented interface
//     uint32_t super_interfaces_count;
//     // [FO] first constant info of interface
//     uint32_t super_interfaces;
//     // number of fields
//     uint32_t fields_count;
//     // [FO] first constant info of field
//     uint32_t fields;
//     // number of methods
//     uint32_t methods_count;
//     // [FO] first constant info of method
//     uint32_t methods;
//     // number of attributes
//     uint32_t attributes_count;
//     // [FO] first attribute info of this class
//     uint32_t attributes;
//     // number of constant info
//     uint32_t constants_count;
//     // [FO] first constant info in this class
//     uint32_t constants;
// } jil_class_header;

#endif
