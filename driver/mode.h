#ifndef VGFB_MODE_H
#define VGFB_MODE_H

#include "vg.h"

struct vgfbm;
struct vgfb_mode {
	int(*create)(struct vgfbm*);
	int(*realloc_screen)(struct vgfbm*);
	void(*free_screen)(struct vgfbm*);
	int(*destroy)(struct vgfbm*);
};

#define VGFB_EXPORT_MODE(NAME,VALUE) const struct vgfb_mode*const vgfb_mode_ ## NAME = (VALUE);

#define X(X) extern const struct vgfb_mode*const vgfb_mode_ ## X;
VGFB_MODES
#undef X

struct fb_var_screeninfo;
struct fb_info;

extern const struct vgfb_mode*const*const vgfb_mode[];
extern const unsigned vgfb_mode_count;

#endif
