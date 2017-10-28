#include <linux/fb.h>
#include <linux/uaccess.h>
#include "vgfb.h"
#include "mode.h"

static int create(struct vgfbm* fb);
static int destroy(struct vgfbm* fb);


static const struct vgfb_mode mode = {
	.create = create,
	.destroy = destroy
};
VGFB_EXPORT_MODE(NONE,&mode)

static ssize_t read(struct fb_info *info, char __user *buf, size_t count, loff_t *ppos)
{
	unsigned long offset = *ppos;
	unsigned long mem_len = info->fix.smem_len;
	if (info->state != FBINFO_STATE_RUNNING)
		return -EPERM;
	if (offset >= mem_len)
		return 0;
	if (mem_len - offset > count)
		count = mem_len - offset;
	if (!count)
		return 0;
	if (clear_user(buf + offset, count))
		return -EFAULT;
	return count;
}

static ssize_t write(struct fb_info *info, const char __user *buf, size_t count, loff_t *ppos)
{
	unsigned long offset = *ppos;
	unsigned long mem_len = info->fix.smem_len;
	if (info->state != FBINFO_STATE_RUNNING)
		return -EPERM;
	if (offset >= mem_len)
		return 0;
	if (mem_len - offset > count)
		count = mem_len - offset;
	return count;
}

static struct fb_ops ops = {
	.owner = THIS_MODULE,
	.fb_read = read,
	.fb_write = write,
	.fb_set_par = vgfb_set_par,
	.fb_setcolreg = vgfb_setcolreg,
	.fb_pan_display = vgfb_pan_display,
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
