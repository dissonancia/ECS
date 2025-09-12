#include "SymbolTable.h"
#include "HashTable.h"
#include <stddef.h>

// Entry (name, type, kind, index) as an entry for the Symbol Table
struct SymbolEntry {
    char *name;    // identifier name
    char *type;    // type name (e.g. "int", "boolean", "MyClass")
    Kind kind;
    size_t index;  // running index
};

// Symbol Table struct
struct SymbolTable {
    HashTable *class_scope;      // STATIC, FIELD
    HashTable *subroutine_scope; // RG, VAR
    size_t counts[4];            // STATIC (0), FIELD(1), ARG(2), VAR(3)
};

// create SymbolTable
SymbolTable *st_create(void) {
    SymbolTable *st = calloc(1, sizeof(*st));
    st->class_scope = ht_create();
    st->subroutine_scope = ht_create();
    st->counts[STATIC] = 0;
    st->counts[FIELD]  = 0;
    st->counts[ARG]    = 0;
    st->counts[VAR]    = 0;
    return st;
}

// helper to free SymbolTable
static void symbol_entry_free(void *v) {
    if (!v) return;
    SymbolEntry *e = (SymbolEntry *)v;
    free(e->name);
    free(e->type);
    free(e);
}

// free SymbolTable
void st_free(SymbolTable *st) {
    if (!st) return;
    ht_free(st->class_scope, symbol_entry_free);
    ht_free(st->subroutine_scope, symbol_entry_free);
    free(st);
}

void st_start_subroutine(SymbolTable *st) {
    if (!st) return;
    // release the subroutine table (with its SymbolEntry) and create a new one
    ht_free(st->subroutine_scope, symbol_entry_free);
    st->subroutine_scope = ht_create();
    st->counts[ARG] = 0;
    st->counts[VAR] = 0;
}

void st_define(SymbolTable *st, const char *name, const char *type, Kind kind) {
    if (!st || !name) {
        fprintf(stderr, "Error in st_define: st or name undefined (SymbolTable).");
        return;
    }

    SymbolEntry *e = malloc(sizeof(*e));
    if (!e) { perror("malloc"); return; }
    e->name = strdup(name);
    e->type = strdup(type ? type : "");
    e->kind = kind;

    if (kind == STATIC) {
        e->index = st->counts[STATIC]++;
        ht_insert(st->class_scope, name, e);
    } else if (kind == FIELD) {
        e->index = st->counts[FIELD]++;
        ht_insert(st->class_scope, name, e);
    } else if (kind == ARG) {
        e->index = st->counts[ARG]++;
        ht_insert(st->subroutine_scope, name, e);
    } else if (kind == VAR) {
        e->index = st->counts[VAR]++;
        ht_insert(st->subroutine_scope, name, e);
    } else {
        // unknown kind: clean up
        free(e->name);
        free(e->type);
        free(e);
    }
}

int st_var_count(SymbolTable *st, Kind kind) {
    if (!st) {
        fprintf(stderr, "Error in st_var_count: st undefined (SymbolTable).");
        return -1;
    } else if (kind == NONE) {
        fprintf(stderr, "Error in st_var_count: NONE kind (SymbolTable).");
        return -1;
    }

    return st->counts[kind];
}

// lookup helper
static SymbolEntry* lookup_entry(SymbolTable *st, const char *name) {
    if (!st || !name) {
		fprintf(stderr, "Error in lookup_entry: st or name undefined (SymbolTable).");
		return NULL;
	}
    SymbolEntry *e = (SymbolEntry *)ht_find(st->subroutine_scope, name);
    if (e) return e;
    e = (SymbolEntry *)ht_find(st->class_scope, name);
    return e;
}

Kind st_kind_of(SymbolTable *st, const char *name) {
    SymbolEntry *e = lookup_entry(st, name);
    if (!e) return NONE;
    return e->kind;
}

const char *st_type_of(SymbolTable *st, const char *name) {
    SymbolEntry *e = lookup_entry(st, name);
    if (!e) return NULL;
    return e->type;
}

int st_index_of(SymbolTable *st, const char *name) {
    SymbolEntry *e = lookup_entry(st, name);
    if (!e) return -1;
    return (int)e->index;
}