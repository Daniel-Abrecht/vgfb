/*
 * Copyright (C) 2017 Daniel Patrick Abrecht
 * 
 * This module is dual licensed under the MIT License and
 * the GNU General Public License v2.0
 */

#ifndef VG_H
#define VG_H

#include <linux/ioctl.h>

#define VG_MAGIC 0x5647

#define VGFB_WAIT_RESIZE_DONE _IO(VG_MAGIC, 0)
#define VGFBM_GET_FB_MINOR _IOW(VG_MAGIC, 1, int*)

#endif
