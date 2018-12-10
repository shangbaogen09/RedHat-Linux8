/* ----------------------------------------------------------------------- *
 *
 *   Copyright (C) 1991, 1992 Linus Torvalds
 *   Copyright 2007 rPath, Inc. - All Rights Reserved
 *
 *   This file is part of the Linux kernel, and is made available under
 *   the terms of the GNU General Public License version 2.
 *
 * ----------------------------------------------------------------------- */

/*
 * The actual transition into protected mode
 */

#include <asm/boot.h>
#include <asm/processor-flags.h>
#include <asm/segment.h>
#include <linux/linkage.h>

	.text
	.code16

/*
 * void protected_mode_jump(u32 entrypoint, u32 bootparams);
 */
/*传入的参数entrypoint放入eax，bootparams放入edx*/
GLOBAL(protected_mode_jump)
	#首先将boot_params地址放入esi寄存器
	movl	%edx, %esi		# Pointer to boot_params table

	#将cs寄存器内容放入bx寄存器,接着执行bx<<4+标号为2的代码
	#的地址,则bx寄存器就包含标号为2的代码的地址,作为堆栈使用
	xorl	%ebx, %ebx
	movw	%cs, %bx
	shll	$4, %ebx
	addl	%ebx, 2f

	#向前跳到1标号
	jmp	1f			# Short jump to serialize on 386/486
1:

	#把数据段索引放入cx寄存器，将TSS段索引放入di寄存器
	##define GDT_ENTRY_BOOT_CS	2
	##define GDT_ENTRY_BOOT_DS	3
	##define GDT_ENTRY_BOOT_TSS	4
	##define __BOOT_CS		(GDT_ENTRY_BOOT_CS*8)
	##define __BOOT_DS		(GDT_ENTRY_BOOT_DS*8)
	##define __BOOT_TSS		(GDT_ENTRY_BOOT_TSS*8)
	movw	$__BOOT_DS, %cx
	movw	$__BOOT_TSS, %di

	#使能cr0寄存器的PE位，进入保护模式
	movl	%cr0, %edx
	orb	$X86_CR0_PE, %dl	# Protected mode
	movl	%edx, %cr0

	#0x66:操作符前缀允许我们混合执行16位和32位代码
	#0xea:跳转指令的操作符
	#in_pm32:跳转地址偏移
	#__BOOT_CS:代码段描述符索引
	# Transition to 32-bit mode
	.byte	0x66, 0xea		# ljmpl opcode
2:	.long	in_pm32			# offset
	.word	__BOOT_CS		# segment
ENDPROC(protected_mode_jump)

#跳转到32位代码执行，该位置为32位代码的第一条指令
	.code32
	.section ".text32","ax"
GLOBAL(in_pm32)
	#进入保护模式的第一步重新设置所有的段寄存器为__BOOT_DS
	# Set up data segments for flat 32-bit mode
	movl	%ecx, %ds
	movl	%ecx, %es
	movl	%ecx, %fs
	movl	%ecx, %gs
	movl	%ecx, %ss

	#设置32位代码的私有栈,也就是上面标号为2的地址
	# The 32-bit code sets up its own stack, but this way we do have
	# a valid stack if some debugging hack wants to use it.
	addl	%ebx, %esp

	#设置tr寄存器,此时di中已经存放了tss段描述符索引
	# Set up TR to make Intel VT happy
	ltr	%di

	#将所有的通用寄存器清零
	# Clear registers to allow for future extensions to the
	# 32-bit boot protocol
	xorl	%ecx, %ecx
	xorl	%edx, %edx
	xorl	%ebx, %ebx
	xorl	%ebp, %ebp
	xorl	%edi, %edi

	#设置LDTR寄存器,此时cx中已经包含ds描述符索引
	# Set up LDTR to make Intel VT happy
	lldt	%cx

	#跳转到位置boot_params.hdr.code32_start,也即是
	#startup_32(arch/x86/boot/compressed/head_64.S)
	jmpl	*%eax			# Jump to the 32-bit entrypoint
ENDPROC(in_pm32)
