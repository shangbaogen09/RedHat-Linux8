/*
 * Copyright 2004 James Cleverdon, IBM.
 * Subject to the GNU Public License, v.2
 *
 * Generic APIC sub-arch probe layer.
 *
 * Hacked for x86-64 by James Cleverdon from i386 architecture code by
 * Martin Bligh, Andi Kleen, James Bottomley, John Stultz, and
 * James Cleverdon.
 */
#include <linux/threads.h>
#include <linux/cpumask.h>
#include <linux/string.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/ctype.h>
#include <linux/hardirq.h>
#include <linux/dmar.h>

#include <asm/smp.h>
#include <asm/apic.h>
#include <asm/ipi.h>
#include <asm/setup.h>

/*
 * Check the APIC IDs in bios_cpu_apicid and choose the APIC mode.
 */
/*该函数会按照各个struct apic结构在.apicdrivers中的顺序，依次调用其probe接口，
第一个调用返回非0的struct apic结构就被初始化到全局变量apic*/
void __init default_setup_apic_routing(void)
{
	struct apic **drv;

	/*使能x2apic*/
	enable_IR_x2apic();

	for (drv = __apicdrivers; drv < __apicdrivers_end; drv++) {
		if ((*drv)->probe && (*drv)->probe()) {
			if (apic != *drv) {
				apic = *drv;
				/*ubuntu系统打印的log:[0.029051] Switched APIC routing to cluster x2apic.*/
				pr_info("Switched APIC routing to %s.\n",
					apic->name);
			}
			break;
		}
	}

	/*x86默认没有定义该函数*/
	if (x86_platform.apic_post_init)
		x86_platform.apic_post_init();
}

/* Same for both flat and physical. */

void apic_send_IPI_self(int vector)
{
	__default_send_IPI_shortcut(APIC_DEST_SELF, vector, APIC_DEST_PHYSICAL);
}

int __init default_acpi_madt_oem_check(char *oem_id, char *oem_table_id)
{
	struct apic **drv;

	for (drv = __apicdrivers; drv < __apicdrivers_end; drv++) {
		if ((*drv)->acpi_madt_oem_check(oem_id, oem_table_id)) {
			if (apic != *drv) {
				apic = *drv;
				pr_info("Setting APIC routing to %s.\n",
					apic->name);
			}
			return 1;
		}
	}
	return 0;
}
