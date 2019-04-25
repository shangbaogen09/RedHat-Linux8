// SPDX-License-Identifier: GPL-2.0
#include <linux/pci.h>
#include <linux/init.h>
#include <asm/pci_x86.h>
#include <asm/x86_init.h>

/* arch_initcall has too random ordering, so call the initializers
   in the right sequence from here. */
static __init int pci_arch_init(void)
{
/*系统默认定义了该宏.如果在编译内核时指定PCI_DIRECT，则将包含支持机制#1访问方式的二进制代码*/
#ifdef CONFIG_PCI_DIRECT
	int type = 0;

	/*先调用pci_direct_probe函数检查机制#1（或机制#2）,默认返回的type为1*/
	type = pci_direct_probe();
#endif

	if (!(pci_probe & PCI_PROBE_NOEARLY))
		pci_mmcfg_early_init();

	if (x86_init.pci.arch_init && !x86_init.pci.arch_init())
		return 0;

/*系统默认没有定义该宏*/
#ifdef CONFIG_PCI_BIOS
	pci_pcbios_init();
#endif
	/*
	 * don't check for raw_pci_ops here because we want pcbios as last
	 * fallback, yet it's needed to run first to set pcibios_last_bus
	 * in case legacy PCI probing is used. otherwise detecting peer busses
	 * fails.
	 */
/*最后调用pci_direct_init函数回过头看是否要使用机制#1（或机制#2），这种调用顺序确保了优先使用机制#1（或机制#2）*/
#ifdef CONFIG_PCI_DIRECT
	pci_direct_init(type);
#endif

	/*如果最终raw_pci_ops和raw_pci_ext_ops都还是NULL，则意味着没有找到可用的配置空间访问方法，输出一条内核错误消息*/
	if (!raw_pci_ops && !raw_pci_ext_ops)
		printk(KERN_ERR
		"PCI: Fatal: No config space access function found\n");

	dmi_check_pciprobe();

	dmi_check_skip_isa_align();

	return 0;
}
arch_initcall(pci_arch_init);
