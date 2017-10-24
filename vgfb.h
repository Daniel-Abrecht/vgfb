#ifndef VGFB_H
#define VGFB_H

#include <linux/list.h>

struct vgfbm;
struct vgfbm {
	struct list_head list;
	int id;
	struct platform_device * pdev;
	struct fb_info * info;
};

int vgfb_create(struct vgfbm*);
void vgfb_destroy(struct vgfbm*);

int vgfb_init(void);
void vgfb_exit(void);

#endif
