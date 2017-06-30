#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/spinlock.h>
#include <linux/fs.h>
#include "vgd.h"
#include "vgmx.h"

static struct vgd_vgmx vgmx = {
	.name = "vgmx"
};

static int vgmx_open(struct inode *inode, struct file *file)
{
	printk(KERN_INFO "vgd vgmx: device opened\n");

	try_module_get(THIS_MODULE);

	return 0;
}

/* 
 * Called when a process closes the device file.
 */
static int vgmx_release(struct inode *inode, struct file *file)
{
	printk(KERN_INFO "vgd vgmx: device closed\n");

	module_put(THIS_MODULE);

	return 0;
}


struct file_operations vgmx_opts = {
	.open = vgmx_open,
	.release = vgmx_release
};

int __init vgmx_init(void)
{
	int ret;
	dev_t dev_major;

	printk(KERN_INFO "vgd vgmx: Initializing device\n");

	vgmx.cdev = cdev_alloc();
	if (!vgmx.cdev) {
		printk(KERN_ERR "vgd vgmx: Failed to allocate cdev\n");
		ret = -ENOMEM;
		goto failed;
	}

	vgmx.cdev->ops = &vgmx_opts;
	vgmx.cdev->owner = THIS_MODULE;
	ret = alloc_chrdev_region(&dev_major, 0, 1, vgmx.name);
	if (ret) {
		printk(KERN_ERR "vgd vgmx: Failed to register chrdev\n");
		goto failed_after_cdev_alloc;
	}
	vgmx.major = MAJOR( dev_major );
	vgmx.dev = MKDEV(vgmx.major,0);

	vgmx.vgd_class = aquire_vgd_class();
	if( !vgmx.vgd_class ){
		printk(KERN_ERR "vgd vgmx: Failed to aquire class\n");
		goto failed_after_alloc_chrdev_region;
	}

	vgmx.device = device_create(vgmx.vgd_class, 0, vgmx.dev, 0, vgmx.name);
	if( !vgmx.device ){
		printk(KERN_ERR "vgd vgmx: Failed to create device\n");
		goto failed_after_aquire_vgd_class;
	}

	ret = cdev_add(vgmx.cdev, vgmx.dev, 1);
	if (ret) {
		printk(KERN_ERR "vgd vgmx: Failed to add cdev\n");
		goto failed_after_aquire_vgd_class;
	}

	printk(KERN_INFO "vgd vgmx: Initialised, device major number: %d\n",vgmx.major);

  return 0;

	failed_after_aquire_vgd_class:
		release_vgd_class();
	failed_after_alloc_chrdev_region:
		unregister_chrdev_region(vgmx.dev, 1);
	failed_after_cdev_alloc:
		cdev_del (vgmx.cdev);
	failed:
		return ret;
}

void __exit vgmx_exit(void)
{
	printk(KERN_INFO "vgd vgmx: Unloading device\n");

	device_destroy(vgmx.vgd_class,vgmx.dev);
	release_vgd_class();
	unregister_chrdev_region(vgmx.dev, 1);
	cdev_del(vgmx.cdev);
}
