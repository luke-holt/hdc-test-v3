#ifndef HDVECTOR_H
#define HDVECTOR_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifndef DIMENSIONS
#define DIMENSIONS (1 << 13)
#endif

#define BITS_IN_U64 (8 * sizeof(uint64_t))
#define HDV_U64_LEN (DIMENSIONS / BITS_IN_U64)

typedef struct { uint64_t data[HDV_U64_LEN]; } HDVector;
typedef struct { uint32_t bitsum[DIMENSIONS]; } HDVectorSum;

void hdvector_init_random(HDVector *vector, size_t count);
void hdvector_shift(HDVector *vector, size_t shift);
void hdvector_copy(HDVector *dst, const HDVector *src);
void hdvector_mult(HDVector *a, HDVector *b, HDVector *out);
bool hdvector_load_from_file(const char *filename, HDVector *profile, HDVector *symbols, size_t count);
bool hdvector_store_to_file(const char *filename, HDVector *profile, HDVector *symbols, size_t count);
float hdvector_distance(HDVector *a, HDVector *b);
void hdvector_form_query(HDVector *symbols, size_t scount, size_t *indices, size_t icount, HDVector *query);
void hdvector_add_to_sum(HDVectorSum *sum, HDVector *vector);
void hdvector_sum_to_vector(HDVectorSum *sum, HDVector *vector, size_t threshold);

#endif // HDVECTOR_H


#ifdef HDVECTOR_IMPLEMENTATION

#include <string.h>

#include "util.h"

#define HDVECTOR_FILE_MAGIC_NUMBER (0x48445653) // 'H' 'D' 'V' 'S'

#define DIM2WORD(d) ((d) / BITS_IN_U64)
#define DIM2BITNUM(d) ((d) & (BITS_IN_U64-1))
#define DIM2BITMASK(d) (1 << DIM2BITNUM(d))

uint32_t rng_state[4] = {
    0xECED57FC, 0xA8A913B8,
    0x646574DF, 0x2021309B,
};

void
hdvector_init_random(HDVector *vector, size_t count)
{
    UTIL_ASSERT(vector);
    for (size_t v = 0; v < count; v++) {
        for (size_t i = 0; i < ARRLEN(vector->data); i++) {
            uint64_t msw = util_xorshift128(rng_state);
            uint64_t lsw = util_xorshift128(rng_state);
            vector[v].data[i] = msw << 32 | lsw;
        }
    }
}

void
hdvector_shift(HDVector *vector, size_t shift)
{
    UTIL_ASSERT(vector);
    for (size_t i = 0; i < ARRLEN(vector->data); i++)
        vector->data[i] = (vector->data[i] << shift) | (vector->data[i] >> (BITS_IN_U64 - shift));
}

void
hdvector_copy(HDVector *dst, const HDVector *src)
{
    UTIL_ASSERT(dst);
    UTIL_ASSERT(src);
    memcpy(dst, src, sizeof(*dst));
}

void
hdvector_mult(HDVector *a, HDVector *b, HDVector *out)
{
    UTIL_ASSERT(a);
    UTIL_ASSERT(b);
    UTIL_ASSERT(out);
    for (size_t i = 0; i < ARRLEN(out->data); i++)
        out->data[i] = a->data[i] ^ b->data[i];
}

bool
hdvector_load_from_file(const char *filename, HDVector *profile, HDVector *symbols, size_t count)
{
    UTIL_ASSERT(filename);
    UTIL_ASSERT(profile);
    UTIL_ASSERT(symbols);

    FILE *file;
    int rc;
    size_t n;
    uint32_t magic_num;
    bool success = false;

    file = fopen(filename, "rb");
    if (!file) return false;

    n = fread(&magic_num, sizeof(magic_num), 1, file);
    if (n != 1) goto defer;
    if (magic_num != HDVECTOR_FILE_MAGIC_NUMBER) {
        util_log("WARN", "%s: invalid HDVector file", __func__);
        goto defer;
    }

    n = fread(profile, sizeof(*profile), 1, file);
    if (n != 1) goto defer;

    n = fread(symbols, sizeof(*symbols), count, file);
    if (n != count) goto defer;

    success = true;

defer:
    (void)fclose(file);
    return success;
}

bool
hdvector_store_to_file(const char *filename, HDVector *profile, HDVector *symbols, size_t count)
{
    UTIL_ASSERT(filename);
    UTIL_ASSERT(profile);
    UTIL_ASSERT(symbols);

    FILE *file;
    int rc;
    size_t n;
    uint32_t magic_num;
    bool success = false;

    file = fopen(filename, "wb");
    if (!file) return false;

    magic_num = HDVECTOR_FILE_MAGIC_NUMBER;
    n = fwrite(&magic_num, sizeof(magic_num), 1, file);
    if (n != 1) goto defer;

    n = fwrite(profile, sizeof(*profile), 1, file);
    if (n != 1) goto defer;

    n = fwrite(symbols, sizeof(*symbols), count, file);
    if (n != count) goto defer;

    success = true;

defer:
    (void)fclose(file);
    return success;
}

float
hdvector_distance(HDVector *a, HDVector *b)
{
    UTIL_ASSERT(a);
    UTIL_ASSERT(b);
    size_t diff = 0;
    for (size_t i = 0; i < ARRLEN(a->data); i++) {
        uint64_t word = a->data[i] ^ b->data[i];
        uint32_t most = (word >> 32) & 0xFFFFFFFF;
        uint32_t least = word & 0xFFFFFFFF;
        diff += util_bitcount(most) + util_bitcount(least);
    }
    return (float)diff / (float)DIMENSIONS;
}

void
hdvector_form_query(HDVector *symbols, size_t scount, size_t *indices, size_t icount, HDVector *query)
{
    UTIL_ASSERT(symbols && scount);
    UTIL_ASSERT(indices && icount);
    UTIL_ASSERT(query);
    HDVector tmp, q = {0};
    for (size_t i = 0; i < icount; i++) {
        hdvector_copy(&tmp, &symbols[indices[i]]);
        hdvector_shift(&tmp, icount-i);
        hdvector_mult(&tmp, &q, &q);
    }
    *query = q;
}

void
hdvector_add_to_sum(HDVectorSum *sum, HDVector *vector)
{
    for (size_t i = 0; i < DIMENSIONS; i++)
        sum->bitsum[i] += (vector->data[DIM2WORD(i)] & DIM2BITMASK(i)) == DIM2BITMASK(i);
}

void
hdvector_sum_to_vector(HDVectorSum *sum, HDVector *vector, size_t threshold)
{
    for (size_t i = 0; i < DIMENSIONS; i++)
        vector->data[DIM2WORD(i)] |= (sum->bitsum[i] > threshold) << DIM2BITNUM(i);
}

#endif //  HDVECTOR_IMPLEMENTATION
