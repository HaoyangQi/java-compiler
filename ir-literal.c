#include "ir.h"
#include "number.h"

#define SELECT_INT_DATA_SIZE(bits) ((bits) == 32 ? IRPV_INTEGER_BIT_32 : IRPV_INTEGER_BIT_64)

/**
 * Token-To-Primitive Routine
 *
 * TODO:
 * JLT_LTR_CHARACTER
 * JLT_LTR_STRING
*/
primitive t2p(java_ir* ir, java_token* t, uint64_t* n)
{
    primitive p = IRPV_MAX;
    char* content;
    number_truncation_status nts = NTS_SUCCESS;

    // find primitive type
    switch (t->type)
    {
        case JLT_RWD_TRUE:
        case JLT_RWD_FALSE:
            p = IRPV_BOOLEAN;
            break;
        case JLT_RWD_NULL:
            p = SELECT_INT_DATA_SIZE(ir->arch->bits);
            break;
        case JLT_LTR_NUMBER:
            switch (t->number.type)
            {
                case JT_NUM_FP_DOUBLE:
                    p = IRPV_PRECISION_DOUBLE;
                    break;
                case JT_NUM_FP_FLOAT:
                    p = IRPV_PRECISION_SINGLE;
                default:
                    p = SELECT_INT_DATA_SIZE(t->number.bits);
                    break;
            }
            break;
        case JLT_LTR_CHARACTER:
            p = IRPV_INTEGER_BIT_U16;
            break;
        default:
            break;
    }

    if (!n || p == IRPV_MAX)
    {
        return p;
    }

    /**
     * sanitize data and issue warning about truncated numbers
     *
     * NOTE: mantissa overflow is perfectly normal as it is
     * the natural flaw of how FP is stored in computers
     * so we ignore that completely
     *
     * NOTE: literal overflow is always non-fatal at compile-
     * time, though it may cause deviation at run-time
    */
    switch (t->type)
    {
        case JLT_RWD_TRUE:
            *n = 1;
            break;
        case JLT_RWD_FALSE:
        case JLT_RWD_NULL:
            *n = 0;
            break;
        case JLT_LTR_NUMBER:
            content = t2s(t);
            nts = s2b(content, t->number.type, n);
            free(content);

            switch (p)
            {
                case IRPV_INTEGER_BIT_8:
                    *n &= 0x7F;
                    if (nts & NTS_OVERFLOW_INT8)
                    {
                        ir_error(ir, JAVA_E_NUMBER_OVERFLOW_INT8);
                    }
                    break;
                case IRPV_INTEGER_BIT_16:
                    *n &= 0x7FFF;
                    if (nts & NTS_OVERFLOW_INT16)
                    {
                        ir_error(ir, JAVA_E_NUMBER_OVERFLOW_INT16);
                    }
                    break;
                case IRPV_INTEGER_BIT_32:
                    *n &= 0x7FFFFFFF;
                    if (nts & NTS_OVERFLOW_INT32)
                    {
                        ir_error(ir, JAVA_E_NUMBER_OVERFLOW_INT32);
                    }
                    break;
                case IRPV_INTEGER_BIT_64:
                    *n &= 0x7FFFFFFFFFFFFFFF;
                    if (nts & NTS_OVERFLOW_INT64)
                    {
                        ir_error(ir, JAVA_E_NUMBER_OVERFLOW_INT64);
                    }
                    break;
                case IRPV_INTEGER_BIT_U16:
                    *n &= 0xFFFF;
                    if (nts & NTS_OVERFLOW_U16)
                    {
                        ir_error(ir, JAVA_E_NUMBER_OVERFLOW_U16);
                    }
                    break;
                case IRPV_PRECISION_SINGLE:
                    if (nts & NTS_OVERFLOW_FP32_EXP)
                    {
                        ir_error(ir, JAVA_E_NUMBER_OVERFLOW_FP32_EXP);
                    }
                    break;
                case IRPV_PRECISION_DOUBLE:
                    if (nts & NTS_OVERFLOW_FP64_EXP)
                    {
                        ir_error(ir, JAVA_E_NUMBER_OVERFLOW_FP64_EXP);
                    }
                    break;
                default:
                    break;
            }
            break;
        case JLT_LTR_CHARACTER:
            break;
        default:
            break;
    }

    return p;
}
