#include <linux/module.h>
#include <linux/init.h>
 
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <asm/io.h>
#include <linux/mm.h>
#include <linux/fs.h>
 
#define MAJOR_NUM 990
#define MM_SIZE 4096
 
static char driver_name[] = "mmap_driver1";
static int dev_major = MAJOR_NUM;
static int dev_minor = 0;
char *buf = NULL;
struct cdev *cdev = NULL;
 
static int device_open(struct inode *inode, struct file *file)
{
	printk(KERN_ALERT"device open\n");
	buf = (char *)kmalloc(MM_SIZE, GFP_KERNEL);
	return 0;
}
 
static int device_close(struct inode *indoe, struct file *file)
{
	printk("device close\n");
	if(buf)
	{
		kfree(buf);
	}
	return 0;
}
 
static int device_mmap(struct file *file, struct vm_area_struct *vma)
{
	vma->vm_flags |= VM_IO;
	vma->vm_flags |= VM_LOCKED;
	if(remap_pfn_range(vma,
					   vma->vm_start,
					   virt_to_phys(buf)>>PAGE_SHIFT,
					   vma->vm_end - vma->vm_start,
					   vma->vm_page_prot))
	{
		return -EAGAIN;
	}
	return 0;
}
 
static struct file_operations device_fops =
{
	.owner = THIS_MODULE,
	.open  = device_open,
	.release = device_close,
	.mmap = device_mmap,
};
 
static int __init char_device_init( void )
{
	int result;
	dev_t dev;
	printk(KERN_ALERT"module init2323\n");
	printk("dev=%d", dev);
	dev = MKDEV(dev_major, dev_minor);
	cdev = cdev_alloc();
	printk(KERN_ALERT"module init\n");
	//if(!dev_major)
	//{
	//	result = register_chrdev_region(dev, 1, driver_name);
	//	printk("result = %d\n", result);
	//}
	//else
	//{
		result = alloc_chrdev_region(&dev, 0, 1, driver_name);
		dev_major = MAJOR(dev);
		printk("major=%d,minor=%d\n",MAJOR(dev),MINOR(dev));
	//}
	
	if(result < 0)
	{
		printk(KERN_WARNING"Cant't get major %d\n", dev_major);
		return result;
	}
	
	
	cdev_init(cdev, &device_fops);
	cdev->ops = &device_fops;
	cdev->owner = THIS_MODULE;
	
	result = cdev_add(cdev, dev, 1);
	printk("dffd = %d\n", result);
	return 0;
}
 
static void __exit char_device_exit( void )
{
	printk(KERN_ALERT"module exit\n");
	cdev_del(cdev);
	unregister_chrdev_region(MKDEV(dev_major, dev_minor), 1);
}
 
module_init(char_device_init);
module_exit(char_device_exit);
 
MODULE_LICENSE("GPL");
MODULE_AUTHOR("ChenShengfa");
