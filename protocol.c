#include "protocol.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

/* Windows compatibility for strtok_r */
#ifdef _WIN32
    #include <stdlib.h>
    /* strtok_s is available in Windows but needs proper declaration */
    char *strtok_s(char *str, const char *delim, char **context);
    #define strtok_r strtok_s
#endif


//helper to upper case converstion 
static void str_toupper(char *str) {
    for (int i = 0; str[i]; i++) {
        str[i] = toupper((unsigned char)str[i]);
    }
}

// helper for trimming spaces 

static char* trim(char *str) {
    // Trim leading
    while (isspace((unsigned char)*str)) str++;
    
    // Trim trailing
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    *(end + 1) = '\0';
    
    return str;
}


int parse_command(const char *line, Command *cmd) {
    /* Initialize command structure */
    memset(cmd, 0, sizeof(*cmd));
    cmd->type = CMD_TYPE_INVALID;
    
    /* Make a mutable copy for parsing */
    char buf[MAX_CMD_LEN];
    strncpy(buf, line, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    /* Trim whitespace */
    char *trimmed = trim(buf);
    if (strlen(trimmed) == 0) {
        return -1;  // Empty command
    }

    /* Parse command type (first token) */
    char *saveptr;
    char *cmd_str = strtok_r(trimmed, " \t", &saveptr);
    if (!cmd_str) return -1;
    
    /* Convert to uppercase for case-insensitive matching */
    str_toupper(cmd_str);

    

    /* ── SET key value ────────────────────────────────────────────────── */
    if (strcmp(cmd_str, "SET") == 0) {
        cmd->type = CMD_TYPE_SET;
        
        // Parse key
        char *key = strtok_r(NULL, " \t", &saveptr);
        if (!key) {
            fprintf(stderr, "[Protocol] SET missing key\n");
            return -1;
        }
        strncpy(cmd->key, key, MAX_KEY_LEN - 1);
        
        // Parse value (rest of line, may contain spaces)
        char *value = strtok_r(NULL, "", &saveptr);
        if (!value) {
            fprintf(stderr, "[Protocol] SET missing value\n");
            return -1;
        }
        value = trim(value);  // Trim leading/trailing spaces
        strncpy(cmd->value, value, MAX_VAL_LEN - 1);
        
        return 0;
    }

     /* ── GET key ──────────────────────────────────────────────────────── */
    else if (strcmp(cmd_str, "GET") == 0) {
        cmd->type = CMD_TYPE_GET;
        
        // Parse key
        char *key = strtok_r(NULL, " \t", &saveptr);
        if (!key) {
            fprintf(stderr, "[Protocol] GET missing key\n");
            return -1;
        }
        strncpy(cmd->key, key, MAX_KEY_LEN - 1);
        
        return 0;
    }

     /* ── BEGIN ────────────────────────────────────────────────────────── */
    else if (strcmp(cmd_str, "BEGIN") == 0) {
        cmd->type = CMD_TYPE_BEGIN;
        return 0;
    }
    
    /* ── COMMIT ───────────────────────────────────────────────────────── */
    else if (strcmp(cmd_str, "COMMIT") == 0) {
        cmd->type = CMD_TYPE_COMMIT;
        return 0;
    }
    
    /* ── QUIT ─────────────────────────────────────────────────────────── */
    else if (strcmp(cmd_str, "QUIT") == 0) {
        cmd->type = CMD_TYPE_QUIT;
        return 0;
    }

    /* ── PING ─────────────────────────────────────────────────────────── */
    else if (strcmp(cmd_str, "PING") == 0) {
        cmd->type = CMD_TYPE_PING;
        return 0;
    }

    /* ── DEL key ──────────────────────────────────────────────────────── */
    else if (strcmp(cmd_str, "DEL") == 0) {
        cmd->type = CMD_TYPE_DEL;
        
        // Parse key
        char *key = strtok_r(NULL, " \t", &saveptr);
        if (!key) {
            fprintf(stderr, "[Protocol] DEL missing key\n");
            return -1;
        }
        strncpy(cmd->key, key, MAX_KEY_LEN - 1);
        
        return 0;
    }

    /* ── EXISTS key ───────────────────────────────────────────────────── */
    else if (strcmp(cmd_str, "EXISTS") == 0) {
        cmd->type = CMD_TYPE_EXISTS;
        
        // Parse key
        char *key = strtok_r(NULL, " \t", &saveptr);
        if (!key) {
            fprintf(stderr, "[Protocol] EXISTS missing key\n");
            return -1;
        }
        strncpy(cmd->key, key, MAX_KEY_LEN - 1);
        
        return 0;
    }

    /* ── INCR key ─────────────────────────────────────────────────────── */
    else if (strcmp(cmd_str, "INCR") == 0) {
        cmd->type = CMD_TYPE_INCR;
        
        // Parse key
        char *key = strtok_r(NULL, " \t", &saveptr);
        if (!key) {
            fprintf(stderr, "[Protocol] INCR missing key\n");
            return -1;
        }
        strncpy(cmd->key, key, MAX_KEY_LEN - 1);
        
        return 0;
    }

    /* ── DECR key ─────────────────────────────────────────────────────── */
    else if (strcmp(cmd_str, "DECR") == 0) {
        cmd->type = CMD_TYPE_DECR;
        
        // Parse key
        char *key = strtok_r(NULL, " \t", &saveptr);
        if (!key) {
            fprintf(stderr, "[Protocol] DECR missing key\n");
            return -1;
        }
        strncpy(cmd->key, key, MAX_KEY_LEN - 1);
        
        return 0;
    }

    /* ── FLUSHALL ─────────────────────────────────────────────────────── */
    else if (strcmp(cmd_str, "FLUSHALL") == 0) {
        cmd->type = CMD_TYPE_FLUSHALL;
        return 0;
    }
            
    /* ── Unknown command ──────────────────────────────────────────────── */
    else {
        fprintf(stderr, "[Protocol] Unknown command: %s\n", cmd_str);
        return -1;
    }
}


/* ══════════════════════════════════════════════════════════════════════════
 * RESPONSE FORMATTING
 * ══════════════════════════════════════════════════════════════════════════ */

void format_ok_response(char *buf, size_t len) {
    snprintf(buf, len, "+OK\r\n");
}

void format_error_response(char *buf, size_t len, const char *msg) {
    snprintf(buf, len, "-ERR %s\r\n", msg);
}

void format_bulk_string(char *buf, size_t len, const char *value) {
   
    size_t value_len = strlen(value);
    /* Use %lu for size_t on Windows (MinGW), %zu on Unix */
    #ifdef _WIN32
        snprintf(buf, len, "$%lu\r\n%s\r\n", (unsigned long)value_len, value);
    #else
        snprintf(buf, len, "$%zu\r\n%s\r\n", value_len, value);
    #endif
}

void format_null_response(char *buf, size_t len) {
    
    snprintf(buf, len, "$-1\r\n");
}

/* Format integer response: :123\r\n */
void format_integer_response(char *buf, size_t len, long long value) {
    #ifdef _WIN32
        snprintf(buf, len, ":%lld\r\n", value);
    #else
        snprintf(buf, len, ":%lld\r\n", value);
    #endif
}

/* Format array response: *2\r\n$3\r\nkey\r\n$5\r\nvalue\r\n */
void format_array_response(char *buf, size_t len, char **items, int count) {
    int offset = 0;
    
    // Array header
    offset += snprintf(buf + offset, len - offset, "*%d\r\n", count);
    
    // Each item as bulk string
    for (int i = 0; i < count && offset < len; i++) {
        size_t item_len = strlen(items[i]);
        #ifdef _WIN32
            offset += snprintf(buf + offset, len - offset, "$%lu\r\n%s\r\n", 
                             (unsigned long)item_len, items[i]);
        #else
            offset += snprintf(buf + offset, len - offset, "$%zu\r\n%s\r\n", 
                             item_len, items[i]);
        #endif
    }
}
    