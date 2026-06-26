#include "metrics/metrics.h"

void    *consumer_print(void *arg)
{
    izq_broker_t    *broker = (izq_broker_t *)arg;
    imq_message_t   *msg;

    while (1)
    {
        msg = broker_consume(broker, "metrics.cpu");
        if (msg)
        {
            printf("\n=== CPU ===\n%s\n", (char *)msg->payload);
            imq_msg_destroy(msg);
        }

        msg = broker_consume(broker, "metrics.memory");
        if (msg)
        {
            printf("\n=== MEMORY ===\n%s\n", (char *)msg->payload);
            imq_msg_destroy(msg);
        }

        msg = broker_consume(broker, "metrics.network");
        if (msg)
        {
            printf("\n=== NETWORK ===\n%s\n", (char *)msg->payload);
            imq_msg_destroy(msg);
        }

        sleep(1);
    }
    return NULL;
}

int     main(void)
{
    izq_broker_t    *broker;

    broker = broker_create();
    if (!broker)
    {
        printf("[FAIL] Failed to create broker\n");
        return 1;
    }

    broker_add_topic(broker, "metrics.cpu",  1024);
    broker_add_topic(broker, "metrics.memory",  1024);
    broker_add_topic(broker, "metrics.network", 1024);

    printf("[DEBUG] Broker started\n");
    broker_print(broker);
    

    pthread_t       t_cpu;
    pthread_t       t_memory;
    pthread_t       t_network;
    pthread_t       t_consumer;

    pthread_create(&t_cpu,      NULL, producer_cpu,     broker);
    pthread_create(&t_memory,   NULL, producer_memory,  broker);
    pthread_create(&t_network,  NULL, producer_network, broker);
    pthread_create(&t_consumer, NULL, consumer_print,   broker);

    pthread_join(t_cpu,      NULL);
    pthread_join(t_memory,   NULL);
    pthread_join(t_network,  NULL);
    pthread_join(t_consumer, NULL);

    broker_destroy(broker);
    return 0;
}