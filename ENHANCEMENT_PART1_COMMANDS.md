# Part 1: Adding More Commands

## Overview
We'll add these Redis-compatible commands:
- **PING** - Test connection
- **DEL key** - Delete a key
- **EXISTS key** - Check if key exists
- **INCR key** - Increment integer value
- **DECR key** - Decrement integer value
- **KEYS pattern** - List keys matching pattern
- **FLUSHALL** - Delete all keys

---

## Step 1: Update Protocol Header (protocol.h)

Add new command types to the enum:

```c
// protocol.h
typedef enum {
    CMD_TYPE_INVALID = 0,
    CMD_TYPE_SET,
    CMD_TYPE_GET,
    CMD_TYPE_DEL,        // NEW
    CMD_TYPE_EXISTS,     // NEW
    CMD_TYPE_PING,       // NEW
    CMD_TYPE_INCR,       // NEW
    CMD_TYPE_DECR,       // NEW
    CMD_TYPE_KEYS,       // NEW
    CMD_TYPE_FLUSHALL,   // NEW
    CMD_TYPE_BEGIN,
    CMD_TYPE_COMMIT,
    CMD_TYPE_QUIT
} CommandType;
```

---

## Step 2: Update Protocol Parser (protocol.c)

Add parsing logic for new commands:

```c
// protocol.c - in parse_command() function

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

/* ── KEYS pattern ─────────────────────────────────────────────────── */
else if (strcmp(cmd_str, "KEYS") == 0) {
    cmd->type = CMD_TYPE_KEYS;
    
    // Parse pattern (default to "*" if not provided)
    char *pattern = strtok_r(NULL, " \t", &saveptr);
    if (pattern) {
        strncpy(cmd->key, pattern, MAX_KEY_LEN - 1);
    } else {
        strcpy(cmd->key, "*");
    }
    
    return 0;
}

/* ── FLUSHALL ─────────────────────────────────────────────────────── */
else if (strcmp(cmd_str, "FLUSHALL") == 0) {
    cmd->type = CMD_TYPE_FLUSHALL;
    return 0;
}
```

---

## Step 3: Add Response Formatters (protocol.c)

Add new response formatting functions:

```c
// protocol.c

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
```

Add declarations to protocol.h:

```c
// protocol.h
void format_integer_response(char *buf, size_t len, long long value);
void format_array_response(char *buf, size_t len, char **items, int count);
```

---

## Step 4: Update IO Operations (io.h and io.c)

Add new IO functions:

```c
// io.h
Response io_del(ResponseQueue *rq, const char *key);
Response io_exists(ResponseQueue *rq, const char *key);
Response io_incr(ResponseQueue *rq, const char *key);
Response io_decr(ResponseQueue *rq, const char *key);
```

```c
// io.c

Response io_del(ResponseQueue *rq, const char *key) {
    int shard_id = hash_key(key);
    
    Request req;
    req.type = CMD_DEL;
    strncpy(req.key, key, MAX_KEY_LEN - 1);
    req.key[MAX_KEY_LEN - 1] = '\0';
    req.response_queue = rq;
    
    request_queue_push(&shards[shard_id].req_queue, req);
    
    return response_queue_pop(rq);
}

Response io_exists(ResponseQueue *rq, const char *key) {
    int shard_id = hash_key(key);
    
    Request req;
    req.type = CMD_EXISTS;
    strncpy(req.key, key, MAX_KEY_LEN - 1);
    req.key[MAX_KEY_LEN - 1] = '\0';
    req.response_queue = rq;
    
    request_queue_push(&shards[shard_id].req_queue, req);
    
    return response_queue_pop(rq);
}

Response io_incr(ResponseQueue *rq, const char *key) {
    int shard_id = hash_key(key);
    
    Request req;
    req.type = CMD_INCR;
    strncpy(req.key, key, MAX_KEY_LEN - 1);
    req.key[MAX_KEY_LEN - 1] = '\0';
    req.response_queue = rq;
    
    request_queue_push(&shards[shard_id].req_queue, req);
    
    return response_queue_pop(rq);
}

Response io_decr(ResponseQueue *rq, const char *key) {
    int shard_id = hash_key(key);
    
    Request req;
    req.type = CMD_DECR;
    strncpy(req.key, key, MAX_KEY_LEN - 1);
    req.key[MAX_KEY_LEN - 1] = '\0';
    req.response_queue = rq;
    
    request_queue_push(&shards[shard_id].req_queue, req);
    
    return response_queue_pop(rq);
}
```

---

## Step 5: Update Shard Worker (shard.c)

Add handling for new commands in the shard worker:

```c
// shard.c - in shard_worker() function

case CMD_DEL: {
    int found = ht_delete(&shard->table, req.key);
    
    resp.ack = found ? ACK_OK : ACK_NOT_FOUND;
    resp.value[0] = '\0';
    
    printf("[Shard %d] DEL  %-20s → %s\n", 
           shard->id, req.key, found ? "deleted" : "not found");
    break;
}

case CMD_EXISTS: {
    char *value = ht_get(&shard->table, req.key);
    
    resp.ack = value ? ACK_OK : ACK_NOT_FOUND;
    resp.value[0] = '\0';
    
    printf("[Shard %d] EXISTS %-20s → %s\n", 
           shard->id, req.key, value ? "yes" : "no");
    break;
}

case CMD_INCR: {
    char *value = ht_get(&shard->table, req.key);
    long long num = 0;
    
    if (value) {
        num = atoll(value);
    }
    num++;
    
    char new_value[MAX_VAL_LEN];
    snprintf(new_value, sizeof(new_value), "%lld", num);
    ht_set(&shard->table, req.key, new_value);
    
    resp.ack = ACK_OK;
    strncpy(resp.value, new_value, MAX_VAL_LEN - 1);
    
    printf("[Shard %d] INCR %-20s → %lld\n", shard->id, req.key, num);
    break;
}

case CMD_DECR: {
    char *value = ht_get(&shard->table, req.key);
    long long num = 0;
    
    if (value) {
        num = atoll(value);
    }
    num--;
    
    char new_value[MAX_VAL_LEN];
    snprintf(new_value, sizeof(new_value), "%lld", num);
    ht_set(&shard->table, req.key, new_value);
    
    resp.ack = ACK_OK;
    strncpy(resp.value, new_value, MAX_VAL_LEN - 1);
    
    printf("[Shard %d] DECR %-20s → %lld\n", shard->id, req.key, num);
    break;
}
```

---

## Step 6: Update Server (server.c)

Add command handlers in client_handler():

```c
// server.c - in client_handler() function

case CMD_TYPE_PING: {
    format_ok_response(send_buf, sizeof(send_buf));
    send_all(client_fd, send_buf, strlen(send_buf));
    break;
}

case CMD_TYPE_DEL: {
    Response resp = io_del(&rq, cmd.key);
    if (resp.ack == ACK_OK) {
        format_integer_response(send_buf, sizeof(send_buf), 1);
    } else {
        format_integer_response(send_buf, sizeof(send_buf), 0);
    }
    send_all(client_fd, send_buf, strlen(send_buf));
    break;
}

case CMD_TYPE_EXISTS: {
    Response resp = io_exists(&rq, cmd.key);
    if (resp.ack == ACK_OK) {
        format_integer_response(send_buf, sizeof(send_buf), 1);
    } else {
        format_integer_response(send_buf, sizeof(send_buf), 0);
    }
    send_all(client_fd, send_buf, strlen(send_buf));
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
    send_all(client_fd, send_buf, strlen(send_buf));
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
    send_all(client_fd, send_buf, strlen(send_buf));
    break;
}
```

---

## Step 7: Update Client (client.c)

Add response handling for integer responses:

```c
// client.c - in main() function, after receiving response

else if (response[0] == ':') {
    /* Integer response: :123 */
    long long value = atoll(response + 1);
    printf("(integer) %lld\n", value);
}
```

Update help command:

```c
if (strcmp(input, "help") == 0) {
    printf("Commands:\n");
    printf("  SET key value    - Set a key to a value\n");
    printf("  GET key          - Get the value of a key\n");
    printf("  DEL key          - Delete a key\n");
    printf("  EXISTS key       - Check if key exists\n");
    printf("  INCR key         - Increment integer value\n");
    printf("  DECR key         - Decrement integer value\n");
    printf("  PING             - Test connection\n");
    printf("  BEGIN            - Start a transaction\n");
    printf("  COMMIT           - Commit a transaction\n");
    printf("  QUIT             - Disconnect from server\n");
    printf("  help             - Show this help\n");
    continue;
}
```

---

## Testing the New Commands

```bash
dragonfly> PING
OK

dragonfly> SET counter 10
OK

dragonfly> INCR counter
(integer) 11

dragonfly> INCR counter
(integer) 12

dragonfly> DECR counter
(integer) 11

dragonfly> EXISTS counter
(integer) 1

dragonfly> EXISTS nonexistent
(integer) 0

dragonfly> DEL counter
(integer) 1

dragonfly> EXISTS counter
(integer) 0
```

---

## Summary

You've now added 5 new commands:
- ✅ PING - Connection test
- ✅ DEL - Delete keys
- ✅ EXISTS - Check existence
- ✅ INCR/DECR - Integer operations

Next: Part 2 - epoll-based server for high performance!
