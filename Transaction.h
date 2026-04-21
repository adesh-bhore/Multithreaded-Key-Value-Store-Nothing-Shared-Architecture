#ifndef TRANSACTION_H
#define TRANSACTION_H

#include "types.h"

// Transaction structure
typedef struct {
    TxOp ops[MAX_TX_KEYS];
    int num_ops;
} Transaction;

// Transaction API
void tx_init(Transaction *tx);
void tx_add(Transaction *tx, CommandType type, const char *key, const char *value);
void tx_run(ResponseQueue *rq, Transaction *tx, int client_id);

#endif // TRANSACTION_H
