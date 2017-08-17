/* 2015. 8. 11
 * 
 * J. Hyun Kim
 *
 * [ Fast System Launch ]
 *
 * This code is based on Linux Suspend To RAM 
 * The differences from STR are 
 * 
 * 1. I added function sync_peripherals()
 * because unlike STR FSL really turns off the system
 * and before resuming, There needs some way to initialize peripherals,
 * and to make use of lagacy codes of suspend/resume callbacks of peripherals
 * 
 * 2. cache flushing functions
 * In that entire system is turned off and CPU cache is volatile,
 * I have to flush whole cache lines, of inner and outer.
 */
#include <linux/syscalls.h>
#include <linux/freezer.h>
#include <linux/console.h>
#include <linux/pm.h>
#include <linux/syscore_ops.h>
#include <linux/suspend.h>
#include <linux/fsl.h>
#include <linux/netdevice.h>
#include <linux/gfp.h>

#include <asm/io.h>
#include <asm/smp_plat.h>
#include <asm/cacheflush.h>
#include <asm/idmap.h>

/* 
 * To make use of lagacy suspend_ops, 
 * I import suspend_ops variable 
 */
extern const struct platform_suspend_ops *suspend_ops;
int device_io_tracing = 0;
int device_io_trace_cnt = 0;
unsigned long dev_trace_addr[NUM_DEV_TRACE];
unsigned long dev_trace_value[NUM_DEV_TRACE];
short dev_trace_write_len[NUM_DEV_TRACE];
struct page* fsl_tmp_page = 0;
int fsl_tmp;

static void dpm_time(ktime_t starttime, pm_message_t state, char *info)
{
	ktime_t calltime;
	u64 usecs64;
	int usecs;

	calltime = ktime_get();
	usecs64 = ktime_to_ns(ktime_sub(calltime, starttime));
	do_div(usecs64, NSEC_PER_USEC);
	usecs = usecs64;
	if (usecs == 0)
		usecs = 1;
	pr_info("PM: time of devices resume complete after %ld.%03ld msecs\n",
		usecs / USEC_PER_MSEC, usecs % USEC_PER_MSEC);
}


static int fsl_freeze_processes(void)
{
	int error;

	if ( error = freeze_processes() )
		return error;

	if ( error = freeze_kernel_threads() )
		thaw_processes();

	return error;
}



static int fsl_suspend_enter(void)
{ 
	int error;
	ktime_t starttime;
	
	if( suspend_ops->prepare )
	{
		if ( error = suspend_ops->prepare() )
			goto Platform_finish;
	}

	if ( error = dpm_suspend_end(PMSG_SUSPEND) )
		goto Platform_finish;

	if( suspend_ops->prepare_late )
	{
		if( error = suspend_ops->prepare_late() )
			goto Platform_wake;
	}

	printk(KERN_ALERT"FSL v1.0: Suspending system core ...\n");
	if ( error = disable_nonboot_cpus() )
		goto Enable_cpus;

	arch_suspend_disable_irqs();

	error = syscore_suspend();
	
	if( !error )
		error = fsl_enter();

	printk(KERN_ALERT"FSL v1.0: Resuming system core ...");
	syscore_resume();
	arch_suspend_enable_irqs();

Enable_cpus:
	enable_nonboot_cpus();
Platform_wake:
	printk(KERN_ALERT"FSL v1.0: Resuming devices ...\n");
	if( suspend_ops->wake )
		suspend_ops->wake();
	dpm_resume_start(PMSG_RESUME);
Platform_finish:
	if ( suspend_ops->finish )
		suspend_ops->finish();
	return error;
}

static int fsl_get_tmp_page(void)
{
	unsigned long tmp_addr;
	unsigned long pgd;
	unsigned long *switch_to;
	struct mm_struct* mm = current->mm;
	if( !fsl_tmp_page )
		fsl_tmp_page = (struct page*)alloc_pages(GFP_KERNEL,8);
	else
	{
//		printk(KERN_ALERT"FSL:Tmp page already allocated\n");
	}
	if( !fsl_tmp_page )
	{
		printk(KERN_ALERT"FSL:OOM\n");
		return -1;
	}
	pgd = (unsigned long)(mm->pgd);
	tmp_addr = phys_to_virt(page_to_pfn(fsl_tmp_page)<<12);

	switch_to = (unsigned long*)(pgd + (tmp_addr>>18)&0xFFFFFFFC );
	*switch_to &= 0x000FFFFF;
	*switch_to |= 0x80000000;

	return 0;
}
/* The main stream of FSL */
asmlinkage long sys_fsl_shutdown(void)
{

	int error;

	ktime_t starttime;
	if( error = fsl_get_tmp_page() )
		return error;

	device_io_tracing = 0;


	printk(KERN_ALERT"FSL v1.0: Syncing filesystem ...\n");
	sys_sync();

	if ( !suspend_ops )
		return -ENOSYS;

	printk(KERN_ALERT"FSL v1.0: Freezing processes ... \n");
	if ( error = fsl_freeze_processes() )
		return error;

	if ( suspend_ops->begin )
	{
		if ( error = suspend_ops->begin(PM_SUSPEND_MEM) )
			goto Close;
	}
	suspend_console();


	printk(KERN_ALERT"FSL v1.0: Suspending devices ... \n");
	if ( error = dpm_suspend_start(PMSG_SUSPEND) )
		goto Recover_platform;

	do
	{
		error = fsl_suspend_enter();
	}while( !error && suspend_ops->suspend_again && suspend_ops->suspend_again() );

Resume_devices:
	dpm_resume_end(PMSG_RESUME);
	resume_console();
Close:
	if( suspend_ops->end )
		suspend_ops->end();

	printk(KERN_ALERT"FSL v1.0: Thawing processes ... \n");
	thaw_processes();
	printk(KERN_ALERT "FSL v1.0: done!\n");
	return error;

Recover_platform:
	if( suspend_ops->recover)
		suspend_ops->recover();
	goto Resume_devices; 

}




