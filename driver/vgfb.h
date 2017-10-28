#ifndef VGFB_H
#define VGFB_H

#include <linux/list.h>

struct vgfbm {
	struct list_head list;
	int id;
	struct platform_device * pdev;
	struct fb_info * info;
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
