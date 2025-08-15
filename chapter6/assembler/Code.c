#include <string.h>
#include "Code.h"

int code_dest(const char *mnemonic) {
    if (strcmp(mnemonic, "NULL") == 0) return 0b000;
    if (strcmp(mnemonic, "M") == 0)    return 0b001;
    if (strcmp(mnemonic, "D") == 0)    return 0b010;
    if (strcmp(mnemonic, "MD") == 0)   return 0b011;
    if (strcmp(mnemonic, "A") == 0)    return 0b100;
    if (strcmp(mnemonic, "AM") == 0)   return 0b101;
    if (strcmp(mnemonic, "AD") == 0)   return 0b110;
    if (strcmp(mnemonic, "AMD") == 0)  return 0b111;

    return -1;
}

int code_comp(const char *mnemonic) {
    if (strcmp(mnemonic, "0") == 0)   return 0b0101010;
    if (strcmp(mnemonic, "1") == 0)   return 0b0111111;
    if (strcmp(mnemonic, "-1") == 0)  return 0b0111010;
    if (strcmp(mnemonic, "D") == 0)   return 0b0001100;
    if (strcmp(mnemonic, "A") == 0)   return 0b0110000;
    if (strcmp(mnemonic, "M") == 0)   return 0b1110000;
    if (strcmp(mnemonic, "!D") == 0)  return 0b0001101;
    if (strcmp(mnemonic, "!A") == 0)  return 0b0110001;
    if (strcmp(mnemonic, "!M") == 0)  return 0b1110001;
    if (strcmp(mnemonic, "-D") == 0)  return 0b0001111;
    if (strcmp(mnemonic, "-A") == 0)  return 0b0110011;
    if (strcmp(mnemonic, "-M") == 0)  return 0b1110011;
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
    if (strcmp(mnemonic, "NULL") == 0) return 0b000;
    if (strcmp(mnemonic, "JGT") == 0)  return 0b001;
    if (strcmp(mnemonic, "JEQ") == 0)  return 0b010;
    if (strcmp(mnemonic, "JGE") == 0)  return 0b011;
    if (strcmp(mnemonic, "JLT") == 0)  return 0b100;
    if (strcmp(mnemonic, "JNE") == 0)  return 0b101;
    if (strcmp(mnemonic, "JLE") == 0)  return 0b110;
    if (strcmp(mnemonic, "JMP") == 0)  return 0b111;

    return -1;
}