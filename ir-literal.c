#include "ir.h"
#include "big-integer.h"

#define SELECT_INT_DATA_SIZE(bits) ((bits) == 32 ? IRPV_INTEGER_BIT_32 : IRPV_INTEGER_BIT_64)
#define U64_CLEAR_MSB_MASK ((uint64_t)0x7FFFFFFFFFFFFFFF) // ~((uint64_t)1 << 63)

// primitive data bit-length
static const int primitive_bit_length[IRPV_MAX] = { 8, 16, 32, 64, 16, 32, 64, 32 };

// number type to base
static const int number_base[JT_NUM_MAX] = { 10, 16, 8, 2, 10, 10 };

// big integer constants
static const big_integer two_pow_23 = { .raw = "8388608" };
static const big_integer two_pow_52 = { .raw = "4503599627370496" };

static bool __int64_sum_safe(int64_t a, int64_t b)
{
    /**
     * oh, swap without additional variable...
     *
     * uggggggh, no thank you (guess why)
    */
    if (a < b)
    {
        int64_t tmp = a;
        a = b;
        b = tmp;
    }

    return a < 0 || INT64_MAX - a < b;
}

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
 * IEEE 754-2008 Standard
 *
 * Single Precision:
 * sign: 1 bit
 * exponent: 8 bits
 * fraction: 23 bits
 *
 * Double Precision:
 * sign: 1 bit
 * exponent: 11 bits
 * fraction: 52 bits
 *
 * Logic (Big-Number Arithmetic):
 * 1. use big number to store original content in form: p = I * 10^e
 *    where I and 10^e are both big number
 *    denominator = e < 0 : 10^e : 1
 * 2. scaling p to IEEE fraction bit length: I * 2^s * 10^e
 *    scale factor: s
 *    float  => [2^23, 2^24)
 *    double => [2^52, 2^53)
 * 3. still big number, find quotient and remainder of scaled p: q * 10^e + r
 * 4. round q up/down to next integer (hence +/-1)
 *    r < 0.5 * denominator: up
 *    r > 0.5 * denominator: down
 *    r = 0.5 * denominator: down if q is even, up if q is odd
 *    now the q is significand
 * 5. edge case: if q = upper bound (2^24 or 2^53), let q = lower bound,
 *    and s--
 * 6. convert significand to binary: bin(q)
 * 7. cancel scale factor: bin(q) * 2^(-s)
 * 8. use binary scientific notation to simplify: bin(q) contains power of fraction bit length
 *    so: bin(q') * 2^(52|23) * 2(-s) = bin(q') * 2^(52|23 - s)
 *    q' is not useful so no need to maintain that part
 * 9. put together
 *    sign: always 0 (positive)
 *    exponent (signed): 2^(11|8) - 1 + 2^(52|23 - s)
 *    fraction: q - 2^(52|23), and the trailing 52 or 23 bits
*/
static uint64_t __u64_parse_ieee754(java_ir* ir, const char* content, bool single_precision)
{
    size_t len = strlen(content);
    size_t bl_exp = single_precision ? 8 : 11;
    size_t bl_frac = single_precision ? 23 : 52;
    int64_t exp_bias = single_precision ? 127 : 1023; // 2^(bl_exp - 1) - 1
    uint64_t n = 0;

    big_integer I, q, r;
    int64_t exp10 = 0, exp10_part = 0, s;
    size_t i;
    bool success;
    char c;

    init_big_integer(&I, 0);

    // first get I and fraction scale info
    for (i = 0; i < len; i++, exp10--)
    {
        c = content[i];

        // exponent part needs to be parsed differently
        if (c == 'e' || c == 'E')
        {
            break;
        }

        if (c == '.')
        {
            exp10 = 0;
            continue;
        }

        bi_append_digit(&I, c, BI_RAW_GROW_RIGHT);
    }

    // adjust fraction scale factor
    if (exp10 == i && (i >= len || tolower(c) == 'e'))
    {
        exp10 = 0;
    }

    /**
     * if we have exponent part, parse it
     *
     * since 10^e has (e+1) digits, so e is the max length
     * of big integer 10^e, so size_t is sufficient
    */
    if (i < len && tolower(c) == 'e')
    {
        bool exp10_part_neg = false;

        i++;
        c = content[i];

        if (!isdigit(c))
        {
            exp10_part_neg = c == '-';
            i++;
        }

        exp10_part = s2n(content + i, JT_NUM_DEC, &success);

        if (!success)
        {
            ir_error(ir, JAVA_E_PART_EXPONENT_OVERFLOW);
            goto end;
        }

        if (exp10_part_neg)
        {
            exp10_part = -exp10_part;
        }
    }

    /**
     * combine 2 parts of exponent gives us final 10^e
     * TODO: is overflow check correct?
    */
    if (__int64_sum_safe(exp10, exp10_part))
    {
        ir_error(ir, JAVA_E_PART_EXPONENT_OVERFLOW);
        goto end;
    }
    exp10 += exp10_part;

    /**
     * further combination: if e > 0, merge into I
    */
    if (exp10 > 0)
    {
        bi_mulpow10(&I, exp10);
        exp10 = 0;
    }

    /**
     * find s = log2(I)
     * where s if the largest value such that: 2^s <= I
     * therefore: s is number of bits of I in binary
     *
     * if e < 0, we take quotient of I * 10^e
     * to find the logrithm
    */
    if (exp10 != 0)
    {
        big_integer qI, rI;

        bi_divpow10(&I, -exp10, &qI, &rI);
        s = bi_count_bits(&qI, &success);

        release_big_integer(&qI);
        release_big_integer(&rI);
    }
    else
    {
        s = bi_count_bits(&I, &success);
    }

    // give up if log2 is out of bound
    if (!success)
    {
        ir_error(ir, JAVA_E_PART_EXPONENT_OVERFLOW);
        goto end;
    }

    /**
     * calculate scale factor: s = 23|52 - s
    */
    s = (int64_t)bl_frac - s;

    /**
     * further combination: if s > 0, merge into I
    */
    while (s > 0)
    {
        bi_mul2(&I);
        s--;
    }

    /**
     * (q, r) = I * 2^s * 10^e
     *
     * if any of s and e is negative, a division will occur
     * otherwise remainder is 0
     *
     * the order matter here and 2^s will be calculated first
    */

    big_integer scale_denom;
    int64_t __s = s; // we need s later
    init_big_integer(&scale_denom, 1);

    while (__s < 0)
    {
        bi_mul2(&scale_denom);
        __s++;
    }

    if (exp10 < 0)
    {
        bi_mulpow10(&scale_denom, -exp10);
    }

    // division will occur one way or another
    bi_div(&I, &scale_denom, &q, &r);
    release_big_integer(&scale_denom);

    /**
     * TODO: round (q +/- 1)
     *
     * 2*r < denom: down
     * 2*r > denom: up
     * 2*r = denom: down if even, up if odd
    */

    // count bits of significand: q
    __s = bi_count_bits(&q, &success);

    /**
     * TODO:
     *
     * s + __s is a huge cancellation, so is
     * the bound check still necessary here?
    */
    if (!success/* || !__int64_sum_safe(__s, s)*/)
    {
        ir_error(ir, JAVA_E_PART_EXPONENT_OVERFLOW);
        goto end_qr;
    }

    // find IEEE exponent field
    s = s + __s + exp_bias;

    /**
     * exponent range
     *
     * exponent part is an unsigned integer in biased form
     *
     * valid float range: [0, 255] (8 bits)
     * valid double range: [0, 2047] (11 bits)
    */
    if (single_precision)
    {
        if (s < 0 || s > 255)
        {
            ir_error(ir, JAVA_E_PART_EXPONENT_OVERFLOW);
            goto end_qr;
        }
    }
    else if (s < 0 || s > 2047)
    {
        ir_error(ir, JAVA_E_PART_EXPONENT_OVERFLOW);
        goto end_qr;
    }

    // calculate full fraction field: q - 2^(23|52)
    bi_sub(&q, single_precision ? &two_pow_23 : &two_pow_52);

    /**
     * combine everything together
     *
     * sign field: always 0
     * exponent: 8 or 11 bits (unsigned integer)
     * fraction field: trailing 23|52 bits of significand
    */
    n = ((uint64_t)s << bl_frac) | // exponent
        bi_truncate(&q, bl_frac) & // fraction
        U64_CLEAR_MSB_MASK;        // clear sign bit

end_qr:
    release_big_integer(&q);
    release_big_integer(&r);

end:
    release_big_integer(&I);

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
