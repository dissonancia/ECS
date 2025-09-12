#include "JackCompiler.h"

static int process_jack_file(const char *jack_path) {
    JackTokenizer jt = {0};
    Tokens tokens = {0};

    if (!tokenizer_init(&jt, jack_path)) {
        fprintf(stderr, "tokenizer_init failed for %s: %s\n", jack_path, strerror(errno));
        return -1;
    }
    tokenizer_scan_all(&jt, &tokens);
    
    size_t len = strlen(jack_path);
    if (len < 5 || strcmp(jack_path + len - 5, ".jack") != 0) {
        fprintf(stderr, "Invalid .jack file: %s\n", jack_path);
        tokenizer_free(&jt); tokens_free(&tokens);
        return -1;
    }

    char output_path[MAX_PATH];
    if (snprintf(output_path, sizeof(output_path), "%.*s.vm",
                 (int)(len - 5), jack_path) >= (int)sizeof(output_path)) {
        fprintf(stderr, "Output path too long for %s\n", jack_path);
        tokenizer_free(&jt); tokens_free(&tokens);
        return -1;
    }

    SymbolTable *symtab = st_create();
    if (!symtab) { perror("st_create"); tokenizer_free(&jt); tokens_free(&tokens); return -1; }

    CompilationEngine ce = {0};
    if (!compilation_engine_init(&ce, tokens, output_path)) {
        fprintf(stderr, "compilation_engine_init failed for %s -> %s\n", jack_path, output_path);
        st_free(symtab); tokenizer_free(&jt); tokens_free(&tokens);
        return -1;
    }

    ce.symtab = symtab;

    compile_class(&ce);
    bool had_error = ce.had_error;

    compilation_engine_free(&ce);
    st_free(symtab);
    tokenizer_free(&jt);
    tokens_free(&tokens);

    if (had_error) {
        fprintf(stderr, "Syntax error while compiling %s\n", jack_path);
        return -1;
    }
    return 0;
}

// process single file or directory
int jack_compiler_run(const char *source_path) {
    if (!source_path) {
        fprintf(stderr, "jack_analyzer_run: null path\n");
        return -1;
    }

    String_View sv_path = sv_from_cstr(source_path);

    // single file
    if (sv_end_with(sv_path, ".jack")) {
        return process_jack_file(source_path);
    }

    // directory: iterate files
    Dir_Paths dp = all_dir_paths(source_path);
    if (dp.words.items == NULL) {
        fprintf(stderr, "Failed to read directory: %s\n", source_path);
        return -1;
    }

    bool any = false;
    int exit_status = 0;

    da_foreach(String_View, sv, &dp.words) {
        if (!sv_end_with(*sv, ".jack")) continue;

        char jack_path[MAX_PATH];
        if (!sv_to_cstr(*sv, jack_path, sizeof(jack_path))) {
            perror("sv_to_cstr");
            exit_status = -1;
            continue;
        }

        any = true;
        if (process_jack_file(jack_path) != 0) {
            fprintf(stderr, "Failed processing: %s\n", jack_path);
            exit_status = -1;
        }
    }

    if (!any) {
        fprintf(stderr, "No .jack files found in directory: %s\n", source_path);
        exit_status = -1;
    }

    free_dir_paths(&dp);
    return exit_status;
}