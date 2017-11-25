/*
 * Copyright (C) 2017 Daniel Patrick Abrecht
 *
 * This module is dual licensed under the MIT License and
 * the GNU General Public License v2.0
 */

#ifndef VGFBMX_H
#define VGFBMX_H

struct vgfbm;
struct fb_info;
struct fb_var_screeninfo;
struct fb_fix_screeninfo;

int vgfbm_get_vscreeninfo_user(const struct fb_info *info,
	struct fb_var_screeninfo __user *var);
int vgfbm_set_vscreeninfo_user(struct fb_info *info,
	struct fb_var_screeninfo __user *var);
int vgfbm_get_fscreeninfo_user(const struct fb_info *info,
	struct fb_fix_screeninfo __user *var);
int vgfbm_pan_display(struct fb_info *info,
	const struct fb_var_screeninfo __user *var);
int vgfbm_set_vscreeninfo(struct fb_info *info,
	struct fb_var_screeninfo *var);
int vgfbm_set_par(struct fb_info *info);
int vgfbm_do_set_par(struct fb_info *info);
int vgfbm_check_var(struct fb_var_screeninfo *var, struct fb_info *info);
ssize_t vgfbmx_read(struct file *file, char __user *buf, size_t count,
	loff_t *ppos);
ssize_t vgfbmx_write(struct file *file, const char __user *buf, size_t count,
	loff_t *ppos);
long vgfbmx_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
int vgfbmx_open(struct inode *inode, struct file *file);
int vgfbmx_close(struct inode *inode, struct file *file);
int vgfbmx_mmap(struct file *file, struct vm_area_struct *vma);
int vgfbm_set_resolution(struct fb_info *info,
			const unsigned long resolution[2]);

struct fb_info *vgfbm_get_info(struct vgfbm *vgfbm);
void vgfbm_put_info(struct fb_info *info);

bool vgfbm_acquire(struct vgfbm *vgfbm);
void vgfbm_release(struct vgfbm *vgfbm);

#endif
