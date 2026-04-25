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
    printf("+================================================================+\n");
    printf("|                                                                |\n");
    printf("|  " COLOR_BRIGHT_WHITE "Connected to DragonFlyDB" COLOR_BRIGHT_CYAN "                                   |\n");
    printf("|  " COLOR_WHITE "Server: " COLOR_YELLOW "%-50s" COLOR_BRIGHT_CYAN " |\n", host);
    printf("|  " COLOR_WHITE "Port:   " COLOR_YELLOW "%-50d" COLOR_BRIGHT_CYAN " |\n", port);
    printf("|                                                                |\n");
    printf("|  " COLOR_DIM "Type 'help' for available commands" COLOR_BRIGHT_CYAN "                          |\n");
    printf("|  " COLOR_DIM "Type 'quit' or press Ctrl+C to exit" COLOR_BRIGHT_CYAN "                        |\n");
    printf("|                                                                |\n");
    printf("+================================================================+\n");
    printf(COLOR_RESET);
    printf("\n");
}

static void print_help(void) {
    printf(COLOR_BRIGHT_YELLOW COLOR_BOLD);
    printf("\n+===============================================================+\n");
    printf("|                      AVAILABLE COMMANDS                       |\n");
    printf("+===============================================================+\n");
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

