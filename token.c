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
 * Toke Data Initializer
*/
void init_token(java_token* token)
{
    token->from = NULL;
    token->class = JT_EOF;
    token->type = JLT_MAX;
    token->keyword = NULL;
    token->number.type = JT_NUM_NONE;
    token->number.bits = JT_NUM_BIT_LENGTH_NORMAL;
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

    init_token(token);
    token->from = buffer->cur;

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
        token->class = JT_IDENTIFIER;

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
        token->class = JT_LITERAL;
        token->type = JLT_LTR_NUMBER;
        token->number.type = JT_NUM_DEC;
        token->number.bits = JT_NUM_BIT_LENGTH_NORMAL;

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
                token->number.type = JT_NUM_HEX;
                buffer_ptr_safe_move(buffer);
            }
            else if (isbinaryindicator(after_first_digit))
            {
                token->number.type = JT_NUM_BIN;
                buffer_ptr_safe_move(buffer);
            }
            else if (isdigit(after_first_digit))
            {
                // for octal, valid digits are 0-7,
                // but we losen the rule a bit because we do not have
                // to go that far
                token->number.type = JT_NUM_OCT;
                buffer_ptr_safe_move(buffer);
            }
        }

        /**
         * pattern acceptance
         *
         * only one pattern is allowed on top-level
        */
        switch (token->number.type)
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
                        token->number.type = JT_NUM_FP_DOUBLE;
                    }
                }

                // exponent part
                if (consume_number_exponent_part(buffer))
                {
                    // since exponent part can exist without fractional
                    // indicator, so we need to set FP type again here

                    // default FP has double type
                    token->number.type = JT_NUM_FP_DOUBLE;
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

        if (token->number.type == JT_NUM_HEX || token->number.type == JT_NUM_BIN)
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
        printf("==== reject check: %d %c %c %c\n", should_reject_suffix_trigger, peeks[0], c, peeks[1]);

        // triggers
        if (islongsuffix(c))
        {
            // only integral number allows such suffix
            // only matters where we cut-off
            switch (token->number.type)
            {
                case JT_NUM_DEC:
                case JT_NUM_HEX:
                case JT_NUM_OCT:
                case JT_NUM_BIN:
                    token->number.bits = JT_NUM_BIT_LENGTH_LONG;
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
                token->number.type = JT_NUM_FP_FLOAT;
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
        else if (isfractionindicator(peeks[0]))
        {
            if (isidchar(c))
            {
                /**
                 * if last is a DOT but no suffix trigger, we only accept the DOT
                 * when the next character does NOT trigger an ID acceptance
                */
                buffer->cur--;
            }
            else
            {
                /**
                 * otherwise, we fix type, default to DOUBLE
                 *
                 * and since we have accepted the DOT, so no further accepting here
                */
                token->number.type = JT_NUM_FP_DOUBLE;
            }
        }
    }
    else
    {
        switch (c)
        {
            case '(':
                token->class = JT_SEPARATOR;
                token->type = JLT_SYM_PARENTHESIS_OPEN;
                buffer_ptr_safe_move(buffer);
                break;
            case ')':
                token->class = JT_SEPARATOR;
                token->type = JLT_SYM_PARENTHESIS_CLOSE;
                buffer_ptr_safe_move(buffer);
                break;
            case '{':
                token->class = JT_SEPARATOR;
                token->type = JLT_SYM_BRACE_OPEN;
                buffer_ptr_safe_move(buffer);
                break;
            case '}':
                token->class = JT_SEPARATOR;
                token->type = JLT_SYM_BRACE_CLOSE;
                buffer_ptr_safe_move(buffer);
                break;
            case '[':
                token->class = JT_SEPARATOR;
                token->type = JLT_SYM_BRACKET_OPEN;
                buffer_ptr_safe_move(buffer);
                break;
            case ']':
                token->class = JT_SEPARATOR;
                token->type = JLT_SYM_BRACKET_CLOSE;
                buffer_ptr_safe_move(buffer);
                break;
            case ';':
                token->class = JT_SEPARATOR;
                token->type = JLT_SYM_SEMICOLON;
                buffer_ptr_safe_move(buffer);
                break;
            case ',':
                token->class = JT_SEPARATOR;
                token->type = JLT_SYM_COMMA;
                buffer_ptr_safe_move(buffer);
                break;
            case '@':
                token->class = JT_SEPARATOR;
                token->type = JLT_SYM_AT;
                buffer_ptr_safe_move(buffer);
                break;
            case '?':
                token->class = JT_SEPARATOR;
                token->type = JLT_SYM_QUESTION;
                buffer_ptr_safe_move(buffer);
                break;
            case '.':
            {
                token->class = JT_SEPARATOR;
                token->type = JLT_SYM_DOT;
                buffer_ptr_safe_move(buffer);

                c = *buffer->cur;

                if (c == '.')
                {
                    /**
                     * accept ...
                    */
                    if (buffer_peek(buffer, 1) == '.')
                    {
                        token->type = JLT_SYM_VARIADIC;

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
                    token->class = JT_LITERAL;
                    token->type = JLT_LTR_NUMBER;
                    token->number.type = JT_NUM_FP_DOUBLE;

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
                        token->number.type = JT_NUM_FP_FLOAT;
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
                token->class = JT_SEPARATOR;
                token->type = JLT_SYM_COLON;
                buffer_ptr_safe_move(buffer);

                /**
                 * accept ::
                */
                if (*buffer->cur == ':')
                {
                    token->type = JLT_SYM_METHOD_REFERENCE;
                    buffer_ptr_safe_move(buffer);
                }

                break;
            case '=':
                token->class = JT_OPERATOR;
                token->type = JLT_SYM_EQUAL;
                buffer_ptr_safe_move(buffer);

                /**
                 * accept ==
                */
                if (*buffer->cur == '=')
                {
                    token->type = JLT_SYM_RELATIONAL_EQUAL;
                    buffer_ptr_safe_move(buffer);
                }

                break;
            case '>':
                token->class = JT_OPERATOR;
                token->type = JLT_SYM_ANGLE_BRACKET_CLOSE;
                buffer_ptr_safe_move(buffer);

                c = *buffer->cur;

                /**
                 * accept >=, >>, >>>, >>=, >>>=
                */
                if (c == '=')
                {
                    // >=
                    token->type = JLT_SYM_GREATER_EQUAL;
                    buffer_ptr_safe_move(buffer);
                }
                else if (c == '>')
                {
                    // >>
                    token->type = JLT_SYM_RIGHT_SHIFT;
                    buffer_ptr_safe_move(buffer);

                    c = *buffer->cur;

                    if (c == '>')
                    {
                        // >>>
                        token->type = JLT_SYM_RIGHT_SHIFT_UNSIGNED;
                        buffer_ptr_safe_move(buffer);

                        if (*buffer->cur == '=')
                        {
                            // >>>=
                            token->type = JLT_SYM_RIGHT_SHIFT_UNSIGNED_ASSIGNMENT;
                            buffer_ptr_safe_move(buffer);
                        }
                    }
                    else if (c == '=')
                    {
                        // >>=
                        token->type = JLT_SYM_RIGHT_SHIFT_ASSIGNMENT;
                        buffer_ptr_safe_move(buffer);
                    }
                }

                break;
            case '<':
                token->class = JT_OPERATOR;
                token->type = JLT_SYM_ANGLE_BRACKET_OPEN;
                buffer_ptr_safe_move(buffer);

                c = *buffer->cur;

                /**
                 * accept <=, <<, <<=
                */
                if (c == '=')
                {
                    // <=
                    token->type = JLT_SYM_LESS_EQUAL;
                    buffer_ptr_safe_move(buffer);
                }
                else if (c == '<')
                {
                    // <<
                    token->type = JLT_SYM_LEFT_SHIFT;
                    buffer_ptr_safe_move(buffer);

                    if (*buffer->cur == '=')
                    {
                        // <<=
                        token->type = JLT_SYM_LEFT_SHIFT_ASSIGNMENT;
                        buffer_ptr_safe_move(buffer);
                    }
                }

                break;
            case '!':
                token->class = JT_OPERATOR;
                token->type = JLT_SYM_EXCALMATION;
                buffer_ptr_safe_move(buffer);

                /**
                 * accept !=
                */
                if (*buffer->cur == '=')
                {
                    token->type = JLT_SYM_NOT_EQUAL;
                    buffer_ptr_safe_move(buffer);
                }

                break;
            case '~':
                token->class = JT_OPERATOR;
                token->type = JLT_SYM_TILDE;
                buffer_ptr_safe_move(buffer);
                break;
            case '+':
                token->class = JT_OPERATOR;
                token->type = JLT_SYM_PLUS;
                buffer_ptr_safe_move(buffer);

                c = *buffer->cur;

                /**
                 * accept +=, ++
                */
                if (c == '=')
                {
                    token->type = JLT_SYM_ADD_ASSIGNMENT;
                    buffer_ptr_safe_move(buffer);
                }
                else if (c == '+')
                {
                    token->type = JLT_SYM_INCREMENT;
                    buffer_ptr_safe_move(buffer);
                }

                break;
            case '-':
                token->class = JT_OPERATOR;
                token->type = JLT_SYM_MINUS;
                buffer_ptr_safe_move(buffer);

                c = *buffer->cur;

                /**
                 * accept -=, --, ->
                */
                if (c == '=')
                {
                    token->type = JLT_SYM_SUBTRACT_ASSIGNMENT;
                    buffer_ptr_safe_move(buffer);
                }
                else if (c == '-')
                {
                    token->type = JLT_SYM_DECREMENT;
                    buffer_ptr_safe_move(buffer);
                }
                else if (c == '>')
                {
                    token->type = JLT_SYM_ARROW;
                    buffer_ptr_safe_move(buffer);
                }

                break;
            case '*':
                token->class = JT_OPERATOR;
                token->type = JLT_SYM_ASTERISK;
                buffer_ptr_safe_move(buffer);

                /**
                 * accept *=
                */
                if (*buffer->cur == '=')
                {
                    token->type = JLT_SYM_MULTIPLY_ASSIGNMENT;
                    buffer_ptr_safe_move(buffer);
                }

                break;
            case '/':
                token->class = JT_OPERATOR;
                token->type = JLT_SYM_FORWARD_SLASH;
                buffer_ptr_safe_move(buffer);

                c = *buffer->cur;

                /**
                 * accept /=, or comment
                */
                if (c == '=')
                {
                    token->type = JLT_SYM_DIVIDE_ASSIGNMENT;
                    buffer_ptr_safe_move(buffer);
                }
                else if (c == '*')
                {
                    // now we trigger multi-line comment
                    token->class = JT_COMMENT;
                    token->type = JLT_CMT_MULTI_LINE;

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
                    token->class = JT_COMMENT;
                    token->type = JLT_CMT_SINGLE_LINE;

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
                token->class = JT_OPERATOR;
                token->type = JLT_SYM_AMPERSAND;
                buffer_ptr_safe_move(buffer);

                c = *buffer->cur;

                /**
                 * accept &=, &&
                */
                if (c == '=')
                {
                    token->type = JLT_SYM_BIT_AND_ASSIGNMENT;
                    buffer_ptr_safe_move(buffer);
                }
                else if (c == '&')
                {
                    token->type = JLT_SYM_LOGIC_AND;
                    buffer_ptr_safe_move(buffer);
                }

                break;
            case '|':
                token->class = JT_OPERATOR;
                token->type = JLT_SYM_PIPE;
                buffer_ptr_safe_move(buffer);

                c = *buffer->cur;

                /**
                 * accept &=, &&
                */
                if (c == '=')
                {
                    token->type = JLT_SYM_BIT_OR_ASSIGNMENT;
                    buffer_ptr_safe_move(buffer);
                }
                else if (c == '|')
                {
                    token->type = JLT_SYM_LOGIC_OR;
                    buffer_ptr_safe_move(buffer);
                }

                break;
            case '^':
                token->class = JT_OPERATOR;
                token->type = JLT_SYM_CARET;
                buffer_ptr_safe_move(buffer);

                c = *buffer->cur;

                /**
                 * accept ^=
                */
                if (c == '=')
                {
                    token->type = JLT_SYM_BIT_XOR_ASSIGNMENT;
                    buffer_ptr_safe_move(buffer);
                }

                break;
            case '%':
                token->class = JT_OPERATOR;
                token->type = JLT_SYM_PERCENT;
                buffer_ptr_safe_move(buffer);

                c = *buffer->cur;

                /**
                 * accept %=
                */
                if (c == '=')
                {
                    token->type = JLT_SYM_MODULO_ASSIGNMENT;
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
                token->class = JT_LITERAL;
                token->type = JLT_LTR_CHARACTER;

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
                token->class = JT_LITERAL;
                token->type = JLT_LTR_STRING;

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
                token->class = JT_ILLEGAL;
                buffer_ptr_safe_move(buffer);
                break;
        }
    }

    // character pointed by "to" is the very first one that is NOT part of the token
    token->to = buffer->cur;

    // check if id is a keyword
    if (token->class == JT_IDENTIFIER)
    {
        size_t len = buffer_count(token->from, token->to);
        // len + 1 because we need \0 to terminate string
        char* content = (char*)malloc_assert(sizeof(char) * (len + 1));

        buffer_substring(content, token->from, len);
        java_symbol* sym = symbol_lookup(table, content);

        if (sym)
        {
            token->class = JT_RESERVED_WORD;
            token->type = sym->word->id;
            token->keyword = sym->word;
        }

        free(content);
    }

    /**
     * enclosure validation
     *
     * validate all enclosure forms have been enclosed properly
     * includes: multi-line comment, character literal, string literal
    */
    c = buffer_peek(buffer, -1);
    if (token->class == JT_LITERAL)
    {
        if (token->type == JLT_LTR_CHARACTER)
        {
            if (c != '\'')
            {
                /**
                 * TODO: log error
                */
                fprintf(stderr, "TODO error: character literal incomplete");
            }
        }
        else if (token->type == JLT_LTR_STRING)
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
    else if (token->class == JT_COMMENT)
    {
        if (token->type == JLT_CMT_MULTI_LINE)
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

/**
 * delete token memory and release all allocated content within
*/
void free_token(java_token* token)
{
    free(token);
}
