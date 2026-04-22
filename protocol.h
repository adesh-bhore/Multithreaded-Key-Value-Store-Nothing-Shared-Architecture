#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stddef.h>

#define MAX_CMD_LEN  256
#define MAX_KEY_LEN  256
#define MAX_VAL_LEN  256



/* ── Command Types ──────────────────────────────────────────────────────── */

typedef enum {
    CMD_TYPE_SET,       // SET key value
    CMD_TYPE_GET,       // GET key
    CMD_TYPE_BEGIN,     // BEGIN (start transaction)
    CMD_TYPE_COMMIT,    // COMMIT (end transaction)
    CMD_TYPE_QUIT,      // QUIT (disconnect)
    CMD_TYPE_INVALID    // Parse error
} CmdType;

/* ── Parsed Command Structure ───────────────────────────────────────────── */

typedef struct {
    CmdType type;
    char key[MAX_KEY_LEN];
    char value[MAX_VAL_LEN];
} Command;


int parse_command(const char *line, Command *cmd);

void format_ok_response(char *buf, size_t len);

void format_error_response(char *buf, size_t len, const char *msg);


void format_bulk_string(char *buf, size_t len, const char *value);


void format_null_response(char *buf, size_t len);

#endif // PROTOCOL_H