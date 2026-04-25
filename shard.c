#include "shard.h"
#include "hashtable.h"
#include "queue.h"
#include "TXNQueue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Shard g_shards[MAX_SHARDS];

static void *shard_worker(void *args) {
    Shard *shard = (Shard *)args;

    printf("[Shard %d] started — thread %lu\n",
           shard->shard_id, (unsigned long)pthread_self());

    while (1) {
        Request req = request_dequeue(&shard->request_queue);

        Response resp;
        resp.request_id = req.request_id;
        resp.tx_id      = req.tx_id;
        resp.ack        = ACK_OK;
        resp.value[0] = '\0';

        switch(req.type){

            //simple key-value ops 
            case CMD_SET:
                ht_set(&shard->table, req.key, req.value);
                resp.ack = ACK_OK;
                printf("[Shard %d] SET  %-18s = %s\n",
                   shard->shard_id, req.key, req.value);
                break;

            case CMD_GET:
                resp.ack = ht_get(&shard->table , req.key , resp.value) ? ACK_OK  :  ACK_NOT_FOUND ;
                printf("[Shard %d] GET  %-18s → %s\n",
                   shard->shard_id, req.key,
                   resp.ack == ACK_OK ? resp.value : "<not found>");
                break;
            
            // 2PC txn 
                
            case CMD_SCHEDULE: 
                txq_insert(&shard->txq, req.tx_id);
                resp.ack = ACK_SCHEDULED;
                printf("[Shard %d] SCHED tx#%lu  TxQueue head=%lu  depth=%d\n",
                    shard->shard_id, req.tx_id,
                    txq_head(&shard->txq), shard->txq.count);
                break;

            case CMD_EXECUTE:
                if (txq_head(&shard->txq) != req.tx_id) {
                resp.ack = ACK_RETRY;
                printf("[Shard %d] RETRY tx#%lu  (head is tx#%lu)\n",
                       shard->shard_id, req.tx_id,
                       txq_head(&shard->txq));
                /* Do NOT push a response yet — break out and let the
                 * coordinator retry.  The reply_to is in req so the
                 * coordinator already knows where to come back. */
                response_enqueue(req.reply_to, resp);
                continue;
                }

                {// we are the head now , safe to move 
                    TxOp *op = &req.tx_op;
                    if (op->type == CMD_SET) {
                        ht_set(&shard->table, op->key, op->value);
                        printf("[Shard %d] EXEC tx#%lu  SET %-16s = %s\n",
                            shard->shard_id, req.tx_id, op->key, op->value);
                    } else if (op->type == CMD_GET) {
                        ht_get(&shard->table, op->key, resp.value);
                        printf("[Shard %d] EXEC tx#%lu  GET %-16s → %s\n",
                            shard->shard_id, req.tx_id, op->key,
                            resp.value[0] ? resp.value : "<not found>");
                    }
                }

                /* Only dequeue once all ops for this tx on this shard are done */
                if (req.shard_op_index >= req.shard_op_total)
                    txq_remove_head(&shard->txq);
                resp.ack = ACK_EXECUTED;
                break;

            case CMD_DEL: {
                int found = ht_delete(&shard->table, req.key);
                
                resp.ack = found ? ACK_OK : ACK_NOT_FOUND;
                resp.value[0] = '\0';
                
                printf("[Shard %d] DEL  %-20s → %s\n", 
                    shard->shard_id, req.key, found ? "deleted" : "not found");
                break;
            }

            case CMD_EXISTS: {
                char value[VAL_LEN];
                int found = ht_get(&shard->table, req.key, value);
                
                resp.ack = found ? ACK_OK : ACK_NOT_FOUND;
                resp.value[0] = '\0';
                
                printf("[Shard %d] EXISTS %-20s → %s\n", 
                    shard->shard_id, req.key, found ? "yes" : "no");
                break;
            }

            case CMD_INCR: {
                char value[VAL_LEN];
                long long num = 0;
                
                if (ht_get(&shard->table, req.key, value)) {
                    num = atoll(value);
                }
                num++;
                
                char new_value[VAL_LEN];
                snprintf(new_value, sizeof(new_value), "%lld", num);
                ht_set(&shard->table, req.key, new_value);
                
                resp.ack = ACK_OK;
                strncpy(resp.value, new_value, VAL_LEN - 1);
                
                printf("[Shard %d] INCR %-20s → %lld\n", shard->shard_id, req.key, num);
                break;
            }

            case CMD_DECR: {
                char value[VAL_LEN];
                long long num = 0;
                
                if (ht_get(&shard->table, req.key, value)) {
                    num = atoll(value);
                }
                num--;
                
                char new_value[VAL_LEN];
                snprintf(new_value, sizeof(new_value), "%lld", num);
                ht_set(&shard->table, req.key, new_value);
                
                resp.ack = ACK_OK;
                strncpy(resp.value, new_value, VAL_LEN - 1);
                
                printf("[Shard %d] DECR %-20s → %lld\n", shard->shard_id, req.key, num);
                break;
            }
            /* ── Graceful shutdown ── */
            case CMD_STOP:
                /*
                * CMD_STOP is a sentinel.  We set running=0 so the while loop
                * exits after this iteration.  No response is sent — the sender
                * (main/shutdown code) doesn't expect one.
                */
                printf("[Shard %d] stopping\n", shard->shard_id);
                shard->running = 0;
                continue;   /* skip rq_push below */
            }

            response_enqueue(req.reply_to, resp);

    }
    
    printf("[Shard %d] exited cleanly\n", shard->shard_id);
    return NULL;
}

void shards_init(void) {
    for (int i = 0; i < MAX_SHARDS; i++) {
        g_shards[i].shard_id = i;
        g_shards[i].running  = 1;
        ht_init(&g_shards[i].table);
        txq_init(&g_shards[i].txq);
        request_queue_init(&g_shards[i].request_queue);
        if (pthread_create(&g_shards[i].thread, NULL,
                           shard_worker, &g_shards[i]) != 0) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
    }
}

void shards_shutdown(void) {
    printf("\n=== shutting down shards ===\n");
    for (int i = 0; i < MAX_SHARDS; i++) {
        Request req;
        memset(&req, 0, sizeof(req));
        req.type = CMD_STOP;
        req.reply_to = NULL;
        req.request_id = next_request_id();
        request_enqueue(&g_shards[i].request_queue, req);
    }
    for (int i = 0; i < MAX_SHARDS; i++) {
        pthread_join(g_shards[i].thread, NULL);
    }
    printf("=== all shards stopped ===\n");
}

Shard* get_shard(int shard_id) {
    if (shard_id >= 0 && shard_id < MAX_SHARDS) {
        return &g_shards[shard_id];
    }
    return NULL;
}
