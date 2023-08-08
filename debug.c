#include "debug.h"

#include "langspec.h"
#include "token.h"
#include "node.h"

#include "report.h"

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

void debug_format_report(byte report_type)
{
    printf("===== COMPILER RUNTIME REPORT =====\n");

    if (report_type & REPORT_INTERNAL)
    {
        printf("Internal:\n");
        printf("    Number of reserved words of compiled Java: %zd\n",
            compiler_debug_report.internal.reserved_words_count);
        printf("    Lookup table size of reserved words: %zd\n",
            compiler_debug_report.internal.reserved_words_lookup_table_size);
        printf("    Number of collisions during table creation: %zd\n",
            compiler_debug_report.internal.reserved_words_hash_collisions);
        printf("    Longest probing of reserved word lookup: %zd\n",
            compiler_debug_report.internal.reserved_words_longest_probing);
    }

    if (report_type & REPORT_GENERAL)
    {
        printf("General:\n");
        printf("    Compiled Java language version: %zd\n",
            compiler_debug_report.general.language_version);
    }

    printf("===== END OF REPORT =====\n");
}

void debug_print_reserved_words()
{
    printf("===== JAVA RESERVED WORDS =====\n");

    for (int i = 0; i < num_java_reserved_words; i++)
    {
        printf("%s %d\n", java_reserved_words[i].content, java_reserved_words[i].id);
    }
}

void debug_print_symbol_table(java_symbol_table* table)
{
    printf("===== RESERVED WORDS LOOKUP =====\n");

    bool in_skip = false;
    int c = 0;

    for (int i = 0; i < table->num_slot; i++)
    {
        if (table->slots[i])
        {
            if (table->slots[i]->word)
            {
                printf("    %lx \"%s\", id=%d\n",
                    table->slots[i]->full_hash,
                    table->slots[i]->word->content,
                    table->slots[i]->word->id);
            }
            else
            {
                printf("    ERROR: word not attached.\n");
            }
            in_skip = false;
            c++;
        }
        else if (!in_skip)
        {
            printf("    ...\n");
            in_skip = true;
        }
    }

    printf("count: %d, should match reserved word count (%d)\n",
        c, num_java_reserved_words);
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

void debug_tokenize(file_buffer* buffer, java_symbol_table* table)
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

static void debug_print_token_list_content(linked_list* list, const char sp)
{
    ll_item* item = list->first;
    while (true)
    {
        debug_print_token_content(item->data);
        item = item->next;

        if (item)
        {
            printf("%c", sp);
        }
        else
        {
            break;
        }
    }
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
        printf("N/A");
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

static void debug_print_ast_node(java_node_query query, void* data)
{
    switch (query)
    {
        case JNT_UNIT:
            printf("Compilation Unit");
            break;
        case JNT_NAME:
            printf("Name");
            break;
        case JNT_NAME_UNIT:
            printf("Unit: ");
            debug_print_token_content(&((node_data_name_unit*)data)->id);
            break;
        case JNT_CLASS_TYPE:
            printf("Class Type");
            break;
        case JNT_CLASS_TYPE_UNIT:
            printf("Unit: ");
            debug_print_token_content(&((node_data_class_type_unit*)data)->id);
            break;
        case JNT_INTERFACE_TYPE:
            printf("Interface Type");
            break;
        case JNT_INTERFACE_TYPE_UNIT:
            printf("Unit: ");
            debug_print_token_content(&((node_data_interface_type_unit*)data)->id);
            break;
        case JNT_INTERFACE_TYPE_LIST:
            printf("Interface Type List");
            break;
        case JNT_PKG_DECL:
            printf("Package Declaration");
            break;
        case JNT_IMPORT_DECL:
            printf("Import Declaration (On-demand: %s)",
                ((node_data_import_decl*)data)->on_demand ? "true" : "false");
            break;
        case JNT_TOP_LEVEL:
            printf("Top Level: ");
            debug_print_modifier_bit_flag(((node_data_top_level*)data)->modifier);
            break;
        case JNT_CLASS_DECL:
            printf("Class Declaration: ");
            debug_print_token_content(&((node_data_class_declaration*)data)->id);
            break;
        case JNT_INTERFACE_DECL:
            printf("Interface Declaration: ");
            debug_print_token_content(&((node_data_interface_declaration*)data)->id);
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
        default:
            printf("Unknown");
            break;
    }
}

static void debug_print_ast(tree_node* node, int depth)
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

        debug_print_ast_node(node->metadata, node->data);
        printf("\n");

        // go deeper first
        if (node->first_child)
        {
            debug_print_ast(node->first_child, depth + 1);
        }

        // then go next sibling
        node = node->next_sibling;
    }
}

void debug_ast(tree_node* root)
{
    printf("===== ABSTRACT SYNTAX TREE =====\n");
    debug_print_ast(root, 0);
}
