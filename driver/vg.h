#ifndef VG_H
#define VG_H

#include "vgfb_mode_list.h"

enum vgfb_mode_name {
#define X(X) VGFB_MODE_ ## X,
VGFB_MODES
#undef X
};

#include "vgioctl.h"

#endif
