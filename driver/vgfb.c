/*
 * Copyright (C) 2017 Daniel Patrick Abrecht
 * 
 * This module is dual licensed under the MIT License and
 * the GNU General Public License v2.0
 */

#include <linux/platform_device.h>
#include <linux/fb.h>
#include <linux/mm.h>
#include <linux/string.h>
#include <linux/signal.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>
#include "vgfbmx.h"
#include "vgfb.h"

static void vm_open(struct vm_area_struct *vma);
static void vm_close(struct vm_area_struct *vma);

static const struct vm_operations_struct vm_default_ops = {
	.open = vm_open,
	.close = vm_close,
};

static const unsigned long initial_resolution[] = {800,600};

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

static struct fb_ops fb_null_ops;

static const struct fb_fix_screeninfo fix_screeninfo_defaults = {
	.id = "vgfb",
	.type = FB_TYPE_PACKED_PIXELS, // FB_TYPE_FOURCC
	.visual = FB_VISUAL_TRUECOLOR, // FB_VISUAL_FOURCC
//	.capabilities = FB_CAP_FOURCC,
	.accel = FB_ACCEL_NONE,
};

static const struct fb_var_screeninfo var_screeninfo_defaults = {
	.bits_per_pixel = 32,
};

int vgfb_create(struct vgfbm* fb)
{
	int ret;
	if( !vgfbm_acquire(fb) ){
		printk(KERN_ERR "vgfb: vgfbm_acquire failed\n");
		return -EBUSY;
	}
	fb->pdev = platform_device_alloc("vgfb", PLATFORM_DEVID_AUTO);
	if (!fb->pdev) {
		printk(KERN_ERR "vgfb: platform_device_alloc failed\n");
		ret = -ENOMEM;
		goto failed;
	}
	platform_set_drvdata(fb->pdev, fb);
	ret = platform_device_add(fb->pdev);
	if (ret) {
		printk(KERN_ERR "vgfb: platform_device_add failed\n");
		goto failed_after_platform_device_alloc;
	}
	fb->remap_signal = SIGHUP;
	return 0;
failed_after_platform_device_alloc:
	platform_device_unregister(fb->pdev);
failed:
	return ret;
}

void vgfb_remove(struct vgfbm* fb)
{
	platform_device_unregister(fb->pdev);
	vgfbm_release(fb);
}

void vgfb_free(struct vgfbm* fb)
{
	printk(KERN_DEBUG "vgfb: vgfb_free\n");
	if (fb->screen_base){
		vfree(fb->screen_base);
		fb->screen_base = 0;
	}
}

static int probe(struct platform_device * dev)
{
	int ret;
	struct vgfbm* fb = platform_get_drvdata(dev);
	if( !vgfbm_acquire(fb) ){
		printk(KERN_ERR "vgfb: vgfbm_acquire failed\n");
		ret = -EBUSY;
		goto failed;
	}
	fb->info = framebuffer_alloc(sizeof(struct vgfbm*),&fb->pdev->dev);
	if (!fb->info) {
		printk(KERN_ERR "vgfb: framebuffer_alloc failed\n");
		ret = -ENOMEM;
		goto failed_after_acquire;
	}
	fb->info->fix = fix_screeninfo_defaults;
	fb->info->var = var_screeninfo_defaults;
	fb->info->flags = FBINFO_FLAG_DEFAULT;
	*(struct vgfbm**)fb->info->par = fb;
	INIT_LIST_HEAD( &fb->info->modelist );
	{
		fb_var_to_videomode(&fb->videomode, &fb->info->var);
		ret = fb_add_videomode(&fb->videomode, &fb->info->modelist);
		if( ret < 0 ){
			printk(KERN_ERR "vgfb: fb_add_videomode failed\n");
			goto failed_after_framebuffer_alloc;
		}
	}
	fb->info->mode = &fb->videomode;
	fb->info->fbops = &fb_default_ops;
	ret = vgfb_set_resolution(fb,initial_resolution);
	if (ret < 0) {
		printk(KERN_ERR "vgfb: vgfb_set_resolution failed\n");
		goto failed_after_framebuffer_alloc;
	}
	fb->info->var.xres = initial_resolution[0];
	fb->info->var.yres = initial_resolution[1];
	ret = vgfb_check_var(&fb->info->var, fb->info);
	if (ret < 0) {
		printk(KERN_ERR "vgfb: vgfb_check_var failed\n");
		goto failed_after_framebuffer_alloc;
	}
	ret = vgfb_set_par(fb->info);
	if (ret < 0) {
		printk(KERN_ERR "vgfb: vgfb_set_par failed\n");
		goto failed_after_framebuffer_alloc;
	}
	ret = register_framebuffer(fb->info);
	if (ret) {
		printk(KERN_ERR "vgfb: register_framebuffer failed (%d)\n",ret);
		goto failed_after_framebuffer_alloc;
	}
	return 0;
failed_after_framebuffer_alloc:
	framebuffer_release(fb->info);
	fb->info = 0;
failed_after_acquire:
	vgfbm_release(fb);
failed:
	return ret;
}

static int remove(struct platform_device * dev)
{
	struct vgfbm* fb = platform_get_drvdata(dev);
	if (!fb) return 0;
	if (fb->info) {
		fb->info->screen_base = 0;
		fb->info->fbops = &fb_null_ops;
		unregister_framebuffer(fb->info);
		fb->info = 0;
	}
	vgfbm_release(fb);
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

	if (tmp.bits_per_pixel != 32)
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

	if (var->bits_per_pixel == 32) {
		var->red    = (struct fb_bitfield){ 0,8,0};
		var->green  = (struct fb_bitfield){ 8,8,0};
		var->blue   = (struct fb_bitfield){16,8,0};
		var->transp = (struct fb_bitfield){24,8,0};
	}

	return 0;
}

static void vm_open(struct vm_area_struct *vma)
{
	printk(KERN_DEBUG "vgfb: vm_open\n");
}

static void vm_close(struct vm_area_struct *vma)
{
	struct vgfbm* fb = vma->vm_private_data;
	printk(KERN_DEBUG "vgfb: vm_close\n");
	vgfb_release_mmap(fb);
	vgfbm_release(fb);
}

bool vgfb_acquire_mmap(struct vgfbm* fb)
{
	unsigned long val = fb->mem_count + 1;
	if (!val)
		return false;
	fb->mem_count = val;
	return true;
}

void vgfb_release_mmap(struct vgfbm* fb)
{
	unsigned long val = fb->mem_count;
	if (!val){
		printk(KERN_CRIT "underflow; use-after-free\n");
		dump_stack();
		return;
	}
	val--;
	fb->mem_count = val;
	if(val)
		return;
	vgfb_check_switch(fb);
}

bool vgfb_check_switch(struct vgfbm* fb)
{
	if (fb->mem_count)
		return false;
	if (!fb->next_screen_base)
	  return false;
	printk(KERN_DEBUG "vgfb: vgfb_check_switch\n");
	if (fb->screen_base)
		vfree(fb->screen_base);
	fb->info->screen_base = fb->screen_base = fb->next_screen_base;
	fb->next_screen_base = 0;
	fb->info->fix.ypanstep = fb->info->var.yres_virtual - fb->info->var.yres;
	fb->info->fix.smem_start = 0;
	fb->info->fix.smem_len = fb->info->var.xres_virtual * fb->info->var.yres_virtual * 4;
	fb->info->fix.line_length = fb->info->var.xres_virtual * 4;
	return true;
}

int vgfb_mmap(struct fb_info *info, struct vm_area_struct *vma)
{
	int ret;
	struct vgfbm* fb = *(struct vgfbm**)info->par;
	if (!vgfbm_acquire(fb)){
		printk(KERN_INFO "vgfb: vgfbm_acquire\n");
		ret = -EBUSY;
		goto failed;
	}
	if (!vgfb_acquire_mmap(fb)) {
		printk(KERN_INFO "vgfb: vgfb_acquire_mmap\n");
		ret = -EBUSY;
		goto failed_2;
	}
	if (fb->next_screen_base) {
		printk(KERN_INFO "vgfb: screen resolution change in progress\n");
		ret = -EBUSY;
		goto failed_3;
	}
	ret = remap_vmalloc_range(vma, info->screen_base, vma->vm_pgoff);
	if (ret < 0)
		goto failed_3;
	vma->vm_flags |= VM_DONTEXPAND | VM_DONTDUMP;
	vma->vm_private_data = fb;
	vma->vm_ops = &vm_default_ops;
	printk(KERN_DEBUG "vgfb: vgfb_mmap\n");
	return 0;
failed_3:
	vgfb_release_mmap(fb);
failed_2:
	vgfbm_release(fb);
failed:
	return ret;
}

int vgfb_set_par(struct fb_info *info)
{
	struct vgfbm* fb = *(struct vgfbm**)info->par;
	struct fb_videomode* mode = &list_entry(fb->info->modelist.next, struct fb_modelist, list)->mode;
	if (info->var.xres != mode->xres || info->var.yres != mode->yres){
		info->var = fb->old_var;
		return -EINVAL;
	}
	if (fb->videomode.xres == mode->xres && fb->videomode.yres == mode->yres)
		return 0;
	vgfb_check_switch(fb);
	info->mode = &fb->videomode;
	fb->videomode = *mode;
	fb->old_var = info->var;
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

int vgfb_set_resolution(struct vgfbm* fb, const unsigned long resolution[2])
{
	size_t size;
	size_t size_aligned;
	void* mem;
	struct fb_videomode tmp_mode, *mode;
	struct fb_var_screeninfo var;

	if (resolution[0] == 0 || resolution[1] == 0)
		return -EINVAL;

	var = fb->info->var;
	mode = &list_entry(fb->info->modelist.next, struct fb_modelist, list)->mode;

	if (mode->xres == resolution[0] && mode->yres == resolution[1])
		return 0;

	var.xres = resolution[0];
	var.yres = resolution[1];
	var.xres_virtual = resolution[0];
	var.yres_virtual = resolution[1] * 2;
	var.pixclock = 1000000000000lu / var.xres / var.yres / VGFB_REFRESH_RATE;
	fb_var_to_videomode(&tmp_mode, &var);
	tmp_mode.refresh = VGFB_REFRESH_RATE; // just in case

	size = var.xres_virtual * var.yres_virtual * 4;
	size_aligned = PAGE_ALIGN(size);

	mem = vmalloc_32_user(size_aligned);
	if (!mem)
		return -ENOMEM;
	if (fb->next_screen_base)
		vfree(fb->next_screen_base);
	fb->next_screen_base = mem;
	*mode = tmp_mode;

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
