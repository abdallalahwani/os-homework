/* message_reader.c code */
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <errno.h>

#define MAJOR_NUM 235
#define MSG_SLOT_CHANNEL _IOW(MAJOR_NUM, 0, unsigned int)
#define MAX_MESSAGE_SIZE 128

int main(int argc, char *argv[]) {
    int fd, ret;
    unsigned int channel_id;
    char buffer[MAX_MESSAGE_SIZE];
    ssize_t bytes_read;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <file_path> <channel_id>\n", argv[0]);
        exit(1);
    }

    channel_id = (unsigned int)atoi(argv[2]);

    fd = open(argv[1], O_RDONLY);
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

    bytes_read = read(fd, buffer, MAX_MESSAGE_SIZE);
    if (bytes_read < 0) {
        perror("read");
        close(fd);
        exit(1);
    }

    write(STDOUT_FILENO, buffer, bytes_read);
    close(fd);
    return 0;
}