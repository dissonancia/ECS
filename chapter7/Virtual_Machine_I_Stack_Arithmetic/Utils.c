#include "Utils.h"

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

UDEF void sv_print(String_View sv) {
    printf(SV_Fmt, SV_Arg(sv));
}

UDEF void sv_println(String_View sv) {
    printf(SV_Fmt "\n", SV_Arg(sv));
}

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

UDEF void init_stack(Stack *s) {
    s->items = NULL;
    s->sp = 0;
    s->capacity = 0;
}

UDEF Stack *create_stack() {
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
    size_t i;
    bool found = false;

    for (i = 0; i + 1 < sv.count; ++i) {
        if (sv.data[i] == '/' && sv.data[i + 1] == '/') {
            found = true;
            break;
        }
    }
    if (found) sv.count = i;
    return sv_trim(sv);
}

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

    return sb;
}

UDEF bool sv_to_cstr(String_View sv, char *buf, size_t buf_size) {
    if (sv.count + 1 > buf_size) {
        fprintf(stderr, "sv_to_cstr: path too long (%zu chars): %.*s\n",
                sv.count, (int)sv.count, sv.data);
        return false;
    }
    memcpy(buf, sv.data, sv.count);
    buf[sv.count] = '\0';
    return true;
}

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