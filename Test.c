#include "message_slot.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>

#define DEVICE_PATH_0 "/dev/slot0"
#define DEVICE_PATH_1 "/dev/slot1"
#define TEST_PASS "\x1B[32mPASS\x1B[0m"
#define TEST_FAIL "\x1B[31mFAIL\x1B[0m"

typedef struct {
    int passed;
    int failed;
} TestStats;

static TestStats stats = {0, 0};

static void assert_test(const char* test_name, int condition, const char* msg) {
    printf("[%s] %s: %s\n", condition ? TEST_PASS : TEST_FAIL, test_name, msg);
    condition ? stats.passed++ : stats.failed++;
    if (!condition) exit(1);
}

// Device Operations Tests
void test_device_operations() {
    int fd0, fd1;
    
    // Test device open
    fd0 = open(DEVICE_PATH_0, O_RDWR);
    assert_test("Device Open 0", fd0 >= 0, "Opening first device");
    
    fd1 = open(DEVICE_PATH_1, O_RDWR);
    assert_test("Device Open 1", fd1 >= 0, "Opening second device");
    
    // Test device close
    assert_test("Device Close", close(fd0) == 0 && close(fd1) == 0, "Closing devices");
}

// Channel Operations Tests
void test_channel_operations() {
    int fd;
    unsigned int channel_ids[] = {1, 100, 220};  // Valid channel range
    
    fd = open(DEVICE_PATH_0, O_RDWR);
    assert_test("Channel Test Setup", fd >= 0, "Opening device");
    
    // Test channel setting
    for (int i = 0; i < 3; i++) {
        assert_test("Channel Set", 
            ioctl(fd, MSG_SLOT_CHANNEL, &channel_ids[i]) >= 0,
            "Setting valid channel");
    }
    
    // Test invalid channel (0)
    unsigned int invalid_channel = 0;
    assert_test("Invalid Channel", 
        ioctl(fd, MSG_SLOT_CHANNEL, &invalid_channel) == -1 && errno == EINVAL,
        "Setting invalid channel");
        
    close(fd);
}

// Message Operations Tests
void test_message_operations() {
    int fd;
    unsigned int channel_id = 1;
    char test_msg[] = "Test Message";
    char empty_msg[] = "";
    char max_msg[MAX_MESSAGE_SIZE];
    char read_buffer[MAX_MESSAGE_SIZE];
    
    fd = open(DEVICE_PATH_0, O_RDWR);
    assert_test("Message Test Setup", fd >= 0 && 
        ioctl(fd, MSG_SLOT_CHANNEL, &channel_id) >= 0, "Setup");
    
    // Test regular message
    assert_test("Write Message", 
        write(fd, test_msg, strlen(test_msg)) == strlen(test_msg),
        "Writing regular message");
        
    assert_test("Read Message",
        read(fd, read_buffer, MAX_MESSAGE_SIZE) == strlen(test_msg) &&
        memcmp(test_msg, read_buffer, strlen(test_msg)) == 0,
        "Reading regular message");
    
    // Test empty message
    assert_test("Empty Message", 
        write(fd, empty_msg, 0) == -1 && errno == EMSGSIZE,
        "Writing empty message");
    
    // Test max size message
    memset(max_msg, 'A', MAX_MESSAGE_SIZE);
    assert_test("Max Message",
        write(fd, max_msg, MAX_MESSAGE_SIZE) == MAX_MESSAGE_SIZE,
        "Writing max size message");
        
    close(fd);
}

// Persistence Tests
void test_persistence() {
    int fd1, fd2;
    unsigned int channel_id = 1;
    char test_msg[] = "Persistence Test";
    char read_buffer[MAX_MESSAGE_SIZE];
    
    // Write with first fd
    fd1 = open(DEVICE_PATH_0, O_RDWR);
    assert_test("Persistence Write Setup",
        fd1 >= 0 && ioctl(fd1, MSG_SLOT_CHANNEL, &channel_id) >= 0,
        "Setup first fd");
    
    assert_test("Persistence Write",
        write(fd1, test_msg, strlen(test_msg)) == strlen(test_msg),
        "Writing message");
    
    close(fd1);
    
    // Read with second fd
    fd2 = open(DEVICE_PATH_0, O_RDWR);
    assert_test("Persistence Read Setup",
        fd2 >= 0 && ioctl(fd2, MSG_SLOT_CHANNEL, &channel_id) >= 0,
        "Setup second fd");
    
    assert_test("Persistence Read",
        read(fd2, read_buffer, MAX_MESSAGE_SIZE) == strlen(test_msg) &&
        memcmp(test_msg, read_buffer, strlen(test_msg)) == 0,
        "Reading persistent message");
        
    close(fd2);
}

// Error Conditions Tests
void test_error_conditions() {
    int fd;
    char buffer[MAX_MESSAGE_SIZE + 1];
    
    fd = open(DEVICE_PATH_0, O_RDWR);
    
    // Test read/write without channel set
    assert_test("No Channel Write",
        write(fd, "test", 4) == -1 && errno == EINVAL,
        "Write without channel");
        
    assert_test("No Channel Read",
        read(fd, buffer, sizeof(buffer)) == -1 && errno == EINVAL,
        "Read without channel");
    
    // Test message too long
    unsigned int channel_id = 1;
    ioctl(fd, MSG_SLOT_CHANNEL, &channel_id);
    memset(buffer, 'A', MAX_MESSAGE_SIZE + 1);
    
    assert_test("Message Too Long",
        write(fd, buffer, MAX_MESSAGE_SIZE + 1) == -1 && errno == EMSGSIZE,
        "Writing oversized message");
        
    close(fd);
}

int main() {
    printf("Message Slot Testing Suite\n");
    printf("=========================\n\n");
    
    test_device_operations();
    test_channel_operations();
    test_message_operations();
    test_persistence();
    test_error_conditions();
    
    printf("\nTest Summary: %d passed, %d failed\n", 
           stats.passed, stats.failed);
    
    return stats.failed ? 1 : 0;
}