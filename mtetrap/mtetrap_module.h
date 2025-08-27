#ifndef MTETRAP_MODULE_H
#define MTETRAP_MODULE_H

#define MTETRAP_DEVICE_NAME "mtetrap"
#define MTETRAP_DEVICE_PATH "/dev/" MTETRAP_DEVICE_NAME
#define MTETRAP_IOCTL_MAGIC_NUMBER (long)0xc31
#define MTETRAP_IOCTL_CMD_SETREDIRECT _IOR(MTETRAP_IOCTL_MAGIC_NUMBER, 1, size_t)

struct ioctl_setredirect_params {
    unsigned long trap_handler;
    unsigned long access_count_addr;
    unsigned long trap_handler_pre;
};

#endif
