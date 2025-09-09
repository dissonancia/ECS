#ifndef PARSER_H_
#define PARSER_H_

#include "SymbolTable.h"

int assembler(HashTable *t, const char *path);
int build_symtable(HashTable *t, const char *path);

#endif // PARSER_H_