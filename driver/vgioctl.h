#ifndef VG_IOCTL_H
#define VG_IOCTL_H

#include <linux/ioctl.h>

#define VG_MAGIC 0x5647

#define IOCTL_VG_SET_RESOLUTION _IOR(VG_MAGIC, 0, unsigned long*)
#define IOCTL_VG_SET_MODE _IOR(VG_MAGIC, 1, unsigned)

#endif
