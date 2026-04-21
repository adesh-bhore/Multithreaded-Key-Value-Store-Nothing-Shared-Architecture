#ifndef HASHTABLE_H
#define HASHTABLE_H

#include "types.h"

// Hash table operations
void ht_init(HashTable *table);
int ht_set(HashTable *table, const char *key, const char *value);
int ht_get(HashTable *table, const char *key, char *out_value);

// Helper: determine which shard a key belongs to
int key_to_shard(const char *key);

#endif // HASHTABLE_H
