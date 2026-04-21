#include "TXNQueue.h"
#include "types.h"
#include <string.h>
#include <stdio.h>

void txq_init(TxQueue *q) {
    q->count = 0;
}

/* Insert tx_id in sorted (ascending) order */
void txq_insert(TxQueue *q, uint64_t tx_id) {
    if (q->count >= TX_QUEUE_SIZE) {
        fprintf(stderr, "[BUG] TxQueue full\n");
        return;
    }
    int i = q->count;
    /* Walk backwards, shifting larger IDs right */
    while (i > 0 && q->tx_ids[i - 1] > tx_id) {
        q->tx_ids[i] = q->tx_ids[i - 1];
        i--;
    }
    q->tx_ids[i] = tx_id;
    q->count++;
}

/* Returns the tx_id at the head (smallest), or 0 if empty */
uint64_t txq_head(TxQueue *q) {
    return (q->count > 0) ? q->tx_ids[0] : 0;
}

/* Remove the head entry */
void txq_remove_head(TxQueue *q) {
    if (q->count == 0) return;
    memmove(&q->tx_ids[0], &q->tx_ids[1],
            (q->count - 1) * sizeof(uint64_t));
    q->count--;
}
