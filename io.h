#ifndef IO_H
#define IO_H

#include "types.h"

// IO operations
Response io_set(ResponseQueue *reply_q, const char *key, const char *value);
Response io_get(ResponseQueue *reply_q, const char *key);
Response io_del(ResponseQueue *rq, const char *key);
Response io_exists(ResponseQueue *rq, const char *key);
Response io_incr(ResponseQueue *rq, const char *key);
Response io_decr(ResponseQueue *rq, const char *key);

#endif // IO_H
