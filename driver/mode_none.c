/*
 * Copyright (C) 2017 Daniel Patrick Abrecht
 * 
 * This module is dual licensed under the MIT License and
 * the GNU General Public License v2.0
 */

#include <linux/fb.h>
#include <linux/poison.h>
#include <linux/uaccess.h>
#include "vgfb.h"
#include "mode.h"

static int m_create(struct vgfbm* fb);
static int m_destroy(struct vgfbm* fb);


static const struct vgfb_mode mode = {
	.create = m_create,
	.destroy = m_destroy
};
VGFB_EXPORT_MODE(NONE,&mode)

static ssize_t m_read(struct fb_info *info, char __user *buf, size_t count, loff_t *ppos)
{
	unsigned long offset = *ppos;
	unsigned long mem_len = info->fix.smem_len;
	if (info->state != FBINFO_STATE_RUNNING)
		return -EPERM;
	if (offset > mem_len)
		return -ENOMEM;
	if (!count)
		return 0;
	if (count > mem_len - offset)
		count = mem_len - offset;
	if (!count)
		return -ENOMEM;
	if (clear_user(buf, count))
		return -EFAULT;
	*ppos += count;
	return count;
}

static ssize_t m_write(struct fb_info *info, const char __user *buf, size_t count, loff_t *ppos)
{
	unsigned long offset = *ppos;
	unsigned long mem_len = info->fix.smem_len;
	if (info->state != FBINFO_STATE_RUNNING)
		return -EPERM;
	if (offset > mem_len)
		return -ENOMEM;
	if (!count)
		return 0;
	if (count > mem_len - offset)
		count = mem_len - offset;
	if (!count)
		return -ENOMEM;
	*ppos += count;
	return count;
}

static int m_mmap(struct fb_info *info, struct vm_area_struct *vma){
	printk("test\n");
	return 0;
}

static struct fb_ops ops = {
	.owner = THIS_MODULE,
	.fb_read = m_read,
	.fb_write = m_write,
	.fb_mmap = m_mmap,
	.fb_set_par = vgfb_set_par,
	.fb_check_var = vgfb_check_var,
	.fb_setcolreg = vgfb_setcolreg,
	.fb_pan_display = vgfb_pan_display,
};

static int m_create(struct vgfbm* fb){
	fb->mode = &mode;
	fb->info->fbops = &ops;
	// I would prefer to leave the following zero, but fb_read and fb_write check if it's set before calling my own one, so I'll use a poison pointer instead.
	fb->info->screen_base = (void*)ATM_POISON;
	if(fb->mode != &mode)
		fb->mode->destroy(fb);
	return 0;
}

static int m_destroy(struct vgfbm* fb){
	return 0;
}
