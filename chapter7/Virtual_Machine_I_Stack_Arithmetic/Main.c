#include "Writer.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Use: %s <\\directory\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *dir_path = argv[1];

    char output_path[MAX_PATH];
    snprintf(output_path, sizeof(output_path), "%s\\output.asm", dir_path);

    Code_Writer code_writer;
    code_writer_init(&code_writer, output_path);

    Dir_Paths paths = all_dir_paths(dir_path);
    if (paths.words.items == NULL) {
        fprintf(stderr, "parser: failed to read directory paths: %s\n", dir_path);
        return EXIT_FAILURE;
    }

    da_foreach(String_View, sv, &paths.words) {
        if (!sv_end_with(*sv, ".vm")) continue;
        
        char path[MAX_PATH];
        if (!sv_to_cstr(*sv, path, sizeof(path))) return EXIT_FAILURE;

        Parser parser;
        if (!parser_init(&parser, path)) return EXIT_FAILURE;

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
            
            default:
                break;
            }
        }

        parser_free(&parser);
    }

    code_writer_close(&code_writer);
    free_dir_paths(&paths);

    return EXIT_SUCCESS;
}
