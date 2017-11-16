#ifndef VG_H
#define VG_H

#include <linux/ioctl.h>

#define VG_MAGIC 0x5647

#define IOCTL_WAIT_RESIZE_DONE _IOR(VG_MAGIC, 0, unsigned long*)

#endif
