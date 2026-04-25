# Part 3: Beautiful CLI with ASCII Art and Colors

## Overview

Transform the CLI from boring text to a beautiful, colorful interface with:
- ASCII Dragon art on startup
- Colored output (green for success, red for errors, cyan for values)
- Command history (arrow keys)
- Tab completion
- Progress indicators
- Formatted output

---

## Step 1: Add ANSI Color Support

Create `colors.h`:

```c
#ifndef COLORS_H
#define COLORS_H

/* ANSI Color Codes */
#ifdef _WIN32
    /* Windows 10+ supports ANSI colors */
    #include <windows.h>
    static void enable_ansi_colors(void) {
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD dwMode = 0;
        GetConsoleMode(hOut, &dwMode);
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOut, dwMode);
    }
#else
    static void enable_ansi_colors(void) { /* No-op on Unix */ }
#endif

/* Color codes */
#define COLOR_RESET   "\033[0m"
#define COLOR_BOLD    "\033[1m"
#define COLOR_DIM     "\033[2m"

/* Foreground colors */
#define COLOR_BLACK   "\033[30m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_WHITE   "\033[37m"

/* Bright foreground colors */
#define COLOR_BRIGHT_BLACK   "\033[90m"
#define COLOR_BRIGHT_RED     "\033[91m"
#define COLOR_BRIGHT_GREEN   "\033[92m"
#define COLOR_BRIGHT_YELLOW  "\033[93m"
#define COLOR_BRIGHT_BLUE    "\033[94m"
#define COLOR_BRIGHT_MAGENTA "\033[95m"
#define COLOR_BRIGHT_CYAN    "\033[96m"
#define COLOR_BRIGHT_WHITE   "\033[97m"

/* Background colors */
#define BG_BLACK   "\033[40m"
#define BG_RED     "\033[41m"
#define BG_GREEN   "\033[42m"
#define BG_YELLOW  "\033[43m"
#define BG_BLUE    "\033[44m"
#define BG_MAGENTA "\033[45m"
#define BG_CYAN    "\033[46m"
#define BG_WHITE   "\033[47m"

#endif /* COLORS_H */
```

---

## Step 2: Create ASCII Art Header

Create `ascii_art.h`:

```c
#ifndef ASCII_ART_H
#define ASCII_ART_H

#include "colors.h"
#include <stdio.h>

static void print_dragon_logo(void) {
    printf(COLOR_CYAN COLOR_BOLD);
    printf("\n");
    printf("    ____                                   ________         ____  ____ \n");
    printf("   / __ \\_________ _____ _____  ____  ____/ ____/ /_  __   / __ \\/ __ )\n");
    printf("  / / / / ___/ __ `/ __ `/ __ \\/ __ \\/ __/ /_  / / / / /  / / / / __  |\n");
    printf(" / /_/ / /  / /_/ / /_/ / /_/ / / / / /_/ __/ / / /_/ /  / /_/ / /_/ / \n");
    printf("/_____/_/   \\__,_/\\__, /\\____/_/ /_/\\__/_/   /_/\\__, /  /_____/_____/  \n");
    printf("                 /____/                        /____/                   \n");
    printf(COLOR_RESET);
    
    printf(COLOR_BRIGHT_BLUE);
    printf("\n");
    printf("                    🐉  Multi-threaded Key-Value Store  🐉\n");
    printf(COLOR_RESET);
    
    printf(COLOR_DIM);
    printf("                    Version 1.0.3 | Redis-Compatible\n");
    printf(COLOR_RESET);
    printf("\n");
}

static void print_dragon_ascii(void) {
    printf(COLOR_GREEN COLOR_BOLD);
    printf("\n");
    printf("                                 ______________\n");
    printf("                            ,===:'.,            `-._\n");
    printf("                                 `:.`---.__         `-._\n");
    printf("                                   `:.     `--.         `.\n");
    printf("                                     \\.        `.         `.\n");
    printf("                             (,,(,    \\.         `.   ____,-`.,\n");
    printf("                          (,'     `/   \\.   ,--.___`.\n");
    printf("                      ,  ,'  ,--.  `,   \\.;'         `\n");
    printf("                       `{D, {    \\  :    \\;\n");
    printf("                         V,,'    /  /    //\n");
    printf("                         j;;    /  ,' ,-//.    ,---.      ,\n");
    printf("                         \\;'   /  ,' /  _  \\  /  _  \\   ,'/\n");
    printf("                               \\   `'  / \\  `'  / \\  `.' /\n");
    printf("                                `.___,'   `.__,'   `.__,'\n");
    printf(COLOR_RESET);
    printf("\n");
}

static void print_welcome_message(const char *host, int port) {
    printf(COLOR_BRIGHT_CYAN);
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║                                                                ║\n");
    printf("║  " COLOR_BRIGHT_WHITE "Connected to DragonFlyDB" COLOR_BRIGHT_CYAN "                                   ║\n");
    printf("║  " COLOR_WHITE "Server: " COLOR_YELLOW "%-50s" COLOR_BRIGHT_CYAN " ║\n", host);
    printf("║  " COLOR_WHITE "Port:   " COLOR_YELLOW "%-50d" COLOR_BRIGHT_CYAN " ║\n", port);
    printf("║                                                                ║\n");
    printf("║  " COLOR_DIM "Type 'help' for available commands" COLOR_BRIGHT_CYAN "                          ║\n");
    printf("║  " COLOR_DIM "Type 'quit' or press Ctrl+C to exit" COLOR_BRIGHT_CYAN "                        ║\n");
    printf("║                                                                ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n");
    printf(COLOR_RESET);
    printf("\n");
}

static void print_help(void) {
    printf(COLOR_BRIGHT_YELLOW COLOR_BOLD);
    printf("\n╔═══════════════════════════════════════════════════════════════╗\n");
    printf("║                      AVAILABLE COMMANDS                       ║\n");
    printf("╚═══════════════════════════════════════════════════════════════╝\n");
    printf(COLOR_RESET);
    
    printf(COLOR_CYAN "  String Commands:\n" COLOR_RESET);
    printf("    " COLOR_GREEN "SET" COLOR_RESET " key value      - Set a key to a value\n");
    printf("    " COLOR_GREEN "GET" COLOR_RESET " key            - Get the value of a key\n");
    printf("    " COLOR_GREEN "DEL" COLOR_RESET " key            - Delete a key\n");
    printf("\n");
    
    printf(COLOR_CYAN "  Integer Commands:\n" COLOR_RESET);
    printf("    " COLOR_GREEN "INCR" COLOR_RESET " key           - Increment integer value\n");
    printf("    " COLOR_GREEN "DECR" COLOR_RESET " key           - Decrement integer value\n");
    printf("\n");
    
    printf(COLOR_CYAN "  Query Commands:\n" COLOR_RESET);
    printf("    " COLOR_GREEN "EXISTS" COLOR_RESET " key         - Check if key exists\n");
    printf("    " COLOR_GREEN "PING" COLOR_RESET "              - Test connection\n");
    printf("\n");
    
    printf(COLOR_CYAN "  Transaction Commands:\n" COLOR_RESET);
    printf("    " COLOR_GREEN "BEGIN" COLOR_RESET "             - Start a transaction\n");
    printf("    " COLOR_GREEN "COMMIT" COLOR_RESET "            - Commit a transaction\n");
    printf("\n");
    
    printf(COLOR_CYAN "  Control Commands:\n" COLOR_RESET);
    printf("    " COLOR_GREEN "QUIT" COLOR_RESET "              - Disconnect from server\n");
    printf("    " COLOR_GREEN "help" COLOR_RESET "              - Show this help message\n");
    printf("\n");
}

#endif /* ASCII_ART_H */
```

---

## Step 3: Update Client with Beautiful Output

Update `client.c`:

```c
#include "network.h"
#include "protocol.h"
#include "colors.h"
#include "ascii_art.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
        printf(COLOR_BRIGHT_MAGENTA "(integer) %lld\n" COLOR_RESET, num);
        
    } else {
        /* Unknown */
        printf("%s\n", response);
    }
}

int main(int argc, char *argv[]) {
    /* Enable ANSI colors */
    enable_ansi_colors();
    
    /* Parse command-line arguments */
    const char *host = DEFAULT_HOST;
    int port = DEFAULT_PORT;

    if (argc >= 2) {
        host = argv[1];
    }
    if (argc >= 3) {
        port = atoi(argv[2]);
    }

    /* Print logo */
    print_dragon_logo();
    print_dragon_ascii();
    
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

    /* Interactive command loop */
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
        
        /* Track transaction state */
        if (strncmp(input, "BEGIN", 5) == 0) {
            in_transaction = 1;
        } else if (strncmp(input, "COMMIT", 6) == 0) {
            in_transaction = 0;
        }

        /* Add protocol terminator \r\n */
        strcat(input, "\r\n");
        
        /* Send command to server */
        if (send_all(sockfd, input, strlen(input)) < 0) {
            printf(COLOR_RED "✗ Send failed\n" COLOR_RESET);
            break;
        }
        
        /* Receive response from server */
        memset(response, 0, sizeof(response));
        
        if (recv_line(sockfd, response, sizeof(response)) <= 0) {
            printf(COLOR_RED "✗ Server disconnected\n" COLOR_RESET);
            break;
        }

        /* Handle different response types */
        if (response[0] == '$') {
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
    printf(COLOR_BRIGHT_CYAN "╔════════════════════════════════════════╗\n");
    printf("║  " COLOR_BRIGHT_WHITE "Disconnected from DragonFlyDB" COLOR_BRIGHT_CYAN "      ║\n");
    printf("║  " COLOR_DIM "Thank you for using DragonFlyDB!" COLOR_BRIGHT_CYAN "   ║\n");
    printf("╚════════════════════════════════════════╝\n");
    printf(COLOR_RESET);
    printf("\n");
    return 0;
}
```

---

## Step 4: Add Command History (Optional - Advanced)

For command history with arrow keys, use the `readline` library:

```c
/* Add to client.c */
#ifdef USE_READLINE
    #include <readline/readline.h>
    #include <readline/history.h>
#endif

/* In main loop, replace fgets with: */
#ifdef USE_READLINE
    char *line = readline("");
    if (!line) break;
    
    if (strlen(line) > 0) {
        add_history(line);
        strncpy(input, line, sizeof(input) - 1);
    }
    free(line);
#else
    if (!fgets(input, sizeof(input), stdin)) {
        break;
    }
    input[strcspn(input, "\n")] = '\0';
#endif
```

Compile with readline:
```bash
gcc -o dragonfly-cli client.c network.c protocol.c -lreadline -pthread
```

---

## Step 5: Update package.json

Add colors.h and ascii_art.h to the npm package:

```json
{
  "files": [
    "client.c",
    "network.c",
    "network.h",
    "protocol.c",
    "protocol.h",
    "colors.h",
    "ascii_art.h",
    "bin/",
    "install.js",
    "README.md"
  ]
}
```

---

## Step 6: Test the Beautiful CLI

```bash
# Compile locally
gcc -o dragonfly-cli client.c network.c protocol.c -pthread -lws2_32  # Windows
gcc -o dragonfly-cli client.c network.c protocol.c -pthread            # Linux

# Run it
./dragonfly-cli 51.21.226.149 6379
```

You should see:
1. ✅ Beautiful ASCII dragon logo
2. ✅ Colored output (green for success, red for errors)
3. ✅ Formatted boxes and borders
4. ✅ Transaction indicator in prompt
5. ✅ Clear command to refresh screen

---

## Step 7: Publish Updated Version

```bash
# Update version
npm version patch

# Publish
npm publish

# Test installation
npm install -g dragonfly-cli@latest

# Run
dragonfly-cli 51.21.226.149 6379
```

---

## Example Session

```
    ____                                   ________         ____  ____ 
   / __ \_________ _____ _____  ____  ____/ ____/ /_  __   / __ \/ __ )
  / / / / ___/ __ `/ __ `/ __ \/ __ \/ __/ /_  / / / / /  / / / / __  |
 / /_/ / /  / /_/ / /_/ / /_/ / / / / /_/ __/ / / /_/ /  / /_/ / /_/ / 
/_____/_/   \__,_/\__, /\____/_/ /_/\__/_/   /_/\__, /  /_____/_____/  
                 /____/                        /____/                   

                    🐉  Multi-threaded Key-Value Store  🐉
                    Version 1.0.3 | Redis-Compatible

╔════════════════════════════════════════════════════════════════╗
║                                                                ║
║  Connected to DragonFlyDB                                      ║
║  Server: 51.21.226.149                                         ║
║  Port:   6379                                                  ║
║                                                                ║
║  Type 'help' for available commands                            ║
║  Type 'quit' or press Ctrl+C to exit                           ║
║                                                                ║
╚════════════════════════════════════════════════════════════════╝

dragonfly> SET user:1 alice
✓ OK

dragonfly> GET user:1
"alice"

dragonfly> INCR counter
(integer) 1

dragonfly> EXISTS user:1
(integer) 1

dragonfly> help

╔═══════════════════════════════════════════════════════════════╗
║                      AVAILABLE COMMANDS                       ║
╚═══════════════════════════════════════════════════════════════╝
  String Commands:
    SET key value      - Set a key to a value
    GET key            - Get the value of a key
    DEL key            - Delete a key
...
```

---

## Summary

You now have:
- ✅ Beautiful ASCII dragon logo
- ✅ Colored output (ANSI colors)
- ✅ Formatted help and welcome messages
- ✅ Transaction indicator in prompt
- ✅ Professional-looking CLI

Your DragonFlyDB client now looks as good as it performs! 🐉
