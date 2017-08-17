#include <linux/syscalls.h>
#include <linux/fsl.h>
#include <linux/freezer.h>
#include <linux/console.h>
#include <linux/pm.h>
#include <linux/syscore_ops.h>
#include <linux/suspend.h>
#include <linux/netdevice.h>
#include <linux/gfp.h>

#include <asm/io.h>
#include <asm/smp_plat.h>
#include <asm/cacheflush.h>
#include <asm/tlbflush.h>
#include <asm/idmap.h>
#include <asm/outercache.h>

#define NETWORK_BASE 0xF001E000
/*
 * To make use of suspend/resume callbacks of peripherals,
 * this function makes peripherals initialized and then suspended
 */
static void sync_peripherals(void)
{
	int i;
	struct net *net = current->nsproxy->net_ns;
	struct net_device *ndev = __dev_get_by_name(net, "eth0");
	struct net_local *lp = netdev_priv(ndev);


	/* re-init char devs... */

	for( i=0 ; i<device_io_trace_cnt ; i++)
	{
		switch( dev_trace_write_len[i] )
		{
			case 1:
				writeb_relaxed( (char)dev_trace_value[i], dev_trace_addr[i]);
				break;
			case 2:
				writew_relaxed( (short)dev_trace_value[i], dev_trace_addr[i]);
				break;
			case 4:
				writel_relaxed( (long)dev_trace_value[i], dev_trace_addr[i]);
				break;
		}
	}


/* 
 * lagacy device re-initializing codes which are 
 * useless for now
 */

/*
	  writel_relaxed(0xDF0D, 0xf0002000 + 0x8 ); 
      writel_relaxed(0x22, 0xf0002000 + 0x244 ); 
	  writel_relaxed(0x40400000, 0xf0002000 + 0x258 ); 
				    
	  writel_relaxed(0x15, 0xf0004000 + 0x00 ); 
	  writel_relaxed(0x0, 0xf0004000 + 0x0c ); 
*/	 
	/* re-init net devs... */
/*
	  writel_relaxed(0x1C0000, NETWORK_BASE + 0x4 );
	  
	  writel_relaxed(0x10, NETWORK_BASE + 0x0 );
	  writel_relaxed(0x638A0000, NETWORK_BASE + 0x34 );
	  writel_relaxed(0x638E0000, NETWORK_BASE + 0x34 );
	  writel_relaxed(0x30, NETWORK_BASE + 0x0 );
	  writel_relaxed(0xFFFFFFFF, NETWORK_BASE + 0x14 );
	  writel_relaxed(0xFFFFFFFF, NETWORK_BASE + 0x20 );
	  writel_relaxed(0xFFFFFFFF, NETWORK_BASE + 0x2C );
	  writel_relaxed(0x350A00, NETWORK_BASE + 0x88 );
	  writel_relaxed(0x2201, NETWORK_BASE + 0x8C );
	  writel_relaxed(0x039E2003, NETWORK_BASE + 0x4 );
	  writel_relaxed(0x67841000, NETWORK_BASE + 0x18 );
	  writel_relaxed(0x67842000, NETWORK_BASE + 0x1C );
	  writel_relaxed(0x00180f10, NETWORK_BASE + 0x10 );
	  writel_relaxed(0x1C, NETWORK_BASE + 0x0 );
	  writel_relaxed(0x03FC7FFE, NETWORK_BASE + 0x28 );

	  netif_stop_queue(ndev);  */
	  //writel_relaxed(NETWORK_BASE + 0x , 0x );

}

extern int __fsl_cpu_suspend( int , int (*)(unsigned long), u32 cpuid );


int fsl_enter(void)
{
	struct mm_struct *mm = current->active_mm;
	u32 __mpdir = cpu_logical_map(smp_processor_id());
	int error;

//	printk(KERN_ALERT"outer: %p\n",&outer_cache);

	if( !idmap_pgd )
		return -EINVAL;

	error = __fsl_cpu_suspend(0,0,__mpdir);
	if(error == 0)
	{
		sync_peripherals();
		cpu_switch_mm(mm->pgd, mm);
		local_flush_bp_all();
		local_flush_tlb_all();
	}
	return error;
}

/* 
 * Not sure, but I heard that after getting ARM co-processors' value,
 * there needs some way to delay...
 * the code is from U-Boot
 */
static void cp_delay(void)
{
	volatile int i;
	for( i=0 ; i<100 ; i++)
		nop();
	asm volatile("" ::: "memory");
}

#define SHARED_LONG 0x80000000
#define MMU_BIT (1<<0)
#define ICACHE_BIT (1<<12)
#define DCACHE_BIT (1<<2)

/* 
 * To share memory with Bootloader,
 * I hard-coded 0x80000000 as sharable
 * I know it is somewhat coarse way to mark,
 * please make this code more beautiful
 */
static void turn_mmu_off_and_mark(void)
{
	int reg;
	unsigned long resume_addr = (unsigned long)virt_to_phys(cpu_resume);
	int tmp;

	/* disable i-cache */
	asm volatile("mrc p15, 0, %0, c1, c0, 0" : "=r"(reg) : : "cc");
	cp_delay();
	reg &= ~ICACHE_BIT;
	asm volatile("mcr p15, 0, %0, c1, c0, 0" : : "r"(reg) : "cc");

	/* invalidate i-cache */
	asm volatile("mcr p15, 0, %0, c7, c5, 0" : : "r"(0) );
	asm volatile("mcr p15, 0, %0, c7, c5, 6" : : "r"(0) );
	asm volatile ("mcr p15, 0, %0, c7, c10, 4" : : "r" (0));
	asm volatile ("mcr p15, 0, %0, c7, c5, 4" : : "r" (0));
	
	/* disable d-cache and MMU*/

	asm volatile("mrc p15, 0, %0, c1, c0, 0" : "=r"(reg) : : "cc");
	cp_delay();
	reg &= ~DCACHE_BIT;
	reg &= ~MMU_BIT;

	
	asm volatile("mov r5, %0" : :"r"(resume_addr) :"cc");
	asm volatile("dsb" );
	asm volatile("isb" );
	asm volatile("mcr p15, 0, %0, c1, c0, 0" : : "r"(reg) : "cc");

	/* Mark resume address at SHARED_LONG region*/
	asm volatile("mov r0, #0x80000000");
	asm volatile("str r5, [r0]" );

	while(1);

	asm volatile("wfi" );

	/* I intended to re-enable MMU and
	   show "Please turn me off manually"
	   without wfi... */
	reg |= MMU_BIT;
	reg |= DCACHE_BIT;
	asm volatile("mcr p15, 0, %0, c1, c0, 0" : : "r"(reg) : "cc");
	isb();	
}

/* 
 * fsl_mark() Not established perfectly, needs rework.
 */
void fsl_mark(void)
{
	struct mm_struct *mm = current->active_mm;
	unsigned long *tmp_addr;
	unsigned long *mark;
	pgd_t *pgd; 
	pud_t *pud; 
	pmd_t *pmd; pmd_t pmd_orig;
	pte_t *pte;
	tmp_addr = phys_to_virt( page_to_pfn(fsl_tmp_page)<<12 );
	
	pgd = pgd_offset(mm,(unsigned long)tmp_addr);
	pud = pud_offset(pgd,(unsigned long)tmp_addr);
	pmd = pmd_offset(pud,(unsigned long)tmp_addr);
	__flush_tlb_all();
	__cpuc_flush_kern_all();
	__cpuc_flush_user_all();

	outer_clean_range( (phys_addr_t)0x0, (phys_addr_t) 0xFFFFFFFF );

	pgd = pgd_offset(mm,(unsigned long)tmp_addr);
	pud = pud_offset(pgd,(unsigned long)tmp_addr);
	pmd = pud_offset(pud,(unsigned long)tmp_addr);
	pte = pte_offset_map(pmd,(unsigned long)tmp_addr);
	*tmp_addr=(unsigned long)(virt_to_phys(cpu_resume));

}
void fsl_suspend_save( u32 *ptr, u32 ptrsz, u32 sp, u32 *save_ptr)
{
	u32 *ctx = ptr;



	*save_ptr = virt_to_phys(ptr);

	*ptr++ = virt_to_phys(idmap_pgd);
	*ptr++ = sp;
	*ptr++ = virt_to_phys(cpu_do_resume);

	cpu_do_suspend(ptr);
	flush_cache_louis();
	device_io_tracing = 0;

	fsl_mark();
	__cpuc_flush_kern_all();
	__cpuc_flush_user_all();

	outer_clean_range( (phys_addr_t)0x0, (phys_addr_t) 0xFFFFFFFF );

	printk(KERN_ALERT"FSL v1.0: Please power me down manually\n");
//	turn_mmu_off_and_mark();

	while(1);
}
