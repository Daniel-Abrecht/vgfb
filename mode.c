#include "mode.h"

const struct vgfb_mode*const*const vgfb_mode[] = {
#define X(X) &vgfb_mode_ ## X,
VGFB_MODES
#undef X
};

const unsigned vgfb_mode_count = sizeof(vgfb_mode)/sizeof(*vgfb_mode);
