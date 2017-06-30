#ifndef VGD_H
#define VGD_H

#include <linux/spinlock.h>

struct cdev;
struct device;
struct class;

struct vgd_vgmx {
	const char* name;

	int major;
	dev_t dev;

	struct cdev * cdev;
	struct device * device;
	struct class * vgd_class;
};

struct class* aquire_vgd_class( void );
int release_vgd_class( void );

#endif
