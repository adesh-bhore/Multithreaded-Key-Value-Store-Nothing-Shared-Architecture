#ifndef IO_H
#define IO_H

#include "types.h"

// IO operations
Response io_set(ResponseQueue *reply_q, const char *key, const char *value);
Response io_get(ResponseQueue *reply_q, const char *key);

#endif // IO_H
