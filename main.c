#include <float.h>

#define HDVECTOR_IMPLEMENTATION
#include "hdvector.h"

#define BPE_IMPLEMENTATION
#include "bpe.h"

#define UTIL_IMPLEMENTATION
#include "util.h"

int next_most_likely(HDVector *profile, HDVector *symbols, int nsymbols, int a, int b) {
    HDVector v0;
    HDVector v1;

    hdvector_copy(&v0, &symbols[a]);
    hdvector_copy(&v1, &symbols[b]);

    hdvector_shift(&v0, 2);
    hdvector_shift(&v1, 1);

    hdvector_mult(&v0, &v1, &v0);

    hdvector_mult(&v0, profile, &v0);

    int idx = 0;
    float mindist = FLT_MAX;
    for (int i = 0; i < nsymbols; i++) {
        float d = hdvector_distance(&symbols[i], &v0);
        if (mindist > d) {
            mindist = d;
            idx = i;
        }
    }
    return idx;
}

int
main(void)
{
    const char *filename = "one-beer.txt";
    String content; // text file content
    BytePairTable bpe_table; // bpe pair table
    BytePairData bpe_data; // bpe encoded data
    HDVector *hdv_symbol_table; // hdv symbol table
    int hdv_table_count;
    HDVectorSum hdv_sum_vector;
    HDVector hdv_profile;

    if (!string_load_from_file(filename, &content)) {
        util_log("FATAL", "could not load '%s' into string", filename);
        exit(1);
    }

    bpe_table = bytepair_table_new();
    bpe_data = bytepair_encode(&bpe_table, 1, content.items, content.count);
    util_log("INFO", "encoded data from '%s'", filename);
    util_log("INFO", "  original size: %zu", content.count);
    util_log("INFO", "  reduced size: %zu", bpe_data.count);
    util_log("INFO", "  byte-pair table size: %zu", bpe_table.count);

    hdv_table_count = bpe_table.count;
    hdv_symbol_table = malloc(sizeof(*hdv_symbol_table)*hdv_table_count);
    UTIL_ASSERT(hdv_symbol_table);

    hdvector_init_random(hdv_symbol_table, hdv_table_count);

    // for each trigram
    memset(&hdv_profile, 0, sizeof(hdv_profile));
    for (int i = 0; i < bpe_data.count-2; i++) {
        HDVector v0, v1, v2;

        hdvector_copy(&v0, &hdv_symbol_table[bpe_data.items[i+0]]);
        hdvector_copy(&v1, &hdv_symbol_table[bpe_data.items[i+1]]);
        hdvector_copy(&v2, &hdv_symbol_table[bpe_data.items[i+2]]);

        hdvector_shift(&v0, 2);
        hdvector_shift(&v1, 1);

        hdvector_mult(&v0, &v1, &v0);
        hdvector_mult(&v0, &v2, &v0);

        hdvector_add_to_sum(&hdv_sum_vector, &v0);
    }

    hdvector_sum_to_vector(&hdv_sum_vector, &hdv_profile, bpe_data.count/2);

    util_log("INFO", "combined all trigrams into profile vector");

    char line[256] = {0};

    BytePairData bpe_encoded_data;
    da_init(&bpe_encoded_data);

    for (;;) {
        util_log("INFO", "enter table indices:");

        int a, b;
        if (fgets(line, ARRLEN(line), stdin)) {
            if (2 == sscanf(line, "%d,%d", &a, &b)) {
                if (!a && !b) break;

                da_append(&bpe_encoded_data, a);
                da_append(&bpe_encoded_data, b);

                for (int i = 0; i < 1000; i++) {
                    int next = next_most_likely(&hdv_profile, hdv_symbol_table, hdv_table_count, a, b);

                    if (BETWEEN(next, 32, 126) || next >= 0x100)
                        da_append(&bpe_encoded_data, next);

                    a = b;
                    b = next;
                }

                String decoded = bytepair_decode(&bpe_table, &bpe_encoded_data);

                for (int i = 0; i < decoded.count; i++) {
                    if (!BETWEEN(decoded.items[i], 32, 126))
                        decoded.items[i] = '.';
                    if (decoded.items[i] == '\n')
                        decoded.items[i] = ' ';
                }

                util_log("INFO", "output: %*s", decoded.count, decoded.items);
                // util_hexdump(decoded.items, decoded.count);
            }
        }
    }

    bytepair_data_delete(&bpe_data);
    bytepair_table_delete(&bpe_table);
    free(hdv_symbol_table);
    da_delete(&bpe_encoded_data);

    return 0;
}
