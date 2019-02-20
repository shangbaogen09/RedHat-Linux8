/*
 * Interrupt descriptor table related code
 *
 * This file is licensed under the GPL V2
 */
#include <linux/interrupt.h>

#include <asm/traps.h>
#include <asm/proto.h>
#include <asm/desc.h>
#include <asm/hw_irq.h>

struct idt_data {
	unsigned int	vector;
	unsigned int	segment;
	struct idt_bits	bits;
	const void	*addr;
};

#define DPL0		0x0
#define DPL3		0x3

#define DEFAULT_STACK	0

#define G(_vector, _addr, _ist, _type, _dpl, _segment)	\
	{						\
		.vector		= _vector,		\
		.bits.ist	= _ist,			\
		.bits.type	= _type,		\
		.bits.dpl	= _dpl,			\
		.bits.p		= 1,			\
		.addr		= _addr,		\
		.segment	= _segment,		\
	}

/*中断门的定义*/
/* Interrupt gate */
#define INTG(_vector, _addr)				\
	G(_vector, _addr, DEFAULT_STACK, GATE_INTERRUPT, DPL0, __KERNEL_CS)

/* System interrupt gate */
#define SYSG(_vector, _addr)				\
	G(_vector, _addr, DEFAULT_STACK, GATE_INTERRUPT, DPL3, __KERNEL_CS)

/* Interrupt gate with interrupt stack */
#define ISTG(_vector, _addr, _ist)			\
	G(_vector, _addr, _ist, GATE_INTERRUPT, DPL0, __KERNEL_CS)

/* System interrupt gate with interrupt stack */
#define SISTG(_vector, _addr, _ist)			\
	G(_vector, _addr, _ist, GATE_INTERRUPT, DPL3, __KERNEL_CS)

/* Task gate */
#define TSKG(_vector, _gdt)				\
	G(_vector, NULL, DEFAULT_STACK, GATE_TASK, DPL0, _gdt << 3)

/*
 * Early traps running on the DEFAULT_STACK because the other interrupt
 * stacks work only after cpu_init().
 */
static const __initconst struct idt_data early_idts[] = {
	INTG(X86_TRAP_DB,		debug),
	SYSG(X86_TRAP_BP,		int3),
#ifdef CONFIG_X86_32
	INTG(X86_TRAP_PF,		page_fault),
#endif
};

/*
 * The default IDT entries which are set up in trap_init() before
 * cpu_init() is invoked. Interrupt stacks cannot be used at that point and
 * the traps which use them are reinitialized with IST after cpu_init() has
 * set up TSS.
 */

/*系统默认的cpu内部中断处理程序*/
static const __initconst struct idt_data def_idts[] = {
	/*除法错误*/
	INTG(X86_TRAP_DE,		divide_error),

	/*NMI中断*/
	INTG(X86_TRAP_NMI,		nmi),

	/*边界监测中断*/
	INTG(X86_TRAP_BR,		bounds),

	/*无效操作码*/
	INTG(X86_TRAP_UD,		invalid_op),

	/*设备不可用*/
	INTG(X86_TRAP_NM,		device_not_available),

	/*协处理器段溢出*/
	INTG(X86_TRAP_OLD_MF,		coprocessor_segment_overrun),

	/*无效TSS*/
	INTG(X86_TRAP_TS,		invalid_TSS),

	/*缺段中断*/
	INTG(X86_TRAP_NP,		segment_not_present),

	/*堆栈异常*/
	INTG(X86_TRAP_SS,		stack_segment),

	/*一般保护异常*/
	INTG(X86_TRAP_GP,		general_protection),
	INTG(X86_TRAP_SPURIOUS,		spurious_interrupt_bug),

	/*协处理器出错*/
	INTG(X86_TRAP_MF,		coprocessor_error),

	/*对齐检查中断*/
	INTG(X86_TRAP_AC,		alignment_check),
	INTG(X86_TRAP_XF,		simd_coprocessor_error),

#ifdef CONFIG_X86_32
	TSKG(X86_TRAP_DF,		GDT_ENTRY_DOUBLEFAULT_TSS),
#else
	INTG(X86_TRAP_DF,		double_fault),
#endif
	/*调试异常*/
	INTG(X86_TRAP_DB,		debug),

#ifdef CONFIG_X86_MCE
	INTG(X86_TRAP_MC,		&machine_check),
#endif

	/*溢出*/
	SYSG(X86_TRAP_OF,		overflow),
#if defined(CONFIG_IA32_EMULATION)
	SYSG(IA32_SYSCALL_VECTOR,	entry_INT80_compat),
#elif defined(CONFIG_X86_32)
	SYSG(IA32_SYSCALL_VECTOR,	entry_INT80_32),
#endif
};

/*
 * The APIC and SMP idt entries
 */
static const __initconst struct idt_data apic_idts[] = {
#ifdef CONFIG_SMP
	/*处理器间中断， 用于cpu之间通信，其他cpu要求重新调度*/
	INTG(RESCHEDULE_VECTOR,		reschedule_interrupt),

	/*处理器间中断， 用于cpu之间通信，让另外的cpu调用某个函数*/
	INTG(CALL_FUNCTION_VECTOR,	call_function_interrupt),
	INTG(CALL_FUNCTION_SINGLE_VECTOR, call_function_single_interrupt),
	INTG(IRQ_MOVE_CLEANUP_VECTOR,	irq_move_cleanup_interrupt),
	INTG(REBOOT_VECTOR,		reboot_interrupt),
#endif

#ifdef CONFIG_X86_THERMAL_VECTOR
	INTG(THERMAL_APIC_VECTOR,	thermal_interrupt),
#endif

#ifdef CONFIG_X86_MCE_THRESHOLD
	INTG(THRESHOLD_APIC_VECTOR,	threshold_interrupt),
#endif

#ifdef CONFIG_X86_MCE_AMD
	INTG(DEFERRED_ERROR_VECTOR,	deferred_error_interrupt),
#endif

#ifdef CONFIG_X86_LOCAL_APIC
	/*APIC定期器中断*/
	INTG(LOCAL_TIMER_VECTOR,	apic_timer_interrupt),
	INTG(X86_PLATFORM_IPI_VECTOR,	x86_platform_ipi),
# ifdef CONFIG_HAVE_KVM
	INTG(POSTED_INTR_VECTOR,	kvm_posted_intr_ipi),
	INTG(POSTED_INTR_WAKEUP_VECTOR, kvm_posted_intr_wakeup_ipi),
	INTG(POSTED_INTR_NESTED_VECTOR, kvm_posted_intr_nested_ipi),
# endif
# ifdef CONFIG_IRQ_WORK
	INTG(IRQ_WORK_VECTOR,		irq_work_interrupt),
# endif
#ifdef CONFIG_X86_UV
	INTG(UV_BAU_MESSAGE,		uv_bau_message_intr1),
#endif
	INTG(SPURIOUS_APIC_VECTOR,	spurious_interrupt),
	INTG(ERROR_APIC_VECTOR,		error_interrupt),
#endif
};

#ifdef CONFIG_X86_64
/*
 * Early traps running on the DEFAULT_STACK because the other interrupt
 * stacks work only after cpu_init().
 */
static const __initconst struct idt_data early_pf_idts[] = {
	INTG(X86_TRAP_PF,		page_fault),
};

/*
 * Override for the debug_idt. Same as the default, but with interrupt
 * stack set to DEFAULT_STACK (0). Required for NMI trap handling.
 */
static const __initconst struct idt_data dbg_idts[] = {
	INTG(X86_TRAP_DB,	debug),
};
#endif

/* Must be page-aligned because the real IDT is used in a fixmap. */
gate_desc idt_table[IDT_ENTRIES] __page_aligned_bss;

struct desc_ptr idt_descr __ro_after_init = {
	.size		= (IDT_ENTRIES * 2 * sizeof(unsigned long)) - 1,
	.address	= (unsigned long) idt_table,
};

#ifdef CONFIG_X86_64
/* No need to be aligned, but done to keep all IDTs defined the same way. */
gate_desc debug_idt_table[IDT_ENTRIES] __page_aligned_bss;

/*
 * The exceptions which use Interrupt stacks. They are setup after
 * cpu_init() when the TSS has been initialized.
 */
static const __initconst struct idt_data ist_idts[] = {
	ISTG(X86_TRAP_DB,	debug,		DEBUG_STACK),
	ISTG(X86_TRAP_NMI,	nmi,		NMI_STACK),
	ISTG(X86_TRAP_DF,	double_fault,	DOUBLEFAULT_STACK),
#ifdef CONFIG_X86_MCE
	ISTG(X86_TRAP_MC,	&machine_check,	MCE_STACK),
#endif
};

/*
 * Override for the debug_idt. Same as the default, but with interrupt
 * stack set to DEFAULT_STACK (0). Required for NMI trap handling.
 */
const struct desc_ptr debug_idt_descr = {
	.size		= IDT_ENTRIES * 16 - 1,
	.address	= (unsigned long) debug_idt_table,
};
#endif

/*使用struct idt_data内容初始化gate_desc　gate指针变量*/
static inline void idt_init_desc(gate_desc *gate, const struct idt_data *d)
{
	unsigned long addr = (unsigned long) d->addr;

	gate->offset_low	= (u16) addr;
	gate->segment		= (u16) d->segment;
	gate->bits		= d->bits;
	gate->offset_middle	= (u16) (addr >> 16);
#ifdef CONFIG_X86_64
	gate->offset_high	= (u32) (addr >> 32);
	gate->reserved		= 0;
#endif
}

static void
idt_setup_from_table(gate_desc *idt, const struct idt_data *t, int size, bool sys)
{
	gate_desc desc;

	/*循环处理传入的数组t*/
	for (; size > 0; t++, size--) {
		/*使用传入的数组项，初始化结构体变量desc*/
		idt_init_desc(&desc, t);

		/*把描述符填入到对应表项idt[t->vector]=desc中*/
		write_idt_entry(idt, t->vector, &desc);

		/*把apic的中断向量记录到位图system_vectors中*/
		if (sys)
			set_bit(t->vector, system_vectors);
	}
}

static void set_intr_gate(unsigned int n, const void *addr)
{
	struct idt_data data;

	BUG_ON(n > 0xFF);

	/*使用传入的函数地址初始化一个struct idt_data变量*/
	memset(&data, 0, sizeof(data));
	data.vector	= n;
	data.addr	= addr;
	data.segment	= __KERNEL_CS;
	data.bits.type	= GATE_INTERRUPT;
	data.bits.p	= 1;

	/*把结构体变量data写入到中断向量表中*/
	idt_setup_from_table(idt_table, &data, 1, false);
}

/**
 * idt_setup_early_traps - Initialize the idt table with early traps
 *
 * On X8664 these traps do not use interrupt stacks as they can't work
 * before cpu_init() is invoked and sets up TSS. The IST variants are
 * installed after that.
 */
void __init idt_setup_early_traps(void)
{
	idt_setup_from_table(idt_table, early_idts, ARRAY_SIZE(early_idts),
			     true);
	load_idt(&idt_descr);
}

/**
 * idt_setup_traps - Initialize the idt table with default traps
 */
void __init idt_setup_traps(void)
{
	idt_setup_from_table(idt_table, def_idts, ARRAY_SIZE(def_idts), true);
}

#ifdef CONFIG_X86_64
/**
 * idt_setup_early_pf - Initialize the idt table with early pagefault handler
 *
 * On X8664 this does not use interrupt stacks as they can't work before
 * cpu_init() is invoked and sets up TSS. The IST variant is installed
 * after that.
 *
 * FIXME: Why is 32bit and 64bit installing the PF handler at different
 * places in the early setup code?
 */
void __init idt_setup_early_pf(void)
{
	idt_setup_from_table(idt_table, early_pf_idts,
			     ARRAY_SIZE(early_pf_idts), true);
}

/**
 * idt_setup_ist_traps - Initialize the idt table with traps using IST
 */
void __init idt_setup_ist_traps(void)
{
	idt_setup_from_table(idt_table, ist_idts, ARRAY_SIZE(ist_idts), true);
}

/**
 * idt_setup_debugidt_traps - Initialize the debug idt table with debug traps
 */
void __init idt_setup_debugidt_traps(void)
{
	memcpy(&debug_idt_table, &idt_table, IDT_ENTRIES * 16);

	idt_setup_from_table(debug_idt_table, dbg_idts, ARRAY_SIZE(dbg_idts), false);
}
#endif

/**
 * idt_setup_apic_and_irq_gates - Setup APIC/SMP and normal interrupt gates
 */
void __init idt_setup_apic_and_irq_gates(void)
{
	/*第一个外部中断向量从32开始*/
	int i = FIRST_EXTERNAL_VECTOR;
	void *entry;

	/*设置apic中断控制器中的中断向量,具体处理是把数组的内容依次赋值到中断描述符表idt_table*/
	idt_setup_from_table(idt_table, apic_idts, ARRAY_SIZE(apic_idts), true);

	for_each_clear_bit_from(i, system_vectors, FIRST_SYSTEM_VECTOR) {
		/*取出初始化后的中断处理片段条目,每个条目的大小为8个字节*/
		entry = irq_entries_start + 8 * (i - FIRST_EXTERNAL_VECTOR);

		/*写入到对应的中断向量号i对应的中断向量表中*/
		set_intr_gate(i, entry);
	}

#ifdef CONFIG_X86_LOCAL_APIC
	/*把没有设置中断向量处理函数的中断向量表设置为默认的spurious_interrupt处理函数*/
	for_each_clear_bit_from(i, system_vectors, NR_VECTORS) {
		set_bit(i, system_vectors);
		set_intr_gate(i, spurious_interrupt);
	}
#endif
}

/**
 * idt_setup_early_handler - Initializes the idt table with early handlers
 */
void __init idt_setup_early_handler(void)
{
	int i;

	/*初始化前32个中断描述符表处理函数*/
	for (i = 0; i < NUM_EXCEPTION_VECTORS; i++)
		set_intr_gate(i, early_idt_handler_array[i]);
#ifdef CONFIG_X86_32
	for ( ; i < NR_VECTORS; i++)
		set_intr_gate(i, early_ignore_irq);
#endif

	/*把中断描述符表加载到中断描述符寄存器*/
	load_idt(&idt_descr);
}

/**
 * idt_invalidate - Invalidate interrupt descriptor table
 * @addr:	The virtual address of the 'invalid' IDT
 */
void idt_invalidate(void *addr)
{
	struct desc_ptr idt = { .address = (unsigned long) addr, .size = 0 };

	load_idt(&idt);
}

void __init update_intr_gate(unsigned int n, const void *addr)
{
	if (WARN_ON_ONCE(!test_bit(n, system_vectors)))
		return;
	set_intr_gate(n, addr);
}

void alloc_intr_gate(unsigned int n, const void *addr)
{
	BUG_ON(n < FIRST_SYSTEM_VECTOR);
	if (!test_and_set_bit(n, system_vectors))
		set_intr_gate(n, addr);
}
