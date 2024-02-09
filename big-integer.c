#include "big-integer.h"

#define __bi_c2d(c) ((int)((char)(c) - '0'))
#define __bi_d2c(d) ((char)((d) + '0'))

static __bi_grow(big_integer* bn, size_t by, bi_grow grow)
{
    size_t len = strlen(bn->raw);
    size_t sz = sizeof(char) * (len + by + 1);
    char* n = (char*)malloc_assert(sz);

    memset(n, 0, sz);

    // move it
    if (grow == BI_RAW_GROW_LEFT)
    {
        memcpy(n + by, bn->raw, sizeof(char) * len);
    }
    else
    {
        memcpy(n, bn->raw, sizeof(char) * len);
    }

    // fill '0' to make sense out of it
    for (size_t f = 0; f < by; f++)
    {
        n[len + f] = '0';
    }

    // terminate
    n[len + by] = 0;

    free(bn->raw);
    bn->raw = n;
}

/**
 * return most significant digit index
 *
 * if number is 0, index returned is length of number
*/
size_t __bi_idx_msb(const big_integer* b)
{
    size_t len = strlen(b->raw);
    size_t i;

    // skip leading 0's
    for (i = 0; i < len; i++)
    {
        if (b->raw[i] != '0')
        {
            break;
        }
    }

    return i;
}

static big_integer* __bi_substr(const big_integer* b, size_t from, size_t len)
{
    size_t blen = strlen(b->raw);

    if (from >= blen || len == 0) return NULL;

    big_integer* n = (big_integer*)malloc_assert(sizeof(big_integer));
    len = min(len, blen); // make sure length does not overflow
    n->raw = (char*)malloc_assert(sizeof(char) * (len + 1));
    n->raw[len] = 0;

    memcpy(n->raw, b->raw + from, sizeof(char) * len);
    return n;
}

static void __bi_dup(big_integer* dest, const big_integer* src)
{
    size_t sz = sizeof(char) * (strlen(src->raw) + 1);
    dest->raw = (char*)malloc_assert(sz);
    dest->raw[strlen(src->raw)] = 0;
    memcpy(dest->raw, src->raw, sz);
}

/**
 * duplicate length of src and fill all digits with same one
*/
static void __bi_dup_shape(big_integer* dest, const big_integer* src, const unsigned int fill)
{
    if (fill > 9) return;

    size_t len = strlen(src->raw);
    size_t i = 0;

    dest->raw = (char*)malloc_assert(sizeof(char) * (len + 1));

    for (i = 0; i < len; i++)
    {
        dest->raw[i] = __bi_d2c(fill);
    }

    dest->raw[i] = 0;
}

void __bi_assign(big_integer* dest, const big_integer* src)
{
    char* rd = dest->raw;
    char* rs = src->raw;
    size_t len1 = strlen(rd);
    size_t len2 = strlen(rs);

    if (len1 < len2)
    {
        release_big_integer(dest);
        __bi_dup(dest, src);
        return;
    }

    for (size_t i = len1 - 1, j = len2 - 1; i >= 0; i--)
    {
        rd[i] = (len1 - i == len2 - j) ? rs[j] : '0';

        if (j > 0) j--;
    }
}

void __bi_swap(big_integer* dest, big_integer* src)
{
    char* tmp = dest->raw;

    dest->raw = src->raw;
    src->raw = tmp;
}

/**
 * initialize a 1-digit integer
*/
void init_big_integer(big_integer* bn, unsigned int digit)
{
    bn->raw = (char*)malloc_assert(sizeof(char) * 2);
    bn->raw[0] = __bi_d2c(digit % 10);
    bn->raw[1] = '\0';
}

void release_big_integer(big_integer* bn)
{
    free(bn->raw);
}

/**
 * single-digit addition
 *
 * maximum carry: 1
*/
void bi_dadd(big_integer* dest, const unsigned int digit)
{
    if (digit > 9) return;

    char* r = dest->raw;
    int carry = digit;
    int full;

    for (size_t i = strlen(r) - 1; i >= 0; i--)
    {
        full = __bi_c2d(r[i]) + carry;
        carry = full / 10;
        r[i] = __bi_d2c(full % 10);
    }

    // now MSB determines if we need to grow
    if (carry)
    {
        bi_append_digit(dest, __bi_d2c(carry), BI_RAW_GROW_LEFT);
    }
}

/**
 * single-digit subtraction
 *
 * maximum carry: 1
 *
 * funtion returns true if result is negative
*/
bool bi_dsub(big_integer* dest, const unsigned int digit)
{
    if (digit > 9) return true;

    char* r = dest->raw;
    int carry = digit;
    int full;
    size_t len = strlen(r);

    // simple case: dest is also single digit
    if (len - __bi_idx_msb(dest) <= 1)
    {
        int result = __bi_c2d(r[0]) - (int)digit;
        r[0] = __bi_d2c(abs(result));

        return result < 0;
    }

    for (size_t i = len - 1; i >= 0; i--)
    {
        full = __bi_c2d(r[i]) - carry;
        carry = full < 0 ? 1 : 0;
        r[i] = __bi_d2c(full < 0 ? (full + 10) : full);
    }

    return false;
}

/**
 * single-digit multiplication
 *
 * maximum carry: 8
*/
void bi_dmul(big_integer* dest, const unsigned int digit)
{
    if (digit > 9)
    {
        return;
    }

    char* r = dest->raw;
    int carry = 0;
    int full;

    for (size_t i = strlen(r) - 1; i >= 0; i--)
    {
        full = __bi_c2d(r[i]) * digit + carry;
        carry = full / 10;
        r[i] = __bi_d2c(full % 10);
    }

    // now MSB determines if we need to grow
    if (carry)
    {
        bi_append_digit(dest, __bi_d2c(carry), BI_RAW_GROW_LEFT);
    }
}

/**
 * digit division where denominator can only be value 1 - 9
 * find a maximum digit such that: denom * digit <= num
 *
 * remainder can only be one digit
*/
void bi_ddiv(const big_integer* num, const unsigned int denom, big_integer* q, big_integer* r)
{
    init_big_integer(q, 0);
    init_big_integer(r, 0);

    if (denom == 0 || denom > 9) return;

    if (denom == 1)
    {
        // q = num, r = 0
        release_big_integer(q);
        __bi_dup(q, num);
        return;
    }

    char* rd = num->raw;
    size_t len = strlen(rd);
    int __r = 0;

    for (size_t i = 0; i < len; i++)
    {
        int __v = __r * 10 + __bi_c2d(rd[0]);
        int __r = __v % denom;

        bi_append_digit(q, __bi_d2c(__v / denom), BI_RAW_GROW_RIGHT);
    }

    r->raw[0] = __bi_d2c(__r);
}

/**
 * multiply by 2
 *
 * digit adder yield result has at most 2 digits
 * carry <= 1
*/
void bi_mul2(big_integer* dest)
{
    bi_dmul(dest, 2);
}

/**
 * multiply by 10
 *
 * simply grow on the right and fill trailing '0'
*/
void bi_mul10(big_integer* dest)
{
    size_t len = strlen(dest->raw);
    size_t sz = sizeof(char) * (len + 1);
    size_t i = __bi_idx_msb(dest);

    if (i == len)
    {
        // if 0
        return;
    }
    else if (i != 0)
    {
        // if we have leading 0s, move content
        char* tmp = (char*)malloc_assert(sz);
        memcpy(tmp, dest->raw, sz);
        memcpy(dest->raw, tmp + 1, sz - sizeof(char));
        dest->raw[len - 1] = '0';
    }
    else
    {
        // otherwise we need to grow
        __bi_grow(dest, 1, BI_RAW_GROW_RIGHT);
    }
}

/**
 * divide by 2
 *
 * produce d = 2 * q + r
 *
 * remainder is always one digit
*/
void bi_div2(const big_integer* d, big_integer* q, big_integer* r)
{
    bi_ddiv(d, 2, q, r);
}

/**
 * divide by 10
 *
 * produce d = 10 * q + r
 * q is d except last digit
 * r is last digit of d
*/
void bi_div10(const big_integer* d, big_integer* q, big_integer* r)
{
    init_big_integer(q, 0);
    init_big_integer(r, 0);

    r->raw[0] = d->raw[strlen(d->raw) - 1];

    size_t sz = sizeof(char) * (strlen(d->raw));
    q->raw = (char*)malloc_assert(sz);
    memset(q->raw, 0, sz);
    memcpy(q->raw, d->raw, sz);
}

/**
 * multiply by power of 10
 *
 * simply grow on the right and fill trailing '0'
*/
void bi_mulpow10(big_integer* dest, size_t e)
{
    size_t len = strlen(dest->raw);
    size_t i = __bi_idx_msb(dest);

    if (i == len || e == 0)
    {
        // if 0 or no grow
        return;
    }

    // if leading zero count is less than required,
    // grow by delta
    if (i < e)
    {
        __bi_grow(dest, e - i, BI_RAW_GROW_LEFT);

        // align data
        len = strlen(dest->raw);
        i += e;
    }

    // now move content to left by e times
    size_t sz = sizeof(char) * (len + 1);
    char* tmp = (char*)malloc_assert(sz);
    memcpy(tmp, dest->raw, sz);
    memcpy(dest->raw + i - e, tmp + i, sizeof(char) * (len - i));

    // for trailing, set to 0
    for (i = len - e; i < len; i++)
    {
        dest->raw[i] = '0';
    }

    dest->raw[len] = 0;
}

/**
 * divide by power of 10
 *
 * produce d = 10^e * q + r
 * q is first len(d) - e digits
 * r is d except q part
 *
 * maximum of 10^e is maximum cha we can access in raw, so size_t is sufficient
*/
void bi_divpow10(const big_integer* d, size_t e, big_integer* q, big_integer* r)
{
    size_t len = strlen(d->raw);
    size_t sz = sizeof(char) * (len + 1);

    // if e is 0, quotient is d, remainder is 0
    if (e == 0)
    {
        q->raw = (char*)malloc_assert(sz);
        q->raw[len] = 0;
        memcpy(q->raw, d->raw, sz);

        init_big_integer(q, 0);
        return;
    }

    // if e is too big, quotient is 0, remainder is d
    if (e >= len)
    {
        r->raw = (char*)malloc_assert(sz);
        r->raw[len] = 0;
        memcpy(r->raw, d->raw, sz);

        init_big_integer(q, 0);
        return;
    }

    sz = sizeof(char) * (len - e + 1);
    q->raw = (char*)malloc_assert(sz);
    q->raw[len - e] = '\0';
    memcpy(q->raw, d->raw, sz);

    if (e == 1)
    {
        init_big_integer(r, __bi_c2d(d->raw[len - 1]));
    }
    else
    {
        sz = sizeof(char) * (e + 1);
        r->raw = (char*)malloc_assert(sz);
        r->raw[e] = '\0';
        memcpy(r->raw, d->raw + len - e, sz);
    }
}

/**
 * append right: dest * 10 + digit
 * append left: MSB = digit
*/
void bi_append_digit(big_integer* dest, const char digit, bi_grow grow)
{
    if (!isdigit(digit)) return;

    if (grow == BI_RAW_GROW_RIGHT)
    {
        bi_mul10(dest);
        dest->raw[strlen(dest->raw) - 1] = digit;
    }
    else
    {
        if (digit == '0') return;

        size_t i = __bi_idx_msb(dest);

        // if there is no space, grow
        if (i == 0)
        {
            __bi_grow(dest, 1, BI_RAW_GROW_LEFT);
            i = 1;
        }

        // push digit
        dest->raw[i - 1] = digit;
    }
}

bi_compare bi_cmp(const big_integer* b1, const big_integer* b2)
{
    size_t len1 = strlen(b1->raw);
    size_t len2 = strlen(b2->raw);
    size_t i = __bi_idx_msb(b1);
    size_t j = __bi_idx_msb(b2);

    // longer number is bigger
    if (len1 - i > len2 - j)
    {
        // b1 > b2
        return BI_CMP_GREATER;
    }
    else if (len1 - i < len2 - j)
    {
        // b1 < b2
        return BI_CMP_LESS;
    }

    // if length match, compare digit-wise
    for (;i < len1; i++, j++)
    {
        // compare ascii is enough
        if (b1->raw[i] > b2->raw[j])
        {
            // b1 > b2
            return BI_CMP_GREATER;
        }
        else if (b1->raw[i] < b2->raw[j])
        {
            // b1 < b2
            return BI_CMP_LESS;
        }
    }

    // here, numbers are equal
    return BI_CMP_EQUAL;
}

/**
 * addition
 *
 * b1 = b1 + b2
*/
void bi_add(big_integer* b1, const big_integer* b2)
{
    char* r1 = b1->raw;
    char* r2 = b2->raw;
    size_t len1 = strlen(r1);
    size_t len2 = strlen(r2);
    int carry = 0;

    // resize b1 if len1 < len2 because it has to be done anyway
    if (len1 < len2)
    {
        __bi_grow(b1, len2 - len1, BI_RAW_GROW_LEFT);

        // need to redo this due to potential pointer change
        r1 = b1->raw;
        len1 = strlen(r1);
    }

    // now len1 >= len2
    for (size_t i = len1 - 1, j = len2 - 1; i >= 0; i--)
    {
        int v = __bi_c2d(r1[i]) + (j >= 0 ? __bi_c2d(r2[j]) : 0) + carry;
        carry = v / 10;
        r1[i] = __bi_d2c(v % 10);

        if (j > 0) j--;
    }

    // fill carry
    if (carry)
    {
        bi_append_digit(b1, '1', BI_RAW_GROW_LEFT);
    }
}

/**
 * subtraction
 *
 * b1 = |b1 - b2|, returns true if the value is negative
*/
bool bi_sub(big_integer* b1, const big_integer* b2)
{
    big_integer s1, s2;
    bi_compare cmp = bi_cmp(b1, b2);
    int carry = 0;

    // pointer is easier for swaping
    big_integer* ps1 = &s1;
    big_integer* ps2 = &s2;

    // initialize copy if parameters
    __bi_dup(ps1, b1);
    __bi_dup(ps2, b2);

    // we always subtract larger one with smaller one
    if (cmp == BI_CMP_LESS)
    {
        // swap
        big_integer* tmp = ps1;
        ps1 = ps2;
        ps2 = tmp;
    }

    // get iterate data
    char* r1 = ps1->raw;
    char* r2 = ps2->raw;
    size_t len1 = strlen(ps1->raw);
    size_t len2 = strlen(ps2->raw);

    // subtract, len1 >= len2
    for (size_t i = len1 - 1, j = len2 - 1; i >= 0; i--)
    {
        int v = __bi_c2d(r1[i]) - (j >= 0 ? __bi_c2d(r2[j]) : 0) - carry;
        carry = v < 0 ? 1 : 0;
        r1[i] = __bi_d2c(v < 0 ? (10 + v) : v);

        if (j > 0) j--;
    }

    // move result to b1
    release_big_integer(b1);
    b1->raw = ps1->raw;
    s1.raw = NULL;

    // delete temp variables
    release_big_integer(ps1);
    release_big_integer(ps2);

    return cmp == BI_CMP_LESS;
}

/**
 * division
 *
 * q, r will have max length to be length of numerator
 * q, r may have leading 0s
 *
 * if div by 0, q=0 and r=0
*/
void bi_div(const big_integer* num, const big_integer* denom, big_integer* q, big_integer* r)
{
    char* rn = num->raw;
    char* rd = denom->raw;
    size_t len1 = strlen(rn);
    size_t len2 = strlen(rd);
    size_t msb_denom = __bi_idx_msb(denom);

    // edge and optimized cases
    if (msb_denom == len2)
    {
        // div by 0
        init_big_integer(q, 0);
        init_big_integer(r, 0);
        return;
    }
    else if (msb_denom == len2 - 1)
    {
        // div by a digit
        bi_ddiv(num, __bi_c2d(rd[len2 - 1]), q, r);
        return;
    }

    // initialize
    __bi_dup_shape(q, num, 0);
    __bi_dup_shape(r, num, 0);

    for (size_t i = 0; i < len1; i++)
    {
        // calculate new candidate numerator: r * 10 + digit
        // CANNOT use append here because remainder may change
        bi_mul10(r);
        bi_dadd(r, __bi_c2d(rn[i]));

        /**
         * now quotient digit can be calculated in the following:
         *
         * since: r / denom = digit, remainder r'
         * so we can simplify the division into:
         *
         * r / digit + r' = denom
         *
         * so it means if digit is a maximum digit that makes
         * following true, then the digit is the quotient:
         *
         * r / digit >= denom
         *
         * then the new remainder is: r - denom * digit
        */

        unsigned int digit;
        big_integer __q, __r;
        bi_compare __cmp = BI_CMP_LESS;

        // find quotient digit
        for (digit = 9, __cmp = BI_CMP_LESS; digit > 0 && __cmp == BI_CMP_LESS; digit--)
        {
            bi_ddiv(r, digit, &__q, &__r);
            __cmp = bi_cmp(&__q, denom);
            release_big_integer(&__q);
            release_big_integer(&__r);
        }

        // if quotient digit non-zero, remainder needs to be updated
        if (digit != 0)
        {
            __bi_dup(&__r, denom);
            bi_dmul(&__r, digit);
            bi_sub(r, &__r);
            release_big_integer(&__r);
        }

        // append quotient digit
        bi_append_digit(q, __bi_d2c(digit), BI_RAW_GROW_RIGHT);
    }
}

/**
 * count bits of binary form
*/
int64_t bi_count_bits(const big_integer* d, bool* success)
{
    big_integer _d, q, r;
    int64_t c = 0;

    // initialize
    __bi_dup(&_d, d);

    while (true)
    {
        bi_div2(&_d, &q, &r);

        // prepare for next iteration
        __bi_swap(&_d, &q);
        release_big_integer(&q);
        release_big_integer(&r);

        // test if quotient is 0, if so we stop
        if (strlen(q.raw) == __bi_idx_msb(&q))
        {
            release_big_integer(&_d);
            break;
        }
        else if (c == INT64_MAX)
        {
            if (success) *success = false;
            break;
        }

        c++;
    }

    if (success) *success = true;

    return c;
}

/**
 * get bits from LSB
 *
 * luckily, long division can be applied here because
 * it generates bits from LSB
*/
uint64_t bi_truncate(const big_integer* d, size_t bits)
{
    uint64_t v = 0;
    bits = min(bits, 64);

    big_integer _d, q, r;

    // initialize
    __bi_dup(&_d, d);

    for (size_t i = 0; i < bits; i++)
    {
        bi_div2(&_d, &q, &r);

        // prepare for next iteration
        __bi_swap(&_d, &q);
        release_big_integer(&q);
        release_big_integer(&r);

        // test if quotient is 0, if so we stop
        if (strlen(q.raw) == __bi_idx_msb(&q))
        {
            release_big_integer(&_d);
            break;
        }

        // otherwise, we write the bit
        if (r.raw[0] == '1')
        {
            v |= (1 << i);
        }
    }

    return v;
}
