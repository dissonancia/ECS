#include "JackCompiler.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <file_path_or_directory>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *path = argv[1];

    int rc = jack_compiler_run(path);
    if (rc != 0) {
        fprintf(stderr, "Error processing: %s\n", path);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
