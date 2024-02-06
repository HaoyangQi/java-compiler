#include "ir.h"

#define SELECT_INT_DATA_SIZE(bits) ((bits) == 32 ? IRPV_INTEGER_BIT_32 : IRPV_INTEGER_BIT_64)

// primitive data bit-length
static const int primitive_bit_length[IRPV_MAX] = { 8, 16, 32, 64, 16, 32, 64, 32 };
// number type to base
static const int number_base[JT_NUM_MAX] = { 10, 16, 8, 2, 10, 10 };

/**
 * safe arithmetic for 64-bit integer: n * base + digit
 *
 * it does not allow overflow to the MSB (signed bit)
 * it returns closest number before the step that overflows
*/
static uint64_t __int64_digits_safe_append(uint64_t n, uint64_t base, uint64_t digits, bool* success)
{
    if (base > 0 && n > INT64_MAX / base)
    {
        if (success) *success = false;
        return n;
    }

    n *= base;

    if (digits > 0 && (INT64_MAX - (int64_t)n < (int64_t)digits))
    {
        if (success) *success = false;
        return n;
    }

    if (success) *success = true;

    return n + digits;
}

/**
 * generic string-to-number routine
 *
 * it skips first non-digit character, and stops at the second one
*/
static uint64_t s2n(const char* s, java_number_type type, bool* success)
{
    size_t len = strlen(s);
    uint64_t n = 0;
    char c;
    bool accepting = false;

    if (success) *success = true;

    for (size_t i = 0; (!success || *success) && i < len; i++)
    {
        c = s[i];

        // skip non-digit stuff
        if (!isxdigit(c))
        {
            if (accepting)
            {
                break;
            }
            else
            {
                accepting = true;
                continue;
            }
        }

        uint64_t digit = c - '0';
        int base = number_base[type];

        // append digit with bound check
        n = __int64_digits_safe_append(n, number_base[type], c - '0', success);
    }

    return n;
}

/**
 * TODO:Char Conversion
*/
static uint64_t __u64_parse_char(java_ir* ir, const char* content)
{
    size_t len = strlen(content);
    uint64_t n = 0;

    return n;
}

/**
 * Binary Conversion
*/
static uint64_t __u64_parse_bin(java_ir* ir, const char* content)
{
    size_t len = strlen(content);
    uint64_t n = 0;
    char c;

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

    return n;
}

/**
 * Headecimal Conversion
*/
static uint64_t __u64_parse_hex(java_ir* ir, const char* content)
{
    size_t len = strlen(content);
    uint64_t n = 0;
    char c;

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

    return n;
}

/**
 * Generic conversion
*/
static uint64_t __u64_parse_generic(java_ir* ir, const char* content, java_number_type type)
{
    bool success;
    uint64_t n = 0;

    n = s2n(content, type, &success);

    if (!success)
    {
        ir_error(ir, JAVA_E_NUMBER_OVERFLOW);
    }

    return n;
}

/**
 * IEEE 754 Standard
 *
 * Single Precision:
 * sign: 1 bit
 * exponent: 8 bits
 * mantissa: 23 bits
 *
 * Double Precision:
 * sign: 1 bit
 * exponent: 11 bits
 * mantissa: 52 bits
*/
static uint64_t __u64_parse_ieee754(java_ir* ir, const char* content, bool single_precision)
{
    size_t len = strlen(content);
    size_t bl_exp = single_precision ? 8 : 11;
    size_t bl_man = single_precision ? 23 : 52;
    uint64_t n = 0;
    bool success;

    /**
     * 1. find decimal place in string
     *
     * because of the exponent part, the decimal place may move
     *
     * if it is negative, it means more leading 0's needs to be filled
     * in decimal part
     *
     * assumption: there is only at most one '.', at most one 'e'
    */
    int64_t idx_decimal = len;
    int64_t decimal_shift = 0;
    success = true;
    for (size_t i = 0; success && i < len; i++)
    {
        switch (content[i])
        {
            case '.':
                // reach decimal
                idx_decimal = i;
                i++;

                // start skipping until end or exponent
                for (; i < len; i++)
                {
                    if (tolower(content[i]) == 'e')
                    {
                        break;
                    }
                }

                // make sure next is 'e'
                i--;
                continue;
            case 'e':
            case 'E':
                // reach exponent part, read it
                decimal_shift = s2n(content + idx_decimal + 1, JT_NUM_DEC, &success);

                if (!success)
                {
                    ir_error(ir, JAVA_E_PART_EXPONENT_OVERFLOW);
                }

                if (content[idx_decimal + 1] == '-')
                {
                    decimal_shift = -decimal_shift;
                }

                break;
            default:
                break;
        }
    }

    /**
     * 2. calculate actual decimal index
     *
     * idx_decimal <= 0 means:
     * 1. no whole number part, AND
     * 2. there are abs(idx_decimal) number of additioanl leading 0s
     *    in decimal part
     *
     * idx_decimal >= len means:
     * 1. no decimal part
     * 2. whole part needs to times 10^(idx_decimal - len)
    */
    idx_decimal -= decimal_shift;

    /**
     * 2. convert whole number part
     *
     * for the positive part of idx_decimal - len:
     * the whole part expands whole * 10^(idx_decimal - len)
    */
    uint64_t part_whole = 0;
    if (idx_decimal > 0)
    {
        part_whole = s2n(content, JT_NUM_DEC, &success);

        for (int64_t i = idx_decimal - len; success && i > 0; i++)
        {
            part_whole = __int64_digits_safe_append(part_whole, JT_NUM_DEC, 0, &success);
        }

        if (!success)
        {
            ir_error(ir, JAVA_E_PART_WHOLE_OVERFLOW);
        }
    }

    /**
     * 3. convert decimal part
     * TODO:
    */
    uint64_t part_decimal = 0;
    if (idx_decimal < len)
    {
    }

    /**
     * TODO: n bound check
    */

    return n;
}

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
 * so we observe the following (FP numbers excluded):
 * 1. a number is always bounded by <= 2 non-digit character
 * 2. character out of the bound does not have ambiguity of value
 * (for prefix there is always 0)
 * 3. the set of valid digits are: isxdigit()
 *
 * therefore going through all characters but skip non-digit ones
 * will cover all integer cases
 *
 * For FP numbers, we follow IEEE-754 and specially detect decimal
 * and exponent part
*/
static uint64_t pt2n(java_ir* ir, primitive p, java_token* t)
{
    char* content = t2s(t);
    uint64_t n = 0;

    /**
     * parse integer number
     *
     * always fill 64 bits as many as possible, check size later
     * power-of-2 bases can use bit-wise to speed up
    */
    if (t->type == JLT_LTR_CHARACTER)
    {
        n = __u64_parse_char(ir, content);
    }
    else
    {
        switch (t->number.type)
        {
            case JT_NUM_BIN:
                n = __u64_parse_bin(ir, content);
                break;
            case JT_NUM_HEX:
                n = __u64_parse_hex(ir, content);
                break;
            case JT_NUM_FP_FLOAT:
                n = __u64_parse_ieee754(ir, content, true);
                break;
            case JT_NUM_FP_DOUBLE:
                n = __u64_parse_ieee754(ir, content, false);
                break;
            case JT_NUM_DEC:
            case JT_NUM_OCT:
                n = __u64_parse_generic(ir, content, t->number.type);
                break;
            default:
                // should never reach here
                n = 0;
                break;
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
