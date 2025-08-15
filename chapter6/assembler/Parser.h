#ifndef PARSER_H_
#define PARSER_H_

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "SymbolTable.h"
#include "Code.h"

typedef enum {
    A_COMMAND,
    C_COMMAND,
    L_COMMAND
} CommandType;

int assembler(HashTable *t, const char *path);
int build_symtable(HashTable *t, const char *path);

#endif // PARSER_H_