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

#define IOCTL_WAIT_RESIZE_DONE _IO(VG_MAGIC, 0)

#endif
