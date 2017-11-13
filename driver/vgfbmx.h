/*
 * Copyright (C) 2017 Daniel Patrick Abrecht
 * 
 * This module is dual licensed under the MIT License and
 * the GNU General Public License v2.0
 */

#ifndef VGFBMX_H
#define VGFBMX_H

struct vgfbm;

bool vgfbm_acquire(struct vgfbm* vgfbm);
void vgfbm_release(struct vgfbm* vgfbm);

#endif
