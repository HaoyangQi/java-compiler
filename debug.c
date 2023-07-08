#include <stdio.h>

#include "file.h"
#include "symtbl.h"
#include "langspec.h"
#include "token.h"

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

void debug_print_reserved_word(rwid id)
{
    switch (id)
    {
        case RWID_PUBLIC:
            printf("RWID_PUBLIC");
            break;
        case RWID_PRIVATE:
            printf("RWID_PRIVATE");
            break;
        case RWID_PROTECTED:
            printf("RWID_PROTECTED");
            break;
        case RWID_FINAL:
            printf("RWID_FINAL");
            break;
        case RWID_STATIC:
            printf("RWID_STATIC");
            break;
        case RWID_ABSTRACT:
            printf("RWID_ABSTRACT");
            break;
        case RWID_TRANSIENT:
            printf("RWID_TRANSIENT");
            break;
        case RWID_SYNCHRONIZED:
            printf("RWID_SYNCHRONIZED");
            break;
        case RWID_VOLATILE:
            printf("RWID_VOLATILE");
            break;
        case RWID_DEFAULT:
            printf("RWID_DEFAULT");
            break;
        case RWID_IF:
            printf("RWID_IF");
            break;
        case RWID_THROW:
            printf("RWID_THROW");
            break;
        case RWID_BOOLEAN:
            printf("RWID_BOOLEAN");
            break;
        case RWID_DO:
            printf("RWID_DO");
            break;
        case RWID_IMPLEMENTS:
            printf("RWID_IMPLEMENTS");
            break;
        case RWID_THROWS:
            printf("RWID_THROWS");
            break;
        case RWID_BREAK:
            printf("RWID_BREAK");
            break;
        case RWID_DOUBLE:
            printf("RWID_DOUBLE");
            break;
        case RWID_IMPORT:
            printf("RWID_IMPORT");
            break;
        case RWID_BYTE:
            printf("RWID_BYTE");
            break;
        case RWID_ELSE:
            printf("RWID_ELSE");
            break;
        case RWID_INSTANCEOF:
            printf("RWID_INSTANCEOF");
            break;
        case RWID_RETURN:
            printf("RWID_RETURN");
            break;
        case RWID_TRY:
            printf("RWID_TRY");
            break;
        case RWID_CASE:
            printf("RWID_CASE");
            break;
        case RWID_EXTENDS:
            printf("RWID_EXTENDS");
            break;
        case RWID_INT:
            printf("RWID_INT");
            break;
        case RWID_SHORT:
            printf("RWID_SHORT");
            break;
        case RWID_VOID:
            printf("RWID_VOID");
            break;
        case RWID_CATCH:
            printf("RWID_CATCH");
            break;
        case RWID_INTERFACE:
            printf("RWID_INTERFACE");
            break;
        case RWID_CHAR:
            printf("RWID_CHAR");
            break;
        case RWID_FINALLY:
            printf("RWID_FINALLY");
            break;
        case RWID_LONG:
            printf("RWID_LONG");
            break;
        case RWID_SUPER:
            printf("RWID_SUPER");
            break;
        case RWID_WHILE:
            printf("RWID_WHILE");
            break;
        case RWID_CLASS:
            printf("RWID_CLASS");
            break;
        case RWID_FLOAT:
            printf("RWID_FLOAT");
            break;
        case RWID_NATIVE:
            printf("RWID_NATIVE");
            break;
        case RWID_SWITCH:
            printf("RWID_SWITCH");
            break;
        case RWID_FOR:
            printf("RWID_FOR");
            break;
        case RWID_NEW:
            printf("RWID_NEW");
            break;
        case RWID_CONTINUE:
            printf("RWID_CONTINUE");
            break;
        case RWID_PACKAGE:
            printf("RWID_PACKAGE");
            break;
        case RWID_THIS:
            printf("RWID_THIS");
            break;
        case RWID_CONST:
            printf("RWID_CONST");
            break;
        case RWID_GOTO:
            printf("RWID_GOTO");
            break;
        case RWID_TRUE:
            printf("RWID_TRUE");
            break;
        case RWID_FALSE:
            printf("RWID_FALSE");
            break;
        case RWID_NULL:
            printf("RWID_NULL");
            break;
        default:
            printf("(UNKNOWN)");
            break;
    }
}

void debug_print_literal_type(java_literal_type li)
{
    switch (li)
    {
        case JT_LI_NUM:
            printf("Number");
            break;
        case JT_LI_CHAR:
            printf("Character");
            break;
        case JT_LI_STR:
            printf("String");
            break;
        default:
            printf("(UNKNOWN)");
            break;
    }
}

void debug_print_operator_type(java_operator_type op)
{
    switch (op)
    {
        case JT_OP_ASN:
            printf("JT_OP_ASN \"=\"");
            break;
        case JT_OP_GT:
            printf("JT_OP_GT \">\"");
            break;
        case JT_OP_LT:
            printf("JT_OP_LT \"<\"");
            break;
        case JT_OP_NEG:
            printf("JT_OP_NEG \"!\"");
            break;
        case JT_OP_CPM:
            printf("JT_OP_CPM \"~\"");
            break;
        case JT_OP_AWR:
            printf("JT_OP_AWR \"->\"");
            break;
        case JT_OP_EQ:
            printf("JT_OP_EQ \"==\"");
            break;
        case JT_OP_LE:
            printf("JT_OP_LE \"<=\"");
            break;
        case JT_OP_GE:
            printf("JT_OP_GE \">=\"");
            break;
        case JT_OP_NE:
            printf("JT_OP_NE \"!=\"");
            break;
        case JT_OP_LAND:
            printf("JT_OP_LAND \"&&\"");
            break;
        case JT_OP_LOR:
            printf("JT_OP_LOR \"||\"");
            break;
        case JT_OP_INC:
            printf("JT_OP_INC \"++\"");
            break;
        case JT_OP_DEC:
            printf("JT_OP_DEC \"--\"");
            break;
        case JT_OP_ADD:
            printf("JT_OP_ADD \"+\"");
            break;
        case JT_OP_SUB:
            printf("JT_OP_SUB \"-\"");
            break;
        case JT_OP_MUL:
            printf("JT_OP_MUL \"*\"");
            break;
        case JT_OP_DIV:
            printf("JT_OP_DIV \"/\"");
            break;
        case JT_OP_AND:
            printf("JT_OP_AND \"&\"");
            break;
        case JT_OP_OR:
            printf("JT_OP_OR \"|\"");
            break;
        case JT_OP_XOR:
            printf("JT_OP_XOR \"^\"");
            break;
        case JT_OP_MOD:
            printf("JT_OP_MOD \"%\"");
            break;
        case JT_OP_LS:
            printf("JT_OP_LS \"<<\"");
            break;
        case JT_OP_RS:
            printf("JT_OP_RS \">>\"");
            break;
        case JT_OP_ZFRS:
            printf("JT_OP_ZFRS \">>>\"");
            break;
        case JT_OP_ADDASN:
            printf("JT_OP_ADDASN \"+=\"");
            break;
        case JT_OP_SUBASN:
            printf("JT_OP_SUBASN \"-=\"");
            break;
        case JT_OP_MULASN:
            printf("JT_OP_MULASN \"*=\"");
            break;
        case JT_OP_DIVASN:
            printf("JT_OP_DIVASN \"/=\"");
            break;
        case JT_OP_ANDASN:
            printf("JT_OP_ANDASN \"&=\"");
            break;
        case JT_OP_ORASN:
            printf("JT_OP_ORASN \"|=\"");
            break;
        case JT_OP_XORASN:
            printf("JT_OP_XORASN \"^=\"");
            break;
        case JT_OP_MODASN:
            printf("JT_OP_MODASN \"%=\"");
            break;
        case JT_OP_LSASN:
            printf("JT_OP_LSASN \"<<=\"");
            break;
        case JT_OP_RSASN:
            printf("JT_OP_RSASN \">>=\"");
            break;
        case JT_OP_ZFRSASN:
            printf("JT_OP_ZFRSASN \">>>=\"");
            break;
        default:
            printf("(UNKNOWN)");
            break;
    }
}

void debug_print_separator_type(java_separator_type sp)
{
    switch (sp)
    {
        case JT_SP_PL:
            printf("JT_SP_PL \"(\"");
            break;
        case JT_SP_PR:
            printf("JT_SP_PR \")\"");
            break;
        case JT_SP_BL:
            printf("JT_SP_BL \"{\"");
            break;
        case JT_SP_BR:
            printf("JT_SP_BR \"}\"");
            break;
        case JT_SP_SL:
            printf("JT_SP_SL \"[\"");
            break;
        case JT_SP_SR:
            printf("JT_SP_SR \"]\"");
            break;
        case JT_SP_SC:
            printf("JT_SP_SC \";\"");
            break;
        case JT_SP_CM:
            printf("JT_SP_CM \",\"");
            break;
        case JT_SP_AT:
            printf("JT_SP_AT \"@\"");
            break;
        case JT_SP_QST:
            printf("JT_SP_QST \"?\"");
            break;
        case JT_SP_CL:
            printf("JT_SP_CL \":\"");
            break;
        case JT_SP_CC:
            printf("JT_SP_CC \"::\"");
            break;
        case JT_SP_DOT:
            printf("JT_SP_DOT \".\"");
            break;
        case JT_SP_DDD:
            printf("JT_SP_DDD \"...\"");
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
    size_t len = buffer_count(token->from, token->to);
    char* content = (char*)malloc(sizeof(char) * (len + 1));
    ASSERT_ALLOCATION(content);

    buffer_substring(content, token->from, len);
    printf(content);

    free(content);
}

void debug_tokenize(file_buffer* buffer, java_symbol_table* table)
{
    printf("===== TOKENIZED BUFFER CONTENT =====\n");

    java_token* token = (java_token*)malloc(sizeof(java_token));
    ASSERT_ALLOCATION(token);

    while (true)
    {
        get_next_token(token, buffer, table);

        if (token->type == JT_EOF)
        {
            break;
        }

        switch (token->type)
        {
            case JT_IDENTIFIER:
                printf("Name");
                if (token->keyword)
                {
                    printf(" Keyword: ");
                    debug_print_reserved_word(token->keyword->id);
                }
                printf("\n");
                debug_print_token_content(token);
                printf("\n");
                break;
            case JT_LITERAL:
                printf("Literal ");
                debug_print_literal_type(token->subtype.li);
                if (token->subtype.li == JT_LI_NUM)
                {
                    printf(" ");
                    debug_print_number_type(token->number);
                    printf(" length: %d", token->number_length);
                }
                printf("\n");
                debug_print_token_content(token);
                printf("\n");
                break;
            case JT_OPERATOR:
                printf("Operator ");
                debug_print_operator_type(token->subtype.op);
                printf("\n");
                debug_print_token_content(token);
                printf("\n");
                break;
            case JT_SEPARATOR:
                printf("Separator ");
                debug_print_separator_type(token->subtype.sp);
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

    free(token);
}
