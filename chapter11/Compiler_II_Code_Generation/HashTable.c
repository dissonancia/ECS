#include "HashTable.h"  

#define INITIAL_TABLE_SIZE 1024
#define LOAD_FACTOR 0.75

typedef struct {
    char *key;
    void *value;
} Slot;

typedef struct {
    Slot **items;
    size_t count;
    size_t capacity;
} Bucket;

struct HashTable {
    size_t T;
    Slot **table;   
    Bucket *buckets;
    size_t count;
    size_t collisions;
};


static size_t hash_str(const char *s) {
    size_t h = 5381;
    while (*s) h = ((h << 5) + h) + (unsigned char)(*s++);
    return h;
}

static void rehash(HashTable *t);

HashTable *ht_create(void) {
    HashTable *t = calloc(1, sizeof(*t));
    if (!t) return NULL;
    t->T = INITIAL_TABLE_SIZE;
    t->table = calloc(t->T, sizeof(*(t->table)));
    t->buckets = calloc(t->T, sizeof(*(t->buckets)));
    t->count = 0;
    t->collisions = 0;
    return t;
}

void ht_free(HashTable *t, ht_value_free_fn value_free) {
    if (!t) return;
    for (size_t i = 0; i < t->T; ++i) {
        if (t->table[i]) {
            free(t->table[i]->key);
            if (value_free && t->table[i]->value) value_free(t->table[i]->value);
            free(t->table[i]);
        }
        for (size_t j = 0; j < t->buckets[i].count; ++j) {
            Slot *s = t->buckets[i].items[j];
            if (s) {
                free(s->key);
                if (value_free && s->value) value_free(s->value);
                free(s);
            }
        }
        da_free(t->buckets[i]);
    }
    free(t->table);
    free(t->buckets);
    free(t);
}

bool ht_insert(HashTable *t, const char *key, void *value) {
    if (!t || !key) return false;
    if ((double)(t->count + 1) / t->T > LOAD_FACTOR) rehash(t);
    size_t i = hash_str(key) % t->T;

    Slot *new_slot = malloc(sizeof(*new_slot));
    if (!new_slot) return false;
    new_slot->key = strdup(key);
    new_slot->value = value;

    if (t->table[i] == NULL) {
        t->table[i] = new_slot;
        t->count++;
        return true;
    }

    if (strcmp(t->table[i]->key, key) == 0) {
        free(new_slot->key);
        free(new_slot);
        t->table[i]->value = value;
        return true;
    }

    for (size_t j = 0; j < t->buckets[i].count; ++j) {
        if (strcmp(t->buckets[i].items[j]->key, key) == 0) {
            free(new_slot->key);
            free(new_slot);
            t->buckets[i].items[j]->value = value;
            return true;
        }
    }
	
    da_append(&t->buckets[i], new_slot);
    t->collisions++;
    t->count++;
    return true;
}

void *ht_find(HashTable *t, const char *key) {
    if (!t || !key) return NULL;
    size_t i = hash_str(key) % t->T;
    if (t->table[i] && strcmp(t->table[i]->key, key) == 0) return t->table[i]->value;
    for (size_t j = 0; j < t->buckets[i].count; ++j) {
        if (strcmp(t->buckets[i].items[j]->key, key) == 0) return t->buckets[i].items[j]->value;
    }
    return NULL;
}

bool ht_contains(HashTable *t, const char *key) {
    return ht_find(t, key) != NULL;
}

static void rehash(HashTable *t) {
    size_t old_T = t->T;
    Slot **old_table = t->table;
    Bucket *old_buckets = t->buckets;

    t->T *= 2;
    t->table = calloc(t->T, sizeof(*(t->table)));
    t->buckets = calloc(t->T, sizeof(*(t->buckets)));
    if (!t->table || !t->buckets) { perror("calloc"); exit(EXIT_FAILURE); }

    t->count = 0;
    t->collisions = 0;

    for (size_t i = 0; i < old_T; ++i) {
        if (old_table[i]) {
            Slot *s = old_table[i];
            size_t idx = hash_str(s->key) % t->T;
            if (t->table[idx] == NULL) {
                t->table[idx] = s;
                t->count++;
            } else {
                da_append(&t->buckets[idx], s);
                t->collisions++;
                t->count++;
            }
        }
        for (size_t j = 0; j < old_buckets[i].count; ++j) {
            Slot *s = old_buckets[i].items[j];
            size_t idx = hash_str(s->key) % t->T;
            if (t->table[idx] == NULL) {
                t->table[idx] = s;
                t->count++;
            } else {
                da_append(&t->buckets[idx], s);
                t->collisions++;
                t->count++;
            }
        }
        da_free(old_buckets[i]);
    }

    free(old_table);
    free(old_buckets);
}