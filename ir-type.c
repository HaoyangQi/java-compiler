/**
 * Type System & Type Inference
*/

#include "ir.h"
#include "jil.h"

char primitive_type_to_jil_type(java_lexeme_type p)
{
    switch (p)
    {
        case JLT_RWD_BOOLEAN: return JIL_TYPE_BOOL;
        case JLT_RWD_DOUBLE: return JIL_TYPE_DOUBLE;
        case JLT_RWD_BYTE: return JIL_TYPE_BYTE;
        case JLT_RWD_INT: return JIL_TYPE_INT;
        case JLT_RWD_SHORT: return JIL_TYPE_SHORT;
        case JLT_RWD_VOID: return JIL_TYPE_VOID;
        case JLT_RWD_CHAR: return JIL_TYPE_CHAR;
        case JLT_RWD_LONG: return JIL_TYPE_LONG;
        case JLT_RWD_FLOAT: return JIL_TYPE_FLOAT;
        default: return 0x00;
    }
}
