#ifndef VGFB_MODE_H
#define VGFB_MODE_H

#include "vgfb_mode_list.h"

struct vgfbm;
struct vgfb_mode {
	int(*create)(struct vgfbm*);
	int(*destroy)(struct vgfbm*);
};

#define VGFB_EXPORT_MODE(NAME,VALUE) const struct vgfb_mode*const vgfb_mode_ ## NAME = (VALUE);

#define X(X) extern const struct vgfb_mode*const vgfb_mode_ ## X;
VGFB_MODES
#undef X

enum vgfb_mode_name {
#define X(X) VGFB_MODE_ ## X,
VGFB_MODES
#undef X
};

extern const struct vgfb_mode*const*const vgfb_mode[];
extern const unsigned vgfb_mode_count;

#endif
