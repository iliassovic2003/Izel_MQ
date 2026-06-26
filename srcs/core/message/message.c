# include "message.h"

static atomic_uint_fast32_t s_next_id = 0;

imq_message_t   *imq_msg_create(imq_msg_type_t type, const char *topic,
                                    const void *payload, uint32_t size) {
    imq_message_t*       message;

    if (type == IMQ_CONTROL && size > IMQ_CTRL_PAYLOAD_MAX)
        return NULL;
    if (type == IMQ_DATA && size > IMQ_DATA_PAYLOAD_MAX)
        return NULL;

    message = iloc(sizeof(imq_message_t) + size);
    if (!message)
        return NULL;

    atomic_thread_fence(memory_order_acquire);
    memset(message, 0, sizeof(imq_message_t) + size);

    message->id = atomic_fetch_add(&s_next_id, 1);
    message->size = size;
    message->type = type;

    strncpy(message->topic, topic, sizeof(message->topic) - 1);
    message->topic[sizeof(message->topic) - 1] = '\0';

    if (payload && size > 0)
        memcpy(message->payload, payload, size);

    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    message->timestamp = (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;

    return message;
}

void             imq_msg_destroy(imq_message_t *msg) {
    if (msg)
        ifree(msg);
}

void    imq_msg_print(const imq_message_t *msg) {
    if (!msg)
        return;
    
    printf("[imq] id=%-5u\ttype=%-7s\ttopic=%-32s\tsize=%-4u\tts=%lu\tpayload=%.*s\n",
        msg->id,
        msg->type == IMQ_CONTROL ? "CONTROL" : "DATA",
        msg->topic,
        msg->size,
        msg->timestamp,
        msg->size,
        (const char *)msg->payload);
}