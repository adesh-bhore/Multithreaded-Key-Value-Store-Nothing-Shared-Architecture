#ifndef SHARD_H
#define SHARD_H

#include "types.h"

// Initialize all shards and start worker threads
void shards_init(void);

// Graceful shutdown of all shards
void shards_shutdown(void);

// Get pointer to shard by ID
Shard* get_shard(int shard_id);

#endif // SHARD_H
