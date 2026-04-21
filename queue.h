#ifndef QUEUE_H
#define QUEUE_H

#include "types.h"

// Response queue operations
void response_queue_init(ResponseQueue *q);
void response_queue_destroy(ResponseQueue *q);
void response_enqueue(ResponseQueue *q, Response resp);
Response response_dequeue(ResponseQueue *q);

// Request queue operations
void request_queue_init(RequestQueue *q);
void request_queue_destroy(RequestQueue *q);
void request_enqueue(RequestQueue *q, Request req);
Request request_dequeue(RequestQueue *q);

// Global ID generators
uint64_t next_request_id(void);
uint64_t next_tx_id(void);

#endif // QUEUE_H
