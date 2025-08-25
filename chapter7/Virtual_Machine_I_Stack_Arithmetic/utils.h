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

UDEF bool read_entire_file(const char *path, String_Builder *sb) {
    bool result = true;

    FILE *f = fopen(path, "rb");
    size_t new_count = 0;
    long long m = 0;
    if (f == NULL)                 return_defer(false);
    if (fseek(f, 0, SEEK_END) < 0) return_defer(false);
#ifndef _WIN32
    m = ftell(f);
#else
    m = _ftelli64(f);
#endif
    if (m < 0)                     return_defer(false);
    if (fseek(f, 0, SEEK_SET) < 0) return_defer(false);

    new_count = sb->count + m;
    if (new_count > sb->capacity) {
        sb->items = U_DECLTYPE_CAST(sb->items)realloc(sb->items, new_count);
        assert(sb->items != NULL && "More RAM!");
        sb->capacity = new_count;
    }

    fread(sb->items + sb->count, m, 1, f);
    if (ferror(f)) return_defer(false);
    sb->count = new_count;

defer:
    if (!result) fprintf(stderr, "ERROR: Could not read file %s: %s\n", path, strerror(errno));
    if (f) fclose(f);
    return result;
}

#define SV_Fmt "%.*s"
#define SV_Arg(sv) (int)(sv).count, (sv).data

typedef struct {
    size_t count;
    const char *data;
} String_View;

UDEF void sv_print(String_View sv) {
    printf(SV_Fmt, SV_Arg(sv));
}

UDEF void sv_println(String_View sv) {
    printf(SV_Fmt "\n", SV_Arg(sv));
}

// sb_to_sv() enables you to just view String_Builder as String_View
#define sb_to_sv(sb) sv_from_parts((sb).items, (sb).count)

UDEF String_View sv_from_parts(const char *data, size_t count) {
    String_View sv;
    sv.count = count;
    sv.data = data;
    return sv;
}

UDEF String_View sv_chop_by_delim(String_View *sv, char delim) {
    size_t i = 0;
    while (i < sv->count && sv->data[i] != delim) i += 1;

    String_View result = sv_from_parts(sv->data, i);

    if (i < sv->count) {
        sv->count -= i + 1;
        sv->data  += i + 1;
    } else {
        sv->count -= i;
        sv->data  += i;
    }

    return result;
}

UDEF String_View sv_chop_left(String_View *sv, size_t n) {
    if (n > sv->count) n = sv->count;

    String_View result = sv_from_parts(sv->data, n);

    sv->data  += n;
    sv->count -= n;
    
    return result;
}

UDEF String_View sv_trim_left(String_View sv) {
    size_t i = 0;
    while (i < sv.count && isspace(sv.data[i])) i += 1;
    return sv_from_parts(sv.data + i, sv.count - i);
}

UDEF String_View sv_trim_right(String_View sv) {
    size_t i = 0;
    while (i < sv.count && isspace(sv.data[sv.count - 1 - i])) i += 1;
    return sv_from_parts(sv.data, sv.count - i);
}

UDEF String_View sv_trim(String_View sv) {
    return sv_trim_right(sv_trim_left(sv));
}

UDEF String_View sv_from_cstr(const char *cstr) {
    return sv_from_parts(cstr, strlen(cstr));
}

UDEF bool sv_eq(String_View a, String_View b) {
    if (a.count != b.count) {
        return false;
    } else {
        return memcmp(a.data, b.data, a.count) == 0;
    }
}

UDEF bool sv_end_with(String_View sv, const char *cstr) {
    size_t cstr_count = strlen(cstr);
    if (sv.count >= cstr_count) {
        size_t ending_start = sv.count - cstr_count;
        String_View sv_ending = sv_from_parts(sv.data + ending_start, cstr_count);
        return sv_eq(sv_ending, sv_from_cstr(cstr));
    }
    return false;
}

UDEF bool sv_starts_with(String_View sv, String_View expected_prefix) {
    if (expected_prefix.count <= sv.count) {
        String_View actual_prefix = sv_from_parts(sv.data, expected_prefix.count);
        return sv_eq(expected_prefix, actual_prefix);
    }
    return false;
}

// Initial capacity of a stack
#ifndef STACK_INIT_CAP
#define STACK_INIT_CAP 256
#endif

typedef struct {
    int *items;
    size_t sp;
    size_t capacity;
} Stack;

UDEF void init_stack(Stack *s) {
    s->items = NULL;
    s->sp = 0;
    s->capacity = 0;
}

Stack *create_stack() {
    Stack *s = malloc(sizeof(Stack));
    assert(s != NULL && "More RAM!");
    init_stack(s);
    return s;
}

UDEF void stack_reserve(Stack *s, size_t expected_capacity) {
    if (expected_capacity > s->capacity) {
        if (s->capacity == 0) {
            s->capacity = STACK_INIT_CAP;
        }
        while (expected_capacity > s->capacity) {
            s->capacity *= 2;
        }
        s->items = (int *)realloc(s->items, s->capacity * sizeof(*s->items));
        assert(s->items != NULL && "More RAM!");
    }
    return;
}

UDEF void stack_push(Stack *s, int item) {
    stack_reserve(s, s->sp + 1);
    s->items[s->sp++] = item;
}

UDEF bool stack_is_empty(Stack *s) {
    return s->sp == 0;
}

UDEF int stack_pop(Stack *s) {
    if (stack_is_empty(s)) return INT_MIN;
    return s->items[--(s->sp)];
}

UDEF int stack_top(Stack *s) {
    if (!stack_is_empty(s)) {
        return s->items[s->sp - 1];
    } else {
        return INT_MIN;
    }
}

UDEF void free_stack(Stack *s) {
    free(s->items);
    s->items = NULL;
    s->sp = 0;
    s->capacity = 0;
}

typedef struct {
    String_View *items;
    size_t count;
    size_t capacity;
} Words;

UDEF Words words(String_View sv) {
    Words result = {0};

    sv = sv_trim(sv);
    while (sv.count > 0) {
        // skip spaces
        sv = sv_trim_left(sv);

        // get to end of word
        size_t i = 0;
        while (i < sv.count && !isspace((unsigned char)sv.data[i])) {
            i++;
        }

        // create word
        String_View word = sv_from_parts(sv.data, i);
        da_append(&result, word);

        // advance the string (discard i characters plus subsequent spaces)
        sv = sv_from_parts(sv.data + i, sv.count - i);
    }

    return result;
}

UDEF int16_t sv_to_i16(String_View sv) {
    bool neg = false;
    int32_t result = 0;
    size_t i = 0;

    if (sv.count > 0 && sv.data[0] == '-') {
        neg = true;
        i++;
    }

    for (; i < sv.count; i++) {
        char c = sv.data[i];
        if (!isdigit(c)) {
            fprintf(stderr, "sv_to_i16: invalid character '%c'\n", c);
            exit(1);
        }
        result = result * 10 + (c - '0');
        if (result > 32768) {
            fprintf(stderr, "sv_to_i16: number outside the 16-bit range\n");
            exit(1);
        }
    }

    if (neg) result = -result;

    if (result < -32767 || result > 32767) {
        fprintf(stderr, "sv_to_i16: number out of range (-32768..32767)\n");
        exit(1);
    }

    return (int16_t) result;
}

UDEF String_View sv_strip_comment(String_View sv) {
    size_t i = 0;
    while (i + 1 < sv.count) {
        if (sv.data[i] == '/' && sv.data[i + 1] == '/') {
            break;
        }
        i++;
    }
    sv.count = i;
    sv = sv_trim(sv);
    return sv;
}

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


UDEF String_Builder dir_files_sb(const char *dirpath) {
    String_Builder sb = {0};

#ifdef _WIN32
    WIN32_FIND_DATA find_data;
    char search_path[MAX_PATH];
    snprintf(search_path, sizeof(search_path), "%s\\*", dirpath);

    HANDLE hfind = FindFirstFile(search_path, &find_data);
    if (hfind == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "ERROR: Could not open directory %s\n", dirpath);
        return sb;
    }

    do {
        if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            char file_path[MAX_PATH];
            snprintf(file_path, sizeof(file_path), "%s\\%s", dirpath, find_data.cFileName);
            sb_append_cstr(&sb, file_path);
            sb_append_cstr(&sb, U_LINE_END);
        }
    } while (FindNextFile(hfind, &find_data) != 0);

    FindClose(hfind);

#else
    DIR *dir = opendir(dirpath);
    if (!dir) {
        fprintf(stderr, "ERROR: Could not open directory %s: %s\n", dirpath, strerror(errno));
        return sb;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            char file_path[MAX_PATH];
            snprintf(file_path, sizeof(file_path), "%s/%s", dirpath, entry->d_name);
            sb_append_cstr(&sb, file_path);
            sb_append_cstr(&sb, U_LINE_END);
        }
    }

    closedir(dir);
#endif

    sb_append_null(&sb);
    return sb;
}

typedef struct {
    Words words;
    String_Builder backing;
} Dir_Paths;

UDEF Dir_Paths all_dir_paths(const char *dirpath) {
    Dir_Paths result;
    result.backing = dir_files_sb(dirpath);
    result.words   = words(sb_to_sv(result.backing));
    return result;
}

UDEF void free_dir_paths(Dir_Paths *dp) {
    sb_free(dp->backing);
    da_free(dp->words);
    dp->words = (Words){0};
    dp->backing = (String_Builder){0};
}

#endif // UTILS_H_