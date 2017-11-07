/*
 * Copyright (C) 2017 Daniel Patrick Abrecht
 * 
 * This module is dual licensed under the MIT License and
 * the GNU General Public License v2.0
 */

#include <linux/platform_device.h>
#include <linux/string.h>
#include <linux/fb.h>
#include <linux/signal.h>
#include <linux/uaccess.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include "vgfb.h"

static struct fb_ops fb_default_ops = {
	.owner = THIS_MODULE,
	.fb_read = vgfb_read,
	.fb_write = vgfb_write,
	.fb_mmap = vgfb_mmap,
	.fb_set_par = vgfb_set_par,
	.fb_check_var = vgfb_check_var,
	.fb_setcolreg = vgfb_setcolreg,
	.fb_pan_display = vgfb_pan_display,
};

static const struct fb_fix_screeninfo fix_screeninfo_defaults = {
	.id = "vgfb",
	.type = FB_TYPE_PACKED_PIXELS, // FB_TYPE_FOURCC
	.visual = FB_VISUAL_TRUECOLOR, // FB_VISUAL_FOURCC
//	.capabilities = FB_CAP_FOURCC,
	.ypanstep = 600,
	.accel = FB_ACCEL_NONE,
};

static const struct fb_var_screeninfo var_screeninfo_defaults = {
	.bits_per_pixel = 32,
	.red   = { 0,8,0},
	.green = { 8,8,0},
	.blue  = {16,8,0},
	.xres = 800,
	.yres = 600,
	.pixclock = 1000000000000lu / 800lu / 600lu / VGFB_REFRESH_RATE,
};

int vgfb_create(struct vgfbm* fb)
{
	int ret;
	fb->pdev = platform_device_alloc("vgfb", PLATFORM_DEVID_AUTO);
	if (!fb->pdev) {
		printk(KERN_INFO "vgfb: platform_device_alloc failed\n");
		ret = -ENOMEM;
		goto failed;
	}
	platform_set_drvdata(fb->pdev, fb);
	ret = platform_device_add(fb->pdev);
	if (ret) {
		printk(KERN_INFO "vgfb: platform_device_add failed\n");
		goto failed_after_platform_device_alloc;
	}
	fb->remap_signal = SIGHUP;
	return 0;
failed_after_platform_device_alloc:
	platform_device_unregister(fb->pdev);
failed:
	return ret;
}

void vgfb_destroy(struct vgfbm* fb)
{
	platform_device_unregister(fb->pdev);
}

static int probe(struct platform_device * dev)
{
	int ret;
	struct vgfbm* fb = platform_get_drvdata(dev);
	fb->info = framebuffer_alloc(sizeof(struct vgfbm*),&fb->pdev->dev);
	if (!fb->info) {
		printk(KERN_INFO "vgfb: framebuffer_alloc failed\n");
		ret = -ENOMEM;
		goto failed;
	}
	fb->info->fix = fix_screeninfo_defaults;
	fb->info->var = var_screeninfo_defaults;
	fb->info->flags = FBINFO_FLAG_DEFAULT;
	*(struct vgfbm**)fb->info->par = fb;
	INIT_LIST_HEAD( &fb->info->modelist );
	{
		fb_var_to_videomode(&fb->videomode, &fb->info->var);
		fb->videomode.refresh = VGFB_REFRESH_RATE;
		ret = fb_add_videomode(&fb->videomode, &fb->info->modelist);
		if( ret < 0 ){
			printk("vgfb: fb_add_videomode failed\n");
			goto failed_after_framebuffer_alloc;
		}
	}
	fb->info->mode = &fb->videomode;
	fb->info->fbops = &fb_default_ops;
	ret = vgfb_check_var(&fb->info->var,fb->info);
	if (ret < 0) {
		printk("vgfb_check_var failed\n");
		goto failed_after_framebuffer_alloc;
	}
	ret = vgfb_set_par(fb->info);
	if (ret < 0) {
		printk("vgfb_set_par failed\n");
		goto failed_after_framebuffer_alloc;
	}
	ret = register_framebuffer(fb->info);
	if (ret) {
		printk(KERN_INFO "vgfb: register_framebuffer failed (%d)\n",ret);
		goto failed_after_framebuffer_alloc;
	}
	return 0;
failed_after_framebuffer_alloc:
	framebuffer_release(fb->info);
	fb->info = 0;
failed:
	return ret;
}

static int remove(struct platform_device * dev)
{
	struct vgfbm* fb = platform_get_drvdata(dev);
	if (!fb) return 0;
	if (fb->info) {
		vgfb_free_screen(fb);
		unregister_framebuffer(fb->info);
		framebuffer_release(fb->info);
		fb->info = 0;
	}
	return 0;
}


ssize_t vgfb_read(struct fb_info *info, char __user *buf, size_t count, loff_t *ppos)
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

ssize_t vgfb_write(struct fb_info *info, const char __user *buf, size_t count, loff_t *ppos)
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

int vgfb_realloc_screen(struct vgfbm* fb)
{
	size_t size = fb->info->var.xres_virtual * fb->info->var.yres_virtual * (fb->info->var.bits_per_pixel / 8);
	size_t size_aligned = PAGE_ALIGN(size);
	vgfb_free_screen(fb);
	if (!size)
		return 0;
	fb->info->screen_base = vmalloc_32_user(size_aligned);
	if (!fb->info->screen_base)
		return -ENOMEM;
	return 0;
}

void vgfb_free_screen(struct vgfbm* fb)
{
	if (fb->info->screen_base)
		vfree(fb->info->screen_base);
	fb->info->screen_base = 0;
}

int vgfb_mmap(struct fb_info *info, struct vm_area_struct *vma)
{
	return remap_vmalloc_range(vma, info->screen_base, vma->vm_pgoff);
}

int vgfb_set_par(struct fb_info *info)
{
	int ret;
	struct vgfbm* fb = *(struct vgfbm**)info->par;
	struct fb_videomode* mode = &list_entry(fb->info->modelist.next, struct fb_modelist, list)->mode;
	if (info->var.xres != mode->xres || info->var.yres != mode->yres){
		info->var = fb->old_var;
		return -EINVAL;
	}
	ret = vgfb_realloc_screen(fb);
	if (ret < 0){
		info->var = fb->old_var;
		return ret;
	}
	info->mode = &fb->videomode;
	fb->videomode = *mode;
	fb->old_var = info->var;
	info->fix.ypanstep = info->var.yres;
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

int vgfb_set_resolution(struct vgfbm* fb, unsigned long resolution[2])
{
	struct fb_var_screeninfo var = fb->info->var;
	struct fb_videomode* mode = &list_entry(fb->info->modelist.next, struct fb_modelist, list)->mode;

	if (resolution[0] == 0 || resolution[1] == 0)
		return -EINVAL;

	var.xres = resolution[0];
	var.yres = resolution[1];
	var.xres_virtual = resolution[0];
	var.yres_virtual = resolution[1] * 2;
	var.pixclock = 1000000000000lu / var.xres / var.yres / VGFB_REFRESH_RATE;
	fb_var_to_videomode(mode, &var);
	mode->refresh = VGFB_REFRESH_RATE; // just in case

	return 0;
}

static struct platform_driver driver = {
	.probe  = probe,
	.remove = remove,
	.driver = {
		.name = "vgfb",
	}
};

int __init vgfb_init(void)
{
  return platform_driver_register(&driver);
}

void __exit vgfb_exit(void)
{
  platform_driver_unregister(&driver);
}
