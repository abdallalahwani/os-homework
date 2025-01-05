
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define MAJOR_NUM 235
#define MSG_SLOT_CHANNEL _IOW(MAJOR_NUM, 0, unsigned int)
#define MAX_MESSAGE_SIZE 128

int main(int argc, char *argv[]) {
    int fd;
    unsigned int channel_id;
    size_t msg_len;
    int ret;

    if (argc != 4) {
        fprintf(stderr, "Usage: %s <file_path> <channel_id> <message>\n", argv[0]);
        exit(1);
    }

    channel_id = (unsigned int)atoi(argv[2]);
    msg_len = strlen(argv[3]);
    if (msg_len == 0 || msg_len > MAX_MESSAGE_SIZE) {
        fprintf(stderr, "Invalid message length (1-128 bytes)\n");
        exit(1);
    }

    fd = open(argv[1], O_WRONLY);
    if (fd < 0) {
        perror("open");
        exit(1);
    }

    ret = ioctl(fd, MSG_SLOT_CHANNEL, &channel_id);
    if (ret < 0) {
        perror("ioctl");
        close(fd);
        exit(1);
    }

    ret = write(fd, argv[3], msg_len);
    if (ret < 0) {
        perror("write");
        close(fd);
        exit(1);
    }

    close(fd);
    return 0;
}