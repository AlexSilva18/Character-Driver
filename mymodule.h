#ifndef MYMODULE_H
#define MYMODULE_H

#include <linux/ioctl.h>

/* typdef struct{ */
/*   int i; */
/*   int j; */
/* }ioctl_struct; */

dev_t devNum;
int majNum;

#define DEVICE_NAME "testDevice"

#define IOCTL_GET_CREATE _IOWR(243, 0, int)
#define IOCTL_DESTROY _IOWR(243, 1, int)

//static int mode = 0;

#endif
