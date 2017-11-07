/*
 * Copyright (C) 2017 Daniel Patrick Abrecht
 * 
 * This module is dual licensed under the MIT License and
 * the GNU General Public License v2.0
 */

#include <linux/errno.h>
#include <linux/fb.h>
#include "vgfb.h"
#include "mode.h"

const struct vgfb_mode*const*const vgfb_mode[] = {
#define X(X) &vgfb_mode_ ## X,
VGFB_MODES
#undef X
};

const unsigned vgfb_mode_count = sizeof(vgfb_mode)/sizeof(*vgfb_mode);
