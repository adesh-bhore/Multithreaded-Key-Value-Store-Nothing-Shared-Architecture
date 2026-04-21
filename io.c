#include "io.h"
#include "shard.h"
#include "hashtable.h"
#include "queue.h"
#include <string.h>

Response io_set(ResponseQueue *reply_q, const char *key, const char *value) {
    int shard_id = key_to_shard(key);
    Shard *shard = get_shard(shard_id);

    Request req;
    req.request_id = next_request_id();
    req.type = CMD_SET;
    req.reply_to = reply_q;
    strncpy(req.key, key, KEY_LEN - 1);
    strncpy(req.value, value, VAL_LEN - 1);
    req.key[KEY_LEN - 1] = '\0';
    req.value[VAL_LEN - 1] = '\0';

    request_enqueue(&shard->request_queue, req);

    return response_dequeue(reply_q);
}

Response io_get(ResponseQueue *reply_q, const char *key) {
    int shard_id = key_to_shard(key);
    Shard *shard = get_shard(shard_id);

    Request req;
    req.request_id = next_request_id();
    req.type = CMD_GET;
    req.reply_to = reply_q;
    strncpy(req.key, key, KEY_LEN - 1);
    req.key[KEY_LEN - 1] = '\0';

    request_enqueue(&shard->request_queue, req);

    return response_dequeue(reply_q);
}
