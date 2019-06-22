/*内核创建的第一个线程*/
static int __ref kernel_init(void *unused)
{
	kernel_init_freeable();
	
	......
}
static noinline void __init kernel_init_freeable(void)
{
    /*内核的soft lockup和hard lockup初始化*/ 
    lockup_detector_init();
        
    /*多处理器系统的初始化*/
    smp_init();

	......
}
void __init lockup_detector_init(void)
{
    /*探测硬件是否支持定期发出nmi中断*/
    if (!watchdog_nmi_probe())
        nmi_watchdog_available = true;

    /*初始化系统的lockup机制*/
    lockup_detector_setup();
}

static __init void lockup_detector_setup(void)
{
    int ret;

    /*根据系统配置，初始化watchdog_enabled变量的值*/
    lockup_detector_update_enable();

    if (!IS_ENABLED(CONFIG_SYSCTL) &&
        !(watchdog_enabled && watchdog_thresh))
        return;

    /*为每个cpu创建一个watchdog线程*/
    ret = smpboot_register_percpu_thread_cpumask(&watchdog_threads, &watchdog_allowed_mask);

    /*标志softlockup线程初始化完毕*/
    softlockup_threads_initialized = true;
}

/*结构体定义*/
struct smp_hotplug_thread {
    struct task_struct __percpu **store;
    struct list_head        list; 
    int             (*thread_should_run)(unsigned int cpu);
    void                (*thread_fn)(unsigned int cpu);
    void                (*create)(unsigned int cpu);
    void                (*setup)(unsigned int cpu);
    void                (*cleanup)(unsigned int cpu, bool online);
    void                (*park)(unsigned int cpu);
    void                (*unpark)(unsigned int cpu);
    cpumask_var_t           cpumask;
    bool                selfparking;
    const char          *thread_comm;
};

/*定义了一个percpu的struct task_struct指针*/
static DEFINE_PER_CPU(struct task_struct *, softlockup_watchdog);

/*提供给kernel的threadd内核线程的要创建的线程信息*/
static struct smp_hotplug_thread watchdog_threads = { 
    .store				= &softlockup_watchdog,
    .thread_should_run  = watchdog_should_run,
    .thread_fn			= watchdog,
    .thread_comm        = "watchdog/%u",
    .setup				= watchdog_enable,
    .cleanup			= watchdog_cleanup,
    .park				= watchdog_disable,
    .unpark				= watchdog_enable,
};


/*注册的实际处理函数*/
int smpboot_register_percpu_thread_cpumask(struct smp_hotplug_thread *plug_thread,
                       const struct cpumask *cpumask)
{
    unsigned int cpu;
    int ret = 0;

	/*分配一个cpumask结构体*/
    if (!alloc_cpumask_var(&plug_thread->cpumask, GFP_KERNEL))
        return -ENOMEM;

	/*把传入的参数拷贝到该结构体中*/
    cpumask_copy(plug_thread->cpumask, cpumask);

	/*为系统中的每一个cpu创建一个线程*/
    for_each_online_cpu(cpu) {
		/*具体的创建函数*/
        ret = __smpboot_create_thread(plug_thread, cpu);

        if (cpumask_test_cpu(cpu, cpumask))
            smpboot_unpark_thread(plug_thread, cpu);
    }

	/*把结构体加入到链表hotplug_threads末尾*/
    list_add(&plug_thread->list, &hotplug_threads);
}
EXPORT_SYMBOL_GPL(smpboot_register_percpu_thread_cpumask);

static int
__smpboot_create_thread(struct smp_hotplug_thread *ht, unsigned int cpu)
{
    /*取出percpu的指针*/
    struct task_struct *tsk = *per_cpu_ptr(ht->store, cpu);
    struct smpboot_thread_data *td;

    /*分配一个局部变量用来保存要创建线程的全部信息*/
    td = kzalloc_node(sizeof(*td), GFP_KERNEL, cpu_to_node(cpu));
    td->cpu = cpu;
    td->ht = ht;

    /*调用该函数创建线程，运行函数为smpboot_thread_fn*/
    tsk = kthread_create_on_cpu(smpboot_thread_fn, td, cpu, ht->thread_comm);
    
	/*设置该线程的状态为park*/
    kthread_park(tsk);
    get_task_struct(tsk);

    /*存储到percpu变量中*/
    *per_cpu_ptr(ht->store, cpu) = tsk;
    if (ht->create) {
        if (!wait_task_inactive(tsk, TASK_PARKED))
            WARN_ON(1);
        else
            ht->create(cpu);
    }
    return 0;
}

