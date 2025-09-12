#ifndef HASH_TABLE_H_
#define HASH_TABLE_H_

#include "Utils.h"
#include <stddef.h>

typedef struct HashTable HashTable;

typedef void (*ht_value_free_fn)(void *value);

// create / free
HashTable *ht_create(void);
void ht_free(HashTable *t, ht_value_free_fn value_free);

// operations
bool  ht_insert(HashTable *t, const char *key, void *value);
void *ht_find(HashTable *t, const char *key);
bool  ht_contains(HashTable *t, const char *key);

#endif // HASH_TABLE_H_