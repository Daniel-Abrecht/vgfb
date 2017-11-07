#ifndef VGFB_MODE_NORMAL_H
#define VGFB_MODE_NORMAL_H

static ssize_t vgfb_m_normal_read(struct fb_info *info, char __user *buf, size_t count, loff_t *ppos);
static ssize_t vgfb_m_normal_write(struct fb_info *info, const char __user *buf, size_t count, loff_t *ppos);
int vgfb_m_normal_check_var(struct fb_var_screeninfo *var, struct fb_info *info);
int vgfb_m_normal_realloc_screen(struct vgfbm* fb);
void vgfb_m_normal_free_screen(struct vgfbm* fb);
int vgfb_m_normal_mmap(struct fb_info *info, struct vm_area_struct *vma);
int vgfb_m_normal_set_par(struct fb_info *info);
int vgfb_m_normal_setcolreg(u_int regno, u_int red, u_int green, u_int blue, u_int transp, struct fb_info *info);
int vgfb_m_normal_pan_display(struct fb_var_screeninfo *var, struct fb_info *info);
static int vgfb_m_normal_create(struct vgfbm* fb);
static int vgfb_m_normal_destroy(struct vgfbm* fb);


#endif
