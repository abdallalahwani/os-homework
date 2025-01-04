/* message_slot.c code */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/ioctl.h>
#include "message_slot.h"

MODULE_LICENSE("GPL");

static message_slot_t* slots[256];

static channel_t* find_channel(message_slot_t* slot, unsigned int channel_id) {
    channel_t* cur = slot->channels;
    while (cur) {
        if (cur->channel_id == channel_id) {
            return cur;
        }
        cur = cur->next;
    }
    return NULL;
}

static channel_t* add_channel(message_slot_t* slot, unsigned int channel_id) {
    channel_t* new_channel = kmalloc(sizeof(channel_t), GFP_KERNEL);
    if (!new_channel) {
        return NULL;
    }
    new_channel->channel_id = channel_id;
    new_channel->message_len = 0;
    new_channel->next = slot->channels;
    slot->channels = new_channel;
    return new_channel;
}


static int device_open(struct inode* inode, struct file* file) {
    int minor = iminor(inode);
    message_slot_t* slot;

    if (!slots[minor]) {
        slots[minor] = kmalloc(sizeof(message_slot_t), GFP_KERNEL);
        if (!slots[minor]) {
            printk(KERN_ERR "Failed to allocate memory slot\n");
            return -ENOMEM;
        }
        slots[minor]->channels = NULL;
        slots[minor]->active_channel_id = 0;  // Initialize to 0
    }
    
    slot = slots[minor];
    slot->active_channel_id = 0;  // Reset for each new open
    file->private_data = (void*)slot;
    return 0;
}

static int device_release(struct inode* inode, struct file* file) {
    return 0;
}

static long device_ioctl(struct file* file, unsigned int cmd, unsigned long arg) {
    message_slot_t* slot = (message_slot_t*)file->private_data;
    unsigned int channel_id;

    if (cmd != MSG_SLOT_CHANNEL) {
        return -EINVAL;
    }
    if (copy_from_user(&channel_id, (unsigned int __user*)arg, sizeof(unsigned int))) {
        return -EFAULT;
    }
    if (channel_id == 0 || channel_id > MAX_CHANNELS) {
        return -EINVAL;
    }
    slot->active_channel_id = channel_id;
    return 0;
}

static ssize_t device_write(struct file* file, const char __user* buffer, size_t length, loff_t* offset) {
    message_slot_t* slot = (message_slot_t*)file->private_data;
    channel_t* chan;

    if (!slot || slot->active_channel_id == 0) {
        return -EINVAL;
    }

    if (length == 0 || length > MAX_MESSAGE_SIZE) {
        return -EMSGSIZE;
    }

    chan = find_channel(slot, slot->active_channel_id);
    if (!chan) {
        chan = add_channel(slot, slot->active_channel_id);
        if (!chan) {
            return -ENOMEM;
        }
    }

    if (copy_from_user(chan->message, buffer, length)) {
        return -EFAULT;
    }
    chan->message_len = length;
    return length;
}
static ssize_t device_read(struct file* file, char __user* buffer, size_t length, loff_t* offset) {
    message_slot_t* slot = (message_slot_t*)file->private_data;
    channel_t* chan;

    // Check slot and channel validity first
    if (!slot || slot->active_channel_id == 0) {
        return -EINVAL;
    }

    chan = find_channel(slot, slot->active_channel_id);
    if (!chan) {
        return -EINVAL;
    }

    if (chan->message_len == 0) {
        return -EWOULDBLOCK;
    }

    if (length < chan->message_len) {
        return -ENOSPC;
    }

    if (copy_to_user(buffer, chan->message, chan->message_len)) {
        return -EFAULT;
    }

    return chan->message_len;
}
static struct file_operations Fops = {
    .owner = THIS_MODULE,
    .open = device_open,
    .read = device_read,
    .write = device_write,
    .unlocked_ioctl = device_ioctl,
    .release = device_release
};

static int __init simple_init(void) {
    int rc = register_chrdev(MAJOR_NUM, DEVICE_NAME, &Fops);
    if (rc < 0) {
        printk(KERN_ERR "Failed registering with %d\n", rc);
        return rc;
    }
    printk(KERN_INFO "message_slot: registered major number %d\n", MAJOR_NUM);
    return 0;
}

static void __exit simple_cleanup(void) {
    int i;
    for (i = 0; i < 256; i++) {
        if (slots[i]) {
            channel_t* chan = slots[i]->channels;
            while (chan) {
                channel_t* tmp = chan;
                chan = chan->next;
                kfree(tmp);
            }
            kfree(slots[i]);
        }
    }
    unregister_chrdev(MAJOR_NUM, DEVICE_NAME);
}

module_init(simple_init);
module_exit(simple_cleanup);