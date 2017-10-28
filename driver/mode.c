#include <linux/errno.h>
#include <linux/fb.h>
#include "mode.h"

const struct vgfb_mode*const*const vgfb_mode[] = {
#define X(X) &vgfb_mode_ ## X,
VGFB_MODES
#undef X
};

const unsigned vgfb_mode_count = sizeof(vgfb_mode)/sizeof(*vgfb_mode);

int vgfb_set_par(struct fb_info *info)
{
	return -EINVAL;
}

int vgfb_setcolreg(u_int regno, u_int red, u_int green, u_int blue, u_int transp, struct fb_info *info)
{
	return -EINVAL;
}

int vgfb_pan_display(struct fb_var_screeninfo *var, struct fb_info *info)
{
	if( var->xoffset || var->yoffset )
		return -EINVAL;
	return 0;
}
