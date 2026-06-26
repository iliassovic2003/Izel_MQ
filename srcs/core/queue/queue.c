#include "queue.h"
#include "../message/message.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

izq_queue_t     *queue_create(const char *name, size_t capacity)
{
    izq_queue_t *q;

    if (!name || capacity == 0 || (capacity & (capacity - 1)) != 0)
        return NULL;

    q = malloc(sizeof(izq_queue_t));
    if (!q)
        return NULL;

    q->slots = malloc(sizeof(_Atomic(imq_message_t *)) * capacity);
    if (!q->slots)
    {
        free(q);
        return NULL;
    }
    memset(q->slots, 0, sizeof(_Atomic(imq_message_t *)) * capacity);

    strncpy(q->name, name, sizeof(q->name) - 1);
    q->name[sizeof(q->name) - 1] = '\0';

    atomic_init(&q->head, 0);
    atomic_init(&q->tail, 0);
    q->capacity = capacity;

    return q;
}

void            queue_destroy(izq_queue_t *queue)
{
    imq_message_t *msg;

    if (!queue)
        return;

    while (!queue_is_empty(queue))
    {
        msg = queue_dequeue(queue);
        if (msg)
            imq_msg_destroy(msg);
    }

    free(queue->slots);
    free(queue);
}

int             queue_enqueue(izq_queue_t *queue, imq_message_t *msg)
{
    uint64_t    expected;
    size_t      slot;

    if (!queue || !msg)
        return 0;

    while (1)
    {
        expected = atomic_load_explicit(&queue->head, memory_order_relaxed);

        if (expected - atomic_load_explicit(&queue->tail, memory_order_acquire)
                == queue->capacity)
            return 0;

        if (atomic_compare_exchange_weak_explicit(
                &queue->head,
                &expected,
                expected + 1,
                memory_order_relaxed,
                memory_order_relaxed))
            break;
    }

    slot = expected & (queue->capacity - 1);
    atomic_store_explicit((_Atomic(imq_message_t *) *)&queue->slots[slot], msg, memory_order_release);
    
    return 1;
}

imq_message_t   *queue_dequeue(izq_queue_t *queue)
{
    uint64_t        tail;
    size_t          slot;
    imq_message_t   *msg;

    if (!queue)
        return NULL;

    tail = atomic_load_explicit(&queue->tail, memory_order_relaxed);
    if (tail == atomic_load_explicit(&queue->head, memory_order_acquire))
        return NULL;

    slot = tail & (queue->capacity - 1);

    msg = atomic_load_explicit((_Atomic(imq_message_t *) *)&queue->slots[slot], memory_order_acquire);
    atomic_store_explicit((_Atomic(imq_message_t *) *)&queue->slots[slot], NULL, memory_order_relaxed);

    atomic_store_explicit(&queue->tail, tail + 1, memory_order_release);
    return msg;
}

int             queue_is_empty(const izq_queue_t *queue)
{
    return (atomic_load(&queue->head) == atomic_load(&queue->tail));
}

int             queue_is_full(const izq_queue_t *queue)
{
    return (atomic_load(&queue->head) - atomic_load(&queue->tail)
            == queue->capacity);
}

size_t          queue_depth(const izq_queue_t *queue)
{
    return (atomic_load(&queue->head) - atomic_load(&queue->tail));
}

void            queue_print(const izq_queue_t *queue)
{
    if (!queue)
        return;
    printf("[IZQ] name=%-24s  depth=%-4zu  capacity=%-4zu  "
           "head=%-8llu  tail=%-8llu  %s\n",
        queue->name,
        queue_depth(queue),
        queue->capacity,
        (unsigned long long)atomic_load(
            (atomic_uint_fast64_t *)&queue->head),
        (unsigned long long)atomic_load(
            (atomic_uint_fast64_t *)&queue->tail),
        queue_is_full(queue) ? "FULL" : "OK");
}