/*
 * Copyright (C) 2017 Daniel Patrick Abrecht
 * 
 * This module is dual licensed under the MIT License and
 * the GNU General Public License v2.0
 */

#include <linux/fb.h>
#include <linux/poison.h>
#include <linux/uaccess.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include "vgfb.h"
#include "mode.h"
#include "mode_normal.h"

static const struct vgfb_mode mode = {
	.create = vgfb_m_normal_create,
	.realloc_screen = vgfb_m_normal_realloc_screen,
	.free_screen = vgfb_m_normal_free_screen,
	.destroy = vgfb_m_normal_destroy
};

static const struct fb_ops ops = {
	.owner = THIS_MODULE,
	.fb_read = vgfb_m_normal_read,
	.fb_write = vgfb_m_normal_write,
	.fb_mmap = vgfb_m_normal_mmap,
	.fb_set_par = vgfb_m_normal_set_par,
	.fb_check_var = vgfb_m_normal_check_var,
	.fb_setcolreg = vgfb_m_normal_setcolreg,
	.fb_pan_display = vgfb_m_normal_pan_display,
};

VGFB_EXPORT_MODE(NORMAL,&mode)

ssize_t vgfb_m_normal_read(struct fb_info *info, char __user *buf, size_t count, loff_t *ppos)
{
	unsigned long offset = *ppos;
	unsigned long mem_len = info->fix.smem_len;
	if (info->state != FBINFO_STATE_RUNNING)
		return -EPERM;
	if (offset > mem_len || !info->screen_base)
		return -ENOMEM;
	if (!count)
		return 0;
	if (count > mem_len - offset)
		count = mem_len - offset;
	if (!count)
		return -ENOMEM;
	if (copy_to_user(buf, info->screen_base + offset, count))
		return -EFAULT;
	*ppos += count;
	return count;
}

ssize_t vgfb_m_normal_write(struct fb_info *info, const char __user *buf, size_t count, loff_t *ppos)
{
	unsigned long offset = *ppos;
	unsigned long mem_len = info->fix.smem_len;
	if (info->state != FBINFO_STATE_RUNNING)
		return -EPERM;
	if (offset > mem_len || !info->screen_base)
		return -ENOMEM;
	if (!count)
		return 0;
	if (count > mem_len - offset)
		count = mem_len - offset;
	if (!count)
		return -ENOMEM;
	if (copy_to_user(info->screen_base + offset, buf, count))
		return -EFAULT;
	*ppos += count;
	return count;
}

int vgfb_m_normal_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
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

int vgfb_m_normal_realloc_screen(struct vgfbm* fb)
{
	size_t size = fb->info->var.xres_virtual * fb->info->var.yres_virtual * (fb->info->var.bits_per_pixel / 8);
	size_t size_aligned = PAGE_ALIGN(size);
	vgfb_m_normal_free_screen(fb);
	if (!size)
		return 0;
	fb->info->screen_base = vmalloc_32_user(size_aligned);
	if (!fb->info->screen_base)
		return -ENOMEM;
	return 0;
}

void vgfb_m_normal_free_screen(struct vgfbm* fb)
{
	if (fb->info->screen_base)
		vfree(fb->info->screen_base);
	fb->info->screen_base = 0;
}

int vgfb_m_normal_mmap(struct fb_info *info, struct vm_area_struct *vma)
{
	return remap_vmalloc_range(vma, info->screen_base, vma->vm_pgoff);
}

int vgfb_m_normal_set_par(struct fb_info *info)
{
	struct vgfbm* fb = *(struct vgfbm**)info->par;
	struct fb_videomode* old_mode = info->mode;
	info->mode = 0;
	if (fb->mode && fb->mode->realloc_screen){
		int ret = fb->mode->realloc_screen(fb);
		if (ret < 0){
			info->var = fb->old_var;
			info->mode = old_mode;
			info->fix.ypanstep = 0;
			info->fix.smem_len = 0;
			info->fix.line_length = 0;
			return ret;
		}
	}
	{
		struct list_head *it, *hit;
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
	fb->old_var = info->var;
	info->fix.ypanstep = info->var.yres;
	info->fix.smem_start = 0;
	info->fix.smem_len = info->var.xres_virtual * info->var.yres_virtual * (info->var.bits_per_pixel / 8);
	info->fix.line_length = info->var.xres_virtual * (info->var.bits_per_pixel / 8);
	return 0;
}

int vgfb_m_normal_setcolreg(u_int regno, u_int red, u_int green, u_int blue, u_int transp, struct fb_info *info)
{
	return -EINVAL;
}

int vgfb_m_normal_pan_display(struct fb_var_screeninfo *var, struct fb_info *info)
{
	if( var->xoffset > info->var.xres_virtual - info->var.xres )
		return -EINVAL;
	if( var->yoffset > info->var.yres_virtual - info->var.yres )
		return -EINVAL;
	info->var.xoffset = var->xoffset;
	info->var.yoffset = var->yoffset;
	return 0;
}

int vgfb_m_normal_create(struct vgfbm* fb)
{
	fb->mode = &mode;
	fb->ops = ops;
	return 0;
}

int vgfb_m_normal_destroy(struct vgfbm* fb)
{
	return 0;
}
