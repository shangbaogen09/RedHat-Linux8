/*
 * legacy.c - traditional, old school PCI bus probing
 */
#include <linux/init.h>
#include <linux/export.h>
#include <linux/pci.h>
#include <asm/jailhouse_para.h>
#include <asm/pci_x86.h>

/*
 * Discover remaining PCI buses in case there are peer host bridges.
 * We use the number of last PCI bus provided by the PCI BIOS.
 */
static void pcibios_fixup_peer_bridges(void)
{
	int n;

	if (pcibios_last_bus <= 0 || pcibios_last_bus > 0xff)
		return;
	DBG("PCI: Peer bridge fixup\n");

	for (n=0; n <= pcibios_last_bus; n++)
		pcibios_scan_specific_bus(n);
}

int __init pci_legacy_init(void)
{
	if (!raw_pci_ops)
		return 1;

	pr_info("PCI: Probing PCI hardware\n");

	/*pcibios_scan_root函数只有一个参数，就是准备扫描的PCI根总线的总线编号，它返回这条根总线对应的pci_bus。
	  如果一切正常，这条根总线的所有下级PCI总线以及PCI设备的数据结构在内存中都会被构建好*/
	pcibios_scan_root(0);
	return 0;
}

void pcibios_scan_specific_bus(int busn)
{
	int stride = jailhouse_paravirt() ? 1 : 8;
	int devfn;
	u32 l;

	if (pci_find_bus(0, busn))
		return;

	for (devfn = 0; devfn < 256; devfn += stride) {
		if (!raw_pci_read(0, busn, devfn, PCI_VENDOR_ID, 2, &l) &&
		    l != 0x0000 && l != 0xffff) {
			DBG("Found device at %02x:%02x [%04x]\n", busn, devfn, l);
			pr_info("PCI: Discovered peer bus %02x\n", busn);
			pcibios_scan_root(busn);
			return;
		}
	}
}
EXPORT_SYMBOL_GPL(pcibios_scan_specific_bus);

/*
●　通过PCI总线扫描发现系统中各级总线上的PCI设备，并对每个PCI设备进行配置；
●　在内存中为每个PCI设备构建对应的数据结构，以便Linux后续部分对它们进行操作；
●　在sysfs文件系统中构建PCI目录树，向用户空间导出信息，并且提供控制接口。
*/
static int __init pci_subsys_init(void)
{
	/*
	 * The init function returns an non zero value when
	 * pci_legacy_init should be invoked.
	 */
	/*x86_init.pci.init回调函数而言，它被定义为x86_default_pci_init,默认配置为pci_acpi_init*/
	if (x86_init.pci.init()) {/*pci_acpi_init函数被用来初始化ACPI模块,这个函数在初始化成功时返回0；否则返回错误码*/

		/*如果返回错误码，将接着调用pci_legacy_init函数*/
		if (pci_legacy_init()) {
			pr_info("PCI: System does not support PCI\n");
			return -ENODEV;
		}
	}

	pcibios_fixup_peer_bridges();
	x86_init.pci.init_irq();
	pcibios_init();

	return 0;
}
subsys_initcall(pci_subsys_init);
