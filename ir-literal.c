#include "ir.h"

#define SELECT_INT_DATA_SIZE(bits) ((bits) == 32 ? IRPV_INTEGER_BIT_32 : IRPV_INTEGER_BIT_64)

/**
 * sanitize binary content using its primitive type AND
 * issue warning using its overflow flag
*/
static uint64_t __bin_sanitize(java_ir* ir, uint64_t n, primitive p, number_truncation_status nts)
{
    switch (p)
    {
        case IRPV_INTEGER_BIT_8:
            n &= 0x7F;
            if (nts & NTS_OVERFLOW_INT8)
            {
                ir_error(ir, JAVA_E_NUMBER_OVERFLOW_INT8);
            }
            break;
        case IRPV_INTEGER_BIT_16:
            n &= 0x7FFF;
            if (nts & NTS_OVERFLOW_INT16)
            {
                ir_error(ir, JAVA_E_NUMBER_OVERFLOW_INT16);
            }
            break;
        case IRPV_INTEGER_BIT_32:
            n &= 0x7FFFFFFF;
            if (nts & NTS_OVERFLOW_INT32)
            {
                ir_error(ir, JAVA_E_NUMBER_OVERFLOW_INT32);
            }
            break;
        case IRPV_INTEGER_BIT_64:
            n &= 0x7FFFFFFFFFFFFFFF;
            if (nts & NTS_OVERFLOW_INT64)
            {
                ir_error(ir, JAVA_E_NUMBER_OVERFLOW_INT64);
            }
            break;
        case IRPV_INTEGER_BIT_U16:
            n &= 0xFFFF;
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

    return n;
}

/**
 * Raw-Source-To-Primitive Routine
 *
 * data->number will be filled if it is a number
 * data->stream will be filled if it is a character or string
 *
 * for character: u16 overflow bit will be set if the token
 * contains more then 2 bytes.
*/
primitive r2p(
    java_ir* ir,
    const char* content,
    binary_data* data,
    java_lexeme_type token_type,
    java_number_type num_type,
    java_number_bit_length num_bits
)
{
    primitive p = IRPV_MAX;
    number_truncation_status nts = NTS_SUCCESS;

    init_binary_data(data);

    // find primitive type
    switch (token_type)
    {
        case JLT_RWD_TRUE:
        case JLT_RWD_FALSE:
            p = IRPV_BOOLEAN;
            break;
        case JLT_RWD_NULL:
            p = SELECT_INT_DATA_SIZE(ir->arch->bits);
            break;
        case JLT_LTR_NUMBER:
            switch (num_type)
            {
                case JT_NUM_FP_DOUBLE:
                    p = IRPV_PRECISION_DOUBLE;
                    break;
                case JT_NUM_FP_FLOAT:
                    p = IRPV_PRECISION_SINGLE;
                default:
                    p = SELECT_INT_DATA_SIZE(num_bits);
                    break;
            }
            break;
        case JLT_LTR_CHARACTER:
            p = IRPV_INTEGER_BIT_U16;
            break;
        default:
            break;
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
    switch (token_type)
    {
        case JLT_RWD_TRUE:
            data->number = 1;
            break;
        case JLT_RWD_FALSE:
        case JLT_RWD_NULL:
            data->number = 0;
            break;
        case JLT_LTR_NUMBER:
            nts = s2b(content, num_type, data);
            data->number = __bin_sanitize(ir, data->number, p, nts);
            break;
        case JLT_LTR_CHARACTER:
            // get binary stream
            nts = s2b(content, JT_NUM_MAX, data);
            data->number = 0;
            // get 16 bits from stream
            switch (min(2, data->len))
            {
                case 2: data->number |= ((uint64_t)(data->stream[1])) << 8;
                case 1: data->number |= ((uint64_t)(data->stream[0]));
                default: break;
            }
            data->number = __bin_sanitize(ir, data->number, p, nts);
            // cleanup
            free(data->stream);
            data->stream = NULL;
            break;
        case JLT_LTR_STRING:
            // keep the entire stream
            s2b(content, JT_NUM_MAX, data);
            break;
        default:
            break;
    }

    return p;
}

/**
 * Token-To-Primitive Routine
 *
 * data->number will be filled if token is a number
 * data->stream will be filled if token is a character or string
 *
 * for character: u16 overflow bit will be set if the token
 * contains more then 2 bytes.
*/
primitive t2p(java_ir* ir, java_token* t, binary_data* data)
{
    char* content = t2s(t);
    primitive p = r2p(ir, content, data, t->type, t->number.type, t->number.bits);
    free(content);

    return p;
}
