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

int vgfb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
	struct fb_var_screeninfo tmp = *var;

	if (tmp.bits_per_pixel != 24 && tmp.bits_per_pixel != 32)
	  return -EINVAL;

	if (tmp.xoffset != 0)
		return -EINVAL;

	if (tmp.yoffset > tmp.yres)
		return -EINVAL;

	if (!( tmp.xres == info->var.xres && tmp.yres == info->var.yres ))
	{
		struct list_head *it;
		bool found = false;
		list_for_each (it, &info->modelist) {
			struct fb_videomode* mode = &list_entry(it, struct fb_modelist, list)->mode;
			if (mode->xres == tmp.xres && mode->yres == tmp.yres)
			{
				found = true;
				break;
			}
		}
		if(!found)
			return -EINVAL;
	}

	*var = info->var;

	var->xres = tmp.xres;
	var->yres = tmp.yres;
	var->xres_virtual = tmp.xres;
	var->yres_virtual = tmp.yres * 2;
	var->xoffset = tmp.xoffset;
	var->yoffset = tmp.yoffset;
	var->pixclock = 1000000000000lu / var->xres / var->yres / VGFB_REFRESH_RATE;

	if (var->bits_per_pixel == 24) {
		var->red    = (struct fb_bitfield){ 0,8,0};
		var->green  = (struct fb_bitfield){ 8,8,0};
		var->blue   = (struct fb_bitfield){16,8,0};
		var->transp = (struct fb_bitfield){ 0,0,0};
	}else if (var->bits_per_pixel == 24) {
		var->red    = (struct fb_bitfield){ 0,8,0};
		var->green  = (struct fb_bitfield){ 8,8,0};
		var->blue   = (struct fb_bitfield){16,8,0};
		var->transp = (struct fb_bitfield){32,8,0};
	}

	return 0;
}

int vgfb_set_par(struct fb_info *info){
	{
		struct list_head *it, *hit;
		info->mode = 0;
		list_for_each_safe (it, hit, &info->modelist) {
			struct fb_videomode* mode = &list_entry(it, struct fb_modelist, list)->mode;
			if (mode->xres == info->var.xres && mode->yres == info->var.yres) {
				info->mode = mode;
			} else {
				list_del(it);
				kfree(it);
			}
		}
	}
	info->fix.ypanstep = info->var.xres;
	info->fix.smem_start = 0;
	info->fix.smem_len = info->var.xres_virtual * info->var.yres_virtual * (info->var.bits_per_pixel / 8);
	info->fix.line_length = info->var.xres_virtual * (info->var.bits_per_pixel / 8);
	return 0;
}

int vgfb_setcolreg(u_int regno, u_int red, u_int green, u_int blue, u_int transp, struct fb_info *info)
{
	return -EINVAL;
}

int vgfb_pan_display(struct fb_var_screeninfo *var, struct fb_info *info)
{
	if( var->xoffset > info->var.xres_virtual - info->var.xres )
		return -EINVAL;
	if( var->yoffset > info->var.yres_virtual - info->var.yres )
		return -EINVAL;
	info->var.xoffset = var->xoffset;
	info->var.yoffset = var->yoffset;
	return 0;
}
