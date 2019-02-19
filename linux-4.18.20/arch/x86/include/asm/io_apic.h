/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_X86_IO_APIC_H
#define _ASM_X86_IO_APIC_H

#include <linux/types.h>
#include <asm/mpspec.h>
#include <asm/apicdef.h>
#include <asm/irq_vectors.h>
#include <asm/x86_init.h>
/*
 * Intel IO-APIC support for SMP and UP systems.
 *
 * Copyright (C) 1997, 1998, 1999, 2000 Ingo Molnar
 */

/* I/O Unit Redirection Table */
#define IO_APIC_REDIR_VECTOR_MASK	0x000FF
#define IO_APIC_REDIR_DEST_LOGICAL	0x00800
#define IO_APIC_REDIR_DEST_PHYSICAL	0x00000
#define IO_APIC_REDIR_SEND_PENDING	(1 << 12)
#define IO_APIC_REDIR_REMOTE_IRR	(1 << 14)
#define IO_APIC_REDIR_LEVEL_TRIGGER	(1 << 15)
#define IO_APIC_REDIR_MASKED		(1 << 16)

/*
 * The structure of the IO-APIC:
 */
union IO_APIC_reg_00 {
	u32	raw;
	struct {
		u32	__reserved_2	: 14,
			LTS		:  1,
			delivery_type	:  1,
			__reserved_1	:  8,
			ID		:  8;
	} __attribute__ ((packed)) bits;
};

union IO_APIC_reg_01 {
	u32	raw;
	struct {
		u32	version		:  8,
			__reserved_2	:  7,
			PRQ		:  1,
			entries		:  8,
			__reserved_1	:  8;
	} __attribute__ ((packed)) bits;
};

union IO_APIC_reg_02 {
	u32	raw;
	struct {
		u32	__reserved_2	: 24,
			arbitration	:  4,
			__reserved_1	:  4;
	} __attribute__ ((packed)) bits;
};

union IO_APIC_reg_03 {
	u32	raw;
	struct {
		u32	boot_DT		:  1,
			__reserved_1	: 31;
	} __attribute__ ((packed)) bits;
};

/*内核用struct IO_APIC_route_entry结构表示RTE*/
struct IO_APIC_route_entry {

	/*Interrupt Vector，中断向量，R/W。指定该中断对应的vector，范围从10h到FEh（x86架构前16个vector被系统预留)*/
	__u32	vector		:  8,

		/*Delivery Mode，传送模式，R/W。用于指定该中断以何种方式发送给目的APIC，各种模式需要和相应的触发方式配合*/
		delivery_mode	:  3,	/* 000: FIXED
					 * 001: lowest prio
					 * 111: ExtINT
					 */

		/*Destination Mode，目的地模式，R/W*/
		dest_mode	:  1,	/* 0: physical, 1: logical */

		/*
		Delivery Status，传送状态，RO。
		0：IDEL，当前没有中断
		1：Send Pending，IOAPIC已经收到该中断，但由于某种原因该中断还未发送给LAPIC
		*/
		delivery_status	:  1,

		/*
		Interrupt Input Pin Polarity（INTPOL），中断管脚的极性，R/W。指定该管脚的有效电平是高电平还是低电平。
		0：高电平
		1：低电平
		*/
		polarity	:  1,

		/*
		Remote IRR，远程IRR，RO（只读）。只对level触发的中断有效，当该中断是edge触发时，该值代表的意义未定义。
		当中断是level触发时，LAPIC接收了该中断，该位置一，LAPIC写EOI时，该位清零。
		*/
		irr		:  1,

		/*
		Trigger Mode，触发模式，R/W。指明该管脚的的中断由什么方式触发。
		1：Level，电平触发
		2：Edge，边沿触发
		*/
		trigger		:  1,	/* 0: edge, 1: level */

		/*Interrupt Mask，中断屏蔽位，R/W。置一时，对应的中断管脚被屏蔽，这时产生的中断将被忽略。清零时，对应管脚产生的中断被发送至LAPIC*/
		mask		:  1,	/* 0: enabled, 1: disabled */

		/*17:55位，Reserved，预留未用*/
		__reserved_2	: 15;

	__u32	__reserved_3	: 24,

		/*
		Destination Field，目的字段，R/W（可读写）。根据Destination Filed（见下）值的不同，该字段值的意义不同，它有两个意义：
		Physical Mode（Destination Mode为0时）： 其值为APIC ID，用于标识一个唯一的APIC。
		Logical Mode（Destination Mode为1时）：其值根据LAPIC的不同配置，代表一组CPU（具体见LAPIC相关内容）
		*/
		dest		:  8;
} __attribute__ ((packed));

struct IR_IO_APIC_route_entry {
	__u64	vector		: 8,
		zero		: 3,
		index2		: 1,
		delivery_status : 1,
		polarity	: 1,
		irr		: 1,
		trigger		: 1,
		mask		: 1,
		reserved	: 31,
		format		: 1,
		index		: 15;
} __attribute__ ((packed));

struct irq_alloc_info;
struct ioapic_domain_cfg;

#define IOAPIC_AUTO			-1
#define IOAPIC_EDGE			0
#define IOAPIC_LEVEL			1

#define IOAPIC_MASKED			1
#define IOAPIC_UNMASKED			0

#define IOAPIC_POL_HIGH			0
#define IOAPIC_POL_LOW			1

#define IOAPIC_DEST_MODE_PHYSICAL	0
#define IOAPIC_DEST_MODE_LOGICAL	1

#define	IOAPIC_MAP_ALLOC		0x1
#define	IOAPIC_MAP_CHECK		0x2

#ifdef CONFIG_X86_IO_APIC

/*
 * # of IO-APICs and # of IRQ routing registers
 */
extern int nr_ioapics;

extern int mpc_ioapic_id(int ioapic);
extern unsigned int mpc_ioapic_addr(int ioapic);

/* # of MP IRQ source entries */
extern int mp_irq_entries;

/* MP IRQ source entries */
extern struct mpc_intsrc mp_irqs[MAX_IRQ_SOURCES];

/* 1 if "noapic" boot option passed */
extern int skip_ioapic_setup;

/* 1 if "noapic" boot option passed */
extern int noioapicquirk;

/* -1 if "noapic" boot option passed */
extern int noioapicreroute;

extern u32 gsi_top;

extern unsigned long io_apic_irqs;

#define IO_APIC_IRQ(x) (((x) >= NR_IRQS_LEGACY) || ((1 << (x)) & io_apic_irqs))

/*
 * If we use the IO-APIC for IRQ routing, disable automatic
 * assignment of PCI IRQ's.
 */
#define io_apic_assign_pci_irqs \
	(mp_irq_entries && !skip_ioapic_setup && io_apic_irqs)

struct irq_cfg;
extern void ioapic_insert_resources(void);
extern int arch_early_ioapic_init(void);

extern int save_ioapic_entries(void);
extern void mask_ioapic_entries(void);
extern int restore_ioapic_entries(void);

extern void setup_ioapic_ids_from_mpc(void);
extern void setup_ioapic_ids_from_mpc_nocheck(void);

extern int mp_find_ioapic(u32 gsi);
extern int mp_find_ioapic_pin(int ioapic, u32 gsi);
extern int mp_map_gsi_to_irq(u32 gsi, unsigned int flags,
			     struct irq_alloc_info *info);
extern void mp_unmap_irq(int irq);
extern int mp_register_ioapic(int id, u32 address, u32 gsi_base,
			      struct ioapic_domain_cfg *cfg);
extern int mp_unregister_ioapic(u32 gsi_base);
extern int mp_ioapic_registered(u32 gsi_base);

extern void ioapic_set_alloc_attr(struct irq_alloc_info *info,
				  int node, int trigger, int polarity);

extern void mp_save_irq(struct mpc_intsrc *m);

extern void disable_ioapic_support(void);

extern void __init io_apic_init_mappings(void);
extern unsigned int native_io_apic_read(unsigned int apic, unsigned int reg);
extern void native_restore_boot_irq_mode(void);

static inline unsigned int io_apic_read(unsigned int apic, unsigned int reg)
{
	/*实际的读写回调函数如下:native_io_apic_read*/
	return x86_apic_ops.io_apic_read(apic, reg);
}

extern void setup_IO_APIC(void);
extern void enable_IO_APIC(void);
extern void clear_IO_APIC(void);
extern void restore_boot_irq_mode(void);
extern int IO_APIC_get_PCI_irq_vector(int bus, int devfn, int pin);
extern void print_IO_APICs(void);
#else  /* !CONFIG_X86_IO_APIC */

#define IO_APIC_IRQ(x)		0
#define io_apic_assign_pci_irqs 0
#define setup_ioapic_ids_from_mpc x86_init_noop
static inline void ioapic_insert_resources(void) { }
static inline int arch_early_ioapic_init(void) { return 0; }
static inline void print_IO_APICs(void) {}
#define gsi_top (NR_IRQS_LEGACY)
static inline int mp_find_ioapic(u32 gsi) { return 0; }
static inline int mp_map_gsi_to_irq(u32 gsi, unsigned int flags,
				    struct irq_alloc_info *info)
{
	return gsi;
}

static inline void mp_unmap_irq(int irq) { }

static inline int save_ioapic_entries(void)
{
	return -ENOMEM;
}

static inline void mask_ioapic_entries(void) { }
static inline int restore_ioapic_entries(void)
{
	return -ENOMEM;
}

static inline void mp_save_irq(struct mpc_intsrc *m) { }
static inline void disable_ioapic_support(void) { }
static inline void io_apic_init_mappings(void) { }
#define native_io_apic_read		NULL
#define native_restore_boot_irq_mode	NULL

static inline void setup_IO_APIC(void) { }
static inline void enable_IO_APIC(void) { }
static inline void restore_boot_irq_mode(void) { }

#endif

#endif /* _ASM_X86_IO_APIC_H */
