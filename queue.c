#include "queue.h"
#include <stdatomic.h>

// Global atomic ID generators
static atomic_uint_fast64_t g_next_request_id = 1;
static atomic_uint_fast64_t g_next_tx_id      = 1;

uint64_t next_request_id(void) {
    return atomic_fetch_add(&g_next_request_id, 1);
}

uint64_t next_tx_id(void) {
    return atomic_fetch_add(&g_next_tx_id, 1);
}

// Response queue operations
void response_queue_init(ResponseQueue *q) {
    q->front = q->rear = q->count = 0;
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->ready, NULL);
}

void response_queue_destroy(ResponseQueue *q) {
    pthread_mutex_destroy(&q->lock);
    pthread_cond_destroy(&q->ready);
}

void response_enqueue(ResponseQueue *q, Response resp) {
    pthread_mutex_lock(&q->lock);

    if (q->count < MAX_QUEUE_SIZE) {
        q->items[q->rear] = resp;
        q->rear = (q->rear + 1) % MAX_QUEUE_SIZE;
        q->count++;
        pthread_cond_signal(&q->ready);
    }

    pthread_mutex_unlock(&q->lock);
}

Response response_dequeue(ResponseQueue *q) {
    pthread_mutex_lock(&q->lock);

    while (q->count == 0) {
        pthread_cond_wait(&q->ready, &q->lock);
    }

    Response resp = q->items[q->front];
    q->front = (q->front + 1) % MAX_QUEUE_SIZE;
    q->count--;

    pthread_mutex_unlock(&q->lock);

    return resp;
}

// Request queue operations
void request_queue_init(RequestQueue *q) {
    q->front = q->rear = q->count = 0;
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->ready, NULL);
}

void request_queue_destroy(RequestQueue *q) {
    pthread_mutex_destroy(&q->lock);
    pthread_cond_destroy(&q->ready);
}

void request_enqueue(RequestQueue *q, Request req) {
    pthread_mutex_lock(&q->lock);

    if (q->count < MAX_QUEUE_SIZE) {
        q->items[q->rear] = req;
        q->rear = (q->rear + 1) % MAX_QUEUE_SIZE;
        q->count++;
        pthread_cond_signal(&q->ready);
    }

    pthread_mutex_unlock(&q->lock);
}

Request request_dequeue(RequestQueue *q) {
    pthread_mutex_lock(&q->lock);

    while (q->count == 0) {
        pthread_cond_wait(&q->ready, &q->lock);
    }

    Request req = q->items[q->front];
    q->front = (q->front + 1) % MAX_QUEUE_SIZE;
    q->count--;

    pthread_mutex_unlock(&q->lock);

    return req;
}


