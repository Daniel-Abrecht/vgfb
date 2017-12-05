#include <linux/console.h>
#include <linux/platform_device.h>
#include <linux/fb.h>
#include <linux/mm.h>
#include <linux/string.h>
#include <linux/signal.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include "vgfbmx.h"
#include "vgfb.h"
#include "vg.h"

static void vm_open(struct vm_area_struct *vma);
static void vm_close(struct vm_area_struct *vma);

static const struct vm_operations_struct vm_default_ops = {
	.open = vm_open,
	.close = vm_close,
};

static const unsigned long initial_resolution[] = {800, 600};

static struct fb_ops fb_default_ops = {
	.owner = THIS_MODULE,
	.fb_read = vgfb_read,
	.fb_write = vgfb_write,
	.fb_mmap = vgfb_mmap,
	.fb_set_par = vgfb_set_par,
	.fb_check_var = vgfb_check_var,
	.fb_setcolreg = vgfb_setcolreg,
	.fb_pan_display = vgfb_pan_display,
	.fb_fillrect = vgfb_fillrect,
	.fb_copyarea = vgfb_copyarea,
	.fb_imageblit = sys_imageblit,
	.fb_destroy = vgfb_fb_destroy,
};

int vgfb_set_par(struct fb_info *info)
{
	if (info->mode->xres != info->var.xres
	 || info->mode->yres != info->var.yres)
		return -EINVAL;

	return 0;
}

int vgfb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
	if (var->bits_per_pixel != 32)
		return -EINVAL;

	if (var->xoffset != 0)
		return -EINVAL;

	if (var->yoffset > var->yres)
		return -EINVAL;

	if (var->xres != info->var.xres || var->yres != info->var.yres)
		return -EINVAL;

	*var = info->var;

	return 0;
}

int vgfb_set_screenbase(struct vgfbm *fb, void *memory)
{
	struct vm_mem_entry *entry;

	if (memory) {
		entry = kzalloc(sizeof(struct vgfbm), GFP_KERNEL);
		if (!entry)
			return -ENOMEM;
		entry->fb = fb;
		entry->memory = memory;
		mutex_init(&entry->lock);
		if (!vgfb_acquire_screen_memory(entry)) {
			pr_err("vgfb: vgfb_acquire_screen_memory failed\n");
			kfree(entry);
			return -EAGAIN;
		}
		if (fb->last_mem_entry) {
			vgfb_release_screen_memory(fb->last_mem_entry);
			fb->last_mem_entry = 0;
		}
		fb->last_mem_entry = entry;
		fb->info->screen_base = memory;
	} else {
		if (fb->last_mem_entry) {
			vgfb_release_screen_memory(fb->last_mem_entry);
			fb->last_mem_entry = 0;
		}
		fb->info->screen_base = 0;
	}
	return 0;
}

void vgfb_fb_destroy(struct fb_info *info)
{
	pr_debug("vgfb: freeing framebuffer info\n");
	framebuffer_release(info);
}

int vgfb_setcolreg(u_int regno, u_int r, u_int g, u_int b,
		 u_int a, struct fb_info *info)
{
	if (regno >= 256)
		return -EINVAL;
	((u32 *)info->pseudo_palette)[regno] =
		(r&0xFF) | ((g&0xFF)<<8) | ((b&0xFF)<<16) | ((a&0xFF)<<24);
	return 0;
}

void vgfb_fillrect(struct fb_info *info, const struct fb_fillrect *r)
{
	u32 *mem;
	u32 i, w, h, d;

	if (info->state != FBINFO_STATE_RUNNING)
		return;

	h = r->height;
	w = r->width;

	if (!w || !h)
		return;

	if (r->dx >= info->var.xres_virtual || r->dy >= info->var.yres_virtual)
		return;

	if (w > info->var.xres_virtual - r->dx)
		w = info->var.xres_virtual - r->dx;

	if (h > info->var.yres_virtual - r->dy)
		h = info->var.yres_virtual - r->dy;

	d = info->var.xres_virtual - w;
	mem = (u32 *)info->screen_base
		+ (r->dy * info->var.xres_virtual + r->dx);

	switch (r->rop) {
	case ROP_COPY:
		while (h--) {
			i = w;
			while (i--)
				(*mem++) = r->color;
			mem += d;
		}
		break;
	case ROP_XOR:
		while (h--) {
			i = w;
			while (i--)
				(*mem++) ^= r->color;
			mem += d;
		}
		break;
	}

}

void vgfb_copyarea(struct fb_info *info, const struct fb_copyarea *r)
{
	u32 *src, *dst;
	u32 w, h, d;

	if (info->state != FBINFO_STATE_RUNNING)
		return;

	h = r->height;
	w = r->width;

	if (!w || !h)
		return;

	if (r->dx >= info->var.xres_virtual || r->dy >= info->var.yres_virtual)
		return;

	if (w > info->var.xres_virtual - r->dx)
		w = info->var.xres_virtual - r->dx;

	if (h > info->var.yres_virtual - r->dy)
		h = info->var.yres_virtual - r->dy;

	if (w > info->var.xres_virtual - r->sx)
		w = info->var.xres_virtual - r->sx;

	if (h > info->var.yres_virtual - r->sy)
		h = info->var.yres_virtual - r->sy;

	d = info->var.xres_virtual - w;
	src = (u32 *)info->screen_base
		+ (r->sy * info->var.xres_virtual + r->sx);
	dst = (u32 *)info->screen_base
		+ (r->dy * info->var.xres_virtual + r->dx);

	while (h--) {
		memmove(dst, src, w*4);
		src += d;
		dst += d;
	}

}

static const struct fb_fix_screeninfo fix_screeninfo_defaults = {
	.id = "vgfb",
	.type = FB_TYPE_PACKED_PIXELS, // FB_TYPE_FOURCC
	.visual = FB_VISUAL_TRUECOLOR, //FB_VISUAL_DIRECTCOLOR, FB_VISUAL_FOURCC
//	.capabilities = FB_CAP_FOURCC,
	.accel = FB_ACCEL_NONE,
};

static const struct fb_var_screeninfo var_screeninfo_defaults = {
	.bits_per_pixel = 32,
};

int vgfb_create(struct vgfbm *fb)
{
	int ret;

	mutex_lock(&fb->lock);
	if (!vgfbm_acquire(fb)) {
		mutex_unlock(&fb->lock);
		pr_err("vgfb: vgfbm_acquire failed\n");
		return -ENOMEM;
	}
	mutex_unlock(&fb->lock);
	fb->pdev = platform_device_alloc("vgfb", PLATFORM_DEVID_AUTO);
	if (!fb->pdev) {
		pr_err("vgfb: platform_device_alloc failed\n");
		ret = -ENOMEM;
		goto failed;
	}
	platform_set_drvdata(fb->pdev, fb);
	ret = platform_device_add(fb->pdev);
	if (ret) {
		pr_err("vgfb: platform_device_add failed\n");
		goto failed_after_platform_device_alloc;
	}
	return 0;

failed_after_platform_device_alloc:
	platform_device_unregister(fb->pdev);
failed:
	return ret;
}

void vgfb_remove(struct vgfbm *fb)
{
	platform_device_unregister(fb->pdev);
	vgfbm_release(fb);
}

void vgfb_free(struct vgfbm *fb)
{
	pr_debug("vgfb: %s\n", __func__);
}

static int probe(struct platform_device *dev)
{
	int ret;
	struct vgfbm *fb = platform_get_drvdata(dev);

	if (!fb)
		return -EINVAL;

	mutex_lock(&fb->lock);
	mutex_lock(&fb->info_lock);
	if (!vgfbm_acquire(fb)) {
		pr_err("vgfb: vgfbm_acquire failed\n");
		ret = -EAGAIN;
		goto failed;
	}
	fb->info = framebuffer_alloc(sizeof(struct vgfbm *), &fb->pdev->dev);
	if (!fb->info) {
		pr_err("vgfb: framebuffer_alloc failed\n");
		ret = -ENOMEM;
		goto failed_after_acquire;
	}
	fb->info->fix = fix_screeninfo_defaults;
	fb->info->var = var_screeninfo_defaults;
	fb->info->flags = FBINFO_FLAG_DEFAULT;
	*(struct vgfbm **)fb->info->par = fb;
	INIT_LIST_HEAD(&fb->info->modelist);
	{
		fb_var_to_videomode(&fb->videomode, &fb->info->var);
		ret = fb_add_videomode(&fb->videomode, &fb->info->modelist);
		if (ret < 0) {
			pr_err("vgfb: fb_add_videomode failed\n");
			goto failed_after_framebuffer_alloc;
		}
	}
	fb->info->mode = &fb->videomode;
	fb->info->fbops = &fb_default_ops;
	fb->info->pseudo_palette = fb->colormap;
	ret = vgfbm_set_resolution(fb->info, initial_resolution);
	if (ret < 0) {
		pr_err("vgfb: vgfb_set_resolution failed\n");
		goto failed_after_framebuffer_alloc;
	}
	fb->info->var.xres = initial_resolution[0];
	fb->info->var.yres = initial_resolution[1];
	ret = fb_alloc_cmap(&fb->info->cmap, 256, 0);
	if (ret < 0) {
		pr_err("vgfb: fb_alloc_cmap failed (%d)\n", ret);
		goto failed_after_framebuffer_alloc;
	}
	ret = register_framebuffer(fb->info);
	if (ret < 0) {
		pr_err("vgfb: register_framebuffer failed (%d)\n", ret);
		goto failed_after_alloc_cmap;
	}
	mutex_unlock(&fb->info_lock);
	mutex_unlock(&fb->lock);
	return 0;

failed_after_alloc_cmap:
	fb_dealloc_cmap(&fb->info->cmap);
failed_after_framebuffer_alloc:
	framebuffer_release(fb->info);
	fb->info = 0;
failed_after_acquire:
	mutex_unlock(&fb->info_lock);
	mutex_unlock(&fb->lock);
	vgfbm_release(fb);
	return ret;

failed:
	mutex_unlock(&fb->info_lock);
	mutex_unlock(&fb->lock);
	return ret;
}

static int remove(struct platform_device *dev)
{
	struct vgfbm *fb = platform_get_drvdata(dev);

	mutex_lock(&fb->lock);
	mutex_lock(&fb->info_lock);
	if (!fb)
		return 0;
	if (fb->info) {
		vgfb_set_screenbase(fb, 0);
		fb->info->state = FBINFO_STATE_SUSPENDED;
		fb_dealloc_cmap(&fb->info->cmap);
		unregister_framebuffer(fb->info);
		fb->info = 0;
	}
	mutex_unlock(&fb->info_lock);
	mutex_unlock(&fb->lock);
	vgfbm_release(fb);

	return 0;
}

ssize_t vgfb_read(struct fb_info *info, char __user *buf, size_t count,
		loff_t *ppos)
{
	ssize_t ret;
	unsigned long offset = *ppos;
	unsigned long mem_len = info->fix.smem_len;
	struct vgfbm *fb = *(struct vgfbm **)info->par;

	mutex_lock(&fb->lock);
	if (info->state != FBINFO_STATE_RUNNING) {
		ret = -EPERM;
		goto end;
	}
	if (offset > mem_len || !info->screen_base) {
		ret = -ENOMEM;
		goto end;
	}
	if (!count) {
		ret = 0;
		goto end;
	}
	if (count > mem_len - offset)
		count = mem_len - offset;
	if (!count) {
		ret = -ENOMEM;
		goto end;
	}
	if (copy_to_user(buf, info->screen_base + offset, count)) {
		ret = -EFAULT;
		goto end;
	}
	*ppos += count;
	ret = count;

end:
	mutex_unlock(&fb->lock);
	return ret;
}

ssize_t vgfb_write(struct fb_info *info, const char __user *buf, size_t count,
		loff_t *ppos)
{
	ssize_t ret = 0;
	unsigned long offset = *ppos;
	unsigned long mem_len = info->fix.smem_len;
	struct vgfbm *fb = *(struct vgfbm **)info->par;

	mutex_lock(&fb->lock);
	if (info->state != FBINFO_STATE_RUNNING) {
		ret = -EPERM;
		goto end;
	}
	if (offset > mem_len || !info->screen_base) {
		ret = -ENOMEM;
		goto end;
	}
	if (!count) {
		ret = 0;
		goto end;
	}
	if (count > mem_len - offset)
		count = mem_len - offset;
	if (!count) {
		ret = -ENOMEM;
		goto end;
	}
	if (copy_from_user(info->screen_base + offset, buf, count)) {
		ret = -EFAULT;
		goto end;
	}
	*ppos += count;
	ret = count;

end:
	mutex_unlock(&fb->lock);
	return ret;
}

static void vm_open(struct vm_area_struct *vma)
{
	struct vm_mem_entry *entry = vma->vm_private_data;

	pr_debug("vgfb: %s\n", __func__);
	if (!vgfb_acquire_screen_memory(entry))
		panic("vgfb: vgfb_acquire_screen_memory failed");
}

static void vm_close(struct vm_area_struct *vma)
{
	struct vm_mem_entry *entry = vma->vm_private_data;

	pr_debug("vgfb: %s\n", __func__);
	vgfb_release_screen_memory(entry);
}

bool vgfb_acquire_screen_memory(struct vm_mem_entry *e)
{
	unsigned long val;

	mutex_lock(&e->lock);
	val = e->count + 1;
	if (val == 1)
		vgfbm_acquire(e->fb);
	if (!val) {
		mutex_unlock(&e->lock);
		return false;
	}
	e->count = val;
	mutex_unlock(&e->lock);
	return true;
}

void vgfb_release_screen_memory(struct vm_mem_entry *e)
{
	unsigned long val;
	struct vgfbm *fb;

	mutex_lock(&e->lock);
	val = e->count;
	if (!val) {
		mutex_unlock(&e->lock);
		pr_crit("underflow; use-after-free\n");
		dump_stack();
		return;
	}
	val--;
	e->count = val;
	mutex_unlock(&e->lock);
	if (val)
		return;
	pr_debug("vgfb: freeing screen memory %p\n", e->memory);
	vfree(e->memory);
	fb = e->fb;
	kfree(e);
	vgfbm_release(fb);
}

int vgfb_mmap(struct fb_info *info, struct vm_area_struct *vma)
{
	int ret = 0;
	struct vgfbm *fb = *(struct vgfbm **)info->par;

	mutex_lock(&fb->lock);
	if (info->state != FBINFO_STATE_RUNNING) {
		ret = -EPERM;
		goto end;
	}
	if (!fb->last_mem_entry) {
		pr_err("vgfb: screen buffer memory unavailable\n");
		ret = -ENOMEM;
		goto end;
	}
	if (!vgfb_acquire_screen_memory(fb->last_mem_entry)) {
		pr_err("vgfb: vgfb_acquire_screen_memory failed\n");
		ret = -EAGAIN;
		goto end;
	}
	ret = remap_vmalloc_range(vma, fb->last_mem_entry->memory,
				  vma->vm_pgoff);
	if (ret < 0) {
		pr_err("vgfb: remap_vmalloc_range failed (%d)\n", ret);
		goto failed;
	}
	vma->vm_flags |= VM_DONTEXPAND | VM_DONTDUMP;
	vma->vm_private_data = fb->last_mem_entry;
	vma->vm_ops = &vm_default_ops;
	pr_debug("vgfb: %s\n", __func__);
end:
	mutex_unlock(&fb->lock);
	return ret;
failed:
	mutex_unlock(&fb->lock);
	vgfb_release_screen_memory(fb->last_mem_entry);
	return ret;
}

int vgfb_pan_display(struct fb_var_screeninfo *var, struct fb_info *info)
{
	if (info->state != FBINFO_STATE_RUNNING)
		return -EPERM;
	if (var->xoffset > info->var.xres_virtual - info->var.xres)
		return -EINVAL;
	if (var->yoffset > info->var.yres_virtual - info->var.yres)
		return -EINVAL;
	info->var.xoffset = var->xoffset;
	info->var.yoffset = var->yoffset;
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
