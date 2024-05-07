#include "debug.h"

void debug_print_indentation(size_t depth)
{
    for (size_t d = 0; d < depth; d++) { printf("    "); }
}

void debug_print_binary_stream(const char* stream, size_t bin_len, size_t depth)
{
    size_t flag;
    char hex_tbl_print[8];
    char hex_tbl_cur;
    unsigned char pr;

    debug_print_indentation(depth);
    printf("            HEX             |  CHAR  \n");
    debug_print_indentation(depth);
    printf("----------------------------+--------\n");

    for (size_t i = 0, j = 0; i < bin_len; i++)
    {
        flag = i % 8;
        hex_tbl_cur = isgraph(stream[i]) ? stream[i] : '.';

        if (i != 0 && (i == bin_len - 1 || flag == 0))
        {
            debug_print_indentation(depth);

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

void debug_print_memory(byte* mem, long size, long line_break)
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

void debug_print_lexeme_type(java_lexeme_type id)
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

void debug_print_operator(java_parser* parser, operator_id id)
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

void debug_print_modifier_bit_flag(lbit_flag modifiers)
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

void debug_print_ast_node(java_parser* parser, tree_node* node)
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

void debug_print_irop(irop op)
{
    switch (op)
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
            printf("(UNDEFINED IROP: %d)", op);
            break;
    }
}

void debug_print_cfg_node_type(block_type type)
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

void debug_print_reference(reference* r)
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

void debug_print_instructions(instruction* inst, size_t* cnt, size_t depth)
{
    if (!inst)
    {
        printf("|");
        debug_print_indentation(depth);
        printf("(no instructions)\n");

        return;
    }

    while (inst)
    {
        printf("|");
        debug_print_indentation(depth);
        printf("[%zd][%p][%zd]: ", inst->id, inst, inst->node->id);

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

void debug_print_cfg(cfg* g, size_t depth)
{
    basic_block* b;
    instruction* inst;
    cfg_edge* edge;
    size_t instruction_count = 0;
    size_t node_out_count = 0;
    size_t node_in_count = 0;

    if (!g || g->nodes.num == 0)
    {
        debug_print_indentation(depth);
        printf("(empty)\n");
        return;
    }

    for (size_t i = 0; i < g->nodes.num; i++)
    {
        printf("|");

        b = g->nodes.arr[i];

        node_out_count += b->out.size;
        node_in_count += b->in.size;

        debug_print_indentation(depth);

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

    printf(">>>>> %zd bytes: %zd/%zd nodes, %zd/%zd edges, %zd instructions\n",
        sizeof(cfg) +
        sizeof(basic_block*) * g->nodes.size +
        sizeof(basic_block) * g->nodes.num +
        sizeof(cfg_edge*) * (g->edges.size + node_in_count + node_out_count) +
        sizeof(cfg_edge) * g->edges.num +
        sizeof(instruction) * instruction_count,
        g->nodes.num,
        g->nodes.size,
        g->edges.num,
        g->edges.size,
        instruction_count
    );
}

void debug_print_definition(definition* v, size_t depth)
{
    static const char* method_regular = "method";
    static const char* method_ctor = "constructor";
    java_lexeme_type lex_type;

    switch (v->type)
    {
        case DEFINITION_VARIABLE:
            switch (v->variable->kind)
            {
                case VARIABLE_KIND_LOCAL:
                    printf("def local var, order %zd, ", v->lid);
                    break;
                case VARIABLE_KIND_MEMBER:
                    printf("def member var, order %zd, ", v->mid);
                    break;
                case VARIABLE_KIND_PARAMETER:
                    printf("def parameter var, order %zd, ", v->lid);
                    break;
                default:
                    printf("def unknown kind var, ");
                    break;
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
            debug_print_binary_stream(v->li_string->stream, v->li_string->length, depth + 1);
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

void debug_print_definition_pool(definition_pool* pool, size_t depth)
{
    printf(">");
    debug_print_indentation(depth);
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

void debug_print_name_definition_table(hash_table* table, size_t depth)
{
    for (size_t i = 0; i < table->bucket_size; i++)
    {
        hash_pair* p = table->bucket[i];

        while (p)
        {
            debug_print_indentation(depth);

            // key
            printf("[%p] %s: ", p->value, (char*)(p->key));

            // print all definitions of this name
            debug_print_definition(p->value, depth);

            p = p->next;
        }
    }
}

void debug_print_error_stack(java_error_stack* stack, size_t depth)
{
    if (!stack || !stack->first)
    {
        debug_print_indentation(depth);
        printf("(null)\n");
        return;
    }

    for (java_error_entry* entry = stack->first; entry != NULL; entry = entry->next)
    {
        debug_print_indentation(depth);

        switch (entry->type)
        {
            case ERROR_ENTRY_NORMAL:
                printf("[%d](%zd, %zd)-(%zd, %zd): %s\n",
                    entry->id,
                    entry->begin.ln,
                    entry->begin.col,
                    entry->end.ln,
                    entry->end.col,
                    entry->msg ? entry->msg : "(null)"
                );
                break;
            case ERROR_ENTRY_AMBIGUITY:
                printf("<AMBIGUOUS ENTRY>: %zd entry\n", entry->ambiguity.len);

                for (size_t i = 0; i < entry->ambiguity.len; i++)
                {
                    debug_print_indentation(depth + 1);
                    printf("Entry [%zd]:\n", i);
                    debug_print_error_stack(&entry->ambiguity.arr[i], depth + 2);
                }
                break;
            default:
                printf("<UNDEFINED ENTRY TYPE: %d>\n", entry->type);
                break;
        }
    }
}

void debug_print_index_set(index_set* ixs)
{
    size_t cnt = 0;

    printf("{");

    for (size_t i = 0; i < ixs->n_cell; i++)
    {
        size_t s = 0;

        for (INDEX_CELL_TYPE j = INDEX_CELL_MASK_IDX0; j != 0; j >>= 1, s++)
        {
            if (ixs->data[i] & j)
            {
                if (cnt > 0) { printf(", "); }
                printf("%zd", i * INDEX_CELL_BITS + s);
                cnt++;
            }
        }
    }

    printf("}");
}
