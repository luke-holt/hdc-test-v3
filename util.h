#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>


#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"


#define BETWEEN(x, lower, upper) (((x) >= (lower)) && ((x) <= (upper)))

#define STREQ(a, b) (0 == strcmp((a), (b)))

#define ARRLEN(arr) (sizeof(arr)/sizeof(*arr))

#define UTIL_ASSERT(cond) if (!(cond)) { util_log("ASSERT", "%s:%s:%d assertion failed", __FILE__, __func__, __LINE__); abort(); }
#define TODO(msg) util_log("TODO", "%s:%s:%d %s", __FILE__, __func__, __LINE__, msg)

#define da_at(da, i) ((da)->items[(i)])
#define da_reset(da) ((da)->count = 0)
#define da_tail_idx(da) ((da)->count-1)
#define da_tail_item(da) ((da)->items[da_tail_idx(da)])
#define da_pop(da) ((da)->count--)
#define da_init(da) \
    do { \
        (da)->count = 0; \
        (da)->capacity = 256; \
        (da)->items = (typeof((da)->items))malloc((da)->capacity * sizeof(*(da)->items)); \
        UTIL_ASSERT((da)->items); \
    } while (0)
#define da_delete(da) \
    do { \
        free((da)->items); \
        (da)->count = 0; \
        (da)->capacity = 0; \
        (da)->items = NULL; \
    } while (0)
#define da_append(da, item) \
    do { \
        if ((da)->count >= (da)->capacity) \
            da_resize(da, 2 * (da)->capacity); \
        (da)->items[(da)->count++] = (item); \
    } while (0)
#define da_resize(da, new_cap) \
    do { \
        (da)->items = (typeof((da)->items))realloc((da)->items, new_cap * sizeof(*(da)->items)); \
        UTIL_ASSERT((da)->items); \
        (da)->capacity = new_cap; \
    } while (0)

#define UTIL_XORSHIFT128_RAND_MAX ((uint32_t)0xFFFFFFFFu)
uint32_t util_xorshift128(uint32_t *state);

uint32_t util_bitcount(uint32_t word);

void util_log(const char *tag, const char *fmt, ...);
void util_hexdump(const char *data, size_t size);
bool util_file_exists(const char *filename);

typedef struct { size_t count, capacity; char *items; } String;
bool string_load_from_file(const char *filename, String *str);
void string_delete(String *str);
void string_alloc(String *str, size_t size);

#endif // UTIL_H


#ifdef UTIL_IMPLEMENTATION

#include <stdarg.h>
#include <unistd.h>
#include <string.h>

bool util_file_exists(const char *filename) {
    return access(filename, F_OK) == 0;
}

void util_log(const char *tag, const char *fmt, ...) {
    fprintf(stderr, "[%s] ", tag);
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
}

uint32_t util_xorshift128(uint32_t *state) {
    uint32_t t, s;
    t = state[3];
    s = state[0];
    state[3] = state[2];
    state[2] = state[1];
    state[1] = s;
    t ^= t << 11;
    t ^= t >> 8;
    state[0] = t ^ s ^ (s >> 19);
    return state[0];
}

uint32_t util_bitcount(uint32_t word) {
    word = word - ((word >> 1) & 0x55555555);
    word = (word & 0x33333333) + ((word >> 2) & 0x33333333);
    word = ((word + (word >> 4) & 0x0F0F0F0F) * 0x01010101) >> 24;
    return word;
}

void string_alloc(String *str, size_t size) {
    UTIL_ASSERT(str);
    str->count = 0;
    str->capacity = size;
    str->items = malloc(size);
    UTIL_ASSERT(str->items);
}

bool string_load_from_file(const char *filename, String *str) {
    UTIL_ASSERT(filename);
    UTIL_ASSERT(str);

    FILE *file;
    int rc;
    bool success = false;

    file = fopen(filename, "r");
    if (!file) return false;

    rc = fseek(file, 0, SEEK_END);
    if (rc) goto defer;

    str->capacity = ftell(file);

    rc = fseek(file, 0, SEEK_SET);
    if (rc) goto defer;

    string_alloc(str, str->capacity);

    rc = fread(str->items, 1, str->capacity, file);
    if (rc != str->capacity) {
        string_delete(str);
        goto defer;
    }

    str->count = str->capacity;
    success = true;

defer:
    (void)fclose(file);
    return success;
}

void string_delete(String *str) {
    UTIL_ASSERT(str);
    UTIL_ASSERT(str->items);
    da_delete(str);
}

void
util_hexdump(const char *data, size_t size)
{
    /* format:
     * XXXXXXXX XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX |XXXXXXXXXXXXXXXX|
     * ^offset  ^bytes                                          ^chars
     * 0        9                                               57
     *
     * total: 76, including null termination
     */
    char eol[16+1];
    for (size_t i = 0; i < size; i++) {
        if (i%16 == 0) {
            // reset buffers
            memset(eol, '\0', sizeof(eol));
            printf("%08zX ", i);
        }

        printf("%02X ", (unsigned char)data[i]);

        eol[i%16] = BETWEEN(data[i], 32, 126) ? data[i] : '.';

        if (i%16 == 15 || (i == size-1)) {

            while (i%16 != 15) {
                printf("   ");
                i++;
                eol[i%16] = '.';
            }

            printf("|%s|\n", eol);
        }
    }
}

#endif
