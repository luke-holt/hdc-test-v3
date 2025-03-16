#ifndef BPE_H
#define BPE_H

#include <stdbool.h>
#include <stddef.h>

#include "util.h"

#define BYTEPAIR_EQ(a, b) (((a).left == (b).left) && ((a).right == (b).right))

typedef uint32_t IBytePair;

typedef struct {
    IBytePair left;
    IBytePair right;
} BytePair;

typedef struct {
    uint32_t count;
    uint32_t capacity;
    BytePair *items;
} BytePairTable;

typedef struct {
    unsigned int freq;
    BytePair pair;
} BytePairFreq;

typedef struct {
    uint32_t count;
    uint32_t capacity;
    BytePairFreq *items;
} BytePairFreqList;

typedef struct {
    uint32_t count;
    uint32_t capacity;
    IBytePair *items;
} BytePairData;

BytePairTable bytepair_table_new(void);
void bytepair_table_delete(BytePairTable *table);
void bytepair_data_delete(BytePairData *data);
BytePairData bytepair_encode(BytePairTable *table, size_t minfreq, const char *data, size_t size);
String bytepair_decode(BytePairTable *table, const BytePairData *data);

#endif // BPE_H


#ifdef BPE_IMPLEMENTATION

void bytepair_replace(BytePairData *data, BytePair pair, IBytePair bytepair_id);
void bytepair_frequency_analysis(const BytePairData *data, BytePairFreqList *list);

int bytepair_freq_cmp(const void *a, const void *b) {
    return ((BytePairFreq *)a)->freq - ((BytePairFreq *)b)->freq;
}

void
bytepair_frequency_analysis(const BytePairData *data, BytePairFreqList *list)
{
    for (uint32_t i = 0; i < data->count-1; i++) {
        BytePair d = { .left = data->items[i], .right = data->items[i+1] };
        int idx = 0;
        while (idx < list->count)
            if (!BYTEPAIR_EQ(list->items[idx].pair, d)) idx++;
            else break;

        if (idx < list->count)
            list->items[idx].freq++;
        else
            da_append(list, ((BytePairFreq){1, d}));
    }

    qsort(list->items, list->count, sizeof(*list->items), bytepair_freq_cmp);
}

void
bytepair_replace(BytePairData *data, BytePair pair, IBytePair bytepair_id)
{
    uint32_t data_end_idx = data->count-1;
    da_reset(data);
    uint32_t i = 0;
    while (i < data_end_idx) {
        if (BYTEPAIR_EQ(*(BytePair *)&data->items[i], pair)) {
            da_append(data, bytepair_id);
            i += 2;
        } else {
            da_append(data, data->items[i]);
            i++;
        }
    }
    // add last item if not added
    if (i == data_end_idx)
        da_append(data, data->items[i]);
}

BytePairTable
bytepair_table_new(void)
{
    BytePairTable table;
    da_init(&table);
    // initial ascii characters
    for (IBytePair c = 0; c < 0x100; c++)
        da_append(&table, ((BytePair){ .left = c, .right = 0}));
    return table;
}

void
bytepair_table_delete(BytePairTable *table)
{
    UTIL_ASSERT(table);
    UTIL_ASSERT(table->items);
    da_delete(table);
}

void
bytepair_data_delete(BytePairData *data)
{
    UTIL_ASSERT(data);
    UTIL_ASSERT(data->items);
    da_delete(data);
}

BytePairData
bytepair_encode(BytePairTable *table, size_t minfreq, const char *data, size_t size)
{
    UTIL_ASSERT(table);
    UTIL_ASSERT(data);

    BytePairFreqList freqs;
    BytePairData encoded;

    da_init(&freqs);
    da_init(&encoded);

    // transfer data
    for (size_t i = 0; i < size; i++)
        da_append(&encoded, (IBytePair)data[i]);

    if (minfreq > 0) {
        BytePairFreq mostfreq;
        do {
            da_reset(&freqs);
            bytepair_frequency_analysis(&encoded, &freqs);

            mostfreq = da_tail_item(&freqs);
            da_append(table, mostfreq.pair);
            bytepair_replace(&encoded, mostfreq.pair, (IBytePair)da_tail_idx(table));
        } while (mostfreq.freq > minfreq);
    }
    else {
        for (size_t i = 0x100; i < table->count; i++)
            bytepair_replace(&encoded, table->items[i], (IBytePair)da_tail_idx(table));
    }

    // resize encoded bytepair array
    da_resize(&encoded, encoded.count);

    da_delete(&freqs);

    return encoded;
}

String
bytepair_decode(BytePairTable *table, const BytePairData *data)
{
    String decoded;

    da_init(&decoded);

    BytePairTable stack;
    da_init(&stack);

    for (size_t i = 0; i < data->count; i++) {
        BytePair pair = table->items[data->items[i]];
        while (stack.count || pair.right) {
            // reached leaf
            if (pair.right == 0) {
                // output decoded char
                da_append(&decoded, (char)pair.left);

                pair = da_tail_item(&stack);
                da_pop(&stack);
            } else {
                da_append(&stack, table->items[pair.right]);
                pair = table->items[pair.left];
            }
        }
        // append final char
        da_append(&decoded, (char)pair.left);
    }

    da_delete(&stack);

    da_resize(&decoded, decoded.count);

    return decoded;
}

#endif // BPE_IMPLEMENTATION
