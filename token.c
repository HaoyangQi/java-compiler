#include "token.h"

/**
 * token helper
 *
 * consume decimal digits, returns false if accepted nothing
*/
static bool consume_digits(file_buffer* buffer)
{
    if (!isdigit(*buffer->cur))
    {
        return false;
    }

    while (isdigit(*buffer->cur))
    {
        buffer_ptr_safe_move(buffer);
    }

    return true;
}

/**
 * token helper
 *
 * start from current, try accepting exponent part of a number
 * true is accepted
*/
static bool consume_number_exponent_part(file_buffer* buffer)
{
    char c = *buffer->cur;

    /**
     * exponent part trigger
     *
     * e|E : can be followed by an optional sign +|-
     * then must exist at least one digit
     *
     * if no trailing digits, cutoff before 'e|E'
    */
    if (isexpindicator(c))
    {
        // roughly peek, fix later
        char sign_peek = buffer_peek(buffer, 1);
        char digit_peek = buffer_peek(buffer, 2);
        bool has_sign = isexpsign(sign_peek);

        // adjust digit peek location for validation
        // do NOT move cursor before validation
        if (!has_sign)
        {
            digit_peek = sign_peek;
        }

        // validation: exponent part must contain >=1 digit(s)
        // hence: digit peek must be validated
        if (isdigit(digit_peek))
        {
            // letter 'e|E' now must be part of FP number now

            // consume exp indicator
            buffer_ptr_safe_move(buffer);

            // consume sign
            if (has_sign)
            {
                buffer_ptr_safe_move(buffer);
            }

            // same idea, consume digits
            consume_digits(buffer);

            return true;
        }
    }

    return false;
}

/**
 * token helper
 *
 * consume a newline sequence: CR, LF, CRLF
*/
static bool consume_newline(file_buffer* buffer)
{
    char c = *buffer->cur;

    if (isvspacechar(c))
    {
        buffer_ptr_safe_move(buffer);

        // consume CRLF as a single newline
        if (c == '\r' && *buffer->cur == '\n')
        {
            buffer_ptr_safe_move(buffer);
        }

        return true;
    }

    return false;
}

/**
 * token helper
 *
 * consume all spaces
*/
static void consume_spaces(file_buffer* buffer)
{
    char c;

    while (true)
    {
        if (consume_newline(buffer))
        {
            continue;
        }

        c = *buffer->cur;

        if (ishspacechar(c))
        {
            // simply consume it one by one
            buffer_ptr_safe_move(buffer);
        }
        else
        {
            // stop when no longer a space character
            break;
        }
    }
}

/**
 * tokenizer
 *
 * longest match will be returned, except for illegal characters,
 * which will be returned one by one
 *
 * a copy of EOF token will keep recurring once buffer reached the end
*/
void get_next_token(java_token* token, file_buffer* buffer, java_symbol_table* table)
{
    /**
     * escape space sequence
     *
     * we have to do this before everything to put
     * stream cursor at right location
    */
    consume_spaces(buffer);

    token->from = buffer->cur;
    token->type = JT_EOF;
    token->keyword = NULL;
    token->number = JT_NUM_NONE;
    token->number_bit_length = JT_NUM_BIT_LENGTH_NORMAL;

    if (is_eof(buffer))
    {
        return;
    }

    char c = *buffer->cur;
    char peeks[2]; // meaning can vary based on context

    /**
     * top-level trigger can be classified as:
     *
     * 1. letter, _, $    =>  it is an id
     * 2. digit           =>  it is a number
     * 3. valid symbols   =>  op, sp, comments
     * 4. remainders      =>  illegal
     *
     * starting here, should never start with space(s)
     * as we have escaped them
     *
     * for number, how it ends is complex and also
     * a factor to determine if it is valid or not:
     *
     * once it starts with a digit, it must be some sort of
     * a number, but things like "0x", "0b", etc, are not
     * valid numbers
    */
    if (isidfirstchar(c))
    {
        token->type = JT_IDENTIFIER;
        token->subtype.id = JT_ID_GENERIC;

        // before 1st iteration, buffer_ptr_safe_move consumes
        // first ID char that just checked
        while (buffer_ptr_safe_move(buffer))
        {
            c = *buffer->cur;

            if (!isidchar(c))
            {
                break;
            }
        }
    }
    else if (isdigit(c))
    {
        token->type = JT_LITERAL;
        token->subtype.li = JT_LI_NUM;
        token->number = JT_NUM_DEC;
        token->number_bit_length = JT_NUM_BIT_LENGTH_NORMAL;

        /**
         * before we start consuming a pattern,
         * let's check prefix to determine some number types
         *
         * 1. 0x|0X: hex
         * 2. 0b|0B: bin
         * 3. 0Digit: oct
         *
         * once matched we consume '0' as it is part of the prefix now,
         * so when loop starts it can consume entire prefix and start
         * at the char behind prefix
        */
        if (c == '0')
        {
            char after_first_digit = buffer_peek(buffer, 1);

            // consume once once matched, in order to skip
            // entire prefix sequence
            if (ishexindicator(after_first_digit))
            {
                token->number = JT_NUM_HEX;
                buffer_ptr_safe_move(buffer);
            }
            else if (isbinaryindicator(after_first_digit))
            {
                token->number = JT_NUM_BIN;
                buffer_ptr_safe_move(buffer);
            }
            else if (isdigit(after_first_digit))
            {
                // for octal, valid digits are 0-7,
                // but we losen the rule a bit because we do not have
                // to go that far
                token->number = JT_NUM_OCT;
                buffer_ptr_safe_move(buffer);
            }
        }

        /**
         * pattern acceptance
         *
         * only one pattern is allowed on top-level
        */
        switch (token->number)
        {
            case JT_NUM_HEX:
                // for hex, stop when not a hex digit
                while (buffer_ptr_safe_move(buffer))
                {
                    if (!isxdigit(*buffer->cur))
                    {
                        break;
                    }
                }
                break;
            case JT_NUM_BIN:
                // for bin, stop when not a bin digit
                while (buffer_ptr_safe_move(buffer))
                {
                    if (!isbindigit(*buffer->cur))
                    {
                        break;
                    }
                }
                break;
            case JT_NUM_OCT:
                // for oct, stop when not a digit
                // this is not correct, but very sufficient
                consume_digits(buffer);
                break;
            default: // JT_NUM_DEC
            {
                /**
                 * here, we handle integral AND FP numbers:
                 *
                 * Integral number could be: bin, dec, oct, hex
                 * and bin and hex must be triggered by suffix
                 * oct has suffix '0' only if the following is a digit
                 *
                 * FP number number could be:
                 *
                 * DecimalFloatingPointLiteral:
                 *     Digits . [Digits] [ExponentPart] [FloatTypeSuffix]
                 *     . Digits [ExponentPart] [FloatTypeSuffix]
                 *     Digits ExponentPart [FloatTypeSuffix]
                 *     Digits [ExponentPart] FloatTypeSuffix
                 *
                 * no logic here for 2nd FP rule though,
                 * as digit is a trigger here while 2nd rule is triggered by DOT
                */

                // integral requires only digits
                consume_digits(buffer);

                /**
                 * FP trigger (FP_DOUBLE without suffix context)
                 *
                 * Digit .
                 * Digit e|E
                 *
                 * after DOT(.), it can still trigger 2nd rule
                 * and hence have exponent part
                 *
                 * when DOT is immediately followed by 'e|E', the next
                 * immediate character must be one of the following:
                 * +, -, Digit
                 * if is sign + or -, then after the sign, there
                 * must exist at least one digit
                 * otherwise, DOT would not trigger FP acceptance
                */

                // fractional part
                if (isfractionindicator(*buffer->cur))
                {
                    // consume the DOT first, if it becomes ambiguous
                    // we will revert later, because in here situation
                    // after DOT is fairly complex and it is not worth
                    // it to determine now
                    buffer_ptr_safe_move(buffer);

                    /**
                     * now, after DOT there could exists Digits, let's
                     * consume them
                     *
                     * if no digits are consumed, then DOT is still ambiguous
                     * otherwise DOT is definitely part of FP number
                     *
                     * it also works with EOF case, when EOF, cur at \0,
                     * before is DOT; but a FP number can end with a DOT,
                     * so it is still a valid FP number
                     *
                     * must consume digits first, then test eof
                    */
                    if (consume_digits(buffer) || is_eof(buffer))
                    {
                        token->number = JT_NUM_FP_DOUBLE;
                    }
                }

                // exponent part
                if (consume_number_exponent_part(buffer))
                {
                    // since exponent part can exist without fractional
                    // indicator, so we need to set FP type again here

                    // default FP has double type
                    token->number = JT_NUM_FP_DOUBLE;
                }

                break;
            }
        }

        /**
         * prefix validation (HEX and BIN only)
         *
         * if prefix exists, then there must exist at least
         * one valid digit (dec, hex, oct, bin) before suffix
         *
         * e.g. 0x and 0b are not valid number
         *
         * but... we do not have to stop it from accepting
         * suffix (rule of longest matching)
        */

        if (token->number == JT_NUM_HEX || token->number == JT_NUM_BIN)
        {
            if (isdigit(*(buffer->cur)))
            {
                /**
                 * TODO: log error
                */
                fprintf(stderr, "TODO error: number must contain digit");
            }
        }

        /**
         * suffix trigger
         *
         * l|L : integral number, long type
         * d|D : FP number, (enforce) double type
         * f|F : FP number, float type
        */

        c = *buffer->cur;
        peeks[0] = buffer_peek(buffer, -1); // char before current
        peeks[1] = buffer_peek(buffer, 1); // char after cur

        /**
         * this is interesting...
         *
         * so if a FP ends with DOT, e.g. 23.
         * now we read: 23.f
         *
         * but we need to know what is behind 'f'
         * because if 'f' is not cutoff, then DOT
         * should not be part of number, hence no
         * longer a FP, e.g. 23.fSomeName
         *
         * and other things are simply for tolerance:
         * 1. when name followed by a [, it becomes array access
         * 2. when name followed by a (, it becomes method invocation
         * in these cases, we also not treat it as FP
         *
         * those are not quite necessary, but parsing result could be
         * more flexible
         *
         * this boolean only works under condition where current character
         * in stream is suffix trigger and number is a FP ends with DOT
        */
        bool should_reject_suffix_trigger =
            isfractionindicator(peeks[0]) && (
                isidchar(peeks[1])
                || peeks[1] == '('
                || peeks[1] == '[');

        // triggers
        if (islongsuffix(c))
        {
            // only integral number allows such suffix
            // only matters where we cut-off
            switch (token->number)
            {
                case JT_NUM_DEC:
                case JT_NUM_HEX:
                case JT_NUM_OCT:
                case JT_NUM_BIN:
                    token->number_bit_length = JT_NUM_BIT_LENGTH_LONG;
                    buffer_ptr_safe_move(buffer);
                    break;
                default:
                    break;
            }
        }
        else if (isfloatsuffix(c))
        {
            if (!should_reject_suffix_trigger)
            {
                token->number = JT_NUM_FP_FLOAT;
                buffer_ptr_safe_move(buffer);
            }
            else if (isfractionindicator(peeks[0]))
            {
                /**
                 * otherwise we simply cut-off
                 *
                 * but if we just rejected suffix and last char of number
                 * is a DOT, we need to reject the DOT as well
                 *
                 * this is a safe move as the number contains at least
                 * one digit in here
                */
                buffer->cur--;
            }
        }
        else if (isdoublesuffix(c))
        {
            if (!should_reject_suffix_trigger)
            {
                // by default we have double type, 
                // so no need to set type again
                buffer_ptr_safe_move(buffer);
            }
            else if (isfractionindicator(peeks[0]))
            {
                /**
                 * otherwise we simply cut-off
                 *
                 * but if we just rejected suffix and last char of number
                 * is a DOT, we need to reject the DOT as well
                 *
                 * this is a safe move as the number contains at least
                 * one digit in here
                */
                buffer->cur--;
            }
        }
    }
    else
    {
        switch (c)
        {
            case '(':
                token->type = JT_SEPARATOR;
                token->subtype.sp = JT_SP_PL;
                buffer_ptr_safe_move(buffer);
                break;
            case ')':
                token->type = JT_SEPARATOR;
                token->subtype.sp = JT_SP_PR;
                buffer_ptr_safe_move(buffer);
                break;
            case '{':
                token->type = JT_SEPARATOR;
                token->subtype.sp = JT_SP_BL;
                buffer_ptr_safe_move(buffer);
                break;
            case '}':
                token->type = JT_SEPARATOR;
                token->subtype.sp = JT_SP_BR;
                buffer_ptr_safe_move(buffer);
                break;
            case '[':
                token->type = JT_SEPARATOR;
                token->subtype.sp = JT_SP_SL;
                buffer_ptr_safe_move(buffer);
                break;
            case ']':
                token->type = JT_SEPARATOR;
                token->subtype.sp = JT_SP_SR;
                buffer_ptr_safe_move(buffer);
                break;
            case ';':
                token->type = JT_SEPARATOR;
                token->subtype.sp = JT_SP_SC;
                buffer_ptr_safe_move(buffer);
                break;
            case ',':
                token->type = JT_SEPARATOR;
                token->subtype.sp = JT_SP_CM;
                buffer_ptr_safe_move(buffer);
                break;
            case '@':
                token->type = JT_SEPARATOR;
                token->subtype.sp = JT_SP_AT;
                buffer_ptr_safe_move(buffer);
                break;
            case '?':
                token->type = JT_SEPARATOR;
                token->subtype.sp = JT_SP_QST;
                buffer_ptr_safe_move(buffer);
                break;
            case '.':
            {
                token->type = JT_SEPARATOR;
                token->subtype.sp = JT_SP_DOT;
                buffer_ptr_safe_move(buffer);

                c = *buffer->cur;

                if (c == '.')
                {
                    /**
                     * accept ...
                    */
                    if (buffer_peek(buffer, 1) == '.')
                    {
                        token->subtype.sp = JT_SP_DDD;

                        // need to accept 2 times for 2 DOTs
                        buffer_ptr_safe_move(buffer);
                        buffer_ptr_safe_move(buffer);
                    }
                }
                else if (isdigit(c))
                {
                    /**
                     * here, we try accepting one of forms of FP number
                     *
                     * DecimalFloatingPointLiteral:
                     *     . Digits [ExponentPart] [FloatTypeSuffix]
                     *
                     * the fact for this to be easier is: there must exist
                     * at least one digit after the DOT, which is already
                     * covered by the if-condition
                    */

                    // once we are here, we are sure it is at least a double
                    token->type = JT_LITERAL;
                    token->subtype.li = JT_LI_NUM;
                    token->number = JT_NUM_FP_DOUBLE;

                    // digits, followed by optional exponent part
                    consume_digits(buffer);
                    consume_number_exponent_part(buffer);

                    c = *buffer->cur;

                    /**
                     * suffix trigger - FP only
                     *
                     * d|D : FP number, (enforce) double type
                     * f|F : FP number, float type
                     *
                     * since we always end with a digit in accepted sequence
                     * so no check is necessary, just recognize and cutoff
                    */
                    if (isfloatsuffix(c))
                    {
                        token->number = JT_NUM_FP_FLOAT;
                        buffer_ptr_safe_move(buffer);
                    }
                    else if (isdoublesuffix(c))
                    {
                        // by default we have double type, 
                        // so no need to set type again
                        buffer_ptr_safe_move(buffer);
                    }
                }

                break;
            }
            case ':':
                token->type = JT_SEPARATOR;
                token->subtype.sp = JT_SP_CL;
                buffer_ptr_safe_move(buffer);

                /**
                 * accept ::
                */
                if (*buffer->cur == ':')
                {
                    token->subtype.sp = JT_SP_CC;
                    buffer_ptr_safe_move(buffer);
                }

                break;
            case '=':
                token->type = JT_OPERATOR;
                token->subtype.op = JT_OP_ASN;
                buffer_ptr_safe_move(buffer);

                /**
                 * accept ==
                */
                if (*buffer->cur == '=')
                {
                    token->subtype.op = JT_OP_EQ;
                    buffer_ptr_safe_move(buffer);
                }

                break;
            case '>':
                token->type = JT_OPERATOR;
                token->subtype.op = JT_OP_GT;
                buffer_ptr_safe_move(buffer);

                c = *buffer->cur;

                /**
                 * accept >=, >>, >>>, >>=, >>>=
                */
                if (c == '=')
                {
                    // >=
                    token->subtype.op = JT_OP_GE;
                    buffer_ptr_safe_move(buffer);
                }
                else if (c == '>')
                {
                    // >>
                    token->subtype.op = JT_OP_RS;
                    buffer_ptr_safe_move(buffer);

                    c = *buffer->cur;

                    if (c == '>')
                    {
                        // >>>
                        token->subtype.op = JT_OP_ZFRS;
                        buffer_ptr_safe_move(buffer);

                        if (*buffer->cur == '=')
                        {
                            // >>>=
                            token->subtype.op = JT_OP_ZFRSASN;
                            buffer_ptr_safe_move(buffer);
                        }
                    }
                    else if (c == '=')
                    {
                        // >>=
                        token->subtype.op = JT_OP_RSASN;
                        buffer_ptr_safe_move(buffer);
                    }
                }

                break;
            case '<':
                token->type = JT_OPERATOR;
                token->subtype.op = JT_OP_LT;
                buffer_ptr_safe_move(buffer);

                c = *buffer->cur;

                /**
                 * accept <=, <<, <<=
                */
                if (c == '=')
                {
                    // <=
                    token->subtype.op = JT_OP_LE;
                    buffer_ptr_safe_move(buffer);
                }
                else if (c == '<')
                {
                    // <<
                    token->subtype.op = JT_OP_LS;
                    buffer_ptr_safe_move(buffer);

                    if (*buffer->cur == '=')
                    {
                        // <<=
                        token->subtype.op = JT_OP_LSASN;
                        buffer_ptr_safe_move(buffer);
                    }
                }

                break;
            case '!':
                token->type = JT_OPERATOR;
                token->subtype.op = JT_OP_NEG;
                buffer_ptr_safe_move(buffer);

                /**
                 * accept !=
                */
                if (*buffer->cur == '=')
                {
                    token->subtype.op = JT_OP_NE;
                    buffer_ptr_safe_move(buffer);
                }

                break;
            case '~':
                token->type = JT_OPERATOR;
                token->subtype.op = JT_OP_CPM;
                buffer_ptr_safe_move(buffer);
                break;
            case '+':
                token->type = JT_OPERATOR;
                token->subtype.op = JT_OP_ADD;
                buffer_ptr_safe_move(buffer);

                c = *buffer->cur;

                /**
                 * accept +=, ++
                */
                if (c == '=')
                {
                    token->subtype.op = JT_OP_ADDASN;
                    buffer_ptr_safe_move(buffer);
                }
                else if (c == '+')
                {
                    token->subtype.op = JT_OP_INC;
                    buffer_ptr_safe_move(buffer);
                }

                break;
            case '-':
                token->type = JT_OPERATOR;
                token->subtype.op = JT_OP_SUB;
                buffer_ptr_safe_move(buffer);

                c = *buffer->cur;

                /**
                 * accept -=, --, ->
                */
                if (c == '=')
                {
                    token->subtype.op = JT_OP_SUBASN;
                    buffer_ptr_safe_move(buffer);
                }
                else if (c == '-')
                {
                    token->subtype.op = JT_OP_DEC;
                    buffer_ptr_safe_move(buffer);
                }
                else if (c == '>')
                {
                    token->subtype.op = JT_OP_AWR;
                    buffer_ptr_safe_move(buffer);
                }

                break;
            case '*':
                token->type = JT_OPERATOR;
                token->subtype.op = JT_OP_MUL;
                buffer_ptr_safe_move(buffer);

                /**
                 * accept *=
                */
                if (*buffer->cur == '=')
                {
                    token->subtype.op = JT_OP_MULASN;
                    buffer_ptr_safe_move(buffer);
                }

                break;
            case '/':
                token->type = JT_OPERATOR;
                token->subtype.op = JT_OP_DIV;
                buffer_ptr_safe_move(buffer);

                c = *buffer->cur;

                /**
                 * accept /=, or comment
                */
                if (c == '=')
                {
                    token->subtype.op = JT_OP_DIVASN;
                    buffer_ptr_safe_move(buffer);
                }
                else if (c == '*')
                {
                    // now we trigger multi-line comment
                    token->type = JT_COMMENT;
                    token->subtype.cm = JT_CM_MULTI_LINE;

                    while (buffer_ptr_safe_move(buffer))
                    {
                        if (*buffer->cur == '*' && buffer_peek(buffer, 1) == '/')
                        {
                            // consume twice for the enclosure sequence
                            buffer_ptr_safe_move(buffer);
                            buffer_ptr_safe_move(buffer);
                            break;
                        }
                    }
                }
                else if (c == '/')
                {
                    // now we trigger one-line comment
                    token->type = JT_COMMENT;
                    token->subtype.cm = JT_CM_SINGLE_LINE;

                    while (buffer_ptr_safe_move(buffer))
                    {
                        // try consuming again to see if next is a newline
                        // sequence, if so we terminate
                        if (consume_newline(buffer))
                        {
                            break;
                        }
                    }
                }

                break;
            case '&':
                token->type = JT_OPERATOR;
                token->subtype.op = JT_OP_AND;
                buffer_ptr_safe_move(buffer);

                c = *buffer->cur;

                /**
                 * accept &=, &&
                */
                if (c == '=')
                {
                    token->subtype.op = JT_OP_ANDASN;
                    buffer_ptr_safe_move(buffer);
                }
                else if (c == '&')
                {
                    token->subtype.op = JT_OP_LAND;
                    buffer_ptr_safe_move(buffer);
                }

                break;
            case '|':
                token->type = JT_OPERATOR;
                token->subtype.op = JT_OP_OR;
                buffer_ptr_safe_move(buffer);

                c = *buffer->cur;

                /**
                 * accept &=, &&
                */
                if (c == '=')
                {
                    token->subtype.op = JT_OP_ORASN;
                    buffer_ptr_safe_move(buffer);
                }
                else if (c == '|')
                {
                    token->subtype.op = JT_OP_LOR;
                    buffer_ptr_safe_move(buffer);
                }

                break;
            case '^':
                token->type = JT_OPERATOR;
                token->subtype.op = JT_OP_XOR;
                buffer_ptr_safe_move(buffer);

                c = *buffer->cur;

                /**
                 * accept ^=
                */
                if (c == '=')
                {
                    token->subtype.op = JT_OP_XORASN;
                    buffer_ptr_safe_move(buffer);
                }

                break;
            case '%':
                token->type = JT_OPERATOR;
                token->subtype.op = JT_OP_MOD;
                buffer_ptr_safe_move(buffer);

                c = *buffer->cur;

                /**
                 * accept %=
                */
                if (c == '=')
                {
                    token->subtype.op = JT_OP_MODASN;
                    buffer_ptr_safe_move(buffer);
                }

                break;
            case '\'':
                /**
                 * character literal trigger
                 *
                 * no validation
                 * but we do need to skip escape sequence triggered by backslash
                 * so that we will not enclose literal with escaped character
                */
                token->type = JT_LITERAL;
                token->subtype.cm = JT_LI_CHAR;

                // here c is previous character
                c = *buffer->cur;

                while (buffer_ptr_safe_move(buffer))
                {
                    // we stop at ' but continue if it is escaped
                    if (*buffer->cur == '\'' && c != '\\')
                    {
                        // consume enclosure sequence
                        buffer_ptr_safe_move(buffer);
                        break;
                    }

                    // keep track of previous character to check
                    // if next one is escaped
                    c = *buffer->cur;
                }

                break;
            case '\"':
                /**
                 * string literal trigger
                 *
                 * no validation
                 * but we do need to skip escape sequence triggered by backslash
                 * so that we will not enclose literal with escaped character
                */
                token->type = JT_LITERAL;
                token->subtype.cm = JT_LI_STR;

                // here c is previous character
                c = *buffer->cur;

                while (buffer_ptr_safe_move(buffer))
                {
                    // we stop at ' but continue if it is escaped
                    if (*buffer->cur == '\"' && c != '\\')
                    {
                        // consume enclosure sequence
                        buffer_ptr_safe_move(buffer);
                        break;
                    }

                    // keep track of previous character to check
                    // if next one is escaped
                    c = *buffer->cur;
                }

                break;
            default:
                // illegal sequence detected, put it in the token
                //
                // funny thing is: longest match could be costly here
                // due to the complexity of condition logic
                // so we accept it one by one
                token->type = JT_ILLEGAL;
                buffer_ptr_safe_move(buffer);
                break;
        }
    }

    // character pointed by "to" is the very first one that is NOT part of the token
    token->to = buffer->cur;

    // check if id is a keyword
    if (token->type == JT_IDENTIFIER)
    {
        size_t len = buffer_count(token->from, token->to);
        char* content = (char*)malloc_assert(sizeof(char) * (len + 1));

        buffer_substring(content, token->from, len);
        java_symbol* sym = symbol_lookup(table, content);

        if (sym)
        {
            token->keyword = sym->word;
        }

        free(content);

        // in tokenization, we cannot classify ID further
        token->subtype.id = JT_ID_GENERIC;
    }

    /**
     * enclosure validation
     *
     * validate all enclosure forms have been enclosed properly
     * includes: multi-line comment, character literal, string literal
    */
    c = buffer_peek(buffer, -1);
    if (token->type == JT_LITERAL)
    {
        if (token->subtype.li == JT_LI_CHAR)
        {
            if (c != '\'')
            {
                /**
                 * TODO: log error
                */
                fprintf(stderr, "TODO error: character literal incomplete");
            }
        }
        else if (token->subtype.li == JT_LI_STR)
        {
            if (c != '\"')
            {
                /**
                 * TODO: log error
                */
                fprintf(stderr, "TODO error: string literal incomplete");
            }
        }
    }
    else if (token->type == JT_COMMENT)
    {
        if (token->subtype.cm == JT_CM_MULTI_LINE)
        {
            if (c != '/' || buffer_peek(buffer, -2) != '*')
            {
                /**
                 * TODO: log error
                */
                fprintf(stderr, "TODO error: comment incomplete");
            }
        }
    }
}
