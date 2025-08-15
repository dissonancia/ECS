#include <string.h>
#include <stdio.h>

#include "Code.h"
#include "SymbolTable.h"

<<<<<<< HEAD
static HashTable *dest_table;
static HashTable *comp_table;
static HashTable *jump_table;
=======
int code_dest(const char *mnemonic) {
    if (strcmp(mnemonic, "NULL") == 0) return 0b000;
    if (strcmp(mnemonic,    "M") == 0) return 0b001;
    if (strcmp(mnemonic,    "D") == 0) return 0b010;
    if (strcmp(mnemonic,   "MD") == 0) return 0b011;
    if (strcmp(mnemonic,    "A") == 0) return 0b100;
    if (strcmp(mnemonic,   "AM") == 0) return 0b101;
    if (strcmp(mnemonic,   "AD") == 0) return 0b110;
    if (strcmp(mnemonic,  "AMD") == 0) return 0b111;
>>>>>>> 9823ece967e126f732d1b074828a34fa84abfe6e

int create_mnemonic_tables() {
    // dest
    dest_table = create_table();
    if (!dest_table) {
        perror("Error creating Dest Mnemonic Table.");
        return -1;
    }

    add_entry(dest_table, "NULL", 0b000);
    add_entry(dest_table,    "M", 0b001);
    add_entry(dest_table,    "D", 0b010);
    add_entry(dest_table,   "MD", 0b011);
    add_entry(dest_table,    "A", 0b100);
    add_entry(dest_table,   "AM", 0b101);
    add_entry(dest_table,   "AD", 0b110);
    add_entry(dest_table,  "AMD", 0b111);

    // comp
    comp_table = create_table();
    if (!comp_table) {
        perror("Error creating Comp Mnemonic Table.");
        return -1;
    }

    add_entry(comp_table,   "0", 0b0101010);
    add_entry(comp_table,   "1", 0b0111111);
    add_entry(comp_table,  "-1", 0b0111010);
    add_entry(comp_table,   "D", 0b0001100);
    add_entry(comp_table,   "A", 0b0110000);
    add_entry(comp_table,   "M", 0b1110000);
    add_entry(comp_table,  "!D", 0b0001101);
    add_entry(comp_table,  "!A", 0b0110001);
    add_entry(comp_table,  "!M", 0b1110001);
    add_entry(comp_table,  "-D", 0b0001111);
    add_entry(comp_table,  "-A", 0b0110011);
    add_entry(comp_table,  "-M", 0b1110011);
    add_entry(comp_table, "D+1", 0b0011111);
    add_entry(comp_table, "A+1", 0b0110111);
    add_entry(comp_table, "M+1", 0b1110111);
    add_entry(comp_table, "D-1", 0b0001110);
    add_entry(comp_table, "A-1", 0b0110010);
    add_entry(comp_table, "M-1", 0b1110010);
    add_entry(comp_table, "D+A", 0b0000010);
    add_entry(comp_table, "D+M", 0b1000010);
    add_entry(comp_table, "D-A", 0b0010011);
    add_entry(comp_table, "D-M", 0b1010011);
    add_entry(comp_table, "A-D", 0b0000111);
    add_entry(comp_table, "M-D", 0b1000111);
    add_entry(comp_table, "D&A", 0b0000000);
    add_entry(comp_table, "D&M", 0b1000000);
    add_entry(comp_table, "D|A", 0b0010101);
    add_entry(comp_table, "D|M", 0b1010101);

    // jump
    jump_table = create_table();
    if (!jump_table) {
        perror("Error creating Jump Mnemonic Table.");
        return -1;
    }

    add_entry(jump_table, "NULL", 0b000);
    add_entry(jump_table,  "JGT", 0b001);
    add_entry(jump_table,  "JEQ", 0b010);
    add_entry(jump_table,  "JGE", 0b011);
    add_entry(jump_table,  "JLT", 0b100);
    add_entry(jump_table,  "JNE", 0b101);
    add_entry(jump_table,  "JLE", 0b110);
    add_entry(jump_table,  "JMP", 0b111);

    return 0;
}

<<<<<<< HEAD
void free_mnemonic_tables() {
    free_table(dest_table);
    free_table(comp_table);
    free_table(jump_table);
}

int code_dest(unsigned char *mnemonic) { return get_address(dest_table, mnemonic); }
int code_comp(unsigned char *mnemonic) { return get_address(comp_table, mnemonic); }
int code_jump(unsigned char *mnemonic) { return get_address(jump_table, mnemonic); }
=======
int code_comp(const char *mnemonic) {
    if (strcmp(mnemonic,   "0") == 0) return 0b0101010;
    if (strcmp(mnemonic,   "1") == 0) return 0b0111111;
    if (strcmp(mnemonic,  "-1") == 0) return 0b0111010;
    if (strcmp(mnemonic,   "D") == 0) return 0b0001100;
    if (strcmp(mnemonic,   "A") == 0) return 0b0110000;
    if (strcmp(mnemonic,   "M") == 0) return 0b1110000;
    if (strcmp(mnemonic,  "!D") == 0) return 0b0001101;
    if (strcmp(mnemonic,  "!A") == 0) return 0b0110001;
    if (strcmp(mnemonic,  "!M") == 0) return 0b1110001;
    if (strcmp(mnemonic,  "-D") == 0) return 0b0001111;
    if (strcmp(mnemonic,  "-A") == 0) return 0b0110011;
    if (strcmp(mnemonic,  "-M") == 0) return 0b1110011;
    if (strcmp(mnemonic, "D+1") == 0) return 0b0011111;
    if (strcmp(mnemonic, "A+1") == 0) return 0b0110111;
    if (strcmp(mnemonic, "M+1") == 0) return 0b1110111;
    if (strcmp(mnemonic, "D-1") == 0) return 0b0001110;
    if (strcmp(mnemonic, "A-1") == 0) return 0b0110010;
    if (strcmp(mnemonic, "M-1") == 0) return 0b1110010;
    if (strcmp(mnemonic, "D+A") == 0) return 0b0000010;
    if (strcmp(mnemonic, "D+M") == 0) return 0b1000010;
    if (strcmp(mnemonic, "D-A") == 0) return 0b0010011;
    if (strcmp(mnemonic, "D-M") == 0) return 0b1010011;
    if (strcmp(mnemonic, "A-D") == 0) return 0b0000111;
    if (strcmp(mnemonic, "M-D") == 0) return 0b1000111;
    if (strcmp(mnemonic, "D&A") == 0) return 0b0000000;
    if (strcmp(mnemonic, "D&M") == 0) return 0b1000000;
    if (strcmp(mnemonic, "D|A") == 0) return 0b0010101;
    if (strcmp(mnemonic, "D|M") == 0) return 0b1010101;

    return -1;
}

int code_jump(const char *mnemonic) {
    if (strcmp(mnemonic, "NULL") == 0)  return 0b000;
    if (strcmp(mnemonic,  "JGT") == 0)  return 0b001;
    if (strcmp(mnemonic,  "JEQ") == 0)  return 0b010;
    if (strcmp(mnemonic,  "JGE") == 0)  return 0b011;
    if (strcmp(mnemonic,  "JLT") == 0)  return 0b100;
    if (strcmp(mnemonic,  "JNE") == 0)  return 0b101;
    if (strcmp(mnemonic,  "JLE") == 0)  return 0b110;
    if (strcmp(mnemonic,  "JMP") == 0)  return 0b111;

    return -1;
}
>>>>>>> 9823ece967e126f732d1b074828a34fa84abfe6e
