# include "broker.h"

izq_broker_t    *broker_create(void) {
    izq_broker_t*   broker;

    broker = malloc(sizeof(izq_broker_t));
    if (!broker)
        return NULL;

    memset(broker, 0, sizeof(*broker));
    return broker;
}

void             broker_destroy(izq_broker_t *broker) {
    if (!broker)
        return;

    size_t  count = atomic_load(&broker->count);
    for (size_t i = 0; i < count; i++)
            queue_destroy(broker->queues[i]);

    free(broker);
}

izq_queue_t     *broker_add_topic(izq_broker_t *broker, const char *name, size_t capacity) {
    if (!broker || !name || !capacity)
        return NULL;

    while (atomic_flag_test_and_set(&broker->lock));

    for (size_t i = 0; i < atomic_load(&broker->count); i++) {
        if (strncmp(broker->topics[i], name, 48) == 0) {
            atomic_flag_clear(&broker->lock);
            return broker->queues[i];
        }
    }

    size_t          idx = atomic_load(&broker->count);
    if (idx >= 32) {
        atomic_flag_clear(&broker->lock);
        return NULL;
    }

    izq_queue_t     *queue = queue_create(name, capacity);
    if (!queue) {
        atomic_flag_clear(&broker->lock);
        return NULL;
    }

    strncpy(broker->topics[idx], name, 47);
    broker->topics[idx][47] = '\0';
    broker->queues[idx] = queue;
    atomic_fetch_add(&broker->count, 1);

    atomic_flag_clear(&broker->lock);
    return queue;
}

izq_queue_t     *broker_find_topic(izq_broker_t *broker, const char *topic)
{
    size_t  count;

    if (!broker || !topic)
        return NULL;

    count = atomic_load(&broker->count);
    for (size_t i = 0; i < count; i++)
        if (strncmp(broker->topics[i], topic, 48) == 0)
            return broker->queues[i];

    return NULL;
}

int              broker_publish(izq_broker_t *broker, const char *topic, imq_message_t *msg) {
    
    izq_queue_t     *queue = broker_find_topic(broker, topic);

    if (!queue)
        return 0;

    return (queue_enqueue(queue, msg));
}

imq_message_t   *broker_consume(izq_broker_t *broker, const char *topic) {
    izq_queue_t     *queue = broker_find_topic(broker, topic);

    if (!queue)
        return NULL;

    return (queue_dequeue(queue));
}

void    broker_print(izq_broker_t *broker) {
    if (!broker)
        return;

    size_t count = atomic_load(&broker->count);
    printf("[IZMQ BROKER]  topics=%zu\n", count);
    for (size_t i = 0; i < count; i++)
        printf("\t[%zu]  %-24s  depth=%-4zu  capacity=%-4zu\n",
            i,
            broker->topics[i],
            queue_depth(broker->queues[i]),
            broker->queues[i]->capacity
        );
}