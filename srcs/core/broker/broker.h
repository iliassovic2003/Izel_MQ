#ifndef BROKER_H
# define BROKER_H

# include "../queue/queue.h"

typedef struct {
    izq_queue_t     *queues[32];
    char            topics[32][48];
    atomic_size_t   count;
    atomic_flag     lock;
}                   izq_broker_t;


izq_broker_t    *broker_create(void);
void             broker_destroy(izq_broker_t *broker);

izq_queue_t     *broker_add_topic(izq_broker_t *broker, const char *name, size_t capacity);
izq_queue_t     *broker_find_topic(izq_broker_t *broker, const char *name);

int              broker_publish(izq_broker_t *broker, const char *topic, imq_message_t *msg);
imq_message_t   *broker_consume(izq_broker_t *broker, const char *topic);

void             broker_print(izq_broker_t *broker);

#endif