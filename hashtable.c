#include "hashtable.h"
#include <string.h>

static uint64_t hash_key(const char *key) {
    // Simple djb2 hash
    uint64_t hash = 5381;
    int c;
    while ((c = *key++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    return hash;
}

void ht_init(HashTable *table) {
    memset(table, 0, sizeof(*table));
}

int ht_set(HashTable *table, const char *key, const char *value) {
    uint64_t hash = hash_key(key);
    size_t idx = hash % TABLE_SIZE;

    // Linear probing
    while (table->entries[idx].occupied &&
           strcmp(table->entries[idx].key, key) != 0) {
        idx = (idx + 1) % TABLE_SIZE;
    }

    strncpy(table->entries[idx].key, key, KEY_LEN - 1);
    strncpy(table->entries[idx].value, value, VAL_LEN - 1);

    table->entries[idx].key[KEY_LEN - 1] = '\0';
    table->entries[idx].value[VAL_LEN - 1] = '\0';

    if (!table->entries[idx].occupied) {
        table->count++;
    }

    table->entries[idx].occupied = 1;
    return 1;
}

int ht_get(HashTable *table, const char *key, char *out_value) {
    uint64_t hash = hash_key(key);
    size_t idx = hash % TABLE_SIZE;

    while (table->entries[idx].occupied) {
        if (strcmp(table->entries[idx].key, key) == 0) {
            strncpy(out_value, table->entries[idx].value, VAL_LEN);
            return 1;
        }
        idx = (idx + 1) % TABLE_SIZE;
    }

    return 0;
}

int key_to_shard(const char *key) {
    return (int)(hash_key(key) % MAX_SHARDS);
}
