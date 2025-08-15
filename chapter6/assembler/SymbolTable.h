#ifndef SYMBOLTABLE_H_
#define SYMBOLTABLE_H_

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

// DYNAMIC_ARRAY --- BUCKETS
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

typedef struct {
    unsigned char *key;
    int address;
} Pair;

typedef struct {
    Pair *items;
    size_t count;
    size_t capacity;
} Bucket;

typedef enum { INSERT, CONTAINS, GET } Operation;

// HASH TABLE

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
