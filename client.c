#include "network.h"
#include "protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Windows compatibility */
#ifdef _WIN32
    #include <winsock2.h>
#endif


/* ── Configuration ──────────────────────────────────────────────────────── */
#define DEFAULT_HOST "51.21.226.149"
#define DEFAULT_PORT 6379
#define INPUT_BUF_SIZE 4096
#define RESPONSE_BUF_SIZE 4096


static void print_response(const char *response) {
    if (response[0] == '+') {
        /* Simple string: +OK\r\n */
        printf("%s\n", response + 1);  // Skip '+'
        
    } else if (response[0] == '-') {
        /* Error: -ERR message\r\n */
        printf("(error) %s\n", response + 1);  // Skip '-'
        
    } else if (response[0] == '$') {
        /* Bulk string: $5\r\nvalue\r\n or $-1\r\n (null) */
        int len = atoi(response + 1);  // Parse length
        
        if (len == -1) {
            printf("(nil)\n");
        } else {
            /* Read the actual value (next line) */
            char value_buf[RESPONSE_BUF_SIZE];
            // For simplicity, we assume value is already in buffer
            // In production, you'd recv() the exact number of bytes
            printf("\"%s\"\n", response + strlen(response) + 1);
        }
        
    } else {
        /* Unknown format */
        printf("%s\n", response);
    }
}





/* ══════════════════════════════════════════════════════════════════════════
 * MAIN - Client entry point
 * ══════════════════════════════════════════════════════════════════════════ */

 int main(int argc, char *argv[]) {
    /* Parse command-line arguments */
    const char *host = DEFAULT_HOST;
    int port = DEFAULT_PORT;

    if (argc >= 2) {
        host = argv[1];
    }
    if (argc >= 3) {
        port = atoi(argv[2]);
    }

    printf("=== DragonFlyDB Client ===\n\n");

    /* Step 1: Connect to server */
    printf("Connecting to %s:%d...\n", host, port);
    int sockfd = connect_to_server(host, port);
    if (sockfd < 0) {
        fprintf(stderr, "Failed to connect to %s:%d\n", host, port);
        return 1;
    }

    printf("Connected to DragonFlyDB at %s:%d\n", host, port);
    printf("Type commands (SET key value, GET key, BEGIN, COMMIT, QUIT)\n");
    printf("Type 'help' for command list\n\n");
    

    /* Buffers */
    char input[INPUT_BUF_SIZE];
    char response[RESPONSE_BUF_SIZE];

     /* Step 2: Interactive command loop */
    while (1) {
        /* Display prompt */
        printf("dragonfly> ");
        fflush(stdout);
        
        /* Read user input */
        if (!fgets(input, sizeof(input), stdin)) {
            break;  // EOF (Ctrl+D)
        }
        
        /* Remove trailing newline */
        input[strcspn(input, "\n")] = '\0';
        
        /* Skip empty lines */
        if (strlen(input) == 0) {
            continue;
        }
        

         /* Handle special commands */
        if (strcmp(input, "help") == 0) {
            printf("Commands:\n");
            printf("  SET key value    - Set a key to a value\n");
            printf("  GET key          - Get the value of a key\n");
            printf("  BEGIN            - Start a transaction\n");
            printf("  COMMIT           - Commit a transaction\n");
            printf("  QUIT             - Disconnect from server\n");
            printf("  help             - Show this help\n");
            continue;
        }


        if (strcmp(input, "exit") == 0 || strcmp(input, "quit") == 0) {
            strcpy(input, "QUIT");
        }

        /* Add protocol terminator \r\n */
        strcat(input, "\r\n");
        

         /* Step 3: Send command to server */
        if (send_all(sockfd, input, strlen(input)) < 0) {
            fprintf(stderr, "Send failed\n");
            break;
        }
        

        /* Step 4: Receive response from server */
        memset(response, 0, sizeof(response));
        
         /* Read first line to determine response type */
        if (recv_line(sockfd, response, sizeof(response)) <= 0) {
            fprintf(stderr, "Server disconnected\n");
            break;
        }


        /* Step 5: Display response based on type */
        if (response[0] == '+') {
            /* Simple string: +OK */
            printf("%s\n", response + 1);
            
        } else if (response[0] == '-') {
            /* Error: -ERR message */
            printf("(error) %s\n", response + 1);
            
        } else if (response[0] == '$') {
            /* Bulk string: need to read value */
            int len = atoi(response + 1);
            
            if (len == -1) {
                printf("(nil)\n");
            } else {
                /* Read exactly 'len' bytes */
                char value[RESPONSE_BUF_SIZE];
                int total_read = 0;
                while (total_read < len) {
                    int n = recv(sockfd, value + total_read, len - total_read, 0);
                    if (n <= 0) {
                        fprintf(stderr, "Failed to read bulk string\n");
                        goto cleanup;
                    }
                    total_read += n;
                }
                value[len] = '\0';
                
                /* Read trailing \r\n */
                char trailing[3];
                recv(sockfd, trailing, 2, 0);
                
                printf("\"%s\"\n", value);
            }
            
        } else {
            /* Unknown format */
            printf("%s\n", response);
        }
        
        /* Check if we sent QUIT */
        if (strncmp(input, "QUIT", 4) == 0) {
            break;
        }
    }
    
cleanup:
    /* Step 6: Cleanup */
    close_socket(sockfd);
    printf("\nDisconnected.\n");
    return 0;
}