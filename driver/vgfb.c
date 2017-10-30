#include <linux/platform_device.h>
#include <linux/string.h>
#include <linux/fb.h>
#include "vgfb.h"
#include "mode.h"

static const struct fb_fix_screeninfo fix_screeninfo_defaults = {
	.id = "vgfb",
	.type = FB_TYPE_PACKED_PIXELS, // FB_TYPE_FOURCC
	.visual = FB_VISUAL_TRUECOLOR, // FB_VISUAL_FOURCC
//	.capabilities = FB_CAP_FOURCC,
	.xpanstep = 1,
	.ypanstep = 1,
	.ywrapstep = 1,
	.accel = FB_ACCEL_NONE,
};

static const struct fb_var_screeninfo var_screeninfo_defaults = {
	.bits_per_pixel = 32,
	.red   = { 0,8,0},
	.green = { 8,8,0},
	.blue  = {16,8,0},
};

int vgfb_create(struct vgfbm* fb)
{
	int ret;
	fb->pdev = platform_device_alloc("vgfb", fb->id);
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
	vgfb_mode_NONE->create(fb);
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
		unregister_framebuffer(fb->info);
		framebuffer_release(fb->info);
		fb->info = 0;
	}
	return 0;
}

int vgfb_set_mode(struct vgfbm* fb, unsigned mode)
{
	const struct vgfb_mode* m;
	if (mode >= vgfb_mode_count)
		return -EINVAL;
	m = *vgfb_mode[mode];
	return m->create(fb);
}

int vgfb_set_resolution(struct vgfbm* fb, unsigned long resolution[2])
{
	int ret;
	struct fb_videomode mode;
	struct fb_var_screeninfo var = fb->info->var;
	struct list_head *it, *hit;
	bool found = false;

	if( resolution[0] == 0 || resolution[1] == 0 )
		return -EINVAL;

	list_for_each_safe (it, hit, &fb->info->modelist) {
		struct fb_videomode* mode = &list_entry(it, struct fb_modelist, list)->mode;
		if(mode->xres == 0)
			continue;
		if(mode->xres == resolution[0] && mode->yres == resolution[1]){
			found = true;
			continue;
		}
		if(mode->xres == fb->info->var.xres && mode->xres == fb->info->var.xres)
			continue;
		list_del(it);
		kfree(it);
	}

	if( found )
		return 0;

	var.xres = resolution[0];
	var.yres = resolution[1];
	var.xres_virtual = resolution[0];
	var.yres_virtual = resolution[1] * 2;
	var.width = resolution[2];
	var.height = resolution[3];
	fb_var_to_videomode(&mode, &var);
	mode.refresh = 60;

	ret = fb_add_videomode(&mode, &fb->info->modelist);
	if (ret < 0)
		goto failed;
	return 0;

failed:
	return ret;
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
