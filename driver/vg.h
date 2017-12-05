#ifndef VG_H
#define VG_H

#include <linux/ioctl.h>

#define VG_MAGIC 0x5647

#define VGFBM_GET_FB_MINOR _IOW(VG_MAGIC, 1, int*)

#endif
