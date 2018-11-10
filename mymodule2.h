#ifndef MYMODULE_H
#define MYMODULE_H

#include <linux/ioctl.h>

dev_t devNum;
int majNum;

#define DEVICE_NAME "cryptctl"

#define IOCTL_CREATE_DEVICE _IOWR(243, 1, int)
#define IOCTL_DESTROY_DEVICE _IOWR(243, 2, int)
#define IOCTL_CHANGE_KEY _IOWR(243, 3, int)
#define IOCTL_ENCRYPT _IOWR(243, 4, int)
#define IOCTL_DECRYPT _IOWR(243, 5, int)

#endif
