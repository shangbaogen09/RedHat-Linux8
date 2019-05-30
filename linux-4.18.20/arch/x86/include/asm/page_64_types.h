/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_X86_PAGE_64_DEFS_H
#define _ASM_X86_PAGE_64_DEFS_H

#ifndef __ASSEMBLY__
#include <asm/kaslr.h>
#endif

/*默认# CONFIG_KASAN is not set*/
#ifdef CONFIG_KASAN
#define KASAN_STACK_ORDER 1
#else
#define KASAN_STACK_ORDER 0
#endif

/*默认order=2*/
#define THREAD_SIZE_ORDER	(2 + KASAN_STACK_ORDER)

/*内核栈的大小为4页*/
#define THREAD_SIZE  (PAGE_SIZE << THREAD_SIZE_ORDER)
#define CURRENT_MASK (~(THREAD_SIZE - 1))

/*异常栈的大小为1页*/
#define EXCEPTION_STACK_ORDER (0 + KASAN_STACK_ORDER)
#define EXCEPTION_STKSZ (PAGE_SIZE << EXCEPTION_STACK_ORDER)

#define DEBUG_STACK_ORDER (EXCEPTION_STACK_ORDER + 1)
#define DEBUG_STKSZ (PAGE_SIZE << DEBUG_STACK_ORDER)

/*IRQ堆栈的大小为4页*/
#define IRQ_STACK_ORDER (2 + KASAN_STACK_ORDER)
#define IRQ_STACK_SIZE (PAGE_SIZE << IRQ_STACK_ORDER)

#define DOUBLEFAULT_STACK 1
#define NMI_STACK 2
#define DEBUG_STACK 3
#define MCE_STACK 4
#define N_EXCEPTION_STACKS 4  /* hw limit: 7 */

/*
 * Set __PAGE_OFFSET to the most negative possible address +
 * PGDIR_SIZE*16 (pgd slot 272).  The gap is to allow a space for a
 * hypervisor to fit.  Choosing 16 slots here is arbitrary, but it's
 * what Xen requires.
 */
#define __PAGE_OFFSET_BASE_L5	_AC(0xff10000000000000, UL)

/*定义内核的虚拟地址空间的起始地址,ffff880000000000 - ffffc7ffffffffff(64TB大小),一致性映射区,直接映射所有的物理内存*/
#define __PAGE_OFFSET_BASE_L4	_AC(0xffff880000000000, UL)

/*默认定义了CONFIG_DYNAMIC_MEMORY_LAYOUT=y*/
#ifdef CONFIG_DYNAMIC_MEMORY_LAYOUT
/*该宏的定义page_offset_base = __PAGE_OFFSET_BASE_L4;*/
#define __PAGE_OFFSET           page_offset_base
#else
#define __PAGE_OFFSET           __PAGE_OFFSET_BASE_L4
#endif /* CONFIG_DYNAMIC_MEMORY_LAYOUT */

/*定义为内核代码映射的虚拟地址ffffffff80000000 - ffffffffa0000000(512MB大小)内核代码(从0号页帧开始512M)*/
#define __START_KERNEL_map	_AC(0xffffffff80000000, UL)

/* See Documentation/x86/x86_64/mm.txt for a description of the memory map. */

#define __PHYSICAL_MASK_SHIFT	52

#ifdef CONFIG_X86_5LEVEL
#define __VIRTUAL_MASK_SHIFT	(pgtable_l5_enabled() ? 56 : 47)
#else
/*x64位系统默认定义为47*/
#define __VIRTUAL_MASK_SHIFT	47
#endif

/*
 * Kernel image size is limited to 1GiB due to the fixmap living in the
 * next 1GiB (see level2_kernel_pgt in arch/x86/kernel/head_64.S). Use
 * 512MiB by default, leaving 1.5GiB for modules once the page tables
 * are fully set up. If kernel ASLR is configured, it can extend the
 * kernel page table mapping, reducing the size of the modules area.
 */
#if defined(CONFIG_RANDOMIZE_BASE)
#define KERNEL_IMAGE_SIZE	(1024 * 1024 * 1024)
#else
#define KERNEL_IMAGE_SIZE	(512 * 1024 * 1024)
#endif

#endif /* _ASM_X86_PAGE_64_DEFS_H */
