// DragonFlyDB - Phase 3: Two-Phase Commit + Graceful Shutdown
#include "types.h"
#include "queue.h"
#include "shard.h"
#include "io.h"
#include "Transaction.h"
#include <stdio.h>
#include <pthread.h>


/* ══════════════════════════════════════════════════════════════════════════
 * DEMO — three concurrent clients, mix of single ops and transactions
 * ══════════════════════════════════════════════════════════════════════════ */
typedef struct {
    int client_id;
} ClientArgs;


static void *client_thread(void *args){

    ClientArgs *a = (ClientArgs*)args;
    int         id = a->client_id;
    ResponseQueue rq;

    response_queue_init(&rq);

    printf("\n========== CLIENT %d STARTING ==========\n", id);

    /* ── Single-key SETs first ── */
    io_set(&rq, "balance:alice",  "1000");
    io_set(&rq, "balance:bob",    "500");
    io_set(&rq, "balance:charlie","750");


    /*
     * ── Simulated bank transfer transaction ──
     *
     * "Debit alice $200, credit bob $200"
     * Both keys may live on different shards.
     * With two-phase commit, either BOTH writes happen or neither does.
     */
    if (id == 1) {
        Transaction tx;
        tx_init(&tx);
        tx_add(&tx, CMD_SET, "balance:alice", "800");   /* alice  -200 */
        tx_add(&tx, CMD_SET, "balance:bob",   "700");   /* bob    +200 */
        tx_run(&rq, &tx, id);
    }
/*
     * ── Multi-key read transaction ──
     *
     * Read alice and bob's balances atomically.
     * In a real system this would use CMD_GET inside the tx;
     * we keep it as SETs here to keep the demo output clear,
     * but the scheduling mechanism is identical.
     */
    if (id == 2) {
        Transaction tx;
        tx_init(&tx);
        tx_add(&tx, CMD_SET, "session:alice",   "active");
        tx_add(&tx, CMD_SET, "lastlogin:alice", "2025-01-15");
        tx_run(&rq, &tx, id);
    }

    if (id == 3) {
        Transaction tx;
        tx_init(&tx);
        tx_add(&tx, CMD_SET, "order:9001",  "shipped");
        tx_add(&tx, CMD_SET, "notif:alice", "your order shipped");
        tx_add(&tx, CMD_SET, "log:9001",    "status_change");
        tx_run(&rq, &tx, id);
    }

    /* ── Verify final state via single-key GETs ── */
    printf("\n[Client %d] final reads:\n", id);
    const char *read_keys[] = {
        "balance:alice", "balance:bob", "balance:charlie", NULL
    };
    for (int i = 0; read_keys[i]; i++) {
        Response r = io_get(&rq, read_keys[i]);
        printf("[Client %d] GET %-22s → %s\n",
               id, read_keys[i],
               r.ack == ACK_OK ? r.value : "<not found>");
    }

    response_queue_destroy(&rq);
    printf("\n========== CLIENT %d DONE ==========\n", id);
    return NULL;

}

int main(void) {
     printf("=== DragonFlyDB Phase 3 — Two-Phase Commit + Graceful Shutdown ===\n\n");

    shards_init();

    
    /* Launch 3 concurrent clients */
    pthread_t   threads[3];
    ClientArgs  args[3] = {{1}, {2}, {3}};

    for (int i = 0; i < 3; i++) {
        pthread_create(&threads[i], NULL, client_thread, &args[i]);
        usleep(100000);  // 100ms delay between client starts for cleaner output
    }

    
    for (int i = 0; i < 3; i++)
        pthread_join(threads[i], NULL);

    /* Clean shutdown — all shard threads exit, no leaks */
    shards_shutdown();

    printf("\n=== process exits cleanly ===\n");
    return 0;
}