/*包含了对模块的结构定义以及模块的版本控制*/
#include <linux/module.h>

/*包含了常用的内核函数*/
#include <linux/kernel.h>
 
/*包含了宏__init和宏__exit*/
#include <linux/init.h>
 
static int8_t* message = "buffer overrun test";

/*lkp_init()是模块初始化函数*/
static int __init lkp_init(void)
{
    printk(KERN_EMERG"Hello,Kernel-KERN_EMERG!\n");
    printk(KERN_ALERT"Hello,Kernel-KERN_ALERT!\n");
    printk(KERN_CRIT"Hello,Kernel-KERN_CRIT!\n");
    printk(KERN_ERR"Hello,Kernel-KERN_ERR!\n");
    printk(KERN_WARNING"Hello,Kernel-KERN_WARNING!\n");
    printk(KERN_NOTICE"Hello,Kernel-KERN_NOTICE!\n");
    printk(KERN_INFO"Hello,Kernel-KERN_INFO!\n");
    printk(KERN_DEBUG"Hello,Kernel-KERN_DEBUG!\n");

	panic(message);
    return 0;
}

/*lkp_cleanup()是模块的退出和清理函数*/
static void __exit lkp_cleanup(void)
{
    printk("<1>Goodbye,World! leaving kernel space...\n");
}

/*向内核注册模块提供新功能*/
module_init(lkp_init);

/*注销由模块提供所有的功能*/
module_exit(lkp_cleanup);

/*告诉内核该模块具有GNU公共许可证*/
MODULE_LICENSE("GPL");
