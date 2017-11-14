/*
 * Copyright (C) 2017 Daniel Patrick Abrecht
 * 
 * This module is dual licensed under the MIT License and
 * the GNU General Public License v2.0
 */

#ifndef VGFBMX_H
#define VGFBMX_H

struct vgfbm;
struct fb_var_screeninfo;
struct fb_fix_screeninfo;

int vgfbm_get_vscreeninfo_user(const struct vgfbm*, struct fb_var_screeninfo __user* var);
int vgfbm_set_vscreeninfo_user(struct vgfbm*, const struct fb_var_screeninfo __user* var);
int vgfbm_get_fscreeninfo_user(const struct vgfbm*, struct fb_fix_screeninfo __user* var);
ssize_t vgfbmx_read(struct file *file, char __user *buf, size_t count, loff_t *ppos);
ssize_t vgfbmx_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos);
long vgfbmx_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
int vgfbmx_open(struct inode *inode, struct file *file);
int vgfbmx_close(struct inode *inode, struct file *file);
int vgfbmx_mmap(struct file *file, struct vm_area_struct *vma);

bool vgfbm_acquire(struct vgfbm* vgfbm);
void vgfbm_release(struct vgfbm* vgfbm);

#endif
