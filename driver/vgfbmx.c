/*
 * Copyright (C) 2017 Daniel Patrick Abrecht
 * 
 * This module is dual licensed under the MIT License and
 * the GNU General Public License v2.0
 */

#include <linux/init.h>
#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include "vgfbmx.h"
#include "vgfb.h"
#include "vg.h"

MODULE_LICENSE("Dual MIT/GPL");
MODULE_AUTHOR("Daniel Patrick Abrecht");
MODULE_DESCRIPTION(
	"Virtual graphics frame buffer driver, allows to "
	"dynamically allocate framebuffer devices. "
	"This is intended to allow container hypervisors to "
	"provide virtual displays to it's containers on the fly."
);

struct vgfbmx {
	int major;
	dev_t dev;

	struct cdev * cdev;
	struct device * device;
	struct class * vgfb_class;
};

static struct vgfbmx vgfbmx;

bool vgfbm_acquire(struct vgfbm* vgfbm){
	unsigned long val = vgfbm->count + 1;
	if (!val)
		return false;
	vgfbm->count = val;
	return true;
}

void vgfbm_release(struct vgfbm* vgfbm){
	unsigned long val;
	mutex_lock(&vgfbm->lock);
	val = vgfbm->count;
	if (!val){
		mutex_unlock(&vgfbm->lock);
		printk(KERN_CRIT "underflow; use-after-free\n");
		dump_stack();
		return;
	}
	val--;
	vgfbm->count = val;
	if (val) {
		mutex_unlock(&vgfbm->lock);
		return;
	}
	vgfb_free(vgfbm);
	mutex_unlock(&vgfbm->lock);
	kfree(vgfbm);
}

int vgfbmx_open(struct inode *inode, struct file *file)
{
	int ret = 0;
	struct vgfbm* vgfbm;

	printk(KERN_INFO "vgfbmx: device opened\n");

	vgfbm = kzalloc(sizeof(struct vgfbm), GFP_KERNEL);
	if (!vgfbm)
		return -ENOMEM;

	mutex_init(&vgfbm->lock);
	mutex_init(&vgfbm->info_lock);

	file->private_data = vgfbm;
	vgfbm_acquire(vgfbm);

	ret = vgfb_create(vgfbm);
	if (ret < 0) {
		vgfbm_release(vgfbm);
		return ret;
	}

	return 0;
}

ssize_t vgfbmx_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	int ret;
	struct vgfbm* vgfbm = file->private_data;
	mutex_lock(&vgfbm->info_lock);
	if (!vgfbm->info) {
		mutex_unlock(&vgfbm->info_lock);
		return -ENODEV;
	}
	ret = vgfb_read(vgfbm->info,buf,count,ppos);
	mutex_unlock(&vgfbm->info_lock);
	return ret;
}

ssize_t vgfbmx_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
	ssize_t ret;
	struct vgfbm* vgfbm = file->private_data;
	mutex_lock(&vgfbm->info_lock);
	if (!vgfbm->info) {
		mutex_unlock(&vgfbm->info_lock);
		return -ENODEV;
	}
	ret = vgfb_write(vgfbm->info,buf,count,ppos);
	mutex_unlock(&vgfbm->info_lock);
	return ret;
}

int vgfbmx_close(struct inode *inode, struct file *file)
{
	struct vgfbm* vgfbm = file->private_data;

	vgfb_remove(vgfbm);
	vgfbm_release(vgfbm);

	printk(KERN_INFO "vgfbmx: device closed\n");
	return 0;
}

int vgfbm_get_vscreeninfo_user(const struct vgfbm* vgfbm, struct fb_var_screeninfo __user* var){
	struct fb_var_screeninfo v;
	v = vgfbm->info->var;
	if (copy_to_user(var,&v,sizeof(v)))
		return -EFAULT;
	return 0;
}

int vgfbm_set_vscreeninfo_user(struct vgfbm* vgfbm, const struct fb_var_screeninfo __user* var) {
	struct fb_var_screeninfo v;
	int ret;

	mutex_lock(&vgfbm->lock);

	if (copy_from_user(&v,var,sizeof(v))) {
		ret = -EFAULT;
		goto end;
	}

	if (v.bits_per_pixel != 32) {
	  ret = -EINVAL;
		goto end;
	}

	if (v.xoffset != 0) {
		ret = -EINVAL;
		goto end;
	}

	if (v.yoffset > v.yres) {
		ret = -EINVAL;
		goto end;
	}

	ret = vgfb_set_resolution(vgfbm,(unsigned long[]){v.xres,v.yres});
	if (ret < 0)
		goto end;

	ret = vgfb_check_var(&v, vgfbm->info);
	if (ret < 0)
		goto end;

	vgfbm->info->var = v;

	do_vgfb_set_par(vgfbm->info);

end:
	mutex_unlock(&vgfbm->lock);
	return 0;
}

int vgfbm_pan_display(struct vgfbm* vgfbm, const struct fb_var_screeninfo __user* var){
	struct fb_var_screeninfo v;
	if (copy_from_user(&v,var,sizeof(v)))
		return -EFAULT;
	return vgfb_pan_display(&v,vgfbm->info);
}


int vgfbm_get_fscreeninfo_user(const struct vgfbm* vgfbm, struct fb_fix_screeninfo __user* fix){
	struct fb_fix_screeninfo f;
	f = vgfbm->info->fix;
	if (copy_to_user(fix,&f,sizeof(f)))
		return -EFAULT;
	return 0;
}

long vgfbmx_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	void __user* argp = (void __user*)arg;
	struct vgfbm* vgfbm = file->private_data;

	mutex_lock(&vgfbm->info_lock);
	if (!vgfbm->info || !lock_fb_info(vgfbm->info)) {
		mutex_unlock(&vgfbm->info_lock);
		return -ENODEV;
	}

	switch (cmd) {
		case FBIOGET_VSCREENINFO: ret = vgfbm_get_vscreeninfo_user(vgfbm,argp); break;
		case FBIOPUT_VSCREENINFO: ret = vgfbm_set_vscreeninfo_user(vgfbm,argp); break;
		case FBIOGET_FSCREENINFO: ret = vgfbm_get_fscreeninfo_user(vgfbm,argp); break;
		case         FBIOPUTCMAP: ret = -EINVAL; break;
		case         FBIOGETCMAP: ret = -EINVAL; break;
		case     FBIOPAN_DISPLAY: ret = vgfbm_pan_display(vgfbm,argp); break;
		case         FBIO_CURSOR: ret = -EINVAL; break;
		case   FBIOGET_CON2FBMAP: ret = -EINVAL; break;
		case   FBIOPUT_CON2FBMAP: ret = -EINVAL; break;
		case           FBIOBLANK: ret = -EINVAL; break;
		default                 : ret = vgfb_ioctl(vgfbm->info,cmd,arg); break;
	}

	unlock_fb_info(vgfbm->info);
	mutex_unlock(&vgfbm->info_lock);

	return ret;
}

int vgfbmx_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct vgfbm* vgfbm = file->private_data;
	return vgfb_mmap(vgfbm->info,vma);
}

struct file_operations vgfbmx_opts = {
	.owner = THIS_MODULE,
	.open = vgfbmx_open,
	.release = vgfbmx_close,
	.unlocked_ioctl = vgfbmx_ioctl,
	.read = vgfbmx_read,
	.write = vgfbmx_write,
	.mmap = vgfbmx_mmap,
};

int __init vgfbmx_init(void)
{
	int ret;
	dev_t dev_major;

	printk(KERN_INFO "vgfbmx: Initializing device\n");

	vgfbmx.cdev = cdev_alloc();
	if (!vgfbmx.cdev) {
		printk(KERN_ERR "vgfbmx: Failed to allocate cdev\n");
		ret = -ENOMEM;
		goto failed;
	}

	vgfbmx.cdev->ops = &vgfbmx_opts;
	vgfbmx.cdev->owner = THIS_MODULE;
	ret = alloc_chrdev_region(&dev_major, 0, 1, "vgfbmx");
	if (ret) {
		printk(KERN_ERR "vgfbmx: Failed to register chrdev\n");
		goto failed_after_cdev_alloc;
	}
	vgfbmx.major = MAJOR( dev_major );
	vgfbmx.dev = MKDEV(vgfbmx.major,0);

	vgfbmx.vgfb_class = class_create(THIS_MODULE, "vgfb");
	if( !vgfbmx.vgfb_class ){
		printk(KERN_ERR "vgfbmx: Failed to create class vgfb\n");
		goto failed_after_alloc_chrdev_region;
	}

	vgfbmx.device = device_create(vgfbmx.vgfb_class, 0, vgfbmx.dev, 0, "vgfbmx");
	if( !vgfbmx.device ){
		printk(KERN_ERR "vgfbmx: Failed to create device\n");
		goto failed_after_class_create;
	}

	ret = cdev_add(vgfbmx.cdev, vgfbmx.dev, 1);
	if (ret) {
		printk(KERN_ERR "vgfbmx: Failed to add cdev\n");
		goto failed_after_device_create;
	}

	printk(KERN_INFO "vgfbmx: Initialised, device major number: %d\n",vgfbmx.major);

	ret = vgfb_init();
	if (ret){
		printk(KERN_ERR "vgfbmx: vgfb_init failed\n");
		goto failed_after_device_create;
	}

	return 0;

	failed_after_device_create:
		device_destroy(vgfbmx.vgfb_class,vgfbmx.dev);
	failed_after_class_create:
		class_destroy(vgfbmx.vgfb_class);
	failed_after_alloc_chrdev_region:
		unregister_chrdev_region(vgfbmx.dev, 1);
	failed_after_cdev_alloc:
		cdev_del (vgfbmx.cdev);
	failed:
		return ret ? ret : -1;
}

void __exit vgfbmx_exit(void)
{
	printk(KERN_INFO "vgfbmx: Unloading device\n");

	vgfb_exit();

	device_destroy(vgfbmx.vgfb_class,vgfbmx.dev);
	class_destroy(vgfbmx.vgfb_class);
	unregister_chrdev_region(vgfbmx.dev, 1);
	cdev_del(vgfbmx.cdev);
}

module_init(vgfbmx_init);
module_exit(vgfbmx_exit);
