#include "token.h"

static bool consume_char_default(java_lexer* lexer)
{
    if (buffer_ptr_safe_move(lexer->buffer))
    {
        lexer->ln_cur.col++;
        return true;
    }

    return false;
}

static bool revert_char_default(java_lexer* lexer)
{
    if (lexer->buffer->base != lexer->buffer->cur)
    {
        lexer->buffer->cur--;
        lexer->ln_cur.col--;
        return true;
    }

    return false;
}

/**
 * token helper
 *
 * consume decimal digits, returns false if accepted nothing
*/
static bool consume_digits(java_lexer* lexer)
{
    if (!isdigit(*lexer->buffer->cur))
    {
        return false;
    }

    while (isdigit(*lexer->buffer->cur))
    {
        consume_char_default(lexer);
    }

    return true;
}

/**
 * token helper
 *
 * start from current, try accepting exponent part of a number
 * true is accepted
*/
static bool consume_number_exponent_part(java_lexer* lexer)
{
    char c = *lexer->buffer->cur;

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
        char sign_peek = buffer_peek(lexer->buffer, 1);
        char digit_peek = buffer_peek(lexer->buffer, 2);
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
            consume_char_default(lexer);

            // consume sign
            if (has_sign)
            {
                consume_char_default(lexer);
            }

            // same idea, consume digits
            consume_digits(lexer);

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
static bool consume_newline(java_lexer* lexer)
{
    char c = *lexer->buffer->cur;

    if (isvspacechar(c))
    {
        buffer_ptr_safe_move(lexer->buffer);

        // consume CRLF as a single newline
        if (c == '\r' && *lexer->buffer->cur == '\n')
        {
            buffer_ptr_safe_move(lexer->buffer);
        }

        line_copy(&lexer->ln_prev, &lexer->ln_cur);
        lexer->ln_cur.ln++;
        lexer->ln_cur.col = 1;

        return true;
    }

    return false;
}

/**
 * token helper
 *
 * consume all spaces
*/
static void consume_spaces(java_lexer* lexer)
{
    char c;

    while (true)
    {
        if (consume_newline(lexer))
        {
            continue;
        }

        c = *lexer->buffer->cur;

        if (ishspacechar(c))
        {
            // simply consume it one by one
            consume_char_default(lexer);
        }
        else
        {
            // stop when no longer a space character
            break;
        }
    }
}

/**
 * Tokenize next as ID
*/
static void lexer_next_as_identifier(java_lexer* lexer, java_token* token)
{
    token->class = JT_IDENTIFIER;

    // before 1st iteration, lexer consumes
    // first ID char that just checked
    while (consume_char_default(lexer))
    {
        char c = *lexer->buffer->cur;

        if (!isidchar(c))
        {
            break;
        }
    }
}

/**
 * Tokenize next as number
*/
static void lexer_next_as_number(java_lexer* lexer, java_token* token)
{
    token->class = JT_LITERAL;
    token->type = JLT_LTR_NUMBER;
    token->number.type = JT_NUM_DEC;
    token->number.bits = JT_NUM_BIT_LENGTH_NORMAL;

    char c = *lexer->buffer->cur;
    file_buffer* buffer = lexer->buffer;
    byte peek_0, peek_1;

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
            consume_char_default(lexer);
        }
        else if (isbinaryindicator(after_first_digit))
        {
            token->number.type = JT_NUM_BIN;
            consume_char_default(lexer);
        }
        else if (isdigit(after_first_digit))
        {
            // for octal, valid digits are 0-7,
            // but we losen the rule a bit because we do not have
            // to go that far
            token->number.type = JT_NUM_OCT;
            consume_char_default(lexer);
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
            while (consume_char_default(lexer))
            {
                if (!isxdigit(*buffer->cur))
                {
                    break;
                }
            }
            break;
        case JT_NUM_BIN:
            // for bin, stop when not a bin digit
            while (consume_char_default(lexer))
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
            consume_digits(lexer);
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
            consume_digits(lexer);

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
                consume_char_default(lexer);

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
                if (consume_digits(lexer) || is_eof(buffer))
                {
                    token->number.type = JT_NUM_FP_DOUBLE;
                }
            }

            // exponent part
            if (consume_number_exponent_part(lexer))
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
            lexer_error(lexer, token, JAVA_E_NO_DIGIT);
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
    peek_0 = buffer_peek(buffer, -1); // char before current
    peek_1 = buffer_peek(buffer, 1); // char after cur

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
        isfractionindicator(peek_0) && (
            isidchar(peek_1)
            || peek_1 == '('
            || peek_1 == '[');

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
                consume_char_default(lexer);
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
            consume_char_default(lexer);
        }
        else if (isfractionindicator(peek_0))
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
            revert_char_default(lexer);
        }
    }
    else if (isdoublesuffix(c))
    {
        if (!should_reject_suffix_trigger)
        {
            // by default we have double type, 
            // so no need to set type again
            consume_char_default(lexer);
        }
        else if (isfractionindicator(peek_0))
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
            revert_char_default(lexer);
        }
    }
    else if (isfractionindicator(peek_0))
    {
        if (isidchar(c))
        {
            /**
             * if last is a DOT but no suffix trigger, we only accept the DOT
             * when the next character does NOT trigger an ID acceptance
            */
            revert_char_default(lexer);
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

/**
 * Toke Data Initializer
*/
void init_token(java_token* token)
{
    token->from = NULL;
    token->class = JT_EOF;
    token->type = JLT_MAX;
    token->ln_begin = LINE(0, 0);
    token->ln_end = LINE(0, 0);
    token->keyword = NULL;
    token->number.type = JT_NUM_MAX;
    token->number.bits = JT_NUM_BIT_LENGTH_NORMAL;
}

/**
 * release token content
*/
void release_token(java_token* token)
{
    // so far there is nothing to do...
}

/**
 * delete token memory and release all allocated content within
*/
void delete_token(java_token* token)
{
    release_token(token);
    free(token);
}

/**
 * Initializer Lexer Instance
*/
void init_lexer(java_lexer* lexer, file_buffer* buffer, hash_table* rw, java_error_logger* logger)
{
    lexer->is_copy = false; // MUST be false from this routine

    lexer->buffer = buffer;
    lexer->rw = rw;
    lexer->logger = logger;

    lexer->expect = JLT_MAX;
    lexer->ln_cur = LINE(1, 1);
    lexer->ln_prev = LINE(1, 1);
}

/**
 * Create a copy of lexer so it can work independently
 *
 * everything simple data needs to be copied, for pointers:
 *
 * rw rable: no need to copy
 * buffer: needs a shallow-isolation, but file map does not need to be copied
*/
java_lexer* copy_lexer(const java_lexer* lexer)
{
    java_lexer* copy = (java_lexer*)malloc_assert(sizeof(java_lexer));

    // shallow copy everything simple
    memcpy(copy, lexer, sizeof(java_lexer));

    // flag it as a copy
    copy->is_copy = true;

    // buffer needs shallow isolation
    copy->buffer = (file_buffer*)malloc_assert(sizeof(file_buffer));
    memcpy(copy->buffer, lexer->buffer, sizeof(file_buffer));

    return copy;
}

/**
 * Release Lexer Instance
*/
void release_lexer(java_lexer* lexer)
{
    if (lexer->is_copy)
    {
        // buffer is a shallow copy, so no release, simply free it
        free(lexer->buffer);
    }
}

/**
 * Error Logger
 *
 * TODO: we need a clever way to get line info from java_lexer instance
*/
void lexer_error(java_lexer* lexer, java_token* token, java_error_id id, ...)
{
    va_list args;

    va_start(args, id);
    error_logger_vslog(
        lexer->logger,
        token ? &token->ln_begin : &lexer->ln_cur,
        token ? &token->ln_end : NULL,
        id,
        &args
    );
    va_end(args);
}

/**
 * Set next expected token
 *
 * this flag is used to resolve lexical ambiguity
*/
void lexer_expect(java_lexer* lexer, java_lexeme_type token_type)
{
    lexer->expect = token_type;
}

/**
 * tokenizer
 *
 * longest match will be returned, except for illegal characters,
 * which will be returned one by one
 *
 * a copy of EOF token will keep recurring once buffer reached the end
*/
void lexer_next_token(java_lexer* lexer, java_token* token)
{
    file_buffer* buffer = lexer->buffer;

    /**
     * escape space sequence
     *
     * we have to do this before everything to put
     * stream cursor at right location
    */
    consume_spaces(lexer);

    init_token(token);
    token->from = buffer->cur;

    if (is_eof(buffer))
    {
        return;
    }

    char c = *buffer->cur;
    line_copy(&token->ln_begin, &lexer->ln_cur);

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
        lexer_next_as_identifier(lexer, token);
    }
    else if (isdigit(c))
    {
        lexer_next_as_number(lexer, token);
    }
    else
    {
        switch (c)
        {
            case '(':
                token->class = JT_SEPARATOR;
                token->type = JLT_SYM_PARENTHESIS_OPEN;
                consume_char_default(lexer);
                break;
            case ')':
                token->class = JT_SEPARATOR;
                token->type = JLT_SYM_PARENTHESIS_CLOSE;
                consume_char_default(lexer);
                break;
            case '{':
                token->class = JT_SEPARATOR;
                token->type = JLT_SYM_BRACE_OPEN;
                consume_char_default(lexer);
                break;
            case '}':
                token->class = JT_SEPARATOR;
                token->type = JLT_SYM_BRACE_CLOSE;
                consume_char_default(lexer);
                break;
            case '[':
                token->class = JT_SEPARATOR;
                token->type = JLT_SYM_BRACKET_OPEN;
                consume_char_default(lexer);
                break;
            case ']':
                token->class = JT_SEPARATOR;
                token->type = JLT_SYM_BRACKET_CLOSE;
                consume_char_default(lexer);
                break;
            case ';':
                token->class = JT_SEPARATOR;
                token->type = JLT_SYM_SEMICOLON;
                consume_char_default(lexer);
                break;
            case ',':
                token->class = JT_SEPARATOR;
                token->type = JLT_SYM_COMMA;
                consume_char_default(lexer);
                break;
            case '@':
                token->class = JT_SEPARATOR;
                token->type = JLT_SYM_AT;
                consume_char_default(lexer);
                break;
            case '?':
                token->class = JT_SEPARATOR;
                token->type = JLT_SYM_QUESTION;
                consume_char_default(lexer);
                break;
            case '.':
            {
                token->class = JT_SEPARATOR;
                token->type = JLT_SYM_DOT;
                consume_char_default(lexer);

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
                        consume_char_default(lexer);
                        consume_char_default(lexer);
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
                    consume_digits(lexer);
                    consume_number_exponent_part(lexer);

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
                        consume_char_default(lexer);
                    }
                    else if (isdoublesuffix(c))
                    {
                        // by default we have double type, 
                        // so no need to set type again
                        consume_char_default(lexer);
                    }
                }

                break;
            }
            case ':':
                token->class = JT_SEPARATOR;
                token->type = JLT_SYM_COLON;
                consume_char_default(lexer);

                /**
                 * accept ::
                */
                if (*buffer->cur == ':')
                {
                    token->type = JLT_SYM_METHOD_REFERENCE;
                    consume_char_default(lexer);
                }

                break;
            case '=':
                token->class = JT_OPERATOR;
                token->type = JLT_SYM_EQUAL;
                consume_char_default(lexer);

                /**
                 * accept ==
                */
                if (*buffer->cur == '=')
                {
                    token->type = JLT_SYM_RELATIONAL_EQUAL;
                    consume_char_default(lexer);
                }

                break;
            case '>':
                token->class = JT_OPERATOR;
                token->type = JLT_SYM_ANGLE_BRACKET_CLOSE;
                consume_char_default(lexer);

                c = *buffer->cur;

                /**
                 * accept >=, >>, >>>, >>=, >>>=
                */
                if (c == '=')
                {
                    // >=
                    token->type = JLT_SYM_GREATER_EQUAL;
                    consume_char_default(lexer);
                }
                else if (c == '>')
                {
                    // >>
                    token->type = JLT_SYM_RIGHT_SHIFT;
                    consume_char_default(lexer);

                    c = *buffer->cur;

                    if (c == '>')
                    {
                        // >>>
                        token->type = JLT_SYM_RIGHT_SHIFT_UNSIGNED;
                        consume_char_default(lexer);

                        if (*buffer->cur == '=')
                        {
                            // >>>=
                            token->type = JLT_SYM_RIGHT_SHIFT_UNSIGNED_ASSIGNMENT;
                            consume_char_default(lexer);
                        }
                    }
                    else if (c == '=')
                    {
                        // >>=
                        token->type = JLT_SYM_RIGHT_SHIFT_ASSIGNMENT;
                        consume_char_default(lexer);
                    }
                }

                break;
            case '<':
                token->class = JT_OPERATOR;
                token->type = JLT_SYM_ANGLE_BRACKET_OPEN;
                consume_char_default(lexer);

                c = *buffer->cur;

                /**
                 * accept <=, <<, <<=
                */
                if (c == '=')
                {
                    // <=
                    token->type = JLT_SYM_LESS_EQUAL;
                    consume_char_default(lexer);
                }
                else if (c == '<')
                {
                    // <<
                    token->type = JLT_SYM_LEFT_SHIFT;
                    consume_char_default(lexer);

                    if (*buffer->cur == '=')
                    {
                        // <<=
                        token->type = JLT_SYM_LEFT_SHIFT_ASSIGNMENT;
                        consume_char_default(lexer);
                    }
                }

                break;
            case '!':
                token->class = JT_OPERATOR;
                token->type = JLT_SYM_EXCALMATION;
                consume_char_default(lexer);

                /**
                 * accept !=
                */
                if (*buffer->cur == '=')
                {
                    token->type = JLT_SYM_NOT_EQUAL;
                    consume_char_default(lexer);
                }

                break;
            case '~':
                token->class = JT_OPERATOR;
                token->type = JLT_SYM_TILDE;
                consume_char_default(lexer);
                break;
            case '+':
                token->class = JT_OPERATOR;
                token->type = JLT_SYM_PLUS;
                consume_char_default(lexer);

                c = *buffer->cur;

                /**
                 * accept +=, ++
                */
                if (c == '=')
                {
                    token->type = JLT_SYM_ADD_ASSIGNMENT;
                    consume_char_default(lexer);
                }
                else if (c == '+')
                {
                    token->type = JLT_SYM_INCREMENT;
                    consume_char_default(lexer);
                }

                break;
            case '-':
                token->class = JT_OPERATOR;
                token->type = JLT_SYM_MINUS;
                consume_char_default(lexer);

                c = *buffer->cur;

                /**
                 * accept -=, --, ->
                */
                if (c == '=')
                {
                    token->type = JLT_SYM_SUBTRACT_ASSIGNMENT;
                    consume_char_default(lexer);
                }
                else if (c == '-')
                {
                    token->type = JLT_SYM_DECREMENT;
                    consume_char_default(lexer);
                }
                else if (c == '>')
                {
                    token->type = JLT_SYM_ARROW;
                    consume_char_default(lexer);
                }

                break;
            case '*':
                token->class = JT_OPERATOR;
                token->type = JLT_SYM_ASTERISK;
                consume_char_default(lexer);

                /**
                 * accept *=
                */
                if (*buffer->cur == '=')
                {
                    token->type = JLT_SYM_MULTIPLY_ASSIGNMENT;
                    consume_char_default(lexer);
                }

                break;
            case '/':
                token->class = JT_OPERATOR;
                token->type = JLT_SYM_FORWARD_SLASH;
                consume_char_default(lexer);

                c = *buffer->cur;

                /**
                 * accept /=, or comment
                */
                if (c == '=')
                {
                    token->type = JLT_SYM_DIVIDE_ASSIGNMENT;
                    consume_char_default(lexer);
                }
                else if (c == '*')
                {
                    // now we trigger multi-line comment
                    token->class = JT_COMMENT;
                    token->type = JLT_CMT_MULTI_LINE;

                    while (consume_char_default(lexer))
                    {
                        if (*buffer->cur == '*' && buffer_peek(buffer, 1) == '/')
                        {
                            // consume twice for the enclosure sequence
                            consume_char_default(lexer);
                            consume_char_default(lexer);
                            break;
                        }
                    }
                }
                else if (c == '/')
                {
                    // now we trigger one-line comment
                    token->class = JT_COMMENT;
                    token->type = JLT_CMT_SINGLE_LINE;

                    while (consume_char_default(lexer))
                    {
                        // try consuming again to see if next is a newline
                        // sequence, if so we terminate
                        if (consume_newline(lexer))
                        {
                            break;
                        }
                    }
                }

                break;
            case '&':
                token->class = JT_OPERATOR;
                token->type = JLT_SYM_AMPERSAND;
                consume_char_default(lexer);

                c = *buffer->cur;

                /**
                 * accept &=, &&
                */
                if (c == '=')
                {
                    token->type = JLT_SYM_BIT_AND_ASSIGNMENT;
                    consume_char_default(lexer);
                }
                else if (c == '&')
                {
                    token->type = JLT_SYM_LOGIC_AND;
                    consume_char_default(lexer);
                }

                break;
            case '|':
                token->class = JT_OPERATOR;
                token->type = JLT_SYM_PIPE;
                consume_char_default(lexer);

                c = *buffer->cur;

                /**
                 * accept &=, &&
                */
                if (c == '=')
                {
                    token->type = JLT_SYM_BIT_OR_ASSIGNMENT;
                    consume_char_default(lexer);
                }
                else if (c == '|')
                {
                    token->type = JLT_SYM_LOGIC_OR;
                    consume_char_default(lexer);
                }

                break;
            case '^':
                token->class = JT_OPERATOR;
                token->type = JLT_SYM_CARET;
                consume_char_default(lexer);

                c = *buffer->cur;

                /**
                 * accept ^=
                */
                if (c == '=')
                {
                    token->type = JLT_SYM_BIT_XOR_ASSIGNMENT;
                    consume_char_default(lexer);
                }

                break;
            case '%':
                token->class = JT_OPERATOR;
                token->type = JLT_SYM_PERCENT;
                consume_char_default(lexer);

                c = *buffer->cur;

                /**
                 * accept %=
                */
                if (c == '=')
                {
                    token->type = JLT_SYM_MODULO_ASSIGNMENT;
                    consume_char_default(lexer);
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

                while (consume_char_default(lexer))
                {
                    // we stop at ' but continue if it is escaped
                    if (*buffer->cur == '\'' && c != '\\')
                    {
                        // consume enclosure sequence
                        consume_char_default(lexer);
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

                while (consume_char_default(lexer))
                {
                    // we stop at ' but continue if it is escaped
                    if (*buffer->cur == '\"' && c != '\\')
                    {
                        // consume enclosure sequence
                        consume_char_default(lexer);
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
                consume_char_default(lexer);
                lexer_error(lexer, token, JAVA_E_ILLEGAL_CHARACTER, (byte)c);
                break;
        }
    }

    // character pointed by "to" is the very first one that is NOT part of the token
    token->to = buffer->cur;
    line_copy(&token->ln_end, &lexer->ln_cur);

    ///// EPILOGUE: NO CURSOR CHANGE BEYOND THIS POINT /////

    // check if id is a keyword
    if (token->class == JT_IDENTIFIER)
    {
        size_t len = buffer_count(token->from, token->to);
        // len + 1 because we need \0 to terminate string
        char* content = (char*)malloc_assert(sizeof(char) * (len + 1));

        buffer_substring(content, token->from, len);
        java_reserved_word* sym = symbol_lookup(lexer->rw, content);

        if (sym)
        {
            token->class = JT_RESERVED_WORD;
            token->type = sym->id;
            token->keyword = sym;
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
                lexer_error_missing_token(lexer, token, JAVA_E_CHARACTER_LITERAL_ENCLOSE, "'");
            }
        }
        else if (token->type == JLT_LTR_STRING)
        {
            if (c != '\"')
            {
                lexer_error_missing_token(lexer, token, JAVA_E_STRING_LITERAL_ENCLOSE, "\"");
            }
        }
    }
    else if (token->class == JT_COMMENT)
    {
        if (token->type == JLT_CMT_MULTI_LINE)
        {
            if (c != '/' || buffer_peek(buffer, -2) != '*')
            {
                lexer_error_missing_token(lexer, token, JAVA_E_MULTILINE_COMMENT_ENCLOSE, "*/");
            }
        }
    }

    // reset lexer expect
    lexer_expect(lexer, JLT_MAX);
}
