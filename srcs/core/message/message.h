#ifndef     MESSAGE_H
# define    MESSAGE_H

# include   <stdint.h>
# include   <stddef.h>
# include   <string.h>
# include   <stdio.h>
# include   <time.h>
# include   "../ialloc/lock_free_malloc.h"

typedef enum {
    IMQ_CONTROL  = 0,
    IMQ_DATA     = 1,
}                   imq_msg_type_t;

typedef struct {
    uint32_t        id;
    uint32_t        size;
    uint64_t        timestamp;
    imq_msg_type_t  type;
    char            topic[48];
    uint8_t         payload[];
}                   imq_message_t;

# define IMQ_CTRL_PAYLOAD_MAX    SMALL_BLOCK_SIZE - sizeof(imq_message_t)
# define IMQ_DATA_PAYLOAD_MAX       BIG_BLOCK_SIZE - sizeof(imq_message_t)


imq_message_t   *imq_msg_create(imq_msg_type_t type, const char *topic, const void *payload, uint32_t size);

void             imq_msg_destroy(imq_message_t *msg);
void             imq_msg_print(const imq_message_t *msg);

#endif