#include "debug.h"

#include "langspec.h"
#include "lexer.h"
#include "node.h"
#include "expression.h"
#include "number.h"

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
        sizeof(irop) * OPID_MAX +
        sizeof(size_t) * OPID_MAX
    );
    printf("Error static data size: %zd bytes\n",
        sizeof(java_error_definition) +
        sizeof(error_type) * JAVA_E_MAX +
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

void debug_reserved_words()
{
    printf("===== JAVA RESERVED WORDS =====\n");

    for (int i = 0; i < num_java_reserved_words; i++)
    {
        printf("%s %d\n", java_reserved_words[i].content, java_reserved_words[i].id);
    }
}

void debug_symbol_table(hash_table* table)
{
    printf("===== RESERVED WORDS LOOKUP =====\n");
    debug_shash_table(table);
}

void debug_tokenize(file_buffer* buffer, hash_table* table, java_error_logger* logger)
{
    printf("===== TOKENIZED BUFFER CONTENT =====\n");

    java_token* token = (java_token*)malloc_assert(sizeof(java_token));
    java_lexer lexer;

    init_lexer(&lexer, buffer, table, logger);

    while (true)
    {
        lexer_next_token(&lexer, token);

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

static void __debug_ast(java_parser* parser, tree_node* node, int depth)
{
    if (!node)
    {
        return;
    }

    // print current level first
    while (node)
    {
        debug_print_indentation(depth);
        debug_print_ast_node(parser, node);
        printf("\n");

        // go deeper first
        if (node->first_child)
        {
            __debug_ast(parser, node->first_child, depth + 1);
        }

        // then go next sibling
        node = node->next_sibling;
    }
}

void debug_ast(java_parser* parser)
{
    printf("===== ABSTRACT SYNTAX TREE =====\n");
    __debug_ast(parser, parser->ast_root, 0);
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

void debug_global_import(java_ir* ir)
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

void debug_error_logger(java_error_logger* logger)
{
    printf("\n===== ERROR LOGGER (MAIN STREAM) =====\n");
    debug_print_error_stack(&logger->main_stream, 0);

    if (logger->current_stream != &logger->main_stream)
    {
        printf("\n===== ERROR LOGGER (CURRENT STREAM) =====\n");
        debug_print_error_stack(logger->current_stream, 0);
    }
}

void debug_optimizer(optimizer* o)
{
    printf("\n===== OPTIMIZER =====\n");

    printf("Immediate Dominators:\n");
    for (size_t i = 0; i < o->graph->nodes.num; i++)
    {
        debug_print_indentation(1);
        printf("[%zd]: %zd\n", i, o->dominance.idom[i]->id);
    }

    printf("\nDominators:\n");
    for (size_t i = 0; i < o->graph->nodes.num; i++)
    {
        debug_print_indentation(1);
        printf("[%zd]: ", i);
        debug_print_index_set(&o->dominance.dom[i]);
        printf("\n");
    }

    printf("\nDominance Frontiers:\n");
    for (size_t i = 0; i < o->graph->nodes.num; i++)
    {
        debug_print_indentation(1);
        printf("[%zd]: ", i);
        debug_print_index_set(&o->dominance.frontier[i]);
        printf("\n");
    }
}
