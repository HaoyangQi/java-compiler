#include "ir.h"

#define SELECT_INT_DATA_SIZE(bits) ((bits) == 32 ? IRPV_INTEGER_BIT_32 : IRPV_INTEGER_BIT_64)

// primitive data bit-length
static const int primitive_bit_length[IRPV_MAX] = { 8, 16, 32, 64, 16, 32, 64, 32 };
// number type to base
static const int number_base[JT_NUM_MAX] = { 10, 16, 8, 2, 10, 10 };

/**
 * Primitive-Token-To-Number Routine
 *
 * primitive string is assumed to be valid without sign
 * primitive type here is the bound of raw number from token
 *
 * special prefix:
 * HEX: length=2 "0x"
 * OCT: length=1 "0"
 * BIN: length=2 "0b"
 *
 * special suffix:
 * LONG: length=1 "l"
 * FLOAT: length=1 "f"
 * DOUBLE: length=1 "d"
 *
 * so we observe the following:
 * 1. a number is always bounded by <= 2 non-digit character
 * 2. character out of the bound does not have ambiguity of value
 * (for prefix there is always 0)
 * 3. the set of valid digits are: isxdigit()
 *
 * therefore going through all characters but skip non-digit ones
 * will cover all cases
*/
static uint64_t pt2n(java_ir* ir, primitive p, java_token* t)
{
    char* content = t2s(t);
    char c;
    size_t len = strlen(content);
    uint64_t n = 0;

    /**
     * TODO: FP numbers are special loop
    */

    /**
     * parse integer number
     *
     * always fill 64 bits as many as possible, check size later
     * power-of-2 bases can use bit-wise to speed up
    */
    if (t->type == JLT_LTR_CHARACTER)
    {
        /**
         * TODO: U16 type conversion
        */
    }
    else if (t->number.type == JT_NUM_BIN)
    {
        for (size_t i = 0; i < len; i++)
        {
            c = content[i];

            // skip non-digit stuff
            if (!isdigit(c))
            {
                continue;
            }

            n <<= 1;

            if (c == '1')
            {
                n |= 1;
            }

            if ((n >> 63) & 1)
            {
                ir_error(ir, JAVA_E_NUMBER_OVERFLOW);
                break;
            }
        }
    }
    else if (t->number.type == JT_NUM_HEX)
    {
        for (size_t i = 0; i < len; i++)
        {
            c = content[i];

            // skip non-digit stuff
            if (!isxdigit(c))
            {
                continue;
            }

            // hex is special: we need to see 4 MSBs
            // to make sure no 1s are in there
            if ((n >> 60) & 0xF)
            {
                ir_error(ir, JAVA_E_NUMBER_OVERFLOW);
                break;
            }

            n <<= 4;

            if (isdigit(c))
            {
                n |= (c - '0');
            }
            else if (isxdigit(c))
            {
                n |= (tolower(c) - 'a');
            }
        }
    }
    else
    {
        // DEC, OCT
        for (size_t i = 0; i < len; i++)
        {
            c = content[i];

            // skip non-digit stuff
            if (!isdigit(c))
            {
                continue;
            }

            uint64_t digit = c - '0';

            // bound check uses signed int64 upper bound because
            // the input does not contain signs
            if (INT64_MAX - n < digit)
            {
                ir_error(ir, JAVA_E_NUMBER_OVERFLOW);
                break;
            }

            n = n * number_base[t->number.type] + digit;
        }
    }

    // check primitive size overflow
    switch (p)
    {
        case IRPV_INTEGER_BIT_8:
        case IRPV_INTEGER_BIT_16:
        case IRPV_INTEGER_BIT_32:
        case IRPV_INTEGER_BIT_64:
            if ((n >> (primitive_bit_length[p] - 1)) & 1)
            {
                ir_error(ir, JAVA_E_NUMBER_OVERFLOW);
            }
            break;
        case IRPV_INTEGER_BIT_U16:
            if ((n >> 16) & 1)
            {
                ir_error(ir, JAVA_E_NUMBER_OVERFLOW);
            }
            break;
        default:
            /**
             * TODO: FP numbers?
            */
            break;
    }

    free(content);
    return n;
}

/**
 * Token-To-Primitive Routine
 *
 * TODO:
 * JLT_RWD_TRUE
 * JLT_RWD_FALSE
 * JLT_RWD_NULL
 * JLT_LTR_NUMBER
 * JLT_LTR_CHARACTER
 * JLT_LTR_STRING
*/
primitive t2p(java_ir* ir, java_token* t, uint64_t* n)
{
    primitive p = IRPV_MAX;

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

    // extract data
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
            *n = pt2n(ir, p, t);
            break;
        case JLT_LTR_CHARACTER:
            break;
        default:
            break;
    }

    return p;
}
