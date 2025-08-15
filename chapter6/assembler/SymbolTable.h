#ifndef SYMBOLTABLE_H_
#define SYMBOLTABLE_H_

#include <stddef.h>

// STRUCTS

// Pair (symbol, address) as an entry for the Symbol Table
typedef struct {
    unsigned char *key;
    int address;
} Pair;

// Dynamic array specification as Symbol Table collision control
typedef struct {
    Pair *items;
    size_t count;
    size_t capacity;
} Bucket;

// Hash Table specification
#define TABLE_SIZE 1024
#define LOAD_FACTOR 0.75

typedef struct {
    size_t T;
    Pair *table;
    Bucket *buckets;
    size_t count;
    size_t collisions;

    size_t rom;
    size_t ram;
} HashTable;

HashTable* create_table();
void free_table(HashTable *table);

// operations
void add_entry(HashTable *t, unsigned char *symbol, int address);
int contains(HashTable *t, unsigned char *symbol);
int get_address(HashTable *t, unsigned char *symbol);

#endif // SYMBOLTABLE_H_