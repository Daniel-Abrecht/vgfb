#include <linux/fb.h>
#include "vgfb.h"
#include "mode.h"

static int create(struct vgfbm* fb);
static int destroy(struct vgfbm* fb);


static const struct vgfb_mode mode = {
	.create = create,
	.destroy = destroy
};
VGFB_EXPORT_MODE(NONE,&mode)

static struct fb_ops ops = {
	.fb_read = 0,
	.fb_write = 0,
	.fb_check_var = 0,
	.fb_set_par = 0,
	.fb_setcolreg = 0,
	.fb_pan_display = 0,
	.fb_fillrect = 0,
	.fb_copyarea = 0,
	.fb_imageblit = 0,
	.fb_mmap = 0
};

static int create(struct vgfbm* fb){
	fb->mode = &mode;
	fb->info->fbops = &ops;
	if(fb->mode != &mode)
		fb->mode->destroy(fb);
	return 0;
}

static int destroy(struct vgfbm* fb){
	return 0;
}
