#include <stdio.h>
#include <stdlib.h>

#include "Code.h"
#include "Parser.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <arquivo.asm>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *path = argv[1];
    HashTable *t = create_table();
    if (!t) {
        perror("Error creating Symbol Table.");
        return EXIT_FAILURE;
    }

    if (build_symtable(t, path) < 0) {
        fprintf(stderr, "Error building the symbol table during the first pass.");
        free_table(t);
        return EXIT_FAILURE;
    }

    if (create_mnemonic_tables() < 0) {
        perror("Error creating Mnemonic Tables.");
        return EXIT_FAILURE;
    }

    if (assembler(t, path) < 0) {
        fprintf(stderr, "Failed to translate ASM code to binary.");
        free_mnemonic_tables();
        free_table(t);
        return EXIT_FAILURE;
    }

    printf("Assembling finished successfully!\n");
    free_mnemonic_tables();
    free_table(t);

    return EXIT_SUCCESS;
}