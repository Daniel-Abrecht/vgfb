/*
 * Copyright (C) 2017 Daniel Patrick Abrecht
 *
 * This module is dual licensed under the MIT License and
 * the GNU General Public License v2.0
 */

#ifndef VGFB_H
#define VGFB_H

#include <linux/completion.h>
#include <linux/mutex.h>
#include <linux/list.h>
#include <linux/fb.h>

#define VGFB_REFRESH_RATE 60lu

struct vm_mem_entry {
	struct mutex lock;
	unsigned long count;
	void *memory;
	struct vgfbm *fb;
};

struct vgfbm {
	struct mutex count_lock;
	unsigned long count;
	struct mutex lock;
	struct platform_device *pdev;
	struct mutex info_lock;
	struct fb_info *info;
	struct vm_mem_entry *last_mem_entry;
	struct fb_var_screeninfo old_var;
	struct fb_videomode videomode;
	u32 colormap[256];
};

ssize_t vgfb_read(struct fb_info *info, char __user *buf, size_t count,
	loff_t *ppos);
ssize_t vgfb_write(struct fb_info *info, const char __user *buf, size_t count,
	loff_t *ppos);
int vgfb_realloc_screen(struct vgfbm *fb);
void vgfb_free_screen(struct vgfbm *fb);
int vgfb_mmap(struct fb_info *info, struct vm_area_struct *vma);
int vgfb_set_par(struct fb_info *info);
int vgfb_check_var(struct fb_var_screeninfo *var, struct fb_info *info);
int vgfb_setcolreg(u_int regno, u_int red, u_int green, u_int blue,
	u_int transp, struct fb_info *info);
int vgfb_pan_display(struct fb_var_screeninfo *var, struct fb_info *info);
void vgfb_fillrect(struct fb_info *info, const struct fb_fillrect *rect);
void vgfb_copyarea(struct fb_info *info, const struct fb_copyarea *region);
void vgfb_imageblit(struct fb_info *info, const struct fb_image *image);
int vfb_setcolreg(u_int regno, u_int red, u_int green, u_int blue,
		 u_int transp, struct fb_info *info);
void vgfb_fb_destroy(struct fb_info *info);

bool vgfb_acquire_screen_memory(struct vm_mem_entry *fb);
void vgfb_release_screen_memory(struct vm_mem_entry *fb);
bool vgfb_check_switch(struct vgfbm *fb);

int vgfb_set_screenbase(struct vgfbm *fb, void *memory);

int vgfb_create(struct vgfbm *vgfb);
void vgfb_remove(struct vgfbm *vgfb);
void vgfb_free(struct vgfbm *vgfb);

int vgfb_init(void);
void vgfb_exit(void);

#endif
