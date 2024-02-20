#include "number.h"

#define FP32_MAN_BIT_LENGTH (23)
#define FP32_EXP_BIT_LENGTH_MASK (0xFF)
#define FP32_MAN_BIT_LENGTH_MASK (0x7FFFFF)
#define FP64_MAN_BIT_LENGTH (52)
#define FP64_EXP_BIT_LENGTH_MASK (0x7FF)
#define FP64_MAN_BIT_LENGTH_MASK (0xFFFFFFFFFFFFF)

#define FP32_ASSEMBLE(exp, man) (                                            \
    (((uint64_t)(exp)) & FP32_EXP_BIT_LENGTH_MASK) << FP32_MAN_BIT_LENGTH) | \
    (((uint64_t)(man)) & FP32_MAN_BIT_LENGTH_MASK                            \
)
#define FP64_ASSEMBLE(exp, man) (                                            \
    (((uint64_t)(exp)) & FP64_EXP_BIT_LENGTH_MASK) << FP64_MAN_BIT_LENGTH) | \
    (((uint64_t)(man)) & FP64_MAN_BIT_LENGTH_MASK                            \
)

#define __number_c2d(c) ((int)((char)(c) - '0'))
#define __number_d2c(d) ((char)((int)(d) + '0'))

typedef struct
{
    char* s;
    size_t len;
    size_t size;
} number;

uint64_t __hex_char_to_half_byte(char c)
{
    if (isdigit(c))
    {
        return __number_c2d(c) & 0xF;
    }
    else if (c >= 'a' & c <= 'z')
    {
        return ((uint64_t)(c - 'a') + 10) & 0xF;
    }
    else if (c >= 'A' & c <= 'Z')
    {
        return ((uint64_t)(c - 'A') + 10) & 0xF;
    }
    else
    {
        // let's hope we will not go this far
        return 0;
    }
}

/**
 * move number to interval
 *
 * we do not use macro min(max(low, n), high) because max() part
 * will be expanded twice
*/
int64_t __number_interval_inclusive(int64_t n, int64_t low, int64_t high)
{
    int64_t l = max(low, n);
    return min(l, high);
}

/**
 * move single-precision FP exponent to biased interal: [0, 255]
*/
int64_t __fp32_exp_biased_interval(int64_t n, number_truncation_status* nts)
{
    if (n <= 0 || n >= 255)
    {
        *nts |= NTS_OVERFLOW_FP32_EXP;
    }

    return __number_interval_inclusive(n, 0, 255);
}

/**
 * move double-precision FP exponent to biased interal: [0, 2047]
*/
int64_t __fp64_exp_biased_interval(int64_t n, number_truncation_status* nts)
{
    if (n <= 0 || n >= 2047)
    {
        *nts |= NTS_OVERFLOW_FP64_EXP;
    }

    return __number_interval_inclusive(n, 0, 2047);
}

/**
 * 32-bit FP mantissa round-up
*/
void __fp32_round_up(int64_t* fp_exp, uint64_t* bin_man_truncated, number_truncation_status* nts)
{
    int64_t exp = *fp_exp;
    uint64_t man = *bin_man_truncated;

    man = (man + 1) & FP32_MAN_BIT_LENGTH_MASK;

    if (man == 0)
    {
        // if mantissa overflows, add exponent by 1
        exp++;

        /**
         * since addition does not underflow, and oveflow into upper bound
         * is, by definition, Inf, so we simply cap the value here
        */
        *fp_exp = __fp32_exp_biased_interval(exp, nts);
    }

    *bin_man_truncated = man;
}

/**
 * 64-bit FP mantissa round-up
*/
void __fp64_round_up(int64_t* fp_exp, uint64_t* bin_man_truncated, number_truncation_status* nts)
{
    int64_t exp = *fp_exp;
    uint64_t man = *bin_man_truncated;

    man = (man + 1) & FP64_MAN_BIT_LENGTH_MASK;

    if (man == 0)
    {
        // if mantissa overflows, add exponent by 1
        exp++;

        /**
         * since addition does not underflow, and oveflow into upper bound
         * is, by definition, Inf, so we simply cap the value here
        */
        *fp_exp = __fp64_exp_biased_interval(exp, nts);
    }

    *bin_man_truncated = man;
}

static void init_number(number* n, size_t sz)
{
    sz = max(sz, 1);

    n->s = (char*)malloc_assert(sizeof(char) * sz);
    n->size = sz;
    n->len = 0;
}

static void release_number(number* n)
{
    free(n->s);
}

static void number_print(const number* n)
{
    if (n->len == 0)
    {
        printf("(null)");
        return;
    }

    for (size_t i = 0; i < n->len; i++)
    {
        printf("%c", (char)(n->s[i]));
    }
}

static void number_reserve(number* n, size_t sz)
{
    n->s = (char*)realloc_assert(n->s, sizeof(char) * sz);
    n->size = sz;
}

static void number_copy(number* dest, const number* src)
{
    memcpy(dest, src, sizeof(number));
    dest->s = (char*)malloc_assert(sizeof(char) * dest->size);
    memcpy(dest->s, src->s, sizeof(char) * dest->size);
}

static bool number_zero(const number* n)
{
    return n->len == 0;
}

static void number_append_char(number* n, char c)
{
    size_t old_size = n->size;

    // yes this is dumb, but let's keep it this way
    while (n->len + 1 > n->size)
    {
        n->size *= 2;
    }

    if (n->size > old_size)
    {
        n->s = (char*)realloc_assert(n->s, sizeof(char) * n->size);
    }

    if (number_zero(n))
    {
        n->s[0] = c;
    }
    else
    {
        n->s[n->len] = c;
    }

    n->len++;
}

static void number_append_char16(number* n, char hi, char lo)
{
    size_t old_size = n->size;

    // yes this is dumb, but let's keep it this way
    while (n->len + 2 > n->size)
    {
        n->size *= 2;
    }

    if (n->size > old_size)
    {
        n->s = (char*)realloc_assert(n->s, sizeof(char) * n->size);
    }

    if (number_zero(n))
    {
        n->s[0] = hi;
        n->s[1] = lo;
    }
    else
    {
        n->s[n->len] = hi;
        n->s[n->len + 1] = lo;
    }

    n->len += 2;
}

static void number_append_digit(number* n, unsigned int d)
{
    number_append_char(n, __number_d2c(d));
}

static void number_reverse(number* n)
{
    if (n->len < 2) { return; }

    size_t idx_last = n->len - 1;
    size_t bound = n->len / 2;
    char c;

    for (size_t i = 0, j = 0; i < bound; i++)
    {
        c = n->s[i];
        j = idx_last - i;

        n->s[i] = n->s[j];
        n->s[j] = c;
    }
}

static number* number_concat(number* n1, number* n2)
{
    number* r = (number*)malloc_assert(sizeof(number));
    bool n1_zero = number_zero(n1);
    bool n2_zero = number_zero(n2);

    if (n1_zero && n2_zero)
    {
        init_number(r, 1);
        return r;
    }

    r->len = n1->len + n2->len;
    r->size = sizeof(char) * r->len;
    r->s = (char*)malloc_assert(r->size);

    if (n2_zero)
    {
        memcpy(r->s, n1->s, sizeof(char) * n1->len);
    }
    else if (n1_zero)
    {
        memcpy(r->s, n2->s, sizeof(char) * n2->len);
    }
    else
    {
        memcpy(r->s, n1->s, sizeof(char) * n1->len);
        memcpy(r->s + n1->len, n2->s, sizeof(char) * n2->len);
    }

    return r;
}

/**
 * treat input as a binary number and flush first 64-bit digits
 * into actual binary memory
 *
 * by default, it truncates MSB side of parts, filling MSB bits to 0
 *
 * if inverse is set to true, it truncates LSB side of parts, filling
 * LSB bits to 0
 *
 * e.g. truncates 0011 1110 into 12-bit long result:
 * default: 0000 0011 1110
 * inverse: 0011 1110 0000
 *
 * e.g. truncates 0011 1110 1111 into 4-bit long result:
 * default: 1111
 * inverse: 0011
*/
static uint64_t number_bin_truncate(number* n, bool inverse)
{
    if (number_zero(n)) { return 0; }

    size_t idx_last = n->len - 1;
    size_t bound = min(n->len, 64);
    uint64_t r = 0;
    uint64_t bit;

    for (size_t i = 0; i < bound; i++)
    {
        bit = n->s[inverse ? i : (idx_last - i)] == '0' ? 0 : 1;
        r |= bit << (inverse ? (63 - i) : i);
    }

    return r;
}

/**
 * multiply by 2, returns MSB carry
*/
static int number_mul2(number* n)
{
    if (number_zero(n))
    {
        return 0;
    }

    size_t idx_last = n->len - 1;
    int carry = 0;
    int full;
    char c;
    bool all_digit_zero = true;

    for (size_t i = 0, j = 0; i < n->len; i++)
    {
        j = idx_last - i;

        full = __number_c2d(n->s[j]) * 2 + carry;
        carry = full / 10;
        c = __number_d2c(full % 10);
        n->s[j] = c;

        all_digit_zero = all_digit_zero && c == '0';
    }

    // fix number
    if (all_digit_zero)
    {
        n->len = 0;
    }

    return carry;
}

/**
 * divided by 2, returns remainder
*/
static int number_div2(number* n)
{
    number r;
    int remainder = 0;

    if (number_zero(n))
    {
        return 0;
    }
    else if (n->len == 1)
    {
        switch (n->s[0])
        {
            case '0':
            case '1':
                remainder = __number_c2d(n->s[0]);
                n->len = 0;
                return remainder;
            default:
                break;
        }
    }

    init_number(&r, n->len);

    for (size_t i = 0; i < n->len; i++)
    {
        remainder = remainder * 10 + __number_c2d(n->s[i]);

        // skip leading zeros in quotient
        if (remainder < 2 && r.len == 0)
        {
            continue;
        }

        r.s[r.len] = __number_d2c(remainder / 2);
        remainder %= 2;
        r.len++;
    }

    release_number(n);
    memcpy(n, &r, sizeof(number));

    return remainder;
}

bool number_equal(const number* n1, const number* n2)
{
    if (n1->len == n2->len)
    {
        return n1->len == 0 || memcmp(n1->s, n2->s, sizeof(char) * n1->len) == 0;
    }

    return false;
}

void number_inc(number* n)
{
    size_t idx_last = n->len - 1;
    int carry = 1;
    int full;

    for (size_t i = 0, j = 0; i < n->len; i++)
    {
        if (number_zero(n))
        {
            number_append_char(n, '1');
        }
        else
        {
            j = idx_last - i;

            full = __number_c2d(n->s[j]) + carry;
            carry = full / 10;
            n->s[j] = __number_d2c(full % 10);
        }
    }

    if (carry)
    {
        number_reverse(n);
        number_append_char(n, '1');
        number_reverse(n);
    }
}

/**
 * decimal power-10 calculation
*/
static void number_pow10(number* integer, number* fraction, const number* exp, bool exp_neg)
{
    if (number_zero(exp))
    {
        return;
    }

    number cnt;
    char c;

    // allocate enough length to avoid reallocation
    init_number(&cnt, exp->len);

    // initialize count to 0
    cnt.s[0] = '0';
    cnt.len = 1;

    while (!number_equal(&cnt, exp))
    {
        c = '0';

        if (exp_neg)
        {
            // integer removed a digit
            if (integer->len)
            {
                c = integer->s[integer->len - 1];
                integer->len--;
            }

            // fraction appends that digit to front
            // only when fraction is not 0 OR appending digit is not 0
            if (!(number_zero(fraction) && c == '0'))
            {
                number_reverse(fraction);
                number_append_char(fraction, c);
                number_reverse(fraction);
            }
        }
        else
        {
            // fraction removed a digit at front
            if (fraction->len)
            {
                c = fraction->s[0];
                number_reverse(fraction);
                fraction->len--;
                number_reverse(fraction);
            }

            // digit appends that digit to end
            number_append_char(integer, c);
        }

        number_inc(&cnt);
    }
}

static number_truncation_status binary_overflow_check(uint64_t n)
{
    number_truncation_status nts = NTS_SUCCESS;

    for (uint64_t i = 0; n != 0 && i < 64; n >>= 1, i++)
    {
        switch (i)
        {
            case 7:
                nts |= NTS_OVERFLOW_INT8;
                break;
            case 8:
                nts |= NTS_OVERFLOW_U8;
                break;
            case 15:
                nts |= NTS_OVERFLOW_INT16;
                break;
            case 16:
                nts |= NTS_OVERFLOW_U16;
                break;
            case 31:
                nts |= NTS_OVERFLOW_INT32;
                break;
            case 32:
                nts |= NTS_OVERFLOW_U32;
                break;
            case 63:
                nts |= NTS_OVERFLOW_INT64;
                break;
            default:
                break;
        }
    }

    return nts;
}

/**
 * power-of-2 base string to binary
 *
 * bin_base = log2(base)
 * 1          2
 * 3          8
 * 4          16
 *
 * it reads left to right, from the index
 * that can be contained by 64-bit integer
 *
*/
static number_truncation_status pow2_base_s2b(const char* content, size_t bin_base, binary_data* data)
{
    size_t len = strlen(content);
    number_truncation_status nts = NTS_SUCCESS;
    size_t total_bits = 0;
    uint64_t n = 0;
    char c;
    size_t i = 0;

    // skip suffix
    if (len >= 2)
    {
        if (content[0] == '0')
        {
            i = 1;
        }

        switch (content[1])
        {
            case 'b':
            case 'B':
            case 'x':
            case 'X':
                i = 2;
                break;
            default:
                break;
        }
    }

    // calculate total bits required excluding prefix
    total_bits = (len - i) * bin_base;

    // u64 overflow test
    // if overflows, align index to the highest we can reach
    if (total_bits > 64)
    {
        nts |= NTS_OVERFLOW_U64 | NTS_OVERFLOW_INT64;
        i = len - 64 / bin_base;
    }
    else if (total_bits == 64)
    {
        // if happen to be 64 bits, no need to adjust index
        // but signed interger will overflow
        nts |= NTS_OVERFLOW_INT64;
    }

    for (; i < len; i++)
    {
        c = content[i];

        if (!isxdigit(c))
        {
            // stop at suffix: no need to continue
            break;
        }

        n = (n << bin_base) | __hex_char_to_half_byte(c);
    }

    data->number = n;

    return nts;
}

/**
 * fast conversion of decimal strings
 *
 * it uses a very rough bound that stays close to maximum of 64-bit unsigned
 * integer
 * max of unsigned 64-bit integer: 18,446,744,073,709,551,615 (20 digits)
 *
 * so the rough upper bound used is: 9,999,999,999,999,999,999 (19 digits)
 * and closer bound will cause the condition become way too complex, considering
 * we only have the string to check the bound
 *
 * if content is not a number that support fast conversion, it is no-op and
 * returns false
 *
 * therefore this methods coverts as far as LONG type decimals
*/
static bool __quick_dec_s2b(const char* content, binary_data* data)
{
    size_t len = strlen(content);
    uint64_t n = 0;

    // this number is carefully selected, see comments above
    if (len > 19)
    {
        return false;
    }

    // this loop should never cause u64 overflow
    for (size_t i = 0; i < len; i++)
    {
        n = n * 10 + __number_c2d(content[i]);
    }

    data->number = n;

    return true;
}

/**
 * String Literal Encode Utility
 *
 * This covers character and string literals
 * It encodes the string in fixed-width 16-bit character
 *
 * pc: points to te first character of the string
 *
*/
static number_truncation_status __string_encode(const char* content, binary_data* data)
{
    number n;
    char* pc = (char*)content;
    bool es = false;
    uint16_t wc;

    // wide char requires 16 bits each
    init_number(&n, 2);
    data->wide_char = false;

    while (*pc)
    {
        if (es)
        {
            // escape-sequence encoder
            switch (*pc)
            {
                case 'b':
                    number_append_char16(&n, 0x00, 0x08);
                    pc++;
                    break;
                case 's':
                    number_append_char16(&n, 0x00, 0x20);
                    pc++;
                    break;
                case 't':
                    number_append_char16(&n, 0x00, 0x09);
                    pc++;
                    break;
                case 'n':
                    number_append_char16(&n, 0x00, 0x0A);
                    pc++;
                    break;
                case 'f':
                    number_append_char16(&n, 0x00, 0x0C);
                    pc++;
                    break;
                case 'r':
                    number_append_char16(&n, 0x00, 0x0D);
                    pc++;
                    break;
                case '"':
                    number_append_char16(&n, 0x00, 0x22);
                    pc++;
                    break;
                case '\'':
                    number_append_char16(&n, 0x00, 0x27);
                    pc++;
                    break;
                case '\\':
                    number_append_char16(&n, 0x00, 0x5C);
                    pc++;
                    break;
                case 'u':
                    wc = 0;
                    // skip header "\uuu..."
                    while (*pc && *pc == 'u') { pc++; }
                    // 4 hexadecimal maximum
                    for (size_t i = 0; i < 4 && *pc && isxdigit(*pc); i++, pc++)
                    {
                        wc = (wc << 4) | __hex_char_to_half_byte(*pc);
                        data->wide_char = i > 1;
                    }
                    // now encode it
                    number_append_char16(&n, (wc & 0xFF00) >> 8, wc & 0x00FF);
                    break;
                case '\r':
                case '\n':
                    // line break escape has no-op
                    break;
                default:
                    if (isdigit(*pc))
                    {
                        wc = 0;

                        /**
                         * octal unicode value
                         *
                         * octal form is a compatibility for ASCII characters only
                         * so the value has ASCII encode range: [0x00, 0xFF]
                         *
                         * maximum 3 octal digits are allowed, otherwise it will not
                         * be included in this escape sequence
                        */
                        for (size_t i = 0; i < 3 && *pc && *pc >= '0' && *pc <= '7'; i++, pc++)
                        {
                            wc = (wc << 3) | __hex_char_to_half_byte(*pc);

                            /**
                             * before we consume this one, we need to test
                             * if exceeds the range, if so, we cancel it
                             * and break so pc will not increament
                            */
                            if (wc >> 8) // wc > 255
                            {
                                wc >>= 3;
                                break;
                            }
                        }

                        // now encode it
                        number_append_char16(&n, 0x00, wc);
                    }
                    // we assume input is valid, so no error process here
                    break;
            }

            // escape sequence ends
            es = false;
        }
        else
        {
            switch (*pc)
            {
                case '\\':
                    // start escape sequence
                    es = true;
                    break;
                case '\r':
                case '\n':
                case '\'':
                case '"':
                    // line break and enclosure symbols are no-op
                    break;
                default:
                    // directly translation
                    number_append_char16(&n, 0x00, *pc);
                    break;
            }

            pc++;
        }
    }

    data->stream = n.s;
    data->len = n.len;

    // string stream only needs to detect 'char' type length overflow
    return n.len > 2 ? NTS_OVERFLOW_U16 : NTS_SUCCESS;
}

/**
 * initialize binary data
*/
void init_binary_data(binary_data* data)
{
    data->number = 0;
    data->stream = NULL;
    data->len = 0;
    data->wide_char = false;
}

/**
 * string to binary data utility
 *
 * NOTE: make sure the number string has a valid form, otherwise the
 * behavior is undefined
 *
 * top level: it uses string as a big number, convert it into binary string,
 * and then convert it into binary data
 *
 * for power-of-2 bases: BIN/OCT/HEX, a faster bit-shifting-masking algorithm
 * can go directly from string to binary character by character
 *
 * for decimal (including FP numbers): things are complicated
 * 1. short devision is applied to convert integer to binary string, which
 *    then be truncated into unsigned 64-bit form
 * 2. for FP numbers: integer part is same as integers; while for fractional
 *    part, following algorithm is applied for IEEE754-2008
 *
 * 1. convert fractional part into binary
 * multiple by 2 until mantissa length is reached OR the fraction becomes 1;
 * and in every step, if integer part becomes 1, then grow bit 1 towards LSB;
 * otherwise grow 0 towards LSB
 *
 * 2. convert binary fraction into binary scientific form (normalized form):
 * same idea as scientific form in decimal
 * e.g. 101100.001111000 becomes 1.01100001111000 * 2^5
 * e.g. 0.001111000 becomes 1.111000 * 2^-3
 *
 * 3. in this form, integer part is always 1, and fractional part, after
 *    truncated, will become mantissa
 *    and the exponent would be the exponent in final form
*/
number_truncation_status s2b(const char* content, java_number_type type, binary_data* data)
{
    size_t len_content = strlen(content);
    number_truncation_status nts = NTS_SUCCESS;
    size_t fp_man_len = type == JT_NUM_FP_FLOAT ? FP32_MAN_BIT_LENGTH :
        type == JT_NUM_FP_DOUBLE ? FP64_MAN_BIT_LENGTH : 0;
    size_t fp_exp_bias = type == JT_NUM_FP_FLOAT ? 127 : 1023;
    int64_t fp_exp = 0;
    number part[3]; // [integer, fraction, exponent]
    bool zero[3] = { true, true, true };
    number bin[2];
    number* bin_man;
    uint64_t bin_man_truncated;
    bool fp_single_precision;
    bool dec_exp_neg;
    uint64_t n;
    char c;
    int bit;

    // power-of-2 numbers can use bit-shift to directly convert into binary
    switch (type)
    {
        case JT_NUM_HEX:
            nts = pow2_base_s2b(content, 4, data);
            return nts | binary_overflow_check(data->number);
        case JT_NUM_OCT:
            nts = pow2_base_s2b(content, 3, data);
            return nts | binary_overflow_check(data->number);
        case JT_NUM_BIN:
            nts = pow2_base_s2b(content, 1, data);
            return nts | binary_overflow_check(data->number);
        case JT_NUM_DEC:
            // a fast pathway for small integers
            if (__quick_dec_s2b(content, data))
            {
                // if quick conversion works, we are done
                return binary_overflow_check(data->number);
            }
            break;
        case JT_NUM_MAX:
            // if no number type provided, we convert it into a 16-bit-char string stream
            return __string_encode(content, data);
        default:
            // big DEC and all FP are painful...
            break;
    }

    init_number(&part[0], 1);
    init_number(&part[1], 1);
    init_number(&part[2], 1);
    init_number(&bin[0], 1);
    init_number(&bin[1], 1);

    // sanitize content : 111.222e-333f
    // this loop's behavior is undefined if content is a invalid number
    for (size_t i = 0, pi = 0; i < len_content; i++)
    {
        c = content[i];

        if (c == '.')
        {
            pi = 1;
            continue;
        }
        else if (c == 'e' || c == 'E')
        {
            pi = 2;
            continue;
        }
        else if (c == '-')
        {
            dec_exp_neg = true;
            continue;
        }
        else if (c == '+')
        {
            dec_exp_neg = false;
            continue;
        }
        else if (!isdigit(c))
        {
            // stop before suffix
            break;
        }

        number_append_char(&part[pi], c);
        zero[pi] = zero[pi] && c == '0';
    }

    // fix number
    if (zero[0]) { part[0].len = 0; }
    if (zero[1]) { part[1].len = 0; }
    if (zero[2]) { part[2].len = 0; }

    // apply decimal exponent
    number_pow10(&part[0], &part[1], &part[2], dec_exp_neg);

    // convert interger part to binary
    while (!number_zero(&part[0]))
    {
        number_append_digit(&bin[0], number_div2(&part[0]));
    }

    // reverse the number string to get binary
    number_reverse(&bin[0]);

    // mantissa, or just a DEC number
    if (fp_man_len)
    {
        fp_single_precision = fp_man_len == FP32_MAN_BIT_LENGTH;
        fp_exp = 0;

        /**
         * get binary form of fractional part
         *
         * this part can get really really small so that very first
         * "1" bit  could appear realy late
         *
         * so, in order to get sufficient bits, we firstly accept
         * all leading zeros until appearance of first "1", and
         * assuming this is the integer part in normalized form,
         * hence we need to calculate fp_man_len more
         *
         * on top of this, we need additional 3 bits as GRS bits
         * to resolve rounding
        */
        while (!number_zero(&part[1]))
        {
            bit = number_mul2(&part[1]);
            number_append_digit(&bin[1], bit);

            // leading 1 reached, now we go to next loop
            if (bit == 1) { break; }
        }
        for (size_t i = 0; i < fp_man_len + 3 && !number_zero(&part[1]); i++)
        {
            number_append_digit(&bin[1], number_mul2(&part[1]));
        }

        /**
         * if it is only partially converted, then we've lost precision
         *
         * see... this is why this form will be inaccurate at all times
         * because based on this algorithm, some fractional part will
         * have infinite length of binary form
         *
         * e.g. 0.2 is 0011 repeating infinitely
         *
         * also, in normalized form, excluding interger part "1"
        */
        if (!number_zero(&part[1]))
        {
            nts |= fp_single_precision ? NTS_OVERFLOW_FP32_MAN : NTS_OVERFLOW_FP64_MAN;
        }

        // calculate exponent of binary scientific form (integer part)
        if (bin[0].len)
        {
            fp_exp = bin[0].len - 1;
        }

        // if integer part has no fp shift, we need to shift right until integer part is 1
        if (fp_exp == 0 && bin[1].len)
        {
            // if fraction is 0, fp_exp will not change
            for (size_t i = 0; i < bin[1].len; i++)
            {
                if (bin[1].s[i] == '1')
                {
                    // '1' needs to be in integer part
                    fp_exp = -(int64_t)(i + 1);

                    // now remove all leading '0's
                    // leading '1' will be truncated later
                    number_reverse(&bin[1]);
                    bin[1].len -= i;
                    number_reverse(&bin[1]);
                    break;
                }
            }
        }

        /**
         * fp range check
         *
         * single precision
         * bias: +127
         * range: [-127, 128]
         * special reserves: -127(signed zeros/subnormals), 128(Inf/NaN)
         *
         * double precision
         * bias: +1023
         * range: [-1023, 1024]
         * special reserves: -1023(signed zeros/subnormals), 1024(Inf/NaN)
         *
         * we simply flag it, data will be truncated anyway
         *
        */
        fp_exp = fp_single_precision ?
            __fp32_exp_biased_interval(fp_exp + fp_exp_bias, &nts) :
            __fp64_exp_biased_interval(fp_exp + fp_exp_bias, &nts);

        // get normalized form (with MSB=1)
        bin_man = number_concat(&bin[0], &bin[1]);

        // truncate MSB to get mantissa
        if (bin_man->len > 0)
        {
            number_reverse(bin_man);
            bin_man->len--;
            number_reverse(bin_man);
        }

        // mantissa precision loss check
        if (bin_man->len > fp_man_len)
        {
            nts |= fp_single_precision ? NTS_OVERFLOW_FP32_MAN : NTS_OVERFLOW_FP64_MAN;
        }

        /**
         * truncate to mantissa length (keep high 23/52 bit, so need a right shift)
         *
         * mantissa shift is: 64 - fp_man_len
         * with GRS bits, shift needs reserve 3 bits more: 64 - (fp_man_len + 3)
         *
         * here we also obtain the rounding info: GRS bits
         * G: Ground Bit
         * R: Round Bit
         * S: Sitcky Bit
         *
         * MSB=>| mantissa | G | R | S | ...
         *
         * they are trailing 3 bits after mantissa, and they define the rounding
         * behavior for mantissa
         *
         * so we need at least 23(or 52) + 3 bits for this (fill 0 if not enough)
         *
         * The rounding algorithm applied is: Round Nearest, ties to Even (RNE)
         * for an FP, find 2 closest representable FP value around it. If the
         * value less than half-way, round down, round up otherwise;
         * if values sits right in the middle (tie), round up if odd, round down
         * otherwise
         *
         * in binary: round up means +1, round down means no-op
         *
         * GRS
         * 0xx: round down (no-op)
         * 100: tie, round up if mantissa is odd (LSB=1), round down (no-op) otherwise
         * 1xx: round up
        */
        bin_man_truncated = number_bin_truncate(bin_man, true) >> (61 - fp_man_len);

        // cleanup
        release_number(bin_man);
        free(bin_man);

        /**
         * Assembling Stage
         *
         * Beyond this point, no more big numbers
         *
        */

        // split GRS bits and mantissa bits
        bit = bin_man_truncated & 0x7;
        bin_man_truncated >>= 3;

        // rounding condition 
        // basically round-up condition, because round down is no-op
        c = (bit > 4 || (bit == 4 && (bin_man_truncated & 1)));

        // assemble
        if (zero[0] && // make sure interger part is 0
            fp_exp - fp_exp_bias == 0 && bin_man_truncated == 0)
        {
            // signed zero (exp and mantissa both set to 0)
            // since we process unsigned numbers, so sign bit is always 0, thus +0
            n = 0;
        }
        else if (fp_single_precision)
        {
            // rounding (basically all round-up cases)
            if (c)
            {
                __fp32_round_up(&fp_exp, &bin_man_truncated, &nts);
            }

            n = FP32_ASSEMBLE(fp_exp, bin_man_truncated);
        }
        else
        {
            // rounding (basically all round-up cases)
            if (c)
            {
                __fp64_round_up(&fp_exp, &bin_man_truncated, &nts);
            }

            n = FP64_ASSEMBLE(fp_exp, bin_man_truncated);
        }
    }
    else
    {
        // flag upper bound
        if (bin[0].len > 64) { nts |= NTS_OVERFLOW_U64 | NTS_OVERFLOW_INT64; }

        n = number_bin_truncate(&bin[0], false);
        nts |= binary_overflow_check(n);
    }

    // pass data
    data->number = n;

    // cleanup
    release_number(&part[0]);
    release_number(&part[1]);
    release_number(&part[2]);
    release_number(&bin[0]);
    release_number(&bin[1]);

    return nts;
}
