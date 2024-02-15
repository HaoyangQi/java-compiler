#include "debug.h"

#include "langspec.h"
#include "token.h"
#include "node.h"
#include "expression.h"

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

void debug_report_hash_table_summary(hash_table* table, const char* title)
{
    if (title)
    {
        printf("%s:\n", title);
    }

    printf("    count: %d\n", num_java_reserved_words);
    printf("    memory: %zd bytes\n", hash_table_memory_size(table));
    printf("    load factor: %.2f%%\n", hash_table_load_factor(table) * 100.0f);
    printf("    longest chain: %zd\n", hash_table_longest_chain_length(table));
}

void debug_report(compiler* compiler)
{
    printf("===== COMPILER RUNTIME REPORT =====\n");

    printf("Language version: %d\n", compiler->version);
    debug_report_hash_table_summary(&compiler->rw_lookup_table, "Reserved word");
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

void debug_print_number_bit_length(java_number_bit_length l)
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

void debug_print_number_type(java_number_type number)
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

void debug_print_token_content(java_token* token)
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

    free_token(token);
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
            debug_print_token_content(node->data->id.complex);
            break;
        case JNT_CLASS_TYPE:
            printf("Class Type");
            break;
        case JNT_CLASS_TYPE_UNIT:
            printf("Unit: ");
            debug_print_token_content(node->data->id.complex);
            break;
        case JNT_INTERFACE_TYPE:
            printf("Interface Type");
            break;
        case JNT_INTERFACE_TYPE_UNIT:
            printf("Unit: ");
            debug_print_token_content(node->data->id.complex);
            break;
        case JNT_INTERFACE_TYPE_LIST:
            printf("Interface Type List");
            break;
        case JNT_PKG_DECL:
            printf("Package Declaration");
            break;
        case JNT_IMPORT_DECL:
            printf("Import Declaration (On-demand: %s)",
                (node->data->import.on_demand ? "true" : "false"));
            break;
        case JNT_TOP_LEVEL:
            printf("Top Level: ");
            debug_print_modifier_bit_flag(node->data->top_level_declaration.modifier);
            break;
        case JNT_CLASS_DECL:
            printf("Class Declaration: ");
            debug_print_token_content(node->data->id.complex);
            break;
        case JNT_INTERFACE_DECL:
            printf("Interface Declaration: ");
            debug_print_token_content(node->data->id.complex);
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
            debug_print_modifier_bit_flag(node->data->top_level_declaration.modifier);
            break;
        case JNT_CLASS_BODY_DECL:
            printf("Class Body Declaration: ");
            debug_print_modifier_bit_flag(node->data->top_level_declaration.modifier);
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
            debug_print_token_content(node->data->id.complex);
            break;
        case JNT_TYPE:
            printf("Type: ");

            if (node->data->declarator.id.simple == JLT_MAX)
            {
                printf("(Complex Type Shown In Sub-Tree)");
            }
            else
            {
                debug_print_lexeme_type(node->data->declarator.id.simple);
            }

            if (node->data->declarator.dimension > 0)
            {
                printf(" Array (dim: %zd)", node->data->declarator.dimension);
            }

            break;
        case JNT_METHOD_HEADER:
            printf("Method Header: ");
            debug_print_token_content(node->data->declarator.id.complex);

            if (node->data->declarator.dimension > 0)
            {
                printf(" (return array dim: %zd)", node->data->declarator.dimension);
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

            debug_print_token_content(node->data->declarator.id.complex);
            if (node->data->declarator.dimension > 0)
            {
                printf(" Array (dim: %zd)", node->data->declarator.dimension);
            }

            break;
        case JNT_CTOR_BODY:
            printf("Constructor Body");
            break;
        case JNT_CTOR_INVOCATION:
            printf("Constructor Invocation: ");

            if (node->data->constructor_invoke.is_super)
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

            debug_print_token_content(node->data->declarator.id.complex);
            if (node->data->declarator.dimension)
            {
                printf(" (dim: %zd)", node->data->declarator.dimension);
            }

            break;
        case JNT_ARRAY_INIT:
            printf("Array Initializer");
            break;
        case JNT_PRIMARY:
            printf("Primary");
            break;
        case JNT_PRIMARY_SIMPLE:
            debug_print_lexeme_type(node->data->id.simple);
            break;
        case JNT_PRIMARY_COMPLEX:
            debug_print_token_content(node->data->id.complex);
            break;
        case JNT_PRIMARY_CREATION:
            printf("Object Creation");
            break;
        case JNT_PRIMARY_ARR_CREATION:
            printf("Array Creation:");
            printf(" (dim: %zd)", node->data->declarator.dimension);
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
            printf("Expression");
            break;
        case JNT_OPERATOR:
            printf("OP[%d]: ", node->data->operator.id);
            debug_print_operator(parser, node->data->operator.id);
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
            debug_print_token_content(node->data->id.complex);
            break;
        case JNT_STATEMENT_CONTINUE:
            printf("Continue Statement: ");
            debug_print_token_content(node->data->id.complex);
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
            debug_print_token_content(node->data->id.complex);
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

            if (node->data->switch_label.is_default)
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

void debug_shash_table(hash_table* table)
{
    printf("===== HASH TABLE =====\n");
    printf("memory: %zd bytes\n", hash_table_memory_size(table));
    printf("load factor: %.2f%%\n", hash_table_load_factor(table) * 100.0f);
    printf("longest chain: %zd\n", hash_table_longest_chain_length(table));

    for (size_t i = 0; i < table->bucket_size; i++)
    {
        printf("    ");

        hash_pair* p = table->bucket[i];

        if (p)
        {
            while (p)
            {
                printf("(%s, %p)->", (char*)(p->key), p->value);
                p = p->next;
            }
        }

        printf("(null)\n");
    }
}

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

void debug_ir_on_demand_imports(java_ir* ir)
{
    hash_table* table = &ir->tbl_on_demand_packages;

    printf("===== ON-DEMAND IMPORT PACKAGES =====\n");
    printf("memory: %zd bytes\n", hash_table_memory_size(table));
    printf("load factor: %.2f%%\n", hash_table_load_factor(table) * 100.0f);
    printf("longest chain: %zd\n", hash_table_longest_chain_length(table));

    for (size_t i = 0; i < table->bucket_size; i++)
    {
        hash_pair* p = table->bucket[i];

        if (p)
        {
            while (p)
            {
                printf("    %s\n", (char*)(p->key));
                p = p->next;
            }
        }
    }
}

static void debug_print_definition(definition* v)
{
    // header
    switch (v->type)
    {
        case JNT_IMPORT_DECL:
            printf("import,");
            break;
        case JNT_CLASS_DECL:
            printf("class,");
            break;
        case JNT_VAR_DECL:
            printf("def %s,", v->variable.is_class_member ? "member var" : "var");
            break;
        case JNT_METHOD_DECL:
            printf("def method,");
            break;
        default:
            // no-op
            printf("(UNREGISTERED: %d),", v->type);
            break;
    }

    // value
    switch (v->type)
    {
        case JNT_IMPORT_DECL:
            printf(" FROM %s\n", v->import.package_name);
            break;
        case JNT_CLASS_DECL:
            printf(" Access: ");
            debug_print_modifier_bit_flag(v->class.modifier);
            if (v->class.extend)
            {
                printf(" extends %s", v->class.extend);
            }
            if (v->class.implement)
            {
                printf(" implements %s", v->class.implement);
            }
            printf("\n");
            break;
        case JNT_VAR_DECL:
            printf(" Access: ");
            debug_print_modifier_bit_flag(v->variable.modifier);
            printf(", Type: ");
            if (v->variable.type.primitive != JLT_MAX)
            {
                debug_print_lexeme_type(v->variable.type.primitive);
            }
            else
            {
                printf("%s", v->variable.type.reference);
            }
            printf("\n");
            break;
        case JNT_METHOD_DECL:
            printf(" Access: ");
            debug_print_modifier_bit_flag(v->method.modifier);
            printf(", Return: ");
            if (v->method.return_type.primitive != JLT_MAX)
            {
                debug_print_lexeme_type(v->method.return_type.primitive);
            }
            else
            {
                printf("%s", v->method.return_type.reference);
            }
            printf("\n");
            break;
        default:
            // no-op
            printf(" (UNREGISTERED VALUE)\n");
            break;
    }
}

static void debug_print_scope_frame_table(hash_table* table)
{
    for (size_t i = 0; i < table->bucket_size; i++)
    {
        hash_pair* p = table->bucket[i];

        if (p)
        {
            while (p)
            {
                // key
                printf("    %s:\n", (char*)(p->key));

                // print all definitions of this name
                definition* v = p->value;
                for (size_t j = 0; v != NULL; j++)
                {
                    printf("      [%zd]: ", j);
                    debug_print_definition(v);
                    v = v->next;
                }

                p = p->next;
            }
        }
    }
}

void debug_ir_global_names(java_ir* ir)
{
    hash_table* table = lookup_global_scope(ir);

    printf("===== GLOBAL NAMES =====\n");
    printf("memory: %zd bytes\n", hash_table_memory_size(table));
    printf("load factor: %.2f%%\n", hash_table_load_factor(table) * 100.0f);
    printf("longest chain: %zd\n", hash_table_longest_chain_length(table));

    debug_print_scope_frame_table(table);
}

void debug_ir_lookup(java_ir* ir)
{
    scope_frame* lh = ir->scope_stack_top;

    if (!lh)
    {
        printf("(lookup stack is empty)\n");
        return;
    }

    while (lh)
    {
        printf("===== LOOKUP STACK =====\n>>>>>>>>>> ");

        switch (lh->type)
        {
            case LST_COMPILATION_UNIT:
                printf("Compilation Scope\n");
                break;
            case LST_CLASS:
                printf("Class Scope\n");
                break;
            case LST_INTERFACE:
                printf("Interface Scope\n");
                break;
            case LST_METHOD:
                printf("Method Scope\n");
                break;
            case LST_NONE:
                printf("Scope\n");
                break;
            case LST_IF:
                printf("If Scope\n");
                break;
            case LST_ELSE:
                printf("Else Scope\n");
                break;
            case LST_FOR:
                printf("For Scope\n");
                break;
            case LST_WHILE:
                printf("While Scope\n");
                break;
            case LST_DO:
                printf("Do Scope\n");
                break;
            case LST_TRY:
                printf("Try Scope\n");
                break;
            case LST_CATCH:
                printf("Catch Scope\n");
                break;
            case LST_FINALLY:
                printf("Finally Scope\n");
                break;
            default:
                printf("(UNKNOWN SCOPE TYPE)\n");
                break;
        }

        hash_table* table = lh->table;

        printf("memory: %zd bytes\n", hash_table_memory_size(table));
        printf("load factor: %.2f%%\n", hash_table_load_factor(table) * 100.0f);
        printf("longest chain: %zd\n", hash_table_longest_chain_length(table));

        debug_print_scope_frame_table(table);

        lh = lh->next;
    }
}
