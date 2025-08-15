#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "SymbolTable.h"

// MACROS

// DYNAMIC ARRAY --- BUCKETS
#define da_append(xs, x)\
    do {\
    if ((xs)->count >= (xs)->capacity) {\
        if ((xs)->capacity == 0) (xs)->capacity = 128;\
        else (xs)->capacity *= 2;\
        (xs)->items = realloc((xs)->items, (xs)->capacity * sizeof(*(xs)->items));\
    }\
    (xs)->items[(xs)->count++] = (x);\
    } while(0)

#define da_free(xs) free((xs).items)

// ---------------------------------------------------------------------------------------

// ENUM
// Since all three operations use the same function, this enum is used as a control.
typedef enum { INSERT, CONTAINS, GET } Operation;

// ---------------------------------------------------------------------------------------

// Functions

static void rehash(HashTable *t);

HashTable* create_table() {
    HashTable *t = malloc(sizeof(HashTable));
    if (!t) return NULL;
    t->T = TABLE_SIZE;
    t->collisions = 0;
    
    t->table = calloc(t->T, sizeof(Pair));
    t->buckets = calloc(t->T, sizeof(Bucket));
    t->count = 0;
    t->rom = 0;
    t->ram = 16;

    // Predefined Symbols
    add_entry(t, "SP", 0);
    add_entry(t, "LCL", 1);
    add_entry(t, "ARG", 2);
    add_entry(t, "THIS", 3);
    add_entry(t, "THAT", 4);
    add_entry(t, "R0", 0);
    add_entry(t, "R1", 1);
    add_entry(t, "R2", 2);
    add_entry(t, "R3", 3);
    add_entry(t, "R4", 4);
    add_entry(t, "R5", 5);
    add_entry(t, "R6", 6);
    add_entry(t, "R7", 7);
    add_entry(t, "R8", 8);
    add_entry(t, "R9", 9);
    add_entry(t, "R10", 10);
    add_entry(t, "R11", 11);
    add_entry(t, "R12", 12);
    add_entry(t, "R13", 13);
    add_entry(t, "R14", 14);
    add_entry(t, "R15", 15);
    add_entry(t, "SCREEN", 16384);
    add_entry(t, "KBD", 24576);

    return t;
}

static size_t hash(const unsigned char *str) {
    size_t hash = 5381;
    while (*str) {
        hash = ((hash << 5) + hash) + (*str++);
    }
    return hash;
}

static int buck_insert(Bucket *b, unsigned char *key, int addr) {
    for(size_t i = 0; i < b->count; ++i) {
        if (strcmp((char *)b->items[i].key, (char *)key) == 0) {
            return b->items[i].address;
        }
    }
    Pair new_pair = {strdup((char *)key), addr};
    da_append(b, new_pair);
    return addr;
}

static int insert_internal(HashTable *t, unsigned char *key, int addr, Operation flag) {
    size_t i = hash(key) % t->T;

    if (t->table[i].key == NULL) {
        if (flag == CONTAINS) return 0;  // contains? no! -> return false
        if (flag == GET) return -1; // symbol isn't in table -> return -1 as error control;
        if (flag == INSERT) {
            t->table[i].key = strdup((char *)key);
            t->table[i].address = addr;
            t->count++;
            return addr;
        }
    } else if (strcmp((char *)t->table[i].key, (char *)key) == 0) {
        if (flag == CONTAINS) return 1; // contains? yes! -> return true
        if (flag == GET) return t->table[i].address; // symbol is in table -> return address;
        if (flag == INSERT && t->table[i].address != addr) t->table[i].address = addr;
    } else {
        for (size_t j = 0; j < t->buckets[i].count; ++j) {
            if (strcmp((char *)t->buckets[i].items[j].key, (char *)key) == 0) {
                if (flag == CONTAINS) return 1; // contains? yes! -> return true
                if (flag == GET) return t->buckets[i].items[j].address; // symbol is in table -> return address;
            }
        }
        if (flag == CONTAINS) return 0;  // contains? no! -> return false
        if (flag == GET) return -1; // symbol isn't in table -> return -1 as error control;

        if (flag == INSERT) {
            Pair new_pair = {strdup((char *)key), addr};
            da_append(&t->buckets[i], new_pair);
            t->collisions++;
            t->count++;
            return addr;
        }
    }
}

void add_entry(HashTable *t, unsigned char *symbol, int address) {
    if ((double)(t->count + 1) / t->T > LOAD_FACTOR) {
        rehash(t);
    }
    insert_internal(t, symbol, address, INSERT);
}

int contains(HashTable *t, unsigned char *symbol) {
    return insert_internal(t, symbol, -1, CONTAINS);
}

int get_address(HashTable *t, unsigned char *symbol) {
    return insert_internal(t, symbol, -1, GET);
}

static void rehash(HashTable *t) {
    size_t old_T = t->T;
    Pair *old_table = t->table;
    Bucket *old_buckets = t->buckets;

    t->T *= 2;
    t->table = calloc(t->T, sizeof(Pair));
    t->buckets = calloc(t->T, sizeof(Bucket));
    t->count = 0;
    t->collisions = 0;

    for (size_t i = 0; i < old_T; ++i) {
        if (old_table[i].key != NULL) {
            insert_internal(t, old_table[i].key, old_table[i].address, INSERT);
            free(old_table[i].key);
        }

        for (size_t j = 0; j < old_buckets[i].count; ++j) {
            insert_internal(t,
                            old_buckets[i].items[j].key,
                            old_buckets[i].items[j].address,
                            INSERT);
            free(old_buckets[i].items[j].key);
        }

        da_free(old_buckets[i]);
    }

    free(old_table);
    free(old_buckets);
}

void free_table(HashTable *table) {
    if (!table) return;
    
    for (size_t i = 0; i < table->T; ++i) {
        free(table->table[i].key);
        for (size_t j = 0; j < table->buckets[i].count; ++j) {
            free(table->buckets[i].items[j].key);
        }
        da_free(table->buckets[i]);
    }
    
    free(table->table);
    free(table->buckets);
    free(table);
}