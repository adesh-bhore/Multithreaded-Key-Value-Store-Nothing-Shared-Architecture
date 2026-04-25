#include "network.h"
#include "protocol.h"
#include "colors.h"
#include "ascii_art.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Windows compatibility */
#ifdef _WIN32
    #include <winsock2.h>
#endif

#ifdef USE_READLINE
    #include <readline/readline.h>
    #include <readline/history.h>
#endif


/* ── Configuration ──────────────────────────────────────────────────────── */
#define DEFAULT_HOST "51.21.226.149"
#define DEFAULT_PORT 6379
#define INPUT_BUF_SIZE 4096
#define RESPONSE_BUF_SIZE 4096

/* Print colored prompt */
static void print_prompt(int in_transaction) {
    if (in_transaction) {
        printf(COLOR_YELLOW "dragonfly" COLOR_BRIGHT_RED "(TX)" COLOR_RESET "> ");
    } else {
        printf(COLOR_CYAN "dragonfly" COLOR_RESET "> ");
    }
    fflush(stdout);
}

/* Print response with colors */
static void print_response_colored(const char *response, const char *value) {
    if (response[0] == '+') {
        /* Success: +OK */
        printf(COLOR_GREEN "✓ " COLOR_RESET "%s\n", response + 1);
        
    } else if (response[0] == '-') {
        /* Error: -ERR message */
        printf(COLOR_RED "✗ Error: " COLOR_RESET "%s\n", response + 5);
        
    } else if (response[0] == '$') {
        /* Bulk string */
        int len = atoi(response + 1);
        if (len == -1) {
            printf(COLOR_DIM "(nil)\n" COLOR_RESET);
        } else {
            printf(COLOR_BRIGHT_CYAN "\"%s\"\n" COLOR_RESET, value);
        }
        
    } else if (response[0] == ':') {
        /* Integer */
        long long num = atoll(response + 1);
        #ifdef _WIN32
            printf(COLOR_BRIGHT_MAGENTA "(integer) %I64d\n" COLOR_RESET, num);
        #else
            printf(COLOR_BRIGHT_MAGENTA "(integer) %lld\n" COLOR_RESET, num);
        #endif
        
    } else {
        /* Unknown */
        printf("%s\n", response);
    }
}




/* ══════════════════════════════════════════════════════════════════════════
 * MAIN - Client entry point
 * ══════════════════════════════════════════════════════════════════════════ */

 int main(int argc, char *argv[]) {

    enable_ansi_colors();

     /* Print logo */
    print_dragon_logo();
    print_dragon_ascii();
    /* Parse command-line arguments */

    const char *host = DEFAULT_HOST;
    int port = DEFAULT_PORT;

    if (argc >= 2) {
        host = argv[1];
    }
    if (argc >= 3) {
        port = atoi(argv[2]);
    }

      /* Connect to server */
    printf(COLOR_BRIGHT_WHITE "Connecting to %s:%d...\n" COLOR_RESET, host, port);
    int sockfd = connect_to_server(host, port);
    if (sockfd < 0) {
        printf(COLOR_RED "✗ Failed to connect to %s:%d\n" COLOR_RESET, host, port);
        return 1;
    }

    /* Print welcome message */
    print_welcome_message(host, port);
    
    /* Buffers */
    char input[INPUT_BUF_SIZE];
    char response[RESPONSE_BUF_SIZE];
    int in_transaction = 0;


     /* Step 2: Interactive command loop */
    while (1) {
       
       /* Display prompt */
        print_prompt(in_transaction);
        
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
            print_help();
            continue;
        }

        if (strcmp(input, "clear") == 0) {
            #ifdef _WIN32
                system("cls");
            #else
                system("clear");
            #endif
            print_dragon_logo();
            continue;
        }


        if (strcmp(input, "exit") == 0 || strcmp(input, "quit") == 0) {
            strcpy(input, "QUIT");
        }


        /* Add protocol terminator \r\n */
        strcat(input, "\r\n");
        

         /* Step 3: Send command to server */
        if (send_all(sockfd, input, strlen(input)) < 0) {
            printf(COLOR_RED "✗ Send failed\n" COLOR_RESET);
            break;
        }
        

        /* Step 4: Receive response from server */
        memset(response, 0, sizeof(response));
        
         /* Read first line to determine response type */
        if (recv_line(sockfd, response, sizeof(response)) <= 0) {
            printf(COLOR_RED "✗ Server disconnected\n" COLOR_RESET);
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
                print_response_colored(response, NULL);
            } else {
                /* Read exactly 'len' bytes */
                char value[RESPONSE_BUF_SIZE];
                int total_read = 0;
                while (total_read < len) {
                    int n = recv(sockfd, value + total_read, len - total_read, 0);
                    if (n <= 0) {
                        printf(COLOR_RED "✗ Failed to read bulk string\n" COLOR_RESET);
                        goto cleanup;
                    }
                    total_read += n;
                }
                value[len] = '\0';
                
                /* Read trailing \r\n */
                char trailing[3];
                recv(sockfd, trailing, 2, 0);
                
                print_response_colored(response, value);
            }
            
        } else {
            /* Unknown format */
            print_response_colored(response, NULL);
        }
        
        /* Check if we sent QUIT */
        if (strncmp(input, "QUIT", 4) == 0) {
            break;
        }
    }
    
cleanup:
    /* Cleanup */
    close_socket(sockfd);
    printf("\n");
   
    printf(" " COLOR_BRIGHT_WHITE "Disconnected from DragonFlyDB" COLOR_BRIGHT_CYAN "      \n");
    printf("  " COLOR_DIM "Thank you for using DragonFlyDB!" COLOR_BRIGHT_CYAN "   \n");
   
    printf(COLOR_RESET);
    printf("\n");
    return 0;
}