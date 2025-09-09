/* This is a modification and extension of: nob - v1.23.0 - Public Domain - https://github.com/tsoding/nob.h */

#ifndef UTILS_H_
#define UTILS_H_
#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS (1)
#endif

#ifndef UDEF
/*
   Goes before declarations and definitions of the functions. Useful to `#define UDEF static inline`
   if your source code is a single file and you want the compiler to remove unused functions.
*/
#define UDEF
#endif /* UDEF */

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <limits.h>
#include <time.h>

#ifdef _WIN32
#    define WIN32_LEAN_AND_MEAN
#    define _WINUSER_
#    define _WINGDI_
#    define _IMM_
#    define _WINCON_
#    include <windows.h>
#    include <direct.h>
#    include <shellapi.h>
#else
#    include <sys/types.h>
#    include <sys/wait.h>
#    include <sys/stat.h>
#    include <unistd.h>
#    include <fcntl.h>
#endif

#ifdef _WIN32
#    define U_LINE_END "\r\n"
#else
#    define U_LINE_END "\n"
#endif

#if defined(__GNUC__) || defined(__clang__)
//   https://gcc.gnu.org/onlinedocs/gcc-4.7.2/gcc/Function-Attributes.html
#    ifdef __MINGW_PRINTF_FORMAT
#        define PRINTF_FORMAT(STRING_INDEX, FIRST_TO_CHECK) __attribute__ ((format (__MINGW_PRINTF_FORMAT, STRING_INDEX, FIRST_TO_CHECK)))
#    else
#        define PRINTF_FORMAT(STRING_INDEX, FIRST_TO_CHECK) __attribute__ ((format (printf, STRING_INDEX, FIRST_TO_CHECK)))
#    endif // __MINGW_PRINTF_FORMAT
#else
//   TODO: implement PRINTF_FORMAT for MSVC
#    define PRINTF_FORMAT(STRING_INDEX, FIRST_TO_CHECK)
#endif

#define return_defer(value) do { result = (value); goto defer; } while(0)

// Initial capacity of a dynamic array
#ifndef DA_INIT_CAP
#define DA_INIT_CAP 256
#endif

#ifdef __cplusplus
#define U_DECLTYPE_CAST(T) (decltype(T))
#else
#define U_DECLTYPE_CAST(T)
#endif // __cplusplus

#define da_reserve(da, expected_capacity)                                                                          \
    do {                                                                                                           \
        if ((expected_capacity) > (da)->capacity) {                                                                \
            if((da)->capacity == 0) {                                                                              \
                (da)->capacity = DA_INIT_CAP;                                                                      \
            }                                                                                                      \
            while ((expected_capacity) > (da)->capacity) {                                                         \
                (da)->capacity *= 2;                                                                               \
            }                                                                                                      \
            (da)->items = U_DECLTYPE_CAST((da)->items)realloc((da)->items, (da)->capacity * sizeof(*(da)->items)); \
            assert((da)->items != NULL && "More RAM!");                                                            \
        }                                                                                                          \
    } while (0)                                                                                                    \

#define da_append(da, item)                  \
    do {                                     \
        da_reserve((da), (da)->count + 1);   \
        (da)->items[(da)->count++] = (item); \
    } while (0)                              \

#define da_free(da) free((da).items)

#define da_append_many(da, new_items, new_items_count)                                            \
    do {                                                                                          \
        da_reserve((da), (da)->count + (new_items_count));                                        \
        memcpy((da)->items + (da)->count, (new_items), (new_items_count) * sizeof(*(da)->items)); \
        (da)->count += (new_items_count);                                                         \
    } while (0)                                                                                   \

#define da_resize(da, new_size)       \
    do {                              \
        da_reserve((da), (new_size)); \
        (da)->count = (new_size);     \
    } while (0)                       \

#define da_drop_last(da) (da)->items[(assert((da)->count > 0), (da)->count - 1)]

#define da_remove_unordored(da, i)                   \
    do {                                             \
        size_t j = (i);                              \
        assert(j < (da)->count);                     \
        (da)->items[j] = (da)->items[--(da)->count]; \
    } while (0)                                      \

// Foreach over Dynamic Arrays. Example:
// ```c
// typedef struct {
//     int *items;
//     size_t count;
//     size_t capacity;
// } Numbers;
//
// Numbers xs = {0};
//
// da_append(&xs, 69);
// da_append(&xs, 420);
// da_append(&xs, 1337);
//
// da_foreach(int, x, &xs) {
//     // `x` here is a pointer to the current element. You can get its index by taking a difference
//     // between `x` and the start of the array which is `x.items`.
//     size_t index = x - xs.items;
//     log(INFO, "%zu: %d", index, *x);
// }
// ``
#define da_foreach(Type, it, da) for (Type *it = (da)->items; it < (da)->items + (da)->count; ++it)

typedef struct {
    char *items;
    size_t count;
    size_t capacity;
} String_Builder;

// Append a sized buffer to a string builder
#define sb_append_buf(sb, buf, size) da_append_many(sb, buf, size)

// Append a NULL-terminated string to a string builder
#define sb_append_cstr(sb, cstr)  \
    do {                          \
        const char *s = (cstr);   \
        size_t n = strlen(s);     \
        da_append_many(sb, s, n); \
    } while (0)                   \

// Append a single NULL character at the end of a string builder. So then you can
// use it a NULL-terminated C string
#define sb_append_null(sb) do { char nul = '\0'; da_append_many(sb, &nul, 1); } while(0)

// Free the memory allocated by a string builder
#define sb_free(sb) free((sb).items)

UDEF bool read_entire_file(const char *path, String_Builder *sb);

#define SV_Fmt "%.*s"
#define SV_Arg(sv) (int)(sv).count, (sv).data

typedef struct {
    size_t count;
    const char *data;
} String_View;

UDEF void sv_print(String_View sv);
UDEF void sv_println(String_View sv);

// sb_to_sv() enables you to just view String_Builder as String_View
#define sb_to_sv(sb) sv_from_parts((sb).items, (sb).count)

UDEF String_View sv_from_parts(const char *data, size_t count);
UDEF String_View sv_chop_by_delim(String_View *sv, char delim);
UDEF String_View sv_chop_left(String_View *sv, size_t n);
UDEF String_View sv_trim_left(String_View sv);
UDEF String_View sv_trim_right(String_View sv);
UDEF String_View sv_trim(String_View sv);
UDEF String_View sv_from_cstr(const char *cstr);
UDEF bool sv_eq(String_View a, String_View b);
UDEF bool sv_end_with(String_View sv, const char *cstr);
UDEF bool sv_starts_with(String_View sv, String_View expected_prefix);

// Initial capacity of a stack
#ifndef STACK_INIT_CAP
#define STACK_INIT_CAP 256
#endif

typedef struct {
    int *items;
    size_t sp;
    size_t capacity;
} Stack;

UDEF void init_stack(Stack *s);
UDEF Stack *create_stack();
UDEF void stack_reserve(Stack *s, size_t expected_capacity);
UDEF void stack_push(Stack *s, int item);
UDEF bool stack_is_empty(Stack *s);
UDEF int stack_pop(Stack *s);
UDEF int stack_top(Stack *s);
UDEF void free_stack(Stack *s);

typedef struct {
    String_View *items;
    size_t count;
    size_t capacity;
} Words;

UDEF Words words(String_View sv);
UDEF int16_t sv_to_i16(String_View sv);
UDEF String_View sv_strip_comment(String_View sv);

#ifndef MAX_PATH
#  ifdef _WIN32
#    define MAX_PATH 260
#  else
#    ifdef PATH_MAX
#      define MAX_PATH PATH_MAX
#    else
#      define MAX_PATH 4096
#    endif
#  endif
#endif


UDEF String_Builder dir_files_sb(const char *dirpath);
UDEF bool sv_to_cstr(String_View sv, char *buf, size_t buf_size);

typedef struct {
    Words words;
    String_Builder backing;
} Dir_Paths;

UDEF Dir_Paths all_dir_paths(const char *dirpath);
UDEF void free_dir_paths(Dir_Paths *dp);

#endif // UTILS_H_
