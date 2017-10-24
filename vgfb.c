#include <linux/platform_device.h>
#include <linux/fb.h>
#include "vgfb.h"

static const struct fb_fix_screeninfo fix_screeninfo_defaults = {
	.id = "VGFB",
	.type = FB_TYPE_PACKED_PIXELS, // FB_TYPE_FOURCC
	.visual = FB_VISUAL_TRUECOLOR, // FB_VISUAL_FOURCC
	.capabilities = FB_CAP_FOURCC,
	.xpanstep = 1,
	.ypanstep = 1,
	.ywrapstep = 1,
	.accel = FB_ACCEL_NONE,
};

static struct fb_ops zero_ops = {
	.fb_read = 0,
	.fb_write = 0,
	.fb_check_var = 0,
	.fb_set_par = 0,
	.fb_setcolreg = 0,
	.fb_pan_display = 0,
	.fb_fillrect = 0,
	.fb_copyarea = 0,
	.fb_imageblit = 0,
	.fb_mmap = 0,
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
	fb->info->flags = FBINFO_FLAG_DEFAULT;
	fb->info->fbops = &zero_ops;
	*(struct vgfbm**)fb->info->par = fb;
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
