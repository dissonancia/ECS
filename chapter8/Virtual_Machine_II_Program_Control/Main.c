#include "Writer.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Use: %s <\\directory\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *dir_path = argv[1];

    char output_path[MAX_PATH];
    if (snprintf(output_path, sizeof(output_path), "%s\\output.asm", dir_path) >= sizeof(output_path)) {
        fprintf(stderr, "Output path too long\n");
        return EXIT_FAILURE;
    }

    Dir_Paths paths = all_dir_paths(dir_path);
    if (paths.words.items == NULL) {
        fprintf(stderr, "parser: failed to read directory paths: %s\n", dir_path);
        return EXIT_FAILURE;
    }

    bool with_bootstrap = false;
    da_foreach(String_View, sv, &paths.words) {
        if (sv_end_with(*sv, "Sys.vm"))
            with_bootstrap = true;
    }

    Code_Writer code_writer;
    if(!code_writer_init(&code_writer, output_path, with_bootstrap)) {
        fprintf(stderr, "Erro %d: %s\n", errno, strerror(errno));
        code_writer_close(&code_writer);
        return EXIT_FAILURE;
    }

    int status = EXIT_SUCCESS;
    Parser parser;
    bool parser_active = false;

    da_foreach(String_View, sv, &paths.words) {
        if (!sv_end_with(*sv, ".vm")) continue;
        
        char path[MAX_PATH];
        if (!sv_to_cstr(*sv, path, sizeof(path))) {
            perror("sv_to_cstr");
            status = EXIT_FAILURE;
            goto cleanup;
        }

        set_file_name(&code_writer, path);

        if (!parser_init(&parser, path)) {
            fprintf(stderr, "parser_init failed for %s: %s\n", path, strerror(errno));
            status = EXIT_FAILURE;
            goto cleanup;
        }

        parser_active = true;

        while (has_more_commands(&parser)) {
            advance(&parser);
            Command_Type type = command_type(&parser);

            switch (type)
            {
            case C_ARITHMETIC:
                write_arithmetic(&code_writer, arg1(&parser));
                break;
            
            case C_PUSH:
            case C_POP:
                write_push_pop(&code_writer, type, arg1(&parser), arg2(&parser));
                break;

            case C_LABEL:
                write_label(&code_writer, arg1(&parser));
                break;

            case C_GOTO:
                write_goto(&code_writer, arg1(&parser));
                break;

            case C_IF:
                write_if(&code_writer, arg1(&parser));
                break;

            case C_CALL:
                write_call(&code_writer, arg1(&parser), arg2(&parser));
                break;

            case C_FUNCTION:
                write_function(&code_writer, arg1(&parser), arg2(&parser));
                break;

            case C_RETURN:
                write_return(&code_writer);
                break;
            
            default:
                break;
            }
        }

        parser_free(&parser);
        parser_active = false;
    }

cleanup:
    if (parser_active) parser_free(&parser);
    code_writer_close(&code_writer);
    free_dir_paths(&paths);

    return status;
}