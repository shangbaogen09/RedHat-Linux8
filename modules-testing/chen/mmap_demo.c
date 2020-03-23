#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/gfp.h>
#include <linux/string.h>
#include <linux/mm_types.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/highmem.h>
#include <asm/io.h>

#include <linux/miscdevice.h>

#define	mmap_printk(args...) do {printk(KERN_ALERT "MMAP_DEMO " args); } while(0)
#define	KSTR_DEF	"hello world from kernel virtual space"
#define	mmap_name	"mmap_demo"

static struct page *pg;
static struct timer_list timer;

static void
timer_func (unsigned long data)
{
  printk ("timer_func:%s\n", (char *) data);
  timer.expires = jiffies + HZ * 10;
  add_timer (&timer);
}

static int
demo_open (struct inode *inode, struct file *filp)
{
  mmap_printk ("mmap_demo device open\n");
  return 0;
}

static int
demo_release (struct inode *inode, struct file *filp)
{
  mmap_printk ("mmap_demo device closed\n");
  return 0;
}

static int
demo_mmap (struct file *filp, struct vm_area_struct *vma)
{
  int err = 0;
  unsigned long start = vma->vm_start;
  unsigned long size = vma->vm_end - vma->vm_start;

  err = remap_pfn_range (vma, start, vma->vm_pgoff, size, vma->vm_page_prot);
  return err;
}

static struct file_operations mmap_fops = {
  .owner = THIS_MODULE,
  .open = demo_open,
  .release = demo_release,
  .mmap = demo_mmap,
};

static struct miscdevice misc = {
  .minor = MISC_DYNAMIC_MINOR,
  .name = mmap_name,
  .fops = &mmap_fops,
};

static int __init
demo_map_init (void)
{
  int ret = 0;

  char *kstr;

  pg = alloc_pages (GFP_HIGHUSER, 0);

  SetPageReserved (pg);

  kstr = (char *) kmap (pg);
  strcpy (kstr, KSTR_DEF);
  printk ("kpa = 0x%lx, kernel string = %s\n", page_to_phys (pg), kstr);

  init_timer (&timer);
  timer.function = timer_func;
  timer.data = (unsigned long) kstr;
  timer.expires = jiffies + HZ * 10;
  add_timer (&timer);

  ret = misc_register (&misc);
  printk ("the mmap miscdevice registered\n");
  return ret;
}

module_init (demo_map_init);

static void
demo_map_exit (void)
{
  del_timer_sync (&timer);

  misc_deregister (&misc);
  printk ("the device misc_mmap deregistered\n");

  kunmap (pg);
  ClearPageReserved (pg);
  __free_pages (pg, 0);
}

module_exit (demo_map_exit);
MODULE_LICENSE ("DUAL BSD/GPL");
MODULE_AUTHOR ("BG2BKK");
