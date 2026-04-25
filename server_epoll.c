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
#include <signal.h>
#include <errno.h>

#ifdef __linux__
    #include <sys/epoll.h>
    #include <fcntl.h>
    #include <unistd.h>
    #define USE_EPOLL 1
#else
    #define USE_EPOLL 0
    #include <pthread.h>
#endif

#define SERVER_PORT 6379
#define MAX_EVENTS 1024
#define RECV_BUF_SIZE 4096
#define SEND_BUF_SIZE 4096

static volatile int server_running = 1;

/* Client state for epoll */
typedef struct {
    int fd;
    int client_id;
    ResponseQueue rq;
    Transaction tx;
    int in_transaction;
    char recv_buf[RECV_BUF_SIZE];
    int recv_len;
} ClientState;

/* Set socket to non-blocking mode */
static int set_nonblocking(int fd) {
#ifdef __linux__
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl F_GETFL");
        return -1;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl F_SETFL");
        return -1;
    }
#endif
    return 0;
}

static void handle_client_command(ClientState* client , const char *line){
    char send_buf[SEND_BUF_SIZE];


     /* Create response queue for this client
     * This is how we receive responses from shard workers */
    ResponseQueue rq;
    response_queue_init(&rq);
    
    /* Transaction state (for BEGIN/COMMIT) */
    Transaction tx;
    int in_transaction = 0;

    /* Parse command */
    Command cmd;
    if (parse_command(line, &cmd) < 0) {
        format_error_response(send_buf, sizeof(send_buf), "Invalid command");
        send_all(client->fd, send_buf, strlen(send_buf));
        return;
    }

     printf("[Server] Client %d: %s\n", client->client_id, line);

      // step 3 switch the command tyooe 

    switch(cmd.type){

        case CMD_TYPE_PING: {
            // send PONG_ADESH 
            send_all(client->fd, "PONG_ADESH", 10);
            break;
        }

        case CMD_TYPE_DEL: {
            Response resp = io_del(&rq, cmd.key);
            if (resp.ack == ACK_OK) {
                format_integer_response(send_buf, sizeof(send_buf), 1);
            } else {
                format_integer_response(send_buf, sizeof(send_buf), 0);
            }
            send_all(client->fd, send_buf, strlen(send_buf));
            break;
        }

        case CMD_TYPE_EXISTS: {
            Response resp = io_exists(&rq, cmd.key);
            if (resp.ack == ACK_OK) {
                format_integer_response(send_buf, sizeof(send_buf), 1);
            } else {
                format_integer_response(send_buf, sizeof(send_buf), 0);
            }
            send_all(client->fd, send_buf, strlen(send_buf));
            break;
        }

        case CMD_TYPE_INCR: {
            Response resp = io_incr(&rq, cmd.key);
            if (resp.ack == ACK_OK) {
                long long value = atoll(resp.value);
                format_integer_response(send_buf, sizeof(send_buf), value);
            } else {
                format_error_response(send_buf, sizeof(send_buf), "INCR failed");
            }
            send_all(client->fd, send_buf, strlen(send_buf));
            break;
        }

        case CMD_TYPE_DECR: {
            Response resp = io_decr(&rq, cmd.key);
            if (resp.ack == ACK_OK) {
                long long value = atoll(resp.value);
                format_integer_response(send_buf, sizeof(send_buf), value);
            } else {
                format_error_response(send_buf, sizeof(send_buf), "DECR failed");
            }
            send_all(client->fd, send_buf, strlen(send_buf));
            break;
        }

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
            send_all(client->fd, send_buf, strlen(send_buf));
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
            send_all(client->fd, send_buf, strlen(send_buf));
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
                printf("[Server] Client %d: BEGIN transaction\n", client->client_id);
            }
            send_all(client->fd, send_buf, strlen(send_buf));
            break;
        }

            /* ── COMMIT (execute transaction) ──────────────────────────── */
        case CMD_TYPE_COMMIT: {
            if (!in_transaction) {
                format_error_response(send_buf, sizeof(send_buf), 
                                    "Not in transaction");
            } else {
                // Execute transaction using Phase 3 two-phase commit
                tx_run(&rq, &tx, client->client_id);
                in_transaction = 0;
                format_ok_response(send_buf, sizeof(send_buf));
                printf("[Server] Client %d: COMMIT transaction\n", client->client_id);
            }
            send_all(client->fd, send_buf, strlen(send_buf));
            break;
        }

            /* ── QUIT (disconnect) ─────────────────────────────────────── */
        case CMD_TYPE_QUIT: {
            format_ok_response(send_buf, sizeof(send_buf));
            send_all(client->fd, send_buf, strlen(send_buf));
            printf("[Server] Client %d: QUIT\n", client->client_id);
            //goto cleanup;  // Exit loop
            client->fd = -1; 
            break;
        }
        
        /* ── Invalid command ───────────────────────────────────────── */
        case CMD_TYPE_INVALID:
        default: {
            format_error_response(send_buf, sizeof(send_buf), "Unknown command");
            send_all(client->fd, send_buf, strlen(send_buf));
            break;
        }
    }
}

static void process_client_data(ClientState* client){
    char temp_buf[RECV_BUF_SIZE];

     /* Read data */
    int n = recv(client->fd, temp_buf, sizeof(temp_buf) - 1, 0);
    if (n <= 0) {
        if (n == 0) {
            printf("[Server] Client %d disconnected\n", client->client_id);
        } else {
            perror("recv");
        }
        close_socket(client->fd);
        client->fd = -1;  // Mark for cleanup
        return;
    }

    temp_buf[n] = '\0';

     /* Append to buffer */
    if (client->recv_len + n >= RECV_BUF_SIZE) {
        fprintf(stderr, "[Server] Client %d buffer overflow\n", client->client_id);
        close_socket(client->fd);
        client->fd = -1;
        return;
    }

    memcpy(client->recv_buf + client->recv_len, temp_buf, n);
    client->recv_len += n;
    client->recv_buf[client->recv_len] = '\0';

    /* Process complete lines */
    char *line_start = client->recv_buf;
    char *line_end;
    
    while ((line_end = strstr(line_start, "\r\n")) != NULL) {
        *line_end = '\0';  // Null-terminate the line
        
        /* Handle command */
        if (strlen(line_start) > 0) {
            handle_client_command(client, line_start);
        }
        
        line_start = line_end + 2;  // Skip \r\n
    }
    

    /* Move remaining data to start of buffer */
    int remaining = client->recv_len - (line_start - client->recv_buf);
    if (remaining > 0) {
        memmove(client->recv_buf, line_start, remaining);
    }
    client->recv_len = remaining;
    client->recv_buf[client->recv_len] = '\0';

    
}

static void signal_handler(int sig) {
    (void)sig;
    printf("\n[Server] Shutting down...\n");
    server_running = 0;
}

int main(void) {
    printf("=== DragonFlyDB Server - epoll Edition ===\n\n");
    
#if !USE_EPOLL
    fprintf(stderr, "Error: epoll not available on this platform\n");
    fprintf(stderr, "Compile on Linux or use the thread-based server\n");
    return 1;
#endif

    /* Initialize shards */
    printf("[Server] Initializing shards...\n");
    shards_init();

     /* Setup signal handler */
    signal(SIGINT, signal_handler);

      /* Create server socket */
    printf("[Server] Creating server socket on port %d...\n", SERVER_PORT);
    int server_fd = create_server_socket(SERVER_PORT);
    if (server_fd < 0) {
        fprintf(stderr, "[Server] Failed to create server socket\n");
        return 1;
    }
    
    set_nonblocking(server_fd);

#ifdef __linux__
    /* Create epoll instance */
    int epoll_fd = epoll_create1(0);
    if(epoll_fd == -1){
         perror("epoll_create1");
        close_socket(server_fd);
        return 1;
    }

    //server soceket adding to epoll
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = server_fd;

    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) == -1){
        perror("epoll_ctl: server_fd");
        close_socket(server_fd);
        close(epoll_fd);
        return 1;
    }

    printf("[Server] Listening for connections (epoll mode)...\n");

    printf("[Server] Listening on port %d (epoll mode)\n", SERVER_PORT);
    printf("[Server] Press Ctrl+C to shutdown\n\n");
    
    ClientState *clients[MAX_EVENTS] = {0};
    int client_counter = 0;

     /* Event loop */
    struct epoll_event events[MAX_EVENTS];
    

    while(server_running){
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            if (errno == EINTR) continue;  // Interrupted by signal
            perror("epoll_wait");
            break;
        }

        for(int i=0; i < nfds; i++){
             if (events[i].data.fd == server_fd) {
               
                 /* Create client state */
                ClientState *client = malloc(sizeof(ClientState));
                 /* New connection */
                client->fd = accept_client(server_fd);
                if (client->fd < 0) continue;
                
                set_nonblocking(client->fd);
                client->recv_buf[0] = '\0';
                client->fd = client->fd;
                client->client_id = ++client_counter;
                response_queue_init(&client->rq);
                client->in_transaction = 0;
                client->recv_len = 0;

                /* Add to epoll */
                ev.events = EPOLLIN | EPOLLET;  // Edge-triggered
                ev.data.ptr = client;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client->fd, &ev) == -1) {
                    perror("epoll_ctl: client->fd");
                    close_socket(client->fd);
                    free(client);
                    continue;
                }
                
                printf("[Server] Client %d connected (fd=%d)\n", 
                       client->client_id, client->fd);
            }
            else{
                /* Client data */
                ClientState *client = (ClientState*)events[i].data.ptr;
                process_client_data(client);

                /* Cleanup if disconnected */
                if (client->fd == -1) {
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client->fd, NULL);
                    response_queue_destroy(&client->rq);
                    free(client);
                }
            }
        }
    }

    /* Cleanup */
    printf("[Server] Cleaning up...\n");
    close(epoll_fd);
    close_socket(server_fd);
    shards_shutdown();
    
    printf("[Server] Shutdown complete\n");
#endif

    return 0;

}