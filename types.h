#ifndef TYPES_H
#define TYPES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <stdatomic.h>
#include <unistd.h>   /* usleep */

// Constants
#define MAX_SHARDS       4
#define TABLE_SIZE       1024
#define MAX_QUEUE_SIZE   256
#define TX_QUEUE_SIZE    64     /* max in-flight transactions per shard */
#define KEY_LEN          256
#define VAL_LEN          256
#define MAX_TX_KEYS      8      /* max keys a single transaction can touch */

// Command types
typedef enum {
    CMD_SET,
    CMD_GET,
    CMD_SCHEDULE,
    CMD_EXECUTE,
    CMD_STOP
} CommandType;

// ACK type Shard to Coordinator 

typedef enum{
    ACK_OK,           /* success */
    ACK_NOT_FOUND,    /* GET miss */
    ACK_SCHEDULED,    /* CMD_SCHEDULE accepted */
    ACK_EXECUTED,     /* CMD_EXECUTE completed */
    ACK_RETRY         /* tx not at head yet — coordinator should retry */
}AckType;

// trasaction operation one key-value op inside a multi-key transaction consists of 
typedef struct{
    CommandType type;
    char key[KEY_LEN];
    char value[VAL_LEN];
}TxOp;

// Forward declarations
typedef struct ResponseQueue ResponseQueue;
typedef struct RequestQueue RequestQueue;
typedef struct HashTable HashTable;
typedef struct Shard Shard;
typedef struct TxQueue TxQueue;

// Response: what Shard sends back to IO
typedef struct {
    uint64_t request_id;
    uint64_t tx_id;             /* for transaction responses */
    AckType  ack;
    char     value[VAL_LEN];    /* GET result */
} Response;

// Request: what IO thread sends to a shard
typedef struct {
    uint64_t        request_id;
    CommandType     type;
    char            key[KEY_LEN];
    char            value[VAL_LEN];
    ResponseQueue   *reply_to; 
    //txn specific 
    uint64_t        tx_id;
    TxOp            tx_op;
    int             shard_op_total;
    int             shard_op_index;
} Request;

// Response queue for IO thread
struct ResponseQueue {
    Response items[MAX_QUEUE_SIZE];
    int front, rear, count;
    pthread_mutex_t lock;
    pthread_cond_t ready;
};

// Request queue for shard thread
struct RequestQueue {
    Request items[MAX_QUEUE_SIZE];
    int front, rear, count;
    pthread_mutex_t lock;
    pthread_cond_t ready;
};

// Hash table entry
typedef struct {
    char key[KEY_LEN];
    char value[VAL_LEN];
    int occupied;
} Entry;

// Hash table
struct HashTable {
    Entry entries[TABLE_SIZE];
    size_t count;
};

// Transaction Queue (per-shard, ordered by tx_id)
struct TxQueue {
    uint64_t tx_ids[TX_QUEUE_SIZE];
    int count;
};

// Shard
struct Shard {
    int shard_id;
    HashTable table;
    TxQueue       txq;          /* ← NEW: per-shard transaction queue */
    RequestQueue request_queue;
    pthread_t thread;
    int running;    /* flag to control shard thread execution , set to 0 on CMD_STOP */
};

#endif // TYPES_H
