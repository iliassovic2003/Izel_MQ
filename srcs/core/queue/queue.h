#ifndef     QUEUE_H
# define    QUEUE_H

# include   <stddef.h>
# include   <stdatomic.h>
# include   <stdlib.h>
# include   <string.h>
# include   <stdio.h>
# include   "../message/message.h"

typedef struct {
    char                        name[48];
    atomic_uint_fast64_t        head;
    atomic_uint_fast64_t        tail;
    size_t                      capacity;
    _Atomic(imq_message_t *)    **slots;
}                               izq_queue_t;

izq_queue_t     *queue_create(const char *name, size_t capacity);
void             queue_destroy(izq_queue_t *queue);

int              queue_enqueue(izq_queue_t *queue, imq_message_t *msg);
imq_message_t   *queue_dequeue(izq_queue_t *queue);

int             queue_is_empty(const izq_queue_t *queue);
int             queue_is_full(const izq_queue_t *queue);
size_t          queue_depth(const izq_queue_t *queue);

void             queue_print(const izq_queue_t *queue);

# endif