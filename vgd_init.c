#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/spinlock.h>
#include "vgd.h"
#include "vgmx.h"


MODULE_LICENSE("Dual MIT/GPL");
MODULE_AUTHOR("Daniel Patrick Abrecht");
MODULE_DESCRIPTION(
  "Virtual graphics driver, allows to dynamically allocate "
  "virtual graphic devices such as framebuffer devices. "
  "This is intended to allow container hypervisors to"
  "provide virtual displays to it's containers on the fly."
);


struct vgd {
	unsigned count;
	struct class * class;
};

DEFINE_SPINLOCK( vgd_lock );
struct vgd vgd = {
	.count = 0,
	.class = 0
};

struct class* aquire_vgd_class( void ){
	spin_lock(&vgd_lock);
	if( vgd.count == UINT_MAX ){
		spin_unlock(&vgd_lock);
		printk(KERN_ERR "vgd: Failed aquire class, counter would overflow\n");
		return 0;
	}
	if (!vgd.count)
		vgd.class = class_create(THIS_MODULE, "vgd");
	if (!vgd.class) {
		spin_unlock(&vgd_lock);
		printk(KERN_ERR "vgd: Failed to create class\n");
		return 0;
	}
	vgd.count++;
	spin_unlock(&vgd_lock);
	return vgd.class;
}

int release_vgd_class( void ){
	spin_lock(&vgd_lock);
	if (!vgd.count) {
		spin_unlock(&vgd_lock);
		printk(KERN_ERR "vgd: class counter underflow\n");
		return -EDOM;
	}
	if (!--vgd.count)
		class_destroy( vgd.class );
	spin_unlock(&vgd_lock);
	return 0;
}

static int __init vgd_init(void){
	return vgmx_init();
}

static void __exit vgd_exit(void){
	vgmx_exit();
}

module_init(vgd_init);
module_exit(vgd_exit);
