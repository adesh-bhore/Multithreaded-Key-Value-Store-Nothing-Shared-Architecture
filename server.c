#include "network.h"
#include "protocol.h"
#include "types.h"
#include "queue.h"
#include "shard.h"
#include "io.h"
#include "Transaction.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>


#define SERVER_PORT 6379
#define RECV_BUF_SIZE 4096
#define SEND_BUF_SIZE 4096


static volatile int server_running = 1;

// client thread handler , eahc thread spoins up per clinet 

typedef struct {
    int client_fd;
    int client_id;  // For logging
} ClientHandlerArgs;

static void *client_handler(void *arg) {
    ClientHandlerArgs *args = (ClientHandlerArgs*)arg;
    int client_fd = args->client_fd;
    int client_id = args->client_id;
    free(args);

     printf("[Server] Client %d handler started (fd=%d)\n", client_id, client_fd);
    

     /* Create response queue for this client
     * This is how we receive responses from shard workers */
    ResponseQueue rq;
    response_queue_init(&rq);
    
    /* Transaction state (for BEGIN/COMMIT) */
    Transaction tx;
    int in_transaction = 0;
    
    /* Buffers for network I/O */
    char recv_buf[RECV_BUF_SIZE];
    char send_buf[SEND_BUF_SIZE];
    
    /* ── Main command processing loop ──────────────────────────────────── */

    while (1) {
        /* Step 1: Receive command from client */
        int n = recv_line(client_fd, recv_buf, sizeof(recv_buf));
        if (n == 0) {
            // Client disconnected gracefully
            printf("[Server] Client %d disconnected\n", client_id);
            break;
        }
        if (n < 0) {
            // Error or connection closed
            fprintf(stderr, "[Server] Client %d recv error\n", client_id);
            break;
        }
        
        printf("[Server] Client %d: %s\n", client_id, recv_buf);

          /* Step 2: Parse command */
        Command cmd;
        if (parse_command(recv_buf, &cmd) < 0) {
            format_error_response(send_buf, sizeof(send_buf), "Invalid command");
            send_all(client_fd, send_buf, strlen(send_buf));
            continue;
        }

        // step 3 switch the command tyooe 

        switch(cmd.type){

              case CMD_TYPE_SET: {
                if (in_transaction) {
                    // Inside transaction: queue operation
                    tx_add(&tx, CMD_SET, cmd.key, cmd.value);
                    format_ok_response(send_buf, sizeof(send_buf));
                }
                else {
                    // Outside transaction: execute immediately
                    Response resp = io_set(&rq, cmd.key, cmd.value);
                    if (resp.ack == ACK_OK) {
                        format_ok_response(send_buf, sizeof(send_buf));
                    } else {
                        format_error_response(send_buf, sizeof(send_buf), "SET failed");
                    }
                }
                send_all(client_fd, send_buf, strlen(send_buf));
                break;
            }

             /* ── GET key ───────────────────────────────────────────────── */
            case CMD_TYPE_GET: {
                if (in_transaction) {
                    // Inside transaction: queue operation
                    tx_add(&tx, CMD_GET, cmd.key, NULL);
                    format_ok_response(send_buf, sizeof(send_buf));
                } else {
                    // Outside transaction: execute immediately
                    Response resp = io_get(&rq, cmd.key);
                    if (resp.ack == ACK_OK) {
                        format_bulk_string(send_buf, sizeof(send_buf), resp.value);
                    } else {
                        format_null_response(send_buf, sizeof(send_buf));
                    }
                }
                send_all(client_fd, send_buf, strlen(send_buf));
                break;
            }

              /* ── BEGIN (start transaction) ─────────────────────────────── */
            case CMD_TYPE_BEGIN: {
                if (in_transaction) {
                    format_error_response(send_buf, sizeof(send_buf), 
                                        "Already in transaction");
                } else {
                    tx_init(&tx);
                    in_transaction = 1;
                    format_ok_response(send_buf, sizeof(send_buf));
                    printf("[Server] Client %d: BEGIN transaction\n", client_id);
                }
                send_all(client_fd, send_buf, strlen(send_buf));
                break;
            }

             /* ── COMMIT (execute transaction) ──────────────────────────── */
            case CMD_TYPE_COMMIT: {
                if (!in_transaction) {
                    format_error_response(send_buf, sizeof(send_buf), 
                                        "Not in transaction");
                } else {
                    // Execute transaction using Phase 3 two-phase commit
                    tx_run(&rq, &tx, client_id);
                    in_transaction = 0;
                    format_ok_response(send_buf, sizeof(send_buf));
                    printf("[Server] Client %d: COMMIT transaction\n", client_id);
                }
                send_all(client_fd, send_buf, strlen(send_buf));
                break;
            }

             /* ── QUIT (disconnect) ─────────────────────────────────────── */
            case CMD_TYPE_QUIT: {
                format_ok_response(send_buf, sizeof(send_buf));
                send_all(client_fd, send_buf, strlen(send_buf));
                printf("[Server] Client %d: QUIT\n", client_id);
                goto cleanup;  // Exit loop
            }
            
            /* ── Invalid command ───────────────────────────────────────── */
            case CMD_TYPE_INVALID:
            default: {
                format_error_response(send_buf, sizeof(send_buf), "Unknown command");
                send_all(client_fd, send_buf, strlen(send_buf));
                break;
            }
        }
    }

cleanup:
    /* Cleanup */
    close_socket(client_fd);
    response_queue_destroy(&rq);
    printf("[Server] Client %d handler exited\n", client_id);
    return NULL;
}

static void signal_handler(int sig) {
    (void)sig;
    printf("\n[Server] Shutting down...\n");
    server_running = 0;
}

int main(void) {
    printf("=== DragonFlyDB Server - Phase 4 ===\n\n");
    
    /* Step 1: Initialize Phase 3 components (shards) */
    printf("[Server] Initializing shards...\n");
    shards_init();

      /* Step 2: Setup signal handler for graceful shutdown (Ctrl+C) */
    signal(SIGINT, signal_handler);

    /* Step 3: Create server socket */
    printf("[Server] Creating server socket on port %d...\n", SERVER_PORT);
    int server_fd = create_server_socket(SERVER_PORT);
    if (server_fd < 0) {
        fprintf(stderr, "[Server] Failed to create server socket\n");
        return 1;
    }
    
    printf("[Server] Listening on port %d\n", SERVER_PORT);
    printf("[Server] Press Ctrl+C to shutdown\n\n");
    
    /* Step 4: Accept loop - spawn thread for each client */
    int client_counter = 0;
    while (server_running) {
        /* Accept blocks until a client connects */
        int client_fd = accept_client(server_fd);
        if (client_fd < 0) {
            if (!server_running) break;  // Shutdown signal received
            fprintf(stderr, "[Server] accept() failed\n");
            continue;
        }

        /* Spawn handler thread for this client */
        pthread_t thread;
        ClientHandlerArgs *args = malloc(sizeof(ClientHandlerArgs));
        args->client_fd = client_fd;
        args->client_id = ++client_counter;
        
        if (pthread_create(&thread, NULL, client_handler, args) != 0) {
            perror("pthread_create");
            close_socket(client_fd);
            free(args);
            continue;
        }
        /* Detach thread so it cleans up after itself */
        pthread_detach(thread);
    }

    /* Step 5: Cleanup */
    printf("[Server] Closing server socket...\n");
    close_socket(server_fd);
    
    printf("[Server] Shutting down shards...\n");
    shards_shutdown();
    
    printf("[Server] Shutdown complete\n");
    return 0;
}
