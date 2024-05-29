#include "debug.h"

typedef struct
{
    char* content;
    java_number_type type;
} debug_number_data;

typedef struct
{
    basic_block** postorder;
    basic_block** idom;
    index_set* dom;
    index_set* frontier;
} dominance_data;

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

void debug_test_number_library()
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

            debug_print_binary_stream(bin.stream, bin.len, 0);

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

static debug_test_case_run_dominance(cfg_worker* worker, size_t case_number)
{
    printf("\n===== DOMINANCE TEST %zd =====\n", case_number);

    cfg g;
    dominance_data d;

    release_cfg_worker(worker, &g, NULL);

    d.postorder = cfg_node_order(&g, DFS_POSTORDER);
    d.idom = cfg_idom(&g, d.postorder);
    d.dom = cfg_dominators(&g, d.idom);
    d.frontier = cfg_dominance_frontiers(&g, d.idom);

    printf("CFG:\n");
    debug_print_cfg(&g, 1);

    printf("Immediate Dominators:\n");
    for (size_t i = 0; i < g.nodes.num; i++)
    {
        debug_print_indentation(1);
        printf("[%zd]: %zd\n", i, d.idom[i]->id);
    }

    printf("\nDominators:\n");
    for (size_t i = 0; i < g.nodes.num; i++)
    {
        debug_print_indentation(1);
        printf("[%zd]: ", i);
        debug_print_index_set(&d.dom[i]);
        printf("\n");
    }

    printf("\nDominance Frontiers:\n");
    for (size_t i = 0; i < g.nodes.num; i++)
    {
        debug_print_indentation(1);
        printf("[%zd]: ", i);
        debug_print_index_set(&d.frontier[i]);
        printf("\n");
    }

    cfg_delete_node_order(d.postorder);
    cfg_delete_idom(d.idom);
    cfg_delete_dominators(&g, d.dom);
    cfg_delete_dominance_frontiers(&g, d.frontier);
    release_cfg(&g);
}

void debug_test_dominance()
{
    cfg_worker worker;
    basic_block* p[10]; // temp variables, expand as needed

    /**
     * Test Case 1
     *
     * Figure 4 in A Simple, Fast Dominance Algorithm by Cooper et al.
     * https://www.cs.tufts.edu/comp/150FP/archive/keith-cooper/dom14.pdf
     *
     * EXPECTED: idom = {0,0,0,0,0,0}
    */
    init_cfg_worker(&worker);
    p[6] = cfg_worker_grow(&worker);
    p[5] = cfg_worker_grow(&worker);
    cfg_worker_jump(&worker, p[6], true, false);
    p[4] = cfg_worker_grow(&worker);
    cfg_worker_jump(&worker, p[5], true, false);
    p[1] = cfg_worker_grow(&worker);
    p[2] = cfg_worker_grow(&worker);
    p[3] = cfg_worker_grow(&worker);
    cfg_worker_jump(&worker, p[2], true, true);
    cfg_worker_jump(&worker, p[1], true, true);
    cfg_worker_jump(&worker, p[4], true, false);
    cfg_worker_jump(&worker, p[2], false, true);
    cfg_worker_jump(&worker, p[3], false, true);

    // Test
    debug_test_case_run_dominance(&worker, 1);

    /**
     * Test Case 2
     *
     * https://pages.cs.wisc.edu/~fischer/cs701.f14/lectures/L.All.4up.pdf
     * Page 24
     *
     * EXPECTED: df =
     *     [0]: {}
     *     [1]: {5}
     *     [2]: {4}
     *     [3]: {4}
     *     [4]: {5}
     *     [5]: {}
     *
     * EXPECTED: dominator tree
     *       0
     *     /   \
     *    1     5
     *  / | \
     * 2  3  4
    */
    init_cfg_worker(&worker);
    p[0] = cfg_worker_grow(&worker);
    p[1] = cfg_worker_grow(&worker);
    p[2] = cfg_worker_grow(&worker);
    cfg_worker_jump(&worker, p[1], true, false);
    p[3] = cfg_worker_grow(&worker);
    p[4] = cfg_worker_grow(&worker);
    p[5] = cfg_worker_grow(&worker);
    cfg_worker_jump(&worker, p[2], true, false);
    cfg_worker_jump(&worker, p[4], false, true);
    cfg_worker_jump(&worker, p[0], true, false);
    cfg_worker_jump(&worker, p[5], false, true);

    // Test
    debug_test_case_run_dominance(&worker, 2);

    /**
     * Test Case 3
     *
     * https://ethz.ch/content/dam/ethz/special-interest/infk/inst-cs/lst-dam/
     *    documents/Education/Classes/Spring2016/2810_Advanced_Compiler_Design/Homework/slides_hw1.pdf
     * Page 39
     *
     * EXPECTED: df =
     *     [0]: {}
     *     [1]: {3}
     *     [2]: {2,3}
     *     [3]: {}
     *     [4]: {2}
     *
     * EXPECTED: dominator tree
     *    0
     *  / | \
     * 1  2  3
     *    |
     *    4
    */
    init_cfg_worker(&worker);
    p[0] = cfg_worker_grow(&worker);
    p[1] = cfg_worker_grow(&worker);
    cfg_worker_jump(&worker, p[0], true, false);
    p[2] = cfg_worker_grow(&worker);
    p[3] = cfg_worker_grow(&worker);
    cfg_worker_jump(&worker, p[1], true, false);
    cfg_worker_jump(&worker, p[3], false, true);
    cfg_worker_jump(&worker, p[2], true, false);
    p[4] = cfg_worker_grow(&worker);
    cfg_worker_jump(&worker, p[2], false, true);

    // Test
    debug_test_case_run_dominance(&worker, 3);
}
