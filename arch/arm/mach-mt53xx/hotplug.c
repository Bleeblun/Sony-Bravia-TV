/*
 *  linux/arch/arm/mach-realview/hotplug.c
 *
 *  Copyright (C) 2002 ARM Ltd.
 *  All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/smp.h>
#include <linux/completion.h>

#include <mach/hardware.h>
#include <asm/delay.h>
#include <asm/smp_scu.h>
#include <asm/cp15.h>
#include <asm/cacheflush.h>
#include <asm/io.h>
#include <asm/smp_plat.h>

#include <mach/mt53xx_linux.h>
#if 1
#define debug_printk(log...)	do {if (!disable_printk_in_caller) {	\
	printk(log); }							\
} while(0)
#else
#define debug_printk(log,...)	do {} while(0)
#endif
void BIM_CpuCA17Down(unsigned int cpu);
void BIM_CpuCA17Up(unsigned int cpu);
void BIM_CpuCA7Down(unsigned int cpu);
void BIM_CpuCA7Up(unsigned int cpu);
void BIM_ClusterDown(unsigned int u4Cluster);
void BIM_ClusterUp(unsigned int u4Cluster);
void BIM_ClusterEnable(unsigned int u4Cluster);
void BIM_ClusterDisable(unsigned int u4Cluster);

extern volatile int pen_release;
#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5882)||defined(CONFIG_ARCH_MT5883) || defined(CONFIG_ARCH_MT5891)
#define CC_NO_EXT_SCU_SETTING
#endif
#if !defined(CC_NO_EXT_SCU_SETTING)
static inline void __iomem *scu_base_addr(void)
{
	return (void __iomem *)MPCORE_SCU_VIRT;
}
#endif

//static DECLARE_COMPLETION(cpu_killed);

#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5882)||defined(CONFIG_ARCH_MT5883) || defined(CONFIG_ARCH_MT5891)
static inline u32 __bim_readl(u32 regaddr32)
{
    return __raw_readl(BIM_VIRT + regaddr32);
}

static inline void __bim_writel(u32 regval32, u32 regaddr32)
{
    __raw_writel(regval32, BIM_VIRT + regaddr32);
}
#endif //CONFIG_ARCH_MT5890

extern void __inner_flush_dcache_all(void);
extern void __inner_flush_dcache_L1(void);
extern void __inner_flush_dcache_L2(void);
extern void __disable_dcache__inner_flush_dcache_L1__inner_clean_dcache_L2(void);   //definition in mt_cache_v7.S
extern void __disable_dcache__inner_flush_dcache_L1__inner_flush_dcache_L2(void);   //definition in mt_cache_v7.S

void inner_dcache_flush_all(void)
{
    __inner_flush_dcache_all();
}

void inner_dcache_flush_L1(void)
{
    __inner_flush_dcache_L1();
}

void inner_dcache_flush_L2(void)
{
    __inner_flush_dcache_L2();
}

extern char TZ_GCPU_FlushInvalidateDCache(void);

static inline void setlog(unsigned int cpu, unsigned int value)
{
    unsigned int address = 0;

    if (cpu == 0) {
        address = 0xf00080e0;
    } else if (cpu == 1) {
        address = 0xf00080e4;
    } else {
        address = 0xf00080e8;
    }

    *(volatile unsigned int *)address = value;
}


static inline void cpu_enter_lowpower(unsigned int cpu)
{
    unsigned int v;
    //HOTPLUG_INFO("cpu_enter_lowpower\n");

    setlog(cpu, 0);

    //printk("cpu enter lowpower\n");
    TZ_GCPU_FlushInvalidateDCache();
    //printk("cache flush done\n");

    setlog(cpu, 1);

    /* Cluster off Only MT5890 ES or above uses this, Gazelle and Capri need to jump to another condition */
    if ((mt53xx_get_ic_version() >= IC_VER_MT5890_AC) && ((cpu==3 && cpu_online(2)==0) || (cpu==2 && cpu_online(3)==0)))
    {
        __disable_dcache__inner_flush_dcache_L1__inner_flush_dcache_L2();
        #if 0
        /* Clear the SCTLR C bit to prevent further data cache allocation */
    	asm volatile(
    		"mcr	p15, 0, %1, c7, c5, 0\n"    // invalidate entire instruction cache
    	"	mcr	p15, 0, %1, c7, c10, 4\n"   // dsb
    	"	dmb\n"   // dmb
    	"	mrc	p15, 0, %0, c1, c0, 0\n"
    	"	bic	%0, %0, %2\n"		    // disable D cache
    	"	mcr	p15, 0, %0, c1, c0, 0\n"
    	  : "=&r" (v)
    	  : "r" (0), "Ir" (CR_C), "Ir" (0x40)
    	  : "cc");
        isb();
        dsb();

        /* Clean and invalidate all data from the L1, L2 data cache */
        inner_dcache_flush_all();
        #endif

        /* Switch the processor from SMP mode to AMP mode by clearing the ACTLR SMP bit */
    	asm volatile(
    	"	mrc	p15, 0, %0, c1, c0, 1\n"
    	"	bic	%0, %0, %3\n"		    // ACTLR.SMP | ACTLR.FW
    	"	mcr	p15, 0, %0, c1, c0, 1\n"
    	  : "=&r" (v)
    	  : "r" (0), "Ir" (CR_C), "Ir" (0x40)
    	  : "cc");

        isb();
        dsb();

        BIM_ClusterDisable(1);
    }
    else
    {
        __disable_dcache__inner_flush_dcache_L1__inner_clean_dcache_L2();

        setlog(cpu, 2);

        #if 0
    	asm volatile(
    		"mcr	p15, 0, %1, c7, c5, 0\n"    // invalidate entire instruction cache
    	"	mcr	p15, 0, %1, c7, c10, 4\n"   // dsb
    	"	dmb\n"   // dmb
    	"	mrc	p15, 0, %0, c1, c0, 0\n"
    	"	bic	%0, %0, %2\n"		    // disable D cache
    	"	mcr	p15, 0, %0, c1, c0, 0\n"
    	  : "=&r" (v)
    	  : "r" (0), "Ir" (CR_C), "Ir" (0x40)
    	  : "cc");
        /* Clear the SCTLR C bit to prevent further data cache allocation */
        //__disable_dcache();
        isb();
        dsb();
        /* Clean and invalidate all data from the L1 data cache */
        inner_dcache_flush_L1();
        #endif

    	udelay(2000);
    	__asm__ __volatile__("clrex"); // Execute a CLREX instruction

        setlog(cpu, 3);

    	asm volatile(
    	"	mrc	p15, 0, %0, c1, c0, 1\n"
    	"	bic	%0, %0, %3\n"		    // ACTLR.SMP
    	"	mcr	p15, 0, %0, c1, c0, 1\n"
    	  : "=&r" (v)
    	  : "r" (0), "Ir" (CR_C), "Ir" (0x40)
    	  : "cc");
    	isb();

        setlog(cpu, 4);
    }

}


static inline void cpu_leave_lowpower(unsigned int cpu)
{
	unsigned int v;

#if !defined(CC_NO_EXT_SCU_SETTING)
	isb();
	dmb();
	dsb();
	isb();
	scu_power_mode(scu_base_addr(), SCU_PM_NORMAL);
#endif
	isb();
	dmb();
	dsb();
	isb();
	asm volatile(
	"	mrc	p15, 0, %0, c1, c0, 1\n"
	"	orr	%0, %0, %2\n"		    // ACTLR.SMP
	"	mcr	p15, 0, %0, c1, c0, 1\n"
	"	mrc	p15, 0, %0, c1, c0, 0\n"
	"	orr	%0, %0, %1\n"		    // enable D cache
	"	mcr	p15, 0, %0, c1, c0, 0\n"
	  : "=&r" (v)
	  : "Ir" (CR_C), "Ir" (0x40)
	  : "cc");
}

static inline void platform_do_lowpower(unsigned int cpu)
{
#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5882)||defined(CONFIG_ARCH_MT5883) || defined(CONFIG_ARCH_MT5891)
	//printk("(yjdbg) platform_do_lowpower: %d, ", cpu);
	cpu = cpu_logical_map(cpu);
	//printk("%d\n", cpu);
#endif
	/*
	 * there is no power-control hardware on this platform, so all
	 * we can do is put the core into WFI; this is safe as the calling
	 * code will have already disabled interrupts
	 */

    setlog(cpu, 5);

	for (;;) {
		/*
		 * here's the WFI
		 */
		asm(".word	0xe320f003\n"
		    :
		    :
		    : "memory", "cc");

		if (pen_release == cpu) {
			/*
			 * OK, proper wakeup, we're done
			 */
			break;
		}

        setlog(cpu, 6);
		/*
		 * getting here, means that we have come out of WFI without
		 * having been woken up - this shouldn't happen
		 *
		 * The trouble is, letting people know about this is not really
		 * possible, since we are currently running incoherently, and
		 * therefore cannot safely call printk() or anything else
		 */
#ifdef DEBUG
		printk("CPU%u: spurious wakeup call\n", cpu);
#endif
	}
}

int volatile _yjdbg_hotplug = 0;
int volatile _yjdbg_delay = 1;
int disable_printk_in_caller=1;
int platform_cpu_kill(unsigned int cpu)
{
#if defined(CONFIG_ARCH_MT5890) ||defined(CONFIG_ARCH_MT5882) ||defined(CONFIG_ARCH_MT5883) || defined(CONFIG_ARCH_MT5891)
    debug_printk("(yjdbg) platform_cpu_kill: %d, ", cpu);
    cpu = cpu_logical_map(cpu);
    debug_printk("%d\n", cpu);

    setlog(2, 0);

    if(cpu == 0)
    {
        printk(KERN_NOTICE "we should not turn off CPU%u\n", cpu);
        return 0;
    }

    if(!(((cpu >= 0) && (cpu <4)) || ((cpu >= 256) && (cpu <258))) )
    {
        printk(KERN_NOTICE "we can't handle CPU%u\n", cpu);
        return 0;
    }
    /*
    // check if CPU is in WFI/WFE
    *(volatile unsigned int *)0xf0078164 = 0x0;
    *(volatile unsigned int *)0xf007811c = 0x1b;
    *(volatile unsigned int *)0xf0078014 = 0x7;
    regval32 = *(volatile unsigned int *)0xf0078018;
    while(!(regval32 & (1 << cpu) || regval32 & (1 << (cpu + 4))))
    {
		printk(KERN_ERR "CPU%u is not in WFI/WFE mode 0x%x\n", cpu, regval32);
    }
    */
    debug_printk(KERN_NOTICE "platform_cpu_kill: CPU%u enter WFI mode\n", cpu);

    if(cpu < 4)
    {
        if (_yjdbg_hotplug & 0x01)
        {
        }
        else
        {
        BIM_CpuCA17Down(cpu);
        }
    }
    else
    {
        BIM_CpuCA7Down((cpu & 0xff));
    }

    if (_yjdbg_hotplug & 0x04)
    {
        int i = 0;
        for (i=0; i<_yjdbg_delay; i++)
        {
            udelay(2000);
        }
    }

    setlog(2, 1);

    if ((_yjdbg_hotplug & 0x02))
        debug_printk(KERN_NOTICE "CPU%u is killed.\n", cpu);
#endif // CONFIG_ARCH_MT5890

    return 1;
}

/*
 * platform-specific code to shutdown a CPU
 *
 * Called with IRQs disabled
 */
void __cpuinitdata platform_cpu_die(unsigned int cpu) // access .cpuinit.data:pen_release; TODO
{
    unsigned int phyCpu;
#ifdef DEBUG
	unsigned int this_cpu = hard_smp_processor_id();
#endif

#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5882)||defined(CONFIG_ARCH_MT5883) || defined(CONFIG_ARCH_MT5891)
	debug_printk("(yjdbg) platform_cpu_die: %d, ", cpu);
	phyCpu = cpu;
	cpu = cpu_logical_map(cpu);
	debug_printk("%d\n", cpu);
#endif

#ifdef DEBUG
	if (cpu != this_cpu) {
		printk(KERN_CRIT "Eek! platform_cpu_die running on %u, should be %u\n",
			   this_cpu, cpu);
		BUG();
	}
#endif

	//printk(KERN_NOTICE "CPU%u: shutdown-2\n", cpu);
	//complete(&cpu_killed);

	/*
	 * we're ready for shutdown now, so do it
	 */
    cpu_enter_lowpower(phyCpu);
	platform_do_lowpower(cpu);

	/*
	 * bring this CPU back into the world of cache
	 * coherency, and then restore interrupts
	 */
	 cpu_leave_lowpower(phyCpu);
}

int platform_cpu_disable(unsigned int cpu)
{
	/*
	 * we don't allow CPU 0 to be shutdown (it is still too special
	 * e.g. clock tick interrupts)
	 */
	return cpu == 0 ? -EPERM : 0;
}

extern void __cpuinit platform_smp_prepare_cpus_wakeup(void);
extern void __cpuinit platform_smp_prepare_cpus_boot(void);

void mt53xx_Core1StubSimple(void)
{
#if defined(CONFIG_ARCH_MT5890)||defined(CONFIG_ARCH_MT5882)
    unsigned int addr = 0xf0008068, magic = 0xffffb007;

    __asm__ ("isb");

    __asm__ ("LOOP:");
    __asm__ ("wfe");
    __asm__ ("MOV     r0, %0" : : "r" (addr));
    __asm__ ("MOV     r1, %0" : : "r" (magic));
    __asm__ ("ldr     r2, [r0]");
    __asm__ ("cmp     r2, r1");
    __asm__ ("bne     LOOP");

    addr = 0xf0008188;

    __asm__ ("MOV     r0, %0" : : "r" (addr));
    __asm__ ("ldr     r1, [r0]");
    __asm__ ("blx     r1");
#endif // defined(CONFIG_ARCH_MT5890)
}

void __ref mt53xx_cpu_boot(unsigned int cpu)
{
   #if defined(CONFIG_ARCH_MT5890) ||defined(CONFIG_ARCH_MT5882) ||defined(CONFIG_ARCH_MT5883) || defined(CONFIG_ARCH_MT5891)
    printk("(yjdbg) mt53xx_cpu_boot: %d\n", cpu);

#if 0 // def CONFIG_ARCH_MT5890
    cpu = cpu_logical_map(cpu);
#endif

    platform_smp_prepare_cpus_boot();

	#if 0
    // Setup Core1Stub in physical address 0x0 only if the first instruction is not ISB(0xF57FF06F); mt53xx_Core1Stub usually is prepared at ko initialization
    if((*(unsigned int *)0xF2020000) != 0xF57FF06F)
    {
        #ifdef CC_TRUSTZONE_SUPPORT
        // printk("mt53xx_cpu_boot is not handling trustzone correctly!!\n");
        #endif //CC_TRUSTZONE_SUPPORT
        memcpy((void *)0xF2020000, mt53xx_Core1StubSimple, 0x100);
    }
	#endif

    if(!(((cpu >= 0) && (cpu <4)) || ((cpu >= 256) && (cpu <258))) )
    {
        printk(KERN_NOTICE "we can't handle CPU%u\n", cpu);
        return;
    }

    if(cpu < 4)
    {
        if (_yjdbg_hotplug & 0x01)
        {
        }
        else
        {
        BIM_CpuCA17Up(cpu);
        }
    }
    else
    {
        if(cpu == 256 || cpu == 257)
        {
            BIM_ClusterEnable(1);
        }
        BIM_CpuCA7Up((cpu & 0xff));
    }

    printk(KERN_NOTICE "we turn on CPU%u\n", cpu);

    platform_smp_prepare_cpus_wakeup();
    #endif // CONFIG_ARCH_MT5890
}


