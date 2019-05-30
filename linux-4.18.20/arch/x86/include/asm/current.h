/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_X86_CURRENT_H
#define _ASM_X86_CURRENT_H

#include <linux/compiler.h>
#include <asm/percpu.h>

#ifndef __ASSEMBLY__
struct task_struct;

DECLARE_PER_CPU(struct task_struct *, current_task);

/*读取当前的per cpu变量获取当前任务*/
static __always_inline struct task_struct *get_current(void)
{
	return this_cpu_read_stable(current_task);
}

/*定义的current宏，用于获取当前进程的task_struct*/
#define current get_current()

#endif /* __ASSEMBLY__ */

#endif /* _ASM_X86_CURRENT_H */
