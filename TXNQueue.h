#ifndef TXN_QUEUE_H
#define TXN_QUEUE_H

#include "types.h"

void txq_init(TxQueue *q);
void txq_insert(TxQueue *q, uint64_t tx_id);
uint64_t txq_head(TxQueue *q);
void txq_remove_head(TxQueue *q);

#endif // TXN_QUEUE_H