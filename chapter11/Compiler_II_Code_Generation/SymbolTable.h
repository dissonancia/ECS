#ifndef SYMBOLTABLE_H_
#define SYMBOLTABLE_H_

typedef enum {
    STATIC = 0,
    FIELD  = 1,
    ARG    = 2,
    VAR    = 3,
    NONE   = 4
} Kind;

// STRUCTS
typedef struct SymbolEntry SymbolEntry;
typedef struct SymbolTable SymbolTable;

// create/free Symbol Table
SymbolTable *st_create(void);
void st_free(SymbolTable *st);

// starts a new subroutine scope
// i.e., resets the subroutine's symbol table
void st_start_subroutine(SymbolTable *st);

// defines a new identifier of a given name, type, and kind and assigns
// it a running index. STATIC and FIELD identifiers have a class scope,
// while ARG and VAR identifiers have a subroutine scope;
void st_define(SymbolTable *st, const char *name, const char *type, Kind kind);

// returns the number of variables of the given kind already defined in
// the current scope
int st_var_count(SymbolTable *st, Kind kind);

// Lookup: first looks in subroutine scope, then in class scope.

// returns the kind of the named identifier in the current scope.
// if the identifier is unknown in the current scope, returns NONE.
Kind st_kind_of(SymbolTable *st, const char *name);

// returns the type of the named identifier in the current scope.
const char *st_type_of(SymbolTable *st, const char *name);

// returns the index of the named identifier in the current scope.
int st_index_of(SymbolTable *st, const char *name);

#endif // SYMBOLTABLE_H_