#ifndef VGFB_MODE_H
#define VGFB_MODE_H

#include "vg.h"


struct vgfbm;
struct vgfb_mode {
	int(*create)(struct vgfbm*);
	int(*destroy)(struct vgfbm*);
};

#define VGFB_EXPORT_MODE(NAME,VALUE) const struct vgfb_mode*const vgfb_mode_ ## NAME = (VALUE);

#define X(X) extern const struct vgfb_mode*const vgfb_mode_ ## X;
VGFB_MODES
#undef X

struct fb_var_screeninfo;
struct fb_info;

int vgfb_set_par(struct fb_info *info);
int vgfb_check_var(struct fb_var_screeninfo *var, struct fb_info *info);
int vgfb_pan_display(struct fb_var_screeninfo *var, struct fb_info *info);
int vgfb_setcolreg(u_int regno, u_int red, u_int green, u_int blue, u_int transp, struct fb_info *info);

extern const struct vgfb_mode*const*const vgfb_mode[];
extern const unsigned vgfb_mode_count;

#endif
