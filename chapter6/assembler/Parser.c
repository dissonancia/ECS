#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "Parser.h"
#include "Code.h"

typedef enum {
    A_COMMAND,
    C_COMMAND,
    L_COMMAND
} CommandType;

static CommandType command_type(char *line) {
    switch (line[0]) {
        case '@':
            return A_COMMAND;
        case '(':
            return L_COMMAND;
        default:
            return C_COMMAND;
    }
}

static void remove_whitespace(char *str) {
    char *src = str;
    char *dst = str;
    while(*src != '\0') {
        if (!isspace((unsigned char)*src))
            *dst++ = *src;
        src++;
    }
    *dst = '\0';
}

static void remove_comment(char *str) {
    char *p = strstr(str, "//");
    if (p) *p = '\0';
}

static char* symbol(const char *command) {
    const char *start = command;
    start++;
    
    if (isdigit((unsigned char)*start)) {
        return (char *)start;
    }

    const char *end = command + strlen(command);
    if (*(end-1) == ')') end--;

    size_t len = end - start;
    char *sy = malloc(len + 1);
    if (!sy) return NULL;

    strncpy(sy, start, len);
    sy[len] = '\0';
    return sy;
}

static char* parser_dest(const char *command) {
    int a = 0, d = 0, m = 0;
    const char *p = command;

    while (*p != '\0' && *p != '=') {
        if (*p == 'A') a = 1;
        else if (*p == 'D') d = 1;
        else if (*p == 'M') m = 1;
        p++;
    }

    const char *dests[8] = {
        "Error", "M", "D", "MD", "A", "AM", "AD", "AMD"
    };
    const char *chosen = (*p == '\0') ? "NULL" : dests[(a << 2) | (d << 1) | m];

    char *result = malloc(strlen(chosen) + 1);
    if (!result) return NULL;
    strcpy(result, chosen);
    return result;
}

static char* parser_comp(const char *command) {
    static const char *comps[] = {
        "0", "1", "-1", "D", "A", "!D", "!A",
        "-D", "-A", "D+1", "A+1", "D-1", "A-1",
        "D+A", "D-A", "A-D", "D&A", "D|A", "M",
        "!M", "-M", "M+1", "M-1", "D+M", "D-M",
        "M-D", "D&M", "D|M"
    };
    const size_t num_comps = sizeof(comps) / sizeof(comps[0]);

    const char *start = strchr(command, '=');
    if (start) start++;
    else start = command;

    const char *end = strchr(start, ';');
    size_t len = end ? (size_t)(end - start) : strlen(start);

    char *cm = malloc(len + 1);
    if (!cm) return NULL;
    strncpy(cm, start, len);
    cm[len] = '\0';

    for (size_t i = 0; i < num_comps; i++) {
        if (strcmp(cm, comps[i]) == 0) {
            return cm;
        }
    }

    free(cm);
    cm = malloc(strlen("Error") + 1);
    if (!cm) return NULL;
    strcpy(cm, "Error");
    return cm;
}

static char* parser_jump(const char *command) {
    static const char *jumps[] = {
        "JGT", "JEQ", "JGE", "JLT", "JNE", "JLE", "JMP"
    };
    const size_t num_jumps = sizeof(jumps) / sizeof(jumps[0]);

    const char *p = strchr(command, ';');
    if (!p) {
        char *null_str = malloc(strlen("NULL") + 1);
        if (!null_str) return NULL;
        strcpy(null_str, "NULL");
        return null_str;
    }
    p++;

    for (size_t i = 0; i < num_jumps; i++) {
        if (strcmp(p, jumps[i]) == 0) {
            char *jmp = malloc(strlen(jumps[i]) + 1);
            if (!jmp) return NULL;
            strcpy(jmp, jumps[i]);
            return jmp;
        }
    }

    char *err = malloc(strlen("Error") + 1);
    if (!err) return NULL;
    strcpy(err, "Error");
    return err;
}

static char* output_name(const char *input_filename) {
    size_t len = strlen(input_filename);
    if (len < 4 || strcmp(input_filename + len - 4, ".asm") != 0)
        return NULL;
    
    char *output_filename = malloc(len + 1);
    if (!output_filename) return NULL;

    strncpy(output_filename, input_filename, len - 4);
    output_filename[len - 4] = '\0';

    strcat(output_filename, ".hack");

    return output_filename;
}

static void int_to_binary16_string(int value, char *buffer) {
    for (int i = 15; i >= 0; i--) {
        buffer[15 - i] = ((value >> i) & 1) ? '1' : '0';
    }
    buffer[16] = '\0';
}

int assembler(HashTable *t, const char *path) {
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        perror("Error opening file (input)");
        return -1;
    }

    char *out_filename = output_name(path);
    if (!out_filename) { perror("Invalid filename."); return -1; }

    FILE *out = fopen(out_filename, "w");
    if (!out) { perror("Error opening file (output)"); return -1; }
    free(out_filename);

    char line[256];
    while(fgets(line, sizeof(line), f) != NULL) {
        remove_whitespace(line);
        remove_comment(line);
        if (line[0] == '\0') continue;
        CommandType ct = command_type(line);
        int instruction; // binary
        char binary_str[17]; // 16 bits + '\0'
        if (ct == A_COMMAND) {
            int addr;
            char *sym = symbol(line);
            // if symbol is not a number
            if (!isdigit((unsigned char)sym[0])) {
                // if symbol is in table
                if (contains(t, sym)) {
                    addr = get_address(t, sym);
                    instruction = addr & 0x7FFF; // mask: 0x7FFF = 0111 1111 1111 1111
                } else {
                    // if symbol is not in table
                    add_entry(t, sym, t->ram);
                    instruction = t->ram & 0x7FFF;
                    t->ram++;   
                }
                free(sym);
            } else {
                addr = atoi(sym);
                instruction = addr & 0x7FFF;
            }

            int_to_binary16_string(instruction, binary_str);
            fprintf(out, "%s\n", binary_str);

        } else if (ct == C_COMMAND) {
            char *mnemonic_comp = parser_comp(line);
            char *mnemonic_dest = parser_dest(line);
            char *mnemonic_jump = parser_jump(line);
            if (!mnemonic_comp || !mnemonic_dest || !mnemonic_jump) { 
                perror("Error allocating (mnemonic_comp, mnemonic_dest, mnemonic_jump)");
                return -1;
            }

            if (strcmp(mnemonic_comp, "Error") == 0) {
                fprintf(stderr, "Error in c-instruction formation (comp).");
                return -1;
            }
            if (strcmp(mnemonic_dest, "Error") == 0) {
                fprintf(stderr, "Error in c-instruction formation (dest).");
                return -1;
            }
            if (strcmp(mnemonic_jump, "Error") == 0) {
                fprintf(stderr, "Error in c-instruction formation (jump).");
                return -1;
            }

            int comp_bits, dest_bits, jump_bits;

            comp_bits = code_comp(mnemonic_comp);
            if (comp_bits < 0) {
                fprintf(stderr, "Error in Translating Hack assembly language mnemonics into binary codes (comp).");
                return -1;
            }
            dest_bits = code_dest(mnemonic_dest);
            if (dest_bits < 0) {
                fprintf(stderr, "Error in Translating Hack assembly language mnemonics into binary codes (dest).");
                return -1;
            }
            jump_bits = code_jump(mnemonic_jump);
            if (jump_bits < 0) {
                fprintf(stderr, "Error in Translating Hack assembly language mnemonics into binary codes (jump).");
                return -1;
            }

            instruction = (0b111 << 13) | (comp_bits << 6) | (dest_bits << 3) | jump_bits;

            int_to_binary16_string(instruction, binary_str);
            fprintf(out, "%s\n", binary_str);
            
            free(mnemonic_comp);
            free(mnemonic_dest);
            free(mnemonic_jump);
        }
    }

    fclose(f);
    fclose(out);
    return 0;
}

int build_symtable(HashTable *t, const char *path) {
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        perror("Error opening file (input)");
        return -1;
    }

    char line[256];
    while(fgets(line, sizeof(line), f) != NULL) {
        remove_whitespace(line);
        remove_comment(line);
        if (line[0] == '\0') continue;
        CommandType ct = command_type(line);
        if (ct == A_COMMAND || ct == C_COMMAND) t->rom++;
        if (ct == L_COMMAND) add_entry(t, symbol(line), t->rom);
    }

    fclose(f);
    return 0;
}