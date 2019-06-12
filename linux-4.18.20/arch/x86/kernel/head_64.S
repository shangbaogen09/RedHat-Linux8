/* SPDX-License-Identifier: GPL-2.0 */
/*
 *  linux/arch/x86/kernel/head_64.S -- start in 32bit and switch to 64bit
 *
 *  Copyright (C) 2000 Andrea Arcangeli <andrea@suse.de> SuSE
 *  Copyright (C) 2000 Pavel Machek <pavel@suse.cz>
 *  Copyright (C) 2000 Karsten Keil <kkeil@suse.de>
 *  Copyright (C) 2001,2002 Andi Kleen <ak@suse.de>
 *  Copyright (C) 2005 Eric Biederman <ebiederm@xmission.com>
 */


#include <linux/linkage.h>
#include <linux/threads.h>
#include <linux/init.h>
#include <asm/segment.h>
#include <asm/pgtable.h>
#include <asm/page.h>
#include <asm/msr.h>
#include <asm/cache.h>
#include <asm/processor-flags.h>
#include <asm/percpu.h>
#include <asm/nops.h>
#include "../entry/calling.h"
#include <asm/export.h>
#include <asm/nospec-branch.h>
#include <asm/fixmap.h>

#ifdef CONFIG_PARAVIRT
#include <asm/asm-offsets.h>
#include <asm/paravirt.h>
#define GET_CR2_INTO(reg) GET_CR2_INTO_RAX ; movq %rax, reg
#else
#define GET_CR2_INTO(reg) movq %cr2, reg
#define INTERRUPT_RETURN iretq
#endif

/* we are not able to switch in one step to the final KERNEL ADDRESS SPACE
 * because we need identity-mapped pages.
 *
 */

#define l4_index(x)	(((x) >> 39) & 511)
#define pud_index(x)	(((x) >> PUD_SHIFT) & (PTRS_PER_PUD-1))

L4_PAGE_OFFSET = l4_index(__PAGE_OFFSET_BASE_L4)
L4_START_KERNEL = l4_index(__START_KERNEL_map)

L3_START_KERNEL = pud_index(__START_KERNEL_map)

/*内核解压缩后的跳转的执行地址*/
	.text
	__HEAD
	.code64
	.globl startup_64
startup_64:
	UNWIND_HINT_EMPTY
	/*
	 * At this point the CPU runs in 64bit mode CS.L = 1 CS.D = 0,
	 * and someone has loaded an identity mapped page table
	 * for us.  These identity mapped page tables map all of the
	 * kernel pages and possibly all of memory.
	 *
	 * %rsi holds a physical pointer to real_mode_data.
	 *
	 * We come here either directly from a 64bit bootloader, or from
	 * arch/x86/boot/compressed/head_64.S.
	 *
	 * We only come here initially at boot nothing else comes here.
	 *
	 * Since we may be loaded at an address different from what we were
	 * compiled to run at we first fixup the physical addresses in our page
	 * tables and then reload them.
	 */

	/*为调用verify_cpu函数设置堆栈*/
	/* Set up the stack for verify_cpu(), similar to initial_stack below */
	leaq	(__end_init_task - SIZEOF_PTREGS)(%rip), %rsp

	/* Sanitize CPU configuration */
	call verify_cpu

	/*
	 * Perform pagetable fixups. Additionally, if SME is active, encrypt
	 * the kernel and retrieve the modifier (SME encryption mask if SME
	 * is active) to be added to the initial pgdir entry that will be
	 * programmed into CR3.
	 */
	/*为调用__startup_64准备参数rdi,rsi*/
	leaq	_text(%rip), %rdi
	pushq	%rsi
	call	__startup_64
	popq	%rsi

	/* Form the CR3 value being sure to include the CR3 modifier */
	addq	$(early_top_pgt - __START_KERNEL_map), %rax

	/*针对主cpu跳转到1f地址处执行*/
	jmp 1f
ENTRY(secondary_startup_64)
	UNWIND_HINT_EMPTY
	/*
	 * At this point the CPU runs in 64bit mode CS.L = 1 CS.D = 0,
	 * and someone has loaded a mapped page table.
	 *
	 * %rsi holds a physical pointer to real_mode_data.
	 *
	 * We come here either from startup_64 (using physical addresses)
	 * or from trampoline.S (using virtual addresses).
	 *
	 * Using virtual addresses from trampoline.S removes the need
	 * to have any identity mapped pages in the kernel page table
	 * after the boot processor executes this code.
	 */

	/* Sanitize CPU configuration */
	call verify_cpu

	/*
	 * Retrieve the modifier (SME encryption mask if SME is active) to be
	 * added to the initial pgdir entry that will be programmed into CR3.
	 */
	pushq	%rsi
	call	__startup_secondary_64
	popq	%rsi

	/* Form the CR3 value being sure to include the CR3 modifier */
	addq	$(init_top_pgt - __START_KERNEL_map), %rax
1:

	/*使能pae模式，设置cr4寄存器*/
	/* Enable PAE mode, PGE and LA57 */
	movl	$(X86_CR4_PAE | X86_CR4_PGE), %ecx
#ifdef CONFIG_X86_5LEVEL
	testl	$1, __pgtable_l5_enabled(%rip)
	jz	1f
	orl	$X86_CR4_LA57, %ecx
1:
#endif
	movq	%rcx, %cr4

	/*设置启动早期的页表*/
	/* Setup early boot stage 4-/5-level pagetables. */
	addq	phys_base(%rip), %rax
	movq	%rax, %cr3

	/*跳转到1f继续执行*/
	/* Ensure I am executing from virtual addresses */
	movq	$1f, %rax
	ANNOTATE_RETPOLINE_SAFE
	jmp	*%rax
1:
	UNWIND_HINT_EMPTY

	/*检查CPU是否支持NX位,将0x80000001放入eax中,然后执行cpuid指令来得到处理器信息.
	  这条指令的结果会存放在edx中，我们把他再放到edi里*/
	/* Check if nx is implemented */
	movl	$0x80000001, %eax
	cpuid
	movl	%edx,%edi

	/*把MSR_EFER放入ecx,然后执行rdmsr指令来读取CPU中的MSR寄存器,返回结果将存放于edx:eax*/
	/* Setup EFER (Extended Feature Enable Register) */
	movl	$MSR_EFER, %ecx
	rdmsr

	/*将EFER读入edx:eax之后,通过btsl来将_EFER_SCE(即第0位)置1，设置SCE位将会启用SYSCALL以及SYSRET指令*/
	btsl	$_EFER_SCE, %eax	/* Enable System Call */

	/*检查edi(即cpuid的结果)中的第20位,如果第20位(即NX位)置位,我们就只把EFER_SCE写入MSR*/
	btl	$20,%edi		/* No Execute supported? */
	jnc     1f

	/*如果支持NX那么我们就把_EFER_NX也写入MSR*/
	btsl	$_EFER_NX, %eax
	btsq	$_PAGE_BIT_NX,early_pmd_flags(%rip)
1:	wrmsr				/* Make changes effective */

	/*
	在设置了NX后，还要对cr0（control register）中的一些位进行设置：
	X86_CR0_PE - 系统处于保护模式;
	X86_CR0_MP - 与CR0的TS标志位一同控制 WAIT/FWAIT 指令的功能；
	X86_CR0_ET - 386允许指定外部数学协处理器为80287或80387;
	X86_CR0_NE - 如果置位，则启用内置的x87浮点错误报告，否则启用PC风格的x87错误检测；
	X86_CR0_WP - 如果置位，则CPU在特权等级为0时无法写入只读内存页;
	X86_CR0_AM - 当AM位置位、EFLGS中的AC位置位、特权等级为3时，进行对齐检查;
	X86_CR0_PG - 启用分页.
	#define CR0_STATE (X86_CR0_PE | X86_CR0_MP | X86_CR0_ET | X86_CR0_NE | X86_CR0_WP 
	| X86_CR0_AM | X86_CR0_PG)
	*/
	/* Setup cr0 */
	movl	$CR0_STATE, %eax
	/* Make changes effective */
	movq	%rax, %cr0

	/*设置一个启动时用的堆栈*/
	/* Setup a boot time stack */
	movq initial_stack(%rip), %rsp

	/*把eflag寄存器清零*/
	/* zero EFLAGS after setting rsp */
	pushq $0
	popfq

	/*
	 * We must switch to a new descriptor in kernel space for the GDT
	 * because soon the kernel won't have access anymore to the userspace
	 * addresses where we're currently running on. We have to do that here
	 * because in 32bit we couldn't load a 64bit linear address.
	 */
	/*需要重新加载全局描述符表的原因:虽然目前内核工作在用户空间的低地址中，
	  但很快内核将会在它自己的内存地址空间中运行*/
	lgdt	early_gdt_descr(%rip)

	/*在加载好了新的全局描述附表之后，跟之前一样我们重新加载一下各个段*/
	/* set up data segments */
	xorl %eax,%eax
	movl %eax,%ds
	movl %eax,%ss
	movl %eax,%es

	/*
	 * We don't really need to load %fs or %gs, but load them anyway
	 * to kill any stale realmode selectors.  This allows execution
	 * under VT hardware.
	 */
	movl %eax,%fs
	movl %eax,%gs

	/* Set up %gs.
	 *
	 * The base of %gs always points to the bottom of the irqstack
	 * union.  If the stack protector canary is enabled, it is
	 * located at %gs:40.  Note that, on SMP, the boot cpu uses
	 * init data section till per cpu areas are set up.
	 */
	/*把MSR_GS_BASE放入ecx寄存器,同时利用wrmsr指令向eax和edx处的地址加载数据(即指向initial_gs)
	cs,fs,ds和ss段寄存器在64位模式下不用来寻址,但fs和gs可以使用.fs和gs有一个隐含的部分(与实模式
	下的cs段寄存器类似)这个隐含部分存储了一个描述符,其指向Model Specific Registers.因此上面的
	0xc0000101是一个gs.baseMSR地址.当发生系统调用或者中断时,入口点处并没有内核栈,因此MSR_GS_BASE
	将会用来存放中断栈。
	*/

	/*设置一下gs寄存器，令它指向一个特殊的栈irqstack ，用于处理中断*/
	movl	$MSR_GS_BASE,%ecx
	movl	initial_gs(%rip),%eax
	movl	initial_gs+4(%rip),%edx
	wrmsr

	/* rsi is pointer to real mode structure with interesting info.
	   pass it to C */
	/*准备向c函数传递参数*/
	movq	%rsi, %rdi

.Ljump_to_C_code:
	/*
	 * Jump to run C code and to be on a real kernel address.
	 * Since we are running on identity-mapped space we have to jump
	 * to the full 64bit address, this is only possible as indirect
	 * jump.  In addition we need to ensure %cs is set so we make this
	 * a far return.
	 *
	 * Note: do not change to far jump indirect with 64bit offset.
	 *
	 * AMD does not support far jump indirect with 64bit offset.
	 * AMD64 Architecture Programmer's Manual, Volume 3: states only
	 *	JMP FAR mem16:16 FF /5 Far jump indirect,
	 *		with the target specified by a far pointer in memory.
	 *	JMP FAR mem16:32 FF /5 Far jump indirect,
	 *		with the target specified by a far pointer in memory.
	 *
	 * Intel64 does support 64bit offset.
	 * Software Developer Manual Vol 2: states:
	 *	FF /5 JMP m16:16 Jump far, absolute indirect,
	 *		address given in m16:16
	 *	FF /5 JMP m16:32 Jump far, absolute indirect,
	 *		address given in m16:32.
	 *	REX.W + FF /5 JMP m16:64 Jump far, absolute indirect,
	 *		address given in m16:64.
	 */
	pushq	$.Lafter_lret	# put return address on stack for unwinder
	xorq	%rbp, %rbp	# clear frame pointer

	/*向栈中压入跳转地址*/
	movq	initial_code(%rip), %rax
	pushq	$__KERNEL_CS	# set correct cs
	pushq	%rax		# target address in negative space

	/*执行该指令，弹出压入的cs:ip值，执行initial_code标签处的指令x86_64_start_kernel
	 针对ap处理器该函数为：start_secondary*/
	lretq
.Lafter_lret:
END(secondary_startup_64)

#include "verify_cpu.S"

#ifdef CONFIG_HOTPLUG_CPU
/*
 * Boot CPU0 entry point. It's called from play_dead(). Everything has been set
 * up already except stack. We just set up stack here. Then call
 * start_secondary() via .Ljump_to_C_code.
 */
ENTRY(start_cpu0)
	movq	initial_stack(%rip), %rsp
	UNWIND_HINT_EMPTY
	jmp	.Ljump_to_C_code
ENDPROC(start_cpu0)
#endif

	/* Both SMP bootup and ACPI suspend change these variables */
	__REFDATA
	.balign	8
	GLOBAL(initial_code)
	.quad	x86_64_start_kernel
	GLOBAL(initial_gs)
	.quad	INIT_PER_CPU_VAR(irq_stack_union)

	/*初始化时0号进程的内核栈*/
	GLOBAL(initial_stack)
	/*
	 * The SIZEOF_PTREGS gap is a convention which helps the in-kernel
	 * unwinder reliably detect the end of the stack.
	 */
	.quad  init_thread_union + THREAD_SIZE - SIZEOF_PTREGS
	__FINITDATA

	__INIT
ENTRY(early_idt_handler_array)
	i = 0
	.rept NUM_EXCEPTION_VECTORS
	.if ((EXCEPTION_ERRCODE_MASK >> i) & 1) == 0
		UNWIND_HINT_IRET_REGS
		pushq $0	# Dummy error code, to make stack frame uniform
	.else
		UNWIND_HINT_IRET_REGS offset=8
	.endif
	pushq $i		# 72(%rsp) Vector number
	jmp early_idt_handler_common
	UNWIND_HINT_IRET_REGS
	i = i + 1
	.fill early_idt_handler_array + i*EARLY_IDT_HANDLER_SIZE - ., 1, 0xcc
	.endr
	UNWIND_HINT_IRET_REGS offset=16
END(early_idt_handler_array)

early_idt_handler_common:
	/*
	 * The stack is the hardware frame, an error code or zero, and the
	 * vector number.
	 */
	cld

	incl early_recursion_flag(%rip)

	/* The vector number is currently in the pt_regs->di slot. */
	pushq %rsi				/* pt_regs->si */
	movq 8(%rsp), %rsi			/* RSI = vector number */
	movq %rdi, 8(%rsp)			/* pt_regs->di = RDI */
	pushq %rdx				/* pt_regs->dx */
	pushq %rcx				/* pt_regs->cx */
	pushq %rax				/* pt_regs->ax */
	pushq %r8				/* pt_regs->r8 */
	pushq %r9				/* pt_regs->r9 */
	pushq %r10				/* pt_regs->r10 */
	pushq %r11				/* pt_regs->r11 */
	pushq %rbx				/* pt_regs->bx */
	pushq %rbp				/* pt_regs->bp */
	pushq %r12				/* pt_regs->r12 */
	pushq %r13				/* pt_regs->r13 */
	pushq %r14				/* pt_regs->r14 */
	pushq %r15				/* pt_regs->r15 */
	UNWIND_HINT_REGS

	cmpq $14,%rsi		/* Page fault? */
	jnz 10f
	GET_CR2_INTO(%rdi)	/* Can clobber any volatile register if pv */
	call early_make_pgtable
	andl %eax,%eax
	jz 20f			/* All good */

10:
	movq %rsp,%rdi		/* RDI = pt_regs; RSI is already trapnr */
	call early_fixup_exception

20:
	decl early_recursion_flag(%rip)
	jmp restore_regs_and_return_to_kernel
END(early_idt_handler_common)

	__INITDATA

	.balign 4
GLOBAL(early_recursion_flag)
	.long 0

#define NEXT_PAGE(name) \
	.balign	PAGE_SIZE; \
GLOBAL(name)

#ifdef CONFIG_PAGE_TABLE_ISOLATION
/*
 * Each PGD needs to be 8k long and 8k aligned.  We do not
 * ever go out to userspace with these, so we do not
 * strictly *need* the second page, but this allows us to
 * have a single set_pgd() implementation that does not
 * need to worry about whether it has 4k or 8k to work
 * with.
 *
 * This ensures PGDs are 8k long:
 */
#define PTI_USER_PGD_FILL	512
/* This ensures they are 8k-aligned: */
#define NEXT_PGD_PAGE(name) \
	.balign 2 * PAGE_SIZE; \
GLOBAL(name)
#else
#define NEXT_PGD_PAGE(name) NEXT_PAGE(name)
#define PTI_USER_PGD_FILL	0
#endif

/* Automate the creation of 1 to 1 mapping pmd entries */
#define PMDS(START, PERM, COUNT)			\
	i = 0 ;						\
	.rept (COUNT) ;					\
	.quad	(START) + (i << PMD_SHIFT) + (PERM) ;	\
	i = i + 1 ;					\
	.endr

	__INITDATA
NEXT_PGD_PAGE(early_top_pgt)
	.fill	512,8,0
	.fill	PTI_USER_PGD_FILL,8,0

NEXT_PAGE(early_dynamic_pgts)
	.fill	512*EARLY_DYNAMIC_PAGE_TABLES,8,0

	.data

#if defined(CONFIG_XEN_PV) || defined(CONFIG_XEN_PVH)
NEXT_PGD_PAGE(init_top_pgt)
	.quad   level3_ident_pgt - __START_KERNEL_map + _KERNPG_TABLE_NOENC
	.org    init_top_pgt + L4_PAGE_OFFSET*8, 0
	.quad   level3_ident_pgt - __START_KERNEL_map + _KERNPG_TABLE_NOENC
	.org    init_top_pgt + L4_START_KERNEL*8, 0
	/* (2^48-(2*1024*1024*1024))/(2^39) = 511 */
	.quad   level3_kernel_pgt - __START_KERNEL_map + _PAGE_TABLE_NOENC
	.fill	PTI_USER_PGD_FILL,8,0

NEXT_PAGE(level3_ident_pgt)
	.quad	level2_ident_pgt - __START_KERNEL_map + _KERNPG_TABLE_NOENC
	.fill	511, 8, 0
NEXT_PAGE(level2_ident_pgt)
	/*
	 * Since I easily can, map the first 1G.
	 * Don't set NX because code runs from these pages.
	 *
	 * Note: This sets _PAGE_GLOBAL despite whether
	 * the CPU supports it or it is enabled.  But,
	 * the CPU should ignore the bit.
	 */
	PMDS(0, __PAGE_KERNEL_IDENT_LARGE_EXEC, PTRS_PER_PMD)
#else
NEXT_PGD_PAGE(init_top_pgt)
	.fill	512,8,0
	.fill	PTI_USER_PGD_FILL,8,0
#endif

#ifdef CONFIG_X86_5LEVEL
NEXT_PAGE(level4_kernel_pgt)
	.fill	511,8,0
	.quad	level3_kernel_pgt - __START_KERNEL_map + _PAGE_TABLE_NOENC
#endif

NEXT_PAGE(level3_kernel_pgt)
	.fill	L3_START_KERNEL,8,0
	/* (2^48-(2*1024*1024*1024)-((2^39)*511))/(2^30) = 510 */
	.quad	level2_kernel_pgt - __START_KERNEL_map + _KERNPG_TABLE_NOENC
	.quad	level2_fixmap_pgt - __START_KERNEL_map + _PAGE_TABLE_NOENC

NEXT_PAGE(level2_kernel_pgt)
	/*
	 * 512 MB kernel mapping. We spend a full page on this pagetable
	 * anyway.
	 *
	 * The kernel code+data+bss must not be bigger than that.
	 *
	 * (NOTE: at +512MB starts the module area, see MODULES_VADDR.
	 *  If you want to increase this then increase MODULES_VADDR
	 *  too.)
	 *
	 *  This table is eventually used by the kernel during normal
	 *  runtime.  Care must be taken to clear out undesired bits
	 *  later, like _PAGE_RW or _PAGE_GLOBAL in some cases.
	 */
	PMDS(0, __PAGE_KERNEL_LARGE_EXEC,
		KERNEL_IMAGE_SIZE/PMD_SIZE)

NEXT_PAGE(level2_fixmap_pgt)
	.fill	(512 - 4 - FIXMAP_PMD_NUM),8,0
	pgtno = 0
	.rept (FIXMAP_PMD_NUM)
	.quad level1_fixmap_pgt + (pgtno << PAGE_SHIFT) - __START_KERNEL_map \
		+ _PAGE_TABLE_NOENC;
	pgtno = pgtno + 1
	.endr
	/* 6 MB reserved space + a 2MB hole */
	.fill	4,8,0

NEXT_PAGE(level1_fixmap_pgt)
	.rept (FIXMAP_PMD_NUM)
	.fill	512,8,0
	.endr

#undef PMDS

	.data
	.align 16
	.globl early_gdt_descr
early_gdt_descr:
	.word	GDT_ENTRIES*8-1
early_gdt_descr_base:
	.quad	INIT_PER_CPU_VAR(gdt_page)

ENTRY(phys_base)
	/* This must match the first entry in level2_kernel_pgt */
	.quad   0x0000000000000000
EXPORT_SYMBOL(phys_base)

#include "../../x86/xen/xen-head.S"

	__PAGE_ALIGNED_BSS
NEXT_PAGE(empty_zero_page)
	.skip PAGE_SIZE
EXPORT_SYMBOL(empty_zero_page)

