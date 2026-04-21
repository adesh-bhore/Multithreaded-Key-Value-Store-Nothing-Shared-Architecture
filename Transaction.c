#include "Transaction.h"
#include "queue.h"
#include "hashtable.h"
#include "shard.h"
#include <stdio.h>
#include <string.h>

void tx_init(Transaction *tx) {
    tx->num_ops = 0;
}

void tx_add(Transaction *tx, CommandType type,
            const char *key, const char *value) {
    if (tx->num_ops >= MAX_TX_KEYS) {
        fprintf(stderr, "[WARN] transaction full\n");
        return;
    }
    TxOp *op = &tx->ops[tx->num_ops++];
    op->type = type;
    strncpy(op->key,   key,   KEY_LEN - 1);
    strncpy(op->value, value ? value : "", VAL_LEN - 1);
    op->key[KEY_LEN - 1]   = '\0';
    op->value[VAL_LEN - 1] = '\0';
}

void tx_run(ResponseQueue *rq, Transaction *tx, int client_id) {
    uint64_t tx_id = next_tx_id();
    printf("\n[Client %d] >>> BEGIN tx#%lu  (%d ops)\n",
           client_id, tx_id, tx->num_ops);

    /* ── Step 1: determine involved shards (unique set) ── */
    int involved[MAX_SHARDS];
    int num_involved = 0;
    memset(involved, -1, sizeof(involved));

    for (int i = 0; i < tx->num_ops; i++) {
        int sid = key_to_shard(tx->ops[i].key);
        int already = 0;
        for (int j = 0; j < num_involved; j++)
            if (involved[j] == sid) { already = 1; break; }
        if (!already)
            involved[num_involved++] = sid;
    }

    /* ── Step 2: SCHEDULE phase ── */
    printf("[Client %d] tx#%lu  SCHEDULE → shards:", client_id, tx_id);
    for (int i = 0; i < num_involved; i++)
        printf(" %d", involved[i]);
    printf("\n");

    for (int i = 0; i < num_involved; i++) {
        Shard *shard = get_shard(involved[i]);
        Request r;
        memset(&r, 0, sizeof(r));
        r.request_id = next_request_id();
        r.tx_id      = tx_id;
        r.type       = CMD_SCHEDULE;
        r.reply_to   = rq;
        request_enqueue(&shard->request_queue, r);
    }

    /* Wait for all ACK_SCHEDULED */
    for (int i = 0; i < num_involved; i++) {
        Response resp = response_dequeue(rq);
        (void)resp;  /* in production: check resp.ack == ACK_SCHEDULED */
    }
    printf("[Client %d] tx#%lu  all shards scheduled\n", client_id, tx_id);

    /* ── Step 3: EXECUTE phase ── */
    printf("[Client %d] tx#%lu  EXECUTE phase\n", client_id, tx_id);

    /* Count how many ops land on each shard — shard dequeues on last op */
    int ops_per_shard[MAX_SHARDS] = {0};
    for (int i = 0; i < tx->num_ops; i++)
        ops_per_shard[key_to_shard(tx->ops[i].key)]++;

    int op_index[MAX_SHARDS] = {0};  /* running index per shard */

    for (int i = 0; i < tx->num_ops; i++) {
        int shard_id = key_to_shard(tx->ops[i].key);
        op_index[shard_id]++;

        int done = 0;
        while (!done) {
            Shard *shard = get_shard(shard_id);
            Request r;
            memset(&r, 0, sizeof(r));
            r.request_id      = next_request_id();
            r.tx_id           = tx_id;
            r.type            = CMD_EXECUTE;
            r.tx_op           = tx->ops[i];
            r.shard_op_total  = ops_per_shard[shard_id];
            r.shard_op_index  = op_index[shard_id];
            r.reply_to        = rq;
            request_enqueue(&shard->request_queue, r);

            Response resp = response_dequeue(rq);
            if (resp.ack == ACK_EXECUTED) {
                done = 1;
            } else if (resp.ack == ACK_RETRY) {
                usleep(1000);
            }
        }
    }

    printf("[Client %d] <<< COMMIT tx#%lu  done\n", client_id, tx_id);
}
