/* message_slot.h code */
#ifndef MESSAGE_SLOT_H
#define MESSAGE_SLOT_H

#include <linux/ioctl.h>

#define MAJOR_NUM 235
#define DEVICE_NAME "message_slot"
#define MSG_SLOT_CHANNEL _IOW(MAJOR_NUM, 0, unsigned int)

#define MAX_MESSAGE_SIZE 128
#define MAX_CHANNELS 220

typedef struct channel {
    unsigned int channel_id;
    char message[MAX_MESSAGE_SIZE];
    int message_len;
    struct channel* next;
} channel_t;

typedef struct message_slot {
    channel_t* channels;
    unsigned int active_channel_id;
} message_slot_t;

#endif /* MESSAGE_SLOT_H */