/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Real-mode blob header; this should match realmode.h and be
 * readonly; for mutable data instead add pointers into the .data
 * or .bss sections as appropriate.
 */

#include <linux/linkage.h>
#include <asm/page_types.h>
#include <asm/segment.h>

#include "realmode.h"
	
	.section ".header", "a"

/*全局变量real_mode_header的定义*/
	.balign	16
GLOBAL(real_mode_header)
	.long	pa_text_start
	.long	pa_ro_end
	/* SMP trampoline */
	/*在文件arch/x86/realmode/rm/pasyms.h中赋值为pa_trampoline_start = trampoline_start*/
	.long	pa_trampoline_start
	.long	pa_trampoline_status
	.long	pa_trampoline_header
#ifdef CONFIG_X86_64
	.long	pa_trampoline_pgd;
#endif
	/* ACPI S3 wakeup */
#ifdef CONFIG_ACPI_SLEEP
	.long	pa_wakeup_start
	.long	pa_wakeup_header
#endif
	/* APM/BIOS reboot */
	.long	pa_machine_real_restart_asm
#ifdef CONFIG_X86_64
	.long	__KERNEL32_CS
#endif
END(real_mode_header)

	/* End signature, used to verify integrity */
	.section ".signature","a"
	.balign 4
GLOBAL(end_signature)
	.long	REALMODE_END_SIGNATURE
END(end_signature)
