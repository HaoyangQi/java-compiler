#include "debug.h"

#include "langspec.h"
#include "token.h"
#include "node.h"
#include "expression.h"
#include "number.h"

typedef struct
{
    char* content;
    java_number_type type;
} debug_number_data;

static const debug_number_data test_numbers[] = {
    { "0x123456789abcdef", JT_NUM_HEX },
    { "0X123456789ABCDEF", JT_NUM_HEX },
    { "0x123456789ABCDEF1234567890", JT_NUM_HEX },

    { "01234567", JT_NUM_OCT },
    { "01234567", JT_NUM_OCT },
    { "01234567111111122222223333333", JT_NUM_OCT },

    { "0b000101010101", JT_NUM_BIN },
    { "0B111000001010", JT_NUM_BIN },

    { "123456789", JT_NUM_DEC },
    { "123456789l", JT_NUM_DEC },
    { "9999999999999999999", JT_NUM_DEC }, // largest integer for fast conversion
    { "10000000000000000000", JT_NUM_DEC }, // larger integer requires big integer library
    { "100000000000000000000", JT_NUM_DEC }, // integer that overflows u64

    { ".0", JT_NUM_FP_DOUBLE },
    { ".0000", JT_NUM_FP_DOUBLE },
    { ".1234567890", JT_NUM_FP_DOUBLE },
    { ".1234567890e10", JT_NUM_FP_DOUBLE },
    { ".1234567890e-10", JT_NUM_FP_DOUBLE },

    { "0.0", JT_NUM_FP_DOUBLE },
    { "0.0000", JT_NUM_FP_DOUBLE },
    { "0.1234567890", JT_NUM_FP_DOUBLE },
    { "0.1234567890e10", JT_NUM_FP_DOUBLE },
    { "0.1234567890e-10", JT_NUM_FP_DOUBLE },

    { "987654321.0", JT_NUM_FP_DOUBLE },
    { "987654321.0000", JT_NUM_FP_DOUBLE },
    { "987654321.1234567890", JT_NUM_FP_DOUBLE },
    { "987654321.1234567890e10", JT_NUM_FP_DOUBLE },
    { "987654321.1234567890e-10", JT_NUM_FP_DOUBLE },

    { ".0", JT_NUM_FP_FLOAT },
    { ".0000", JT_NUM_FP_FLOAT },
    { ".1234567890", JT_NUM_FP_FLOAT },
    { ".1234567890e10", JT_NUM_FP_FLOAT },
    { ".1234567890e-10", JT_NUM_FP_FLOAT },

    { "0.0", JT_NUM_FP_FLOAT },
    { "0.0000", JT_NUM_FP_FLOAT },
    { "0.1234567890", JT_NUM_FP_FLOAT },
    { "0.1234567890e10", JT_NUM_FP_FLOAT },
    { "0.1234567890e-10", JT_NUM_FP_FLOAT },

    { "987654321.0", JT_NUM_FP_FLOAT },
    { "987654321.0000", JT_NUM_FP_FLOAT },
    { "987654321.1234567890", JT_NUM_FP_FLOAT },
    { "987654321.1234567890e10", JT_NUM_FP_FLOAT },
    { "987654321.1234567890e-10", JT_NUM_FP_FLOAT },

    // the following are not numbers, but are expected to be handeled
    { "", JT_NUM_MAX },
    { "0", JT_NUM_MAX },
    { "01", JT_NUM_MAX },
    { "hello, world!", JT_NUM_MAX },
    { "'char'", JT_NUM_MAX },
    { "\"string\"", JT_NUM_MAX },
    { "\\b\\s\\t\\n\\f\\r\\\"\\'\\\\", JT_NUM_MAX },
    { "a\\ba\\sa\\ta\\na\\fa\\ra\\\"a\\'a\\\\a", JT_NUM_MAX },
    { "\\uc0feabcd", JT_NUM_MAX },
    { "\\uuuuuuc0feabcd", JT_NUM_MAX },
    { "\\377", JT_NUM_MAX },
    { "\\78", JT_NUM_MAX },
    { "\\777", JT_NUM_MAX },
    { "\\1234567", JT_NUM_MAX },
};

static void __format_print_indentation(size_t depth)
{
    for (size_t d = 0; d < depth; d++) { printf("    "); }
}

static void __format_print_binary_stream(const char* stream, size_t bin_len, size_t depth)
{
    size_t flag;
    char hex_tbl_print[8];
    char hex_tbl_cur;
    unsigned char pr;

    __format_print_indentation(depth);
    printf("            HEX             |  CHAR  \n");
    __format_print_indentation(depth);
    printf("----------------------------+--------\n");

    for (size_t i = 0, j = 0; i < bin_len; i++)
    {
        flag = i % 8;
        hex_tbl_cur = isgraph(stream[i]) ? stream[i] : '.';

        if (i != 0 && (i == bin_len - 1 || flag == 0))
        {
            __format_print_indentation(depth);

            if (i == bin_len - 1)
            {
                hex_tbl_print[flag] = hex_tbl_cur;
                i++;
            }

            for (size_t k = j; k < j + 8; k++)
            {
                pr = stream[k];

                if (k >= i)
                {
                    if (k - j == 4) { printf("      "); }
                    else { printf("   "); }
                }
                else if (k - j == 4) { printf("    %02X", pr); }
                else if (k - j > 0) { printf(" %02X", pr); }
                else { printf("%02X", pr); }
            }

            printf("   ");

            for (size_t k = j; k < i; k++)
            {
                printf("%c", hex_tbl_print[k - j]);
            }

            printf("\n");
            j = i;
        }

        hex_tbl_print[flag] = hex_tbl_cur;
    }
}

static void debug_print_memory(byte* mem, long size, long line_break)
{
    for (long i = 0; i < size; i++)
    {
        if (i % line_break == 0)
        {
            printf("    ");
        }

        byte c = mem[i];
        bool should_break = (i + 1) % line_break == 0 && (i + 1) != size;

        printf("%02X%c%c%s", c,
            isprint(c) ? '/' : ' ',
            isprint(c) ? c : ' ',
            should_break ? "\n" : "    ");
    }
}

static void debug_shash_table(hash_table* table)
{
    printf("===== HASH TABLE =====\n");
    printf("memory: %zd bytes\n", hash_table_memory_size(table));
    printf("load factor: %.2f%%\n", hash_table_load_factor(table) * 100.0f);
    printf("longest chain: %zd\n", hash_table_longest_chain_length(table));

    for (size_t i = 0; i < table->bucket_size; i++)
    {
        printf("    ");

        hash_pair* p = table->bucket[i];

        while (p)
        {
            printf("(%s, %p)->", (char*)(p->key), p->value);
            p = p->next;
        }

        printf("(null)\n");
    }
}

void debug_report(compiler* compiler)
{
    printf("===== COMPILER RUNTIME REPORT =====\n");

    printf("Language version: %d\n", compiler->version);
    printf("Reserved word:\n");
    printf("    count: %d\n", num_java_reserved_words);
    printf("    memory: %zd bytes\n", hash_table_memory_size(&compiler->rw_lookup_table));
    printf("    load factor: %.2f%%\n", hash_table_load_factor(&compiler->rw_lookup_table) * 100.0f);
    printf("    longest chain: %zd\n", hash_table_longest_chain_length(&compiler->rw_lookup_table));
    printf("Expression static data size: %zd bytes\n",
        sizeof(java_expression) +
        sizeof(java_operator) * OPID_MAX +
        sizeof(operator_id) * JLT_MAX +
        sizeof(operation) * OPID_MAX +
        sizeof(size_t) * OPID_MAX
    );
    printf("Error static data size: %zd bytes\n",
        sizeof(java_error_definition) +
        sizeof(error_descriptor) * JAVA_E_MAX +
        sizeof(char*) * JAVA_E_MAX
    );

    printf("===== END OF REPORT =====\n");
}

void debug_file_buffer(file_buffer* reader)
{
    printf("===== FILE BUFFER =====\n");
    printf("size: %ld byte(s)\n", reader->size);
    printf("buffer: 0x%p\n", (void*)(reader->base));
    printf("cur: 0x%p", (void*)(reader->cur));

    if (reader->base)
    {
        if (reader->size <= 0)
        {
            printf("\n    (buffer registered but size is invalid)");
        }

        printf("\n");

        debug_print_memory(reader->base, reader->size, 10);

        printf("\n");
    }
    else
    {
        printf("\n    (NULL)\n");
    }
}

void debug_print_reserved_words()
{
    printf("===== JAVA RESERVED WORDS =====\n");

    for (int i = 0; i < num_java_reserved_words; i++)
    {
        printf("%s %d\n", java_reserved_words[i].content, java_reserved_words[i].id);
    }
}

void debug_print_symbol_table(hash_table* table)
{
    printf("===== RESERVED WORDS LOOKUP =====\n");
    debug_shash_table(table);
}

static void debug_print_lexeme_type(java_lexeme_type id)
{
    switch (id)
    {
        case JLT_RWD_PUBLIC:
            printf("JLT_RWD_PUBLIC");
            break;
        case JLT_RWD_PRIVATE:
            printf("JLT_RWD_PRIVATE");
            break;
        case JLT_RWD_PROTECTED:
            printf("JLT_RWD_PROTECTED");
            break;
        case JLT_RWD_FINAL:
            printf("JLT_RWD_FINAL");
            break;
        case JLT_RWD_STATIC:
            printf("JLT_RWD_STATIC");
            break;
        case JLT_RWD_ABSTRACT:
            printf("JLT_RWD_ABSTRACT");
            break;
        case JLT_RWD_TRANSIENT:
            printf("JLT_RWD_TRANSIENT");
            break;
        case JLT_RWD_SYNCHRONIZED:
            printf("JLT_RWD_SYNCHRONIZED");
            break;
        case JLT_RWD_VOLATILE:
            printf("JLT_RWD_VOLATILE");
            break;
        case JLT_RWD_DEFAULT:
            printf("JLT_RWD_DEFAULT");
            break;
        case JLT_RWD_IF:
            printf("JLT_RWD_IF");
            break;
        case JLT_RWD_THROW:
            printf("JLT_RWD_THROW");
            break;
        case JLT_RWD_BOOLEAN:
            printf("JLT_RWD_BOOLEAN");
            break;
        case JLT_RWD_DO:
            printf("JLT_RWD_DO");
            break;
        case JLT_RWD_IMPLEMENTS:
            printf("JLT_RWD_IMPLEMENTS");
            break;
        case JLT_RWD_THROWS:
            printf("JLT_RWD_THROWS");
            break;
        case JLT_RWD_BREAK:
            printf("JLT_RWD_BREAK");
            break;
        case JLT_RWD_DOUBLE:
            printf("JLT_RWD_DOUBLE");
            break;
        case JLT_RWD_IMPORT:
            printf("JLT_RWD_IMPORT");
            break;
        case JLT_RWD_BYTE:
            printf("JLT_RWD_BYTE");
            break;
        case JLT_RWD_ELSE:
            printf("JLT_RWD_ELSE");
            break;
        case JLT_RWD_INSTANCEOF:
            printf("JLT_RWD_INSTANCEOF");
            break;
        case JLT_RWD_RETURN:
            printf("JLT_RWD_RETURN");
            break;
        case JLT_RWD_TRY:
            printf("JLT_RWD_TRY");
            break;
        case JLT_RWD_CASE:
            printf("JLT_RWD_CASE");
            break;
        case JLT_RWD_EXTENDS:
            printf("JLT_RWD_EXTENDS");
            break;
        case JLT_RWD_INT:
            printf("JLT_RWD_INT");
            break;
        case JLT_RWD_SHORT:
            printf("JLT_RWD_SHORT");
            break;
        case JLT_RWD_VOID:
            printf("JLT_RWD_VOID");
            break;
        case JLT_RWD_CATCH:
            printf("JLT_RWD_CATCH");
            break;
        case JLT_RWD_INTERFACE:
            printf("JLT_RWD_INTERFACE");
            break;
        case JLT_RWD_CHAR:
            printf("JLT_RWD_CHAR");
            break;
        case JLT_RWD_FINALLY:
            printf("JLT_RWD_FINALLY");
            break;
        case JLT_RWD_LONG:
            printf("JLT_RWD_LONG");
            break;
        case JLT_RWD_SUPER:
            printf("JLT_RWD_SUPER");
            break;
        case JLT_RWD_WHILE:
            printf("JLT_RWD_WHILE");
            break;
        case JLT_RWD_CLASS:
            printf("JLT_RWD_CLASS");
            break;
        case JLT_RWD_FLOAT:
            printf("JLT_RWD_FLOAT");
            break;
        case JLT_RWD_NATIVE:
            printf("JLT_RWD_NATIVE");
            break;
        case JLT_RWD_SWITCH:
            printf("JLT_RWD_SWITCH");
            break;
        case JLT_RWD_FOR:
            printf("JLT_RWD_FOR");
            break;
        case JLT_RWD_NEW:
            printf("JLT_RWD_NEW");
            break;
        case JLT_RWD_CONTINUE:
            printf("JLT_RWD_CONTINUE");
            break;
        case JLT_RWD_PACKAGE:
            printf("JLT_RWD_PACKAGE");
            break;
        case JLT_RWD_THIS:
            printf("JLT_RWD_THIS");
            break;
        case JLT_RWD_CONST:
            printf("JLT_RWD_CONST");
            break;
        case JLT_RWD_GOTO:
            printf("JLT_RWD_GOTO");
            break;
        case JLT_RWD_TRUE:
            printf("JLT_RWD_TRUE");
            break;
        case JLT_RWD_FALSE:
            printf("JLT_RWD_FALSE");
            break;
        case JLT_RWD_NULL:
            printf("JLT_RWD_NULL");
            break;
        case JLT_LTR_NUMBER:
            printf("Number");
            break;
        case JLT_LTR_CHARACTER:
            printf("Character");
            break;
        case JLT_LTR_STRING:
            printf("String");
            break;
        case JLT_SYM_EQUAL:
            printf("JLT_SYM_EQUAL \"=\"");
            break;
        case JLT_SYM_ANGLE_BRACKET_CLOSE:
            printf("JLT_SYM_ANGLE_BRACKET_CLOSE \">\"");
            break;
        case JLT_SYM_ANGLE_BRACKET_OPEN:
            printf("JLT_SYM_ANGLE_BRACKET_OPEN \"<\"");
            break;
        case JLT_SYM_EXCALMATION:
            printf("JLT_SYM_EXCALMATION \"!\"");
            break;
        case JLT_SYM_TILDE:
            printf("JLT_SYM_TILDE \"~\"");
            break;
        case JLT_SYM_ARROW:
            printf("JLT_SYM_ARROW \"->\"");
            break;
        case JLT_SYM_RELATIONAL_EQUAL:
            printf("JLT_SYM_RELATIONAL_EQUAL \"==\"");
            break;
        case JLT_SYM_LESS_EQUAL:
            printf("JLT_SYM_LESS_EQUAL \"<=\"");
            break;
        case JLT_SYM_GREATER_EQUAL:
            printf("JLT_SYM_GREATER_EQUAL \">=\"");
            break;
        case JLT_SYM_NOT_EQUAL:
            printf("JLT_SYM_NOT_EQUAL \"!=\"");
            break;
        case JLT_SYM_LOGIC_AND:
            printf("JLT_SYM_LOGIC_AND \"&&\"");
            break;
        case JLT_SYM_LOGIC_OR:
            printf("JLT_SYM_LOGIC_OR \"||\"");
            break;
        case JLT_SYM_INCREMENT:
            printf("JLT_SYM_INCREMENT \"++\"");
            break;
        case JLT_SYM_DECREMENT:
            printf("JLT_SYM_DECREMENT \"--\"");
            break;
        case JLT_SYM_PLUS:
            printf("JLT_SYM_PLUS \"+\"");
            break;
        case JLT_SYM_MINUS:
            printf("JLT_SYM_MINUS \"-\"");
            break;
        case JLT_SYM_ASTERISK:
            printf("JLT_SYM_ASTERISK \"*\"");
            break;
        case JLT_SYM_FORWARD_SLASH:
            printf("JLT_SYM_FORWARD_SLASH \"/\"");
            break;
        case JLT_SYM_AMPERSAND:
            printf("JLT_SYM_AMPERSAND \"&\"");
            break;
        case JLT_SYM_PIPE:
            printf("JLT_SYM_PIPE \"|\"");
            break;
        case JLT_SYM_CARET:
            printf("JLT_SYM_CARET \"^\"");
            break;
        case JLT_SYM_PERCENT:
            printf("JLT_SYM_PERCENT \"%\"");
            break;
        case JLT_SYM_LEFT_SHIFT:
            printf("JLT_SYM_LEFT_SHIFT \"<<\"");
            break;
        case JLT_SYM_RIGHT_SHIFT:
            printf("JLT_SYM_RIGHT_SHIFT \">>\"");
            break;
        case JLT_SYM_RIGHT_SHIFT_UNSIGNED:
            printf("JLT_SYM_RIGHT_SHIFT_UNSIGNED \">>>\"");
            break;
        case JLT_SYM_ADD_ASSIGNMENT:
            printf("JLT_SYM_ADD_ASSIGNMENT \"+=\"");
            break;
        case JLT_SYM_SUBTRACT_ASSIGNMENT:
            printf("JLT_SYM_SUBTRACT_ASSIGNMENT \"-=\"");
            break;
        case JLT_SYM_MULTIPLY_ASSIGNMENT:
            printf("JLT_SYM_MULTIPLY_ASSIGNMENT \"*=\"");
            break;
        case JLT_SYM_DIVIDE_ASSIGNMENT:
            printf("JLT_SYM_DIVIDE_ASSIGNMENT \"/=\"");
            break;
        case JLT_SYM_BIT_AND_ASSIGNMENT:
            printf("JLT_SYM_BIT_AND_ASSIGNMENT \"&=\"");
            break;
        case JLT_SYM_BIT_OR_ASSIGNMENT:
            printf("JLT_SYM_BIT_OR_ASSIGNMENT \"|=\"");
            break;
        case JLT_SYM_BIT_XOR_ASSIGNMENT:
            printf("JLT_SYM_BIT_XOR_ASSIGNMENT \"^=\"");
            break;
        case JLT_SYM_MODULO_ASSIGNMENT:
            printf("JLT_SYM_MODULO_ASSIGNMENT \"%=\"");
            break;
        case JLT_SYM_LEFT_SHIFT_ASSIGNMENT:
            printf("JLT_SYM_LEFT_SHIFT_ASSIGNMENT \"<<=\"");
            break;
        case JLT_SYM_RIGHT_SHIFT_ASSIGNMENT:
            printf("JLT_SYM_RIGHT_SHIFT_ASSIGNMENT \">>=\"");
            break;
        case JLT_SYM_RIGHT_SHIFT_UNSIGNED_ASSIGNMENT:
            printf("JLT_SYM_RIGHT_SHIFT_UNSIGNED_ASSIGNMENT \">>>=\"");
            break;
        case JLT_SYM_PARENTHESIS_OPEN:
            printf("JLT_SYM_PARENTHESIS_OPEN \"(\"");
            break;
        case JLT_SYM_PARENTHESIS_CLOSE:
            printf("JLT_SYM_PARENTHESIS_CLOSE \")\"");
            break;
        case JLT_SYM_BRACE_OPEN:
            printf("JLT_SYM_BRACE_OPEN \"{\"");
            break;
        case JLT_SYM_BRACE_CLOSE:
            printf("JLT_SYM_BRACE_CLOSE \"}\"");
            break;
        case JLT_SYM_BRACKET_OPEN:
            printf("JLT_SYM_BRACKET_OPEN \"[\"");
            break;
        case JLT_SYM_BRACKET_CLOSE:
            printf("JLT_SYM_BRACKET_CLOSE \"]\"");
            break;
        case JLT_SYM_SEMICOLON:
            printf("JLT_SYM_SEMICOLON \";\"");
            break;
        case JLT_SYM_COMMA:
            printf("JLT_SYM_COMMA \",\"");
            break;
        case JLT_SYM_AT:
            printf("JLT_SYM_AT \"@\"");
            break;
        case JLT_SYM_QUESTION:
            printf("JLT_SYM_QUESTION \"?\"");
            break;
        case JLT_SYM_COLON:
            printf("JLT_SYM_COLON \":\"");
            break;
        case JLT_SYM_METHOD_REFERENCE:
            printf("JLT_SYM_METHOD_REFERENCE \"::\"");
            break;
        case JLT_SYM_DOT:
            printf("JLT_SYM_DOT \".\"");
            break;
        case JLT_SYM_VARIADIC:
            printf("JLT_SYM_VARIADIC \"...\"");
            break;
        default:
            printf("(UNKNOWN)");
            break;
    }
}

static void debug_print_operator(java_parser* parser, operator_id id)
{
    if (id == OPID_UNDEFINED)
    {
        printf("(undefined: %d)", id);
        return;
    }

    switch (id)
    {
        case OPID_POST_INC:
            printf("OPID_POST_INC -> ");
            break;
        case OPID_POST_DEC:
            printf("OPID_POST_DEC -> ");
            break;
        case OPID_SIGN_POS:
            printf("OPID_SIGN_POS -> ");
            break;
        case OPID_SIGN_NEG:
            printf("OPID_SIGN_NEG -> ");
            break;
        case OPID_LOGIC_NOT:
            printf("OPID_LOGIC_NOT -> ");
            break;
        case OPID_BIT_NOT:
            printf("OPID_BIT_NOT -> ");
            break;
        case OPID_PRE_INC:
            printf("OPID_PRE_INC -> ");
            break;
        case OPID_PRE_DEC:
            printf("OPID_PRE_DEC -> ");
            break;
        case OPID_MUL:
            printf("OPID_MUL -> ");
            break;
        case OPID_DIV:
            printf("OPID_DIV -> ");
            break;
        case OPID_MOD:
            printf("OPID_MOD -> ");
            break;
        case OPID_ADD:
            printf("OPID_ADD -> ");
            break;
        case OPID_SUB:
            printf("OPID_SUB -> ");
            break;
        case OPID_SHIFT_L:
            printf("OPID_SHIFT_L -> ");
            break;
        case OPID_SHIFT_R:
            printf("OPID_SHIFT_R -> ");
            break;
        case OPID_SHIFT_UR:
            printf("OPID_SHIFT_UR -> ");
            break;
        case OPID_LESS:
            printf("OPID_LESS -> ");
            break;
        case OPID_LESS_EQ:
            printf("OPID_LESS_EQ -> ");
            break;
        case OPID_GREAT:
            printf("OPID_GREAT -> ");
            break;
        case OPID_GREAT_EQ:
            printf("OPID_GREAT_EQ -> ");
            break;
        case OPID_INSTANCE_OF:
            printf("OPID_INSTANCE_OF -> ");
            break;
        case OPID_EQ:
            printf("OPID_EQ -> ");
            break;
        case OPID_NOT_EQ:
            printf("OPID_NOT_EQ -> ");
            break;
        case OPID_BIT_AND:
            printf("OPID_BIT_AND -> ");
            break;
        case OPID_BIT_XOR:
            printf("OPID_BIT_XOR -> ");
            break;
        case OPID_BIT_OR:
            printf("OPID_BIT_OR -> ");
            break;
        case OPID_LOGIC_AND:
            printf("OPID_LOGIC_AND -> ");
            break;
        case OPID_LOGIC_OR:
            printf("OPID_LOGIC_OR -> ");
            break;
        case OPID_TERNARY_1:
            printf("OPID_TERNARY_1 -> ");
            break;
        case OPID_TERNARY_2:
            printf("OPID_TERNARY_2 -> ");
            break;
        case OPID_ASN:
            printf("OPID_ASN -> ");
            break;
        case OPID_ADD_ASN:
            printf("OPID_ADD_ASN -> ");
            break;
        case OPID_SUB_ASN:
            printf("OPID_SUB_ASN -> ");
            break;
        case OPID_MUL_ASN:
            printf("OPID_MUL_ASN -> ");
            break;
        case OPID_DIV_ASN:
            printf("OPID_DIV_ASN -> ");
            break;
        case OPID_MOD_ASN:
            printf("OPID_MOD_ASN -> ");
            break;
        case OPID_AND_ASN:
            printf("OPID_AND_ASN -> ");
            break;
        case OPID_XOR_ASN:
            printf("OPID_XOR_ASN -> ");
            break;
        case OPID_OR_ASN:
            printf("OPID_OR_ASN -> ");
            break;
        case OPID_SHIFT_L_ASN:
            printf("OPID_SHIFT_L_ASN -> ");
            break;
        case OPID_SHIFT_R_ASN:
            printf("OPID_SHIFT_R_ASN -> ");
            break;
        case OPID_SHIFT_UR_ASN:
            printf("OPID_SHIFT_UR_ASN -> ");
            break;
        case OPID_LAMBDA:
            printf("OPID_LAMBDA -> ");
            break;
        default:
            printf("(UNKNOWN OPID) -> ");
            break;
    }

    debug_print_lexeme_type(OP_TOKEN(parser->expression->definition[id]));
}

static void debug_print_number_bit_length(java_number_bit_length l)
{
    switch (l)
    {
        case JT_NUM_BIT_LENGTH_NORMAL:
            printf("Regular");
            break;
        case JT_NUM_BIT_LENGTH_LONG:
            printf("Long");
            break;
        default:
            printf("(UNKNOWN)");
            break;
    }
}

static void debug_print_number_type(java_number_type number)
{
    switch (number)
    {
        case JT_NUM_DEC:
            printf("Decimal Integer");
            break;
        case JT_NUM_HEX:
            printf("Hexadecimal Integer");
            break;
        case JT_NUM_OCT:
            printf("Octal Integer");
            break;
        case JT_NUM_BIN:
            printf("Binary Integer");
            break;
        case JT_NUM_FP_DOUBLE:
            printf("Double Floating-Point");
            break;
        case JT_NUM_FP_FLOAT:
            printf("Float Floating-Point");
            break;
        default:
            printf("(UNKNOWN)");
            break;
    }
}

static void debug_print_token_content(java_token* token)
{
    if (!token || !token->from || !token->to)
    {
        printf("(null)");
        return;
    }

    size_t len = buffer_count(token->from, token->to);
    char* content = (char*)malloc_assert(sizeof(char) * (len + 1));

    buffer_substring(content, token->from, len);
    printf(content);

    free(content);
}

void debug_tokenize(file_buffer* buffer, hash_table* table)
{
    printf("===== TOKENIZED BUFFER CONTENT =====\n");

    java_token* token = (java_token*)malloc_assert(sizeof(java_token));

    while (true)
    {
        get_next_token(token, buffer, table);

        if (token->class == JT_EOF)
        {
            break;
        }

        switch (token->class)
        {
            case JT_IDENTIFIER:
                printf("Name\n");
                debug_print_token_content(token);
                printf("\n");
                break;
            case JT_RESERVED_WORD:
                printf("Reserved Word: ");
                if (token->keyword)
                {
                    debug_print_lexeme_type(token->keyword->id);
                }
                printf("\n");
                debug_print_token_content(token);
                printf("\n");
                break;
            case JT_LITERAL:
                printf("Literal ");
                debug_print_lexeme_type(token->type);
                if (token->type == JLT_LTR_NUMBER)
                {
                    printf(" ");
                    debug_print_number_type(token->number.type);
                    printf(" length: ");
                    debug_print_number_bit_length(token->number.bits);
                }
                printf("\n");
                debug_print_token_content(token);
                printf("\n");
                break;
            case JT_OPERATOR:
                printf("Operator ");
                debug_print_lexeme_type(token->type);
                printf("\n");
                debug_print_token_content(token);
                printf("\n");
                break;
            case JT_SEPARATOR:
                printf("Separator ");
                debug_print_lexeme_type(token->type);
                printf("\n");
                debug_print_token_content(token);
                printf("\n");
                break;
            case JT_COMMENT:
                printf("Comment\n");
                debug_print_token_content(token);
                printf("\n");
                break;
            case JT_EOF:
                printf("EOF\n");
                break;
            case JT_ILLEGAL:
                printf("Illegal (0x%02x)\n", *token->from);
                break;
            default:
                printf("Unknown");
                break;
        }

        printf("\n");
    }

    delete_token(token);
}

static void debug_print_modifier_bit_flag(lbit_flag modifiers)
{
    // although we have limited flags as of current,
    // we still add robustness and check every bit
    lbit_flag mask = 1;
    bool space = false;
    int bit_len = 8 * sizeof(lbit_flag);

    if (modifiers == 0)
    {
        printf("No Modifier");
        return;
    }

    for (int i = 0; i < bit_len; i++, mask <<= 1)
    {
        if (modifiers & mask)
        {
            if (space)
            {
                printf(" ");
            }

            space = true;
            switch (i)
            {
                case JLT_RWD_PUBLIC:
                    printf("public");
                    break;
                case JLT_RWD_PRIVATE:
                    printf("private");
                    break;
                case JLT_RWD_PROTECTED:
                    printf("protected");
                    break;
                case JLT_RWD_FINAL:
                    printf("final");
                    break;
                case JLT_RWD_STATIC:
                    printf("static");
                    break;
                case JLT_RWD_ABSTRACT:
                    printf("abstract");
                    break;
                case JLT_RWD_TRANSIENT:
                    printf("transient");
                    break;
                case JLT_RWD_SYNCHRONIZED:
                    printf("synchronized");
                    break;
                case JLT_RWD_VOLATILE:
                    printf("volatile");
                    break;
                default:
                    printf("UNKNOWN");
                    space = false;
                    break;
            }
        }
    }
}

static void debug_print_ast_node(java_parser* parser, tree_node* node)
{
    switch (node->type)
    {
        case JNT_UNIT:
            printf("Compilation Unit");
            break;
        case JNT_NAME:
            printf("Name");
            break;
        case JNT_NAME_UNIT:
            printf("Unit: ");
            debug_print_token_content(node->data.id->complex);
            break;
        case JNT_CLASS_TYPE:
            printf("Class Type");
            break;
        case JNT_CLASS_TYPE_UNIT:
            printf("Unit: ");
            debug_print_token_content(node->data.id->complex);
            break;
        case JNT_INTERFACE_TYPE:
            printf("Interface Type");
            break;
        case JNT_INTERFACE_TYPE_UNIT:
            printf("Unit: ");
            debug_print_token_content(node->data.id->complex);
            break;
        case JNT_INTERFACE_TYPE_LIST:
            printf("Interface Type List");
            break;
        case JNT_PKG_DECL:
            printf("Package Declaration");
            break;
        case JNT_IMPORT_DECL:
            printf("Import Declaration (On-demand: %s)",
                (node->data.import->on_demand ? "true" : "false"));
            break;
        case JNT_TOP_LEVEL:
            printf("Top Level: ");
            debug_print_modifier_bit_flag(node->data.top_level->modifier);
            break;
        case JNT_CLASS_DECL:
            printf("Class Declaration: ");
            debug_print_token_content(node->data.id->complex);
            break;
        case JNT_INTERFACE_DECL:
            printf("Interface Declaration: ");
            debug_print_token_content(node->data.id->complex);
            break;
        case JNT_CLASS_EXTENDS:
            printf("Class Extends");
            break;
        case JNT_CLASS_IMPLEMENTS:
            printf("Class Implements");
            break;
        case JNT_CLASS_BODY:
            printf("Class Body");
            break;
        case JNT_INTERFACE_BODY_DECL:
            printf("Interface Body Declaration: ");
            debug_print_modifier_bit_flag(node->data.top_level->modifier);
            break;
        case JNT_CLASS_BODY_DECL:
            printf("Class Body Declaration: ");
            debug_print_modifier_bit_flag(node->data.top_level->modifier);
            break;
        case JNT_INTERFACE_EXTENDS:
            printf("Interface Extends");
            break;
        case JNT_INTERFACE_BODY:
            printf("Interface Body");
            break;
        case JNT_STATIC_INIT:
            printf("Static Initializer");
            break;
        case JNT_BLOCK:
            printf("Block");
            break;
        case JNT_CTOR_DECL:
            printf("Constructor Declaration: ");
            debug_print_token_content(node->data.declarator->id.complex);
            break;
        case JNT_TYPE:
            printf("Type: ");

            if (node->data.declarator->id.simple == JLT_MAX)
            {
                printf("(Complex Type Shown In Sub-Tree)");
            }
            else
            {
                debug_print_lexeme_type(node->data.declarator->id.simple);
            }

            if (node->data.declarator->dimension > 0)
            {
                printf(" Array (dim: %zd)", node->data.declarator->dimension);
            }

            break;
        case JNT_METHOD_HEADER:
            printf("Method Header: ");
            debug_print_token_content(node->data.declarator->id.complex);

            if (node->data.declarator->dimension > 0)
            {
                printf(" (return array dim: %zd)", node->data.declarator->dimension);
            }

            break;
        case JNT_METHOD_DECL:
            printf("Method Declaration");
            break;
        case JNT_VAR_DECLARATORS:
            printf("Variable Declarators");
            break;
        case JNT_FORMAL_PARAM_LIST:
            printf("Formal Parameter List");
            break;
        case JNT_THROWS:
            printf("Throws");
            break;
        case JNT_ARGUMENT_LIST:
            printf("Argument List");
            break;
        case JNT_FORMAL_PARAM:
            printf("Formal Parameter: ");

            debug_print_token_content(node->data.declarator->id.complex);
            if (node->data.declarator->dimension > 0)
            {
                printf(" Array (dim: %zd)", node->data.declarator->dimension);
            }

            break;
        case JNT_CTOR_BODY:
            printf("Constructor Body");
            break;
        case JNT_CTOR_INVOCATION:
            printf("Constructor Invocation: ");

            if (node->data.constructor_invoke->is_super)
            {
                printf("super");
            }
            else
            {
                printf("this");
            }

            break;
        case JNT_METHOD_BODY:
            printf("Method Body");
            break;
        case JNT_VAR_DECL:
            printf("Variable Declarator: ");

            debug_print_token_content(node->data.declarator->id.complex);
            if (node->data.declarator->dimension)
            {
                printf(" (dim: %zd)", node->data.declarator->dimension);
            }

            break;
        case JNT_ARRAY_INIT:
            printf("Array Initializer");
            break;
        case JNT_PRIMARY:
            printf("Primary");
            break;
        case JNT_PRIMARY_SIMPLE:
            debug_print_lexeme_type(node->data.id->simple);
            break;
        case JNT_PRIMARY_COMPLEX:
            debug_print_token_content(node->data.id->complex);
            break;
        case JNT_PRIMARY_CREATION:
            printf("Object Creation");
            break;
        case JNT_PRIMARY_ARR_CREATION:
            printf("Array Creation:");
            printf(" (dim: %zd)", node->data.declarator->dimension);
            break;
        case JNT_PRIMARY_CLS_CREATION:
            printf("Class Creation");
            break;
        case JNT_PRIMARY_METHOD_INVOKE:
            printf("Method Call");
            break;
        case JNT_PRIMARY_ARR_ACCESS:
            printf("Array Access");
            break;
        case JNT_PRIMARY_CLS_LITERAL:
            printf("Class Literal");
            break;
        case JNT_EXPRESSION:
            printf("Expression OP[%d]: ", node->data.expression->op);
            debug_print_operator(parser, node->data.expression->op);
            break;
        case JNT_STATEMENT:
            printf("Statement (Invalid)");
            break;
        case JNT_STATEMENT_EMPTY:
            printf("Empty Statement (Should not appear)");
            break;
        case JNT_STATEMENT_SWITCH:
            printf("Switch Statement");
            break;
        case JNT_STATEMENT_DO:
            printf("Do-While Statement");
            break;
        case JNT_STATEMENT_BREAK:
            printf("Break Statement: ");
            debug_print_token_content(node->data.id->complex);
            break;
        case JNT_STATEMENT_CONTINUE:
            printf("Continue Statement: ");
            debug_print_token_content(node->data.id->complex);
            break;
        case JNT_STATEMENT_RETURN:
            printf("Return Statement");
            break;
        case JNT_STATEMENT_SYNCHRONIZED:
            printf("Synchronized Statement");
            break;
        case JNT_STATEMENT_THROW:
            printf("Throw Statement");
            break;
        case JNT_STATEMENT_TRY:
            printf("Try Statement");
            break;
        case JNT_STATEMENT_IF:
            printf("If Statement");
            break;
        case JNT_STATEMENT_WHILE:
            printf("While Statement");
            break;
        case JNT_STATEMENT_FOR:
            printf("For Statement");
            break;
        case JNT_STATEMENT_LABEL:
            printf("Label Statement: ");
            debug_print_token_content(node->data.id->complex);
            break;
        case JNT_STATEMENT_EXPRESSION:
            printf("Expression Statement");
            break;
        case JNT_STATEMENT_VAR_DECL:
            printf("Variable Declaration");
            break;
        case JNT_STATEMENT_CATCH:
            printf("Catch Clause");
            break;
        case JNT_STATEMENT_FINALLY:
            printf("Finally Clause");
            break;
        case JNT_SWITCH_LABEL:
            printf("Switch Label: ");

            if (node->data.switch_label->is_default)
            {
                printf("default");
            }
            else
            {
                printf("case");
            }

            break;
        case JNT_EXPRESSION_LIST:
            printf("Expression List");
            break;
        case JNT_FOR_INIT:
            printf("For Initialization");
            break;
        case JNT_FOR_UPDATE:
            printf("For Update");
            break;
        case JNT_LOCAL_VAR_DECL:
            printf("Local Variable Declaration");
            break;
        case JNT_AMBIGUOUS:
            printf("Ambiguous Node");
            break;
        default:
            printf("Unknown: %d", node->type);
            break;
    }
}

static void debug_print_ast(java_parser* parser, tree_node* node, int depth)
{
    if (!node)
    {
        return;
    }

    // print current level first
    while (node)
    {
        for (int i = 0; i < depth; i++)
        {
            printf("  ");
        }

        debug_print_ast_node(parser, node);
        printf("\n");

        // go deeper first
        if (node->first_child)
        {
            debug_print_ast(parser, node->first_child, depth + 1);
        }

        // then go next sibling
        node = node->next_sibling;
    }
}

void debug_ast(java_parser* parser)
{
    printf("===== ABSTRACT SYNTAX TREE =====\n");
    debug_print_ast(parser, parser->ast_root, 0);
}

/**
 * NOTE: do NOT delete this one as it is useful for bucket size calculation
*/
void debug_java_symbol_lookup_table_no_collision_test(bool use_prime_size)
{
    printf("=====symbol table hash test (enforce prime size: %s)=====\n", use_prime_size ? "true" : "false");

    hash_table t;
    size_t bucket_size = use_prime_size ? find_next_prime(num_java_reserved_words) : num_java_reserved_words;

    while (true)
    {
        printf("testing collision with bucket size %zd...", bucket_size);

        init_hash_table(&t, bucket_size);
        for (int idx_word = 0; idx_word < num_java_reserved_words; idx_word++)
        {
            shash_table_insert(&t, java_reserved_words[idx_word].content, &java_reserved_words[idx_word]);

            if (t.num_filled != idx_word + 1)
            {
                printf("failed\n");
                break;
            }
        }

        if (t.num_filled == num_java_reserved_words)
        {
            printf("success (load factor: %f = %f)\n", hash_table_load_factor(&t), (float)num_java_reserved_words / bucket_size);
            break;
        }
        else
        {
            release_hash_table(&t, NULL);
            bucket_size = use_prime_size ? find_next_prime(bucket_size + 1) : (bucket_size + 1);
        }
    }

    printf(">>>Lookup Test<<<\n");
    const char* not_found = "(null)";
    for (int idx_word = 0; idx_word < num_java_reserved_words; idx_word++)
    {
        java_reserved_word* w = shash_table_find(&t, java_reserved_words[idx_word].content);
        printf("key: %s -> %s\n", java_reserved_words[idx_word].content, w ? w->content : not_found);
    }
    debug_shash_table(&t);

    release_hash_table(&t, NULL);

    printf("================================\n");
}

static void debug_print_irop(operation irop)
{
    switch (irop)
    {
        case IROP_UNDEFINED:
            printf("(Invalid: IROP_UNDEFINED)");
            break;
        case IROP_POS:
            printf("IROP_POS");
            break;
        case IROP_NEG:
            printf("IROP_NEG");
            break;
        case IROP_ADD:
            printf("IROP_ADD");
            break;
        case IROP_SUB:
            printf("IROP_SUB");
            break;
        case IROP_MUL:
            printf("IROP_MUL");
            break;
        case IROP_DIV:
            printf("IROP_DIV");
            break;
        case IROP_MOD:
            printf("IROP_MOD");
            break;
        case IROP_BINC:
            printf("IROP_BINC");
            break;
        case IROP_AINC:
            printf("IROP_AINC");
            break;
        case IROP_BDEC:
            printf("IROP_BDEC");
            break;
        case IROP_ADEC:
            printf("IROP_ADEC");
            break;
        case IROP_SLS:
            printf("IROP_SLS");
            break;
        case IROP_SRS:
            printf("IROP_SRS");
            break;
        case IROP_URS:
            printf("IROP_URS");
            break;
        case IROP_LT:
            printf("IROP_LT");
            break;
        case IROP_GT:
            printf("IROP_GT");
            break;
        case IROP_LE:
            printf("IROP_LE");
            break;
        case IROP_GE:
            printf("IROP_GE");
            break;
        case IROP_EQ:
            printf("IROP_EQ");
            break;
        case IROP_NE:
            printf("IROP_NE");
            break;
        case IROP_IO:
            printf("IROP_IO");
            break;
        case IROP_LNEG:
            printf("IROP_LNEG");
            break;
        case IROP_LAND:
            printf("IROP_LAND");
            break;
        case IROP_LOR:
            printf("IROP_LOR");
            break;
        case IROP_BNEG:
            printf("IROP_BNEG");
            break;
        case IROP_BAND:
            printf("IROP_BAND");
            break;
        case IROP_BOR:
            printf("IROP_BOR");
            break;
        case IROP_XOR:
            printf("IROP_XOR");
            break;
        case IROP_ASN:
            printf("IROP_ASN");
            break;
        case IROP_TC:
            printf("IROP_TC");
            break;
        case IROP_TB:
            printf("IROP_TB");
            break;
        case IROP_LMD:
            printf("IROP_LMD");
            break;
        case IROP_STORE:
            printf("IROP_STORE");
            break;
        case IROP_INIT:
            printf("IROP_INIT");
            break;
        case IROP_JMP:
            printf("IROP_JMP");
            break;
        case IROP_RET:
            printf("IROP_RET");
            break;
        case IROP_TEST:
            printf("IROP_TEST");
            break;
        case IROP_PHI:
            printf("IROP_PHI");
            break;
        case IROP_NOOP:
            printf("IROP_NOOP");
            break;
        case IROP_BOOL:
            printf("IROP_BOOL");
            break;
        case IROP_MAX:
            printf("(Invalid: IROP_MAX)");
            break;
        default:
            printf("(UNDEFINED IROP: %d)", irop);
            break;
    }
}

static void debug_print_cfg_node_type(block_type type)
{
    switch (type)
    {
        case BLOCK_ANY:
            printf("<ANY>");
            break;
        case BLOCK_RETURN:
            printf("<RETURN>");
            break;
        case BLOCK_BREAK:
            printf("<BREAK>");
            break;
        case BLOCK_CONTINUE:
            printf("<CONTINUE>");
            break;
        case BLOCK_TEST:
            printf("<TEST>");
            break;
        default:
            printf("<UNKNOWN NODE TYPE>");
            break;
    }
}

static void debug_print_reference(reference* r)
{
    definition* d;
    uint64_t n;
    float nf;
    double nd;

    if (!r)
    {
        printf("(null)");
        return;
    }

    switch (r->type)
    {
        case IR_ASN_REF_DEFINITION:
            printf("(def[%zd]: %p)", r->ver, r->doi);
            break;
        case IR_ASN_REF_INSTRUCTION:
            printf("(inst: %p)", r->doi);
            break;
        case IR_ASN_REF_LITERAL:
            d = r->doi;

            switch (d->type)
            {
                case DEFINITION_NULL:
                    printf("(li: null object)");
                    break;
                case DEFINITION_STRING:
                    printf("(li: string [%p])", d);
                    break;
                case DEFINITION_NUMBER:
                case DEFINITION_CHARACTER:
                case DEFINITION_BOOLEAN:
                    n = d->li_number->imm;

                    switch (d->li_number->type)
                    {
                        case IRPV_INTEGER_BIT_8:
                            printf("(li: 0x%llx{%d})", n, (int8_t)n);
                            break;
                        case IRPV_INTEGER_BIT_16:
                            printf("(li: 0x%llx{%d})", n, (int16_t)n);
                            break;
                        case IRPV_INTEGER_BIT_32:
                            printf("(li: 0x%llx{%d})", n, (int32_t)n);
                            break;
                        case IRPV_INTEGER_BIT_64:
                            printf("(li: 0x%llx{%lld})", n, (int64_t)n);
                            break;
                        case IRPV_INTEGER_BIT_U16:
                            printf("(li: 0x%llx{%d})", n, (uint16_t)n);
                            break;
                        case IRPV_PRECISION_SINGLE:
                            memcpy(&nf, (byte*)(&n) + 4, sizeof(double));
                            printf("(li: 0x%llx{%f})", n, nf);
                            break;
                        case IRPV_PRECISION_DOUBLE:
                            memcpy(&nd, &n, sizeof(double));
                            printf("(li: 0x%llx{%lf})", n, nd);
                            break;
                        case IRPV_BOOLEAN:
                            printf("(li: 0x%llx{%08x}{%d})", n, (uint32_t)n, (bool)n);
                            break;
                        default:
                            printf("(li unknown number def [%p])", d);
                            break;
                    }
                    break;
                default:
                    printf("(li: undefined [%p])", d);
                    break;
            }

            break;
        default:
            printf("(undefined(%d): %p)", r->type, r->doi);
            break;
    }
}

static void debug_print_instructions(instruction* inst, size_t* cnt, size_t depth)
{
    if (!inst)
    {
        printf("|");
        __format_print_indentation(depth);
        printf("(no instructions)\n");

        return;
    }

    while (inst)
    {
        printf("|");
        __format_print_indentation(depth);
        printf("[%p][%zd]: ", inst, inst->node->id);

        if (inst->lvalue)
        {
            debug_print_reference(inst->lvalue);
            printf(" <- ");
        }

        debug_print_reference(inst->operand_1);
        printf(" ");
        debug_print_irop(inst->op);
        printf(" ");
        debug_print_reference(inst->operand_2);
        printf("\n");

        if (cnt) { (*cnt)++; }

        inst = inst->next;
    }
}

static void debug_print_cfg(cfg* g, size_t depth)
{
    basic_block* b;
    instruction* inst;
    cfg_edge* edge;
    size_t instruction_count = 0;
    size_t node_out_count = 0;
    size_t node_in_count = 0;

    if (!g || g->nodes.num == 0)
    {
        __format_print_indentation(depth);
        printf("(empty)\n");
        return;
    }

    for (size_t i = 0; i < g->nodes.num; i++)
    {
        printf("|");

        b = g->nodes.arr[i];

        node_out_count += b->out.size;
        node_in_count += b->in.size;

        __format_print_indentation(depth);

        // print node header
        printf("node[%zd]%s", b->id, b == g->entry ? " (entry point) " : " ");
        debug_print_cfg_node_type(b->type);

        // print node edges
        // since the graph is directed, so print out edge is sufficient
        for (size_t j = 0; j < b->out.num; j++)
        {
            edge = b->out.arr[j];

            printf(" -> %zd", edge->to->id);

            switch (edge->type)
            {
                case EDGE_TRUE:
                    printf("(TRUE)");
                    break;
                case EDGE_FALSE:
                    printf("(FALSE)");
                    break;
                case EDGE_JUMP:
                    printf("(JMP)");
                default:
                    break;
            }
        }
        printf("\n");

        debug_print_instructions(b->inst_first, &instruction_count, depth + 1);
    }

    printf(">>>>> SUMMARY <<<<<\n");
    printf("node count: %zd\n", g->nodes.num);
    printf("node arr size: %zd\n", g->nodes.size);
    printf("edge count: %zd\n", g->edges.num);
    printf("edge arr size: %zd\n", g->edges.size);
    printf("instruction count: %zd\n", instruction_count);
    printf("memory size: %zd bytes\n",
        sizeof(cfg) +
        sizeof(basic_block*) * g->nodes.size +
        sizeof(basic_block) * g->nodes.num +
        sizeof(cfg_edge*) * (g->edges.size + node_in_count + node_out_count) +
        sizeof(cfg_edge) * g->edges.num +
        sizeof(instruction) * instruction_count
    );
}

static void debug_print_definition_pool(definition_pool* pool, size_t depth);

static void debug_print_definition(definition* v, size_t depth)
{
    static const char* method_regular = "method";
    static const char* method_ctor = "constructor";
    java_lexeme_type lex_type;

    switch (v->type)
    {
        case DEFINITION_VARIABLE:
            if (v->variable->is_class_member)
            {
                printf("def member var, order %zd, ", v->sid);
            }
            else
            {
                printf("def var, ");
            }

            printf("Access: ");
            debug_print_modifier_bit_flag(v->variable->modifier);

            printf(", Type: ");
            if (v->variable->type.primitive != JLT_MAX)
            {
                debug_print_lexeme_type(v->variable->type.primitive);
            }
            else
            {
                printf("%s", v->variable->type.reference);
            }

            printf("\n");
            break;
        case DEFINITION_METHOD:
            printf("def %s, ", v->method->is_constructor ? method_ctor : method_regular);

            printf("Access: ");
            debug_print_modifier_bit_flag(v->method->modifier);

            printf(", Parameter Count: %zd(", v->method->parameter_count);
            for (size_t i = 0; i < v->method->parameter_count; i++)
            {
                if (i > 0) { printf(" "); }

                lex_type = v->method->parameters[i]->variable->type.primitive;

                for (size_t j = v->method->parameters[i]->variable->type.dim; j > 0; j--)
                {
                    printf("[");
                }

                if (lex_type == JLT_MAX)
                {
                    printf("L%s;", v->method->parameters[i]->variable->type.reference);
                }
                else
                {
                    printf("%c", primitive_type_to_jil_type(lex_type));
                }
            }
            printf(")");

            printf(", Return: ");
            if (v->method->return_type.primitive != JLT_MAX)
            {
                debug_print_lexeme_type(v->method->return_type.primitive);
            }
            else
            {
                printf("%s", v->method->return_type.reference);
            }

            printf("\n");

            debug_print_definition_pool(&v->method->local_variables, depth + 1);
            debug_print_cfg(&v->method->code, depth + 1);

            printf("\n");
            break;
        case DEFINITION_NUMBER:
            printf("number, 0x%llx\n", v->li_number->imm);
            break;
        case DEFINITION_CHARACTER:
            printf("character, 0x%llx (16-bit wide-char)\n", v->li_number->imm);
            break;
        case DEFINITION_BOOLEAN:
            printf("boolean, 0x%llx (%s)\n", v->li_number->imm, v->li_number->imm ? "true" : "false");
            break;
        case DEFINITION_STRING:
            printf("string, %zd byte(s), %s wide character\n", v->li_string->length, v->li_string->wide_char ? "has" : "no");
            __format_print_binary_stream(v->li_string->stream, v->li_string->length, depth + 1);
            printf("\n");
            break;
        case DEFINITION_NULL:
            printf("null\n");
            break;
        default:
            // no-op
            printf("(UNREGISTERED VALUE)\n");
            break;
    }
}

static void debug_print_definition_pool(definition_pool* pool, size_t depth)
{
    printf(">");
    __format_print_indentation(depth);
    printf("Definition Pool: %zd definition(s), %zd byte(s)\n",
        pool->num,
        sizeof(definition_pool) +
        sizeof(definition*) * pool->size +
        sizeof(definition) * pool->num
    );

    definition* v;

    for (size_t i = 0; i < pool->num; i++)
    {
        v = pool->arr[i];

        printf(">");
        for (size_t d = 0; d < depth + 1; d++) { printf("    "); }
        printf("[%zd](%p): ", i, v);
        debug_print_definition(v, depth + 1);
    }
}

static void debug_print_name_definition_table(hash_table* table, size_t depth)
{
    for (size_t i = 0; i < table->bucket_size; i++)
    {
        hash_pair* p = table->bucket[i];

        while (p)
        {
            __format_print_indentation(depth);

            // key
            printf("[%p] %s: ", p->value, (char*)(p->key));

            // print all definitions of this name
            debug_print_definition(p->value, depth);

            p = p->next;
        }
    }
}

void debug_print_global_import(java_ir* ir)
{
    hash_table* table = &ir->tbl_import;

    printf("\n===== IMPORTS =====\n");
    printf("count: %zd\n", table->num_pairs);
    printf("memory: %zd bytes\n", hash_table_memory_size(table));
    printf("load factor: %.2f%%\n", hash_table_load_factor(table) * 100.0f);
    printf("longest chain: %zd\n", hash_table_longest_chain_length(table));

    for (size_t i = 0; i < ir->tbl_import.bucket_size; i++)
    {
        hash_pair* p = ir->tbl_import.bucket[i];

        while (p)
        {
            // key
            printf("    %s: %s%s\n",
                (char*)(p->key),
                p->value ? "FROM " : "(ON-DEMAND)",
                p->value ? ((global_import*)p->value)->package_name : ""
            );

            p = p->next;
        }
    }
}

void debug_ir_global_names(java_ir* ir)
{
    hash_table* table = lookup_global_scope(ir);
    global_top_level* top;

    printf("\n===== GLOBAL NAMES =====\n");
    printf("count: %zd\n", table->num_pairs);
    printf("memory: %zd bytes\n", hash_table_memory_size(table));
    printf("load factor: %.2f%%\n", hash_table_load_factor(table) * 100.0f);
    printf("longest chain: %zd\n", hash_table_longest_chain_length(table));

    for (size_t i = 0; i < table->bucket_size; i++)
    {
        for (hash_pair* p = table->bucket[i]; p != NULL; p = p->next)
        {
            printf("    %s: ", (char*)(p->key));

            top = p->value;

            // ill-formed
            if (!top)
            {
                printf("(null)\n");
                continue;
            }

            switch (top->type)
            {
                case TOP_LEVEL_CLASS:
                    printf("Access: ");
                    debug_print_modifier_bit_flag(top->modifier);

                    if (top->extend)
                    {
                        printf(" extends %s", top->extend);
                    }

                    if (top->implement)
                    {
                        printf(" implements: ");

                        for (size_t j = 0; j < top->num_implement; j++)
                        {
                            printf("%s%s", top->implement[j], j < top->num_implement - 1 ? ", " : " ");
                        }
                    }

                    printf("\n");

                    printf("*       Member Variable Initialization:\n");
                    debug_print_definition_pool(&top->member_init_variables, 3);
                    debug_print_cfg(top->code_member_init, 3);
                    printf("\n");

                    printf("*       Member: %zd member(s)\n", top->tbl_member.num_pairs);
                    debug_print_name_definition_table(&top->tbl_member, 3);
                    printf("\n");

                    printf("*       Literals: %zd literal(s)\n", top->tbl_literal.num_pairs);
                    debug_print_name_definition_table(&top->tbl_literal, 3);
                    printf("\n");

                    break;
                case TOP_LEVEL_INTERFACE:
                    /**
                     * TODO:
                    */
                    break;
                default:
                    break;
            }
        }
    }
}

void debug_ir_lookup(java_ir* ir)
{
    printf("\n===== LOOKUP STACK =====\n");

    scope_frame* lh = ir->scope_stack_top;

    if (!lh)
    {
        printf("(lookup stack is empty)\n");
        return;
    }

    while (lh)
    {
        printf(">>>>>>>>>>\n");

        hash_table* table = lh->table;

        printf("memory: %zd bytes\n", hash_table_memory_size(table));
        printf("load factor: %.2f%%\n", hash_table_load_factor(table) * 100.0f);
        printf("longest chain: %zd\n", hash_table_longest_chain_length(table));

        debug_print_name_definition_table(table, 1);

        lh = lh->next;
    }
}

void debug_number_library()
{
    printf("\n===== NUMBER LIBRARY TEST =====\n");
    printf("max int64: %lld\n", 0x7FFFFFFFFFFFFFFF);
    printf("max uint64: %llu\n", 0xFFFFFFFFFFFFFFFF);

    size_t len = ARRAY_SIZE(test_numbers);
    number_truncation_status nts;
    binary_data bin;
    uint32_t n32;
    char* c;
    java_number_type t;
    double fp_double;
    float fp_single;
    size_t flag;
    bool pr_bar;

    for (size_t i = 0; i < len; i++)
    {
        c = test_numbers[i].content;
        t = test_numbers[i].type;
        nts = s2b(c, t, &bin);
        pr_bar = false;

        // string stream is special
        if (t == JT_NUM_MAX)
        {
            printf("stream\n");
            printf("    bytes: %zd\n", bin.len);
            printf("    has wide char: %d\n", bin.wide_char);

            __format_print_binary_stream(bin.stream, bin.len, 0);

            printf("\n");
            continue;
        }

        debug_print_number_type(t);
        printf(" \"%s\"\n", c);

        printf("    raw: 0x%llX\n", bin.number);
        printf("    overflow: ");
        for (size_t j = 0; j < 12; j++)
        {
            flag = 1 << j;

            if (nts & flag)
            {
                if (pr_bar) { printf(" | "); }
                else { pr_bar = true; }

                switch (flag)
                {
                    case NTS_OVERFLOW_U8:
                        printf("u8");
                        break;
                    case NTS_OVERFLOW_U16:
                        printf("u16");
                        break;
                    case NTS_OVERFLOW_U32:
                        printf("u32");
                        break;
                    case NTS_OVERFLOW_U64:
                        printf("u64");
                        break;
                    case NTS_OVERFLOW_FP32_EXP:
                        printf("exp32");
                        break;
                    case NTS_OVERFLOW_FP32_MAN:
                        printf("man32");
                        break;
                    case NTS_OVERFLOW_FP64_EXP:
                        printf("exp64");
                        break;
                    case NTS_OVERFLOW_FP64_MAN:
                        printf("man64");
                        break;
                    case NTS_OVERFLOW_INT8:
                        printf("i8");
                        break;
                    case NTS_OVERFLOW_INT16:
                        printf("i16");
                        break;
                    case NTS_OVERFLOW_INT32:
                        printf("i32");
                        break;
                    case NTS_OVERFLOW_INT64:
                        printf("i64");
                        break;
                    default:
                        printf("(UNKNOWN: %04llx)", flag);
                        break;
                }
            }
        }
        printf("\n");

        printf("    formatted: ");
        switch (t)
        {
            case JT_NUM_DEC:
            case JT_NUM_HEX:
            case JT_NUM_OCT:
            case JT_NUM_BIN:
                printf("%llu", bin.number);
                break;
            case JT_NUM_FP_DOUBLE:
                memcpy(&fp_double, &bin.number, 8);
                printf("%10.20f", fp_double);
                break;
            case JT_NUM_FP_FLOAT:
                n32 = (uint32_t)bin.number;
                memcpy(&fp_single, &n32, 4);
                printf("%10.20f", fp_single);
                // test simple value
                // printf("%10.20f(%10.20f)", fp_single, (float)987654321.1234567890e-10);
                break;
            default:
                printf("(UNKNOWN FORMAT)");
                break;
        }

        printf("\n\n");
    }

    printf("\n");
}
