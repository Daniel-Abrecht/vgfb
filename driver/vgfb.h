#ifndef VGFB_H
#define VGFB_H

#include <linux/list.h>
#include <linux/fb.h>

#define VGFB_REFRESH_RATE 60lu

struct vgfbm {
	struct list_head list;
	struct platform_device * pdev;
	struct fb_info * info;
	struct fb_ops ops;
	struct fb_var_screeninfo old_var;
	const struct vgfb_mode * mode;
	void * mode_private;
};

int vgfb_create(struct vgfbm*);
void vgfb_destroy(struct vgfbm*);

int vgfb_set_mode(struct vgfbm*, unsigned);
int vgfb_set_resolution(struct vgfbm*, unsigned long[2]);

int vgfb_init(void);
void vgfb_exit(void);

#endif
