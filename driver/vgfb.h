#ifndef VGFB_H
#define VGFB_H

#include <linux/list.h>
#include <linux/fb.h>

#define VGFB_REFRESH_RATE 60lu

struct vgfbm {
	struct list_head list;
	struct platform_device * pdev;
	struct fb_info * info;
	struct fb_var_screeninfo old_var;
	struct fb_videomode videomode;
	int remap_signal;
	void * mode_private;
};

ssize_t vgfb_read(struct fb_info *info, char __user *buf, size_t count, loff_t *ppos);
ssize_t vgfb_write(struct fb_info *info, const char __user *buf, size_t count, loff_t *ppos);
int vgfb_check_var(struct fb_var_screeninfo *var, struct fb_info *info);
int vgfb_realloc_screen(struct vgfbm* fb);
void vgfb_free_screen(struct vgfbm* fb);
int vgfb_mmap(struct fb_info *info, struct vm_area_struct *vma);
int vgfb_set_par(struct fb_info *info);
int vgfb_setcolreg(u_int regno, u_int red, u_int green, u_int blue, u_int transp, struct fb_info *info);
int vgfb_pan_display(struct fb_var_screeninfo *var, struct fb_info *info);

int vgfb_create(struct vgfbm*);
void vgfb_destroy(struct vgfbm*);

int vgfb_set_resolution(struct vgfbm*, unsigned long[2]);

int vgfb_init(void);
void vgfb_exit(void);

#endif
