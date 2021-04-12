/*-----------------------------------------------------------------------------
 * drivers/usb/musb_qmu/mtk_cust.c
 *
 * MT53xx USB driver
 *
 * Copyright (c) 2008-2014 MediaTek Inc.
 * $Author: jason.chen $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 *
 *---------------------------------------------------------------------------*/


/** @file mtk_cust.c
 *  This C file implements the mtk53xx USB host controller customization driver.
 */

//---------------------------------------------------------------------------
// Include files
//---------------------------------------------------------------------------
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/usb.h>
#include <linux/kthread.h>
#include <asm/byteorder.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/system.h>
#include <asm/unaligned.h>
#include <asm/uaccess.h>
#include <linux/dma-mapping.h>
#include <linux/usb/hcd.h>
#ifdef CONFIG_MT53XX_NATIVE_GPIO
#include <mach/mtk_gpio.h>
#endif
#include "mtk_hcd.h"
#include "mtk_qmu_api.h"

#include <mach/mt53xx_linux.h>
extern uint8_t bPowerStatus[4];
extern uint usb_HDDREGFlag; 
extern uint usb_HDDWDFlag; 

#ifdef CONFIG_MT53XX_NATIVE_GPIO
extern int MUC_aPwrGpio[MUC_NUM_MAX_CONTROLLER];
extern int MUC_aPwrPolarity[MUC_NUM_MAX_CONTROLLER];
extern int MUC_aOCGpio[MUC_NUM_MAX_CONTROLLER];
extern int MUC_aOCPolarity[MUC_NUM_MAX_CONTROLLER];
#endif

#ifndef DEMO_BOARD
#ifdef CONFIG_USB_OC_SUPPORT
extern uint8_t u1MUSBVbusEanble;   // USB Vbus status, true = Vbus On / false = Vbus off
extern uint8_t u1MUSBOCEnable;      // USB OC function enable status, true= enable oc detect /false = not
extern uint8_t u1MUSBOCStatus;      // USB OC status, true = oc o / false=oc not cours
extern uint8_t u1MUSBOCPort;      // USB OC port, 1,2,3...
#endif
#endif

#ifdef MUSB_DEBUG
extern int mgc_debuglevel;
#endif

int MGC_PhyReset(void * pBase, void * pPhyBase)
{
    uint32_t u4Reg = 0;
    uint8_t nDevCtl = 0;
    uint32_t *pu4MemTmp;

	// USB DMA enable
	#if defined(CONFIG_ARCH_MT5882)
	pu4MemTmp = (uint32_t *)0xf0061B00;
	//defined(CONFIG_ARCH_MT5399) || defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5861)
	#else
	pu4MemTmp = (uint32_t *)0xf0061A00;
	#endif
    *pu4MemTmp = 0x00cfff00;
  
    MU_MB();
	
	#if !defined(CONFIG_ARCH_MT5890)//5890 no need set to 1 
    // VRT insert R enable 
    u4Reg = MGC_PHY_Read32(pPhyBase, 0x00);
    u4Reg |= 0x4000; //RG_USB20_INTR_EN, 5890 is 0x00[5], others is 0x00[14]
    MGC_PHY_Write32(pPhyBase, 0x00, u4Reg);
	#endif

    //Soft Reset, RG_RESET for Soft RESET, reset MAC
    u4Reg = MGC_PHY_Read32(pPhyBase,(uint32_t)0x68);
    u4Reg |=   0x00004000; 
    MGC_PHY_Write32(pPhyBase, (uint32_t)0x68, u4Reg);
	MU_MB();
	
	#if defined(CONFIG_ARCH_MT5890)
	//add for Oryx USB20 PLL fail
	u4Reg = MGC_PHY_Read32(pPhyBase,(uint32_t)0x00); //RG_USB20_USBPLL_FORCE_ON=1b
	u4Reg |=   0x00008000; 
	//u4Reg =   0x0000940a; //for register overwrite by others issue
	MGC_PHY_Write32(pPhyBase, (uint32_t)0x00, u4Reg);

	u4Reg = MGC_PHY_Read32(pPhyBase,(uint32_t)0x14); //RG_USB20_HS_100U_U3_EN=1b
	u4Reg |=   0x00000800; 
	MGC_PHY_Write32(pPhyBase, (uint32_t)0x14, u4Reg);

	if(IS_IC_MT5890_ES1())
	{
	//driving adjust
	u4Reg = MGC_PHY_Read32(pPhyBase,(uint32_t)0x04);
	u4Reg &=  ~0x00007700;
	MGC_PHY_Write32(pPhyBase, (uint32_t)0x04, u4Reg);
	}
	#endif

    u4Reg = MGC_PHY_Read32(pPhyBase,(uint32_t)0x68);
    u4Reg &=  ~0x00004000; 
    MGC_PHY_Write32(pPhyBase, (uint32_t)0x68, u4Reg);
	MU_MB();

    //otg bit setting
    u4Reg = MGC_PHY_Read32(pPhyBase,(uint32_t)0x6C);
    u4Reg &= ~0x3f3f;
    u4Reg |=  0x403f2c;
    MGC_PHY_Write32(pPhyBase, (uint32_t)0x6C, u4Reg);

    //suspendom control
    u4Reg = MGC_PHY_Read32(pPhyBase,(uint32_t)0x68);
    u4Reg &=  ~0x00040000; 
    MGC_PHY_Write32(pPhyBase, (uint32_t)0x68, u4Reg);
    
    //eye diagram improve   
    //TX_Current_EN setting 01 , 25:24 = 01 //enable TX current when
    //EN_HS_TX_I=1 or EN_HS_TERM=1
    u4Reg = MGC_PHY_Read32(pPhyBase,(uint32_t)0x1C);
    u4Reg |=  0x01000000; 
    MGC_PHY_Write32(pPhyBase, (uint32_t)0x1C, u4Reg);

    //disconnect threshold,set 620mV
    u4Reg = MGC_PHY_Read32(pPhyBase,(uint32_t)0x18); //RG_USB20_DISCTH, 5890 is 0x18[3:0], others is 0x18[19:16]
	#if defined(CONFIG_ARCH_MT5890)
	if(IS_IC_MT5890_ES1())
	{
    u4Reg &= ~0x0000000f;
    u4Reg |=  0x0000000f;
	}
	else
	{
		u4Reg &= ~0x000000f0;
		u4Reg |=  0x000000b0; //0x18[7:4]
	}
	#else
    u4Reg &= ~0x000f0000;
    u4Reg |=  0x000b0000;
	#endif
    MGC_PHY_Write32(pPhyBase, (uint32_t)0x18, u4Reg);

    u4Reg = MGC_Read8(pBase,M_REG_PERFORMANCE3);
    u4Reg |=  0x80;
    u4Reg &= ~0x40;
    MGC_Write8(pBase, M_REG_PERFORMANCE3, (uint8_t)u4Reg);

	//HW debounce enable,  write 0 means enable
    u4Reg = MGC_Read32(pBase,M_REG_PERFORMANCE1);
	u4Reg &=  ~0x4000;
	MGC_Write32(pBase, M_REG_PERFORMANCE1, u4Reg);


    #if defined(CONFIG_ARCH_MT5890)
    u4Reg = MGC_Read8(pBase, (uint32_t)0x7C);
    u4Reg |= 0x04;
    MGC_Write8(pBase, 0x7C, (uint8_t)u4Reg);
    #else
    // MT5368/MT5396/MT5389 
    //0xf0050070[10]= 1 for Fs Hub + Ls Device 
    u4Reg = MGC_Read8(pBase, (uint32_t)0x71);
    u4Reg |= 0x04; //RGO_USB20_PRBS7_LOCK
    MGC_Write8(pBase, 0x71, (uint8_t)u4Reg);
	#endif

    // open session.
    nDevCtl = MGC_Read8(pBase, MGC_O_HDRC_DEVCTL);     
    nDevCtl |= MGC_M_DEVCTL_SESSION;        
    MGC_Write8(pBase, MGC_O_HDRC_DEVCTL, nDevCtl);

    // This is important: TM1_EN
    // speed up OTG setting for checking device connect event after MUC_Initial().
    u4Reg = MGC_Read32(pBase, 0x604);
    u4Reg |= 0x01;
    MGC_Write32(pBase, 0x604, u4Reg);
	
    // open session.
    nDevCtl = MGC_Read8(pBase, MGC_O_HDRC_DEVCTL);   
    nDevCtl |= MGC_M_DEVCTL_SESSION;        
    MGC_Write8(pBase, MGC_O_HDRC_DEVCTL, nDevCtl);

    // FS/LS Slew Rate change
    u4Reg = MGC_PHY_Read32(pPhyBase, 0x10);
    u4Reg &= (uint32_t)(~0x000000ff);
    u4Reg |= (uint32_t)0x00000011;
    MGC_PHY_Write32(pPhyBase, 0x10, u4Reg);
	
    // HS Slew Rate, RG_USB20_HSTX_SRCTRL
	#if defined(CONFIG_ARCH_MT5890)
    u4Reg = MGC_PHY_Read32(pPhyBase, 0x14); //bit[14:12]
	if(IS_IC_MT5890_ES1())
	{
    u4Reg &= (uint32_t)(~0x00007000);
    u4Reg |= (uint32_t)0x00001000;
	}
	else
	{
		u4Reg &= (uint32_t)(~0x00007000);
		u4Reg |= (uint32_t)0x00004000;
	}
    MGC_PHY_Write32(pPhyBase, 0x14, u4Reg);
	#else
    u4Reg = MGC_PHY_Read32(pPhyBase, 0x10);
    u4Reg &= (uint32_t)(~0x00070000);
    u4Reg |= (uint32_t)0x00040000;
    MGC_PHY_Write32(pPhyBase, 0x10, u4Reg);
	#endif
	
#if defined(CC_MT5890)
	u4Reg = MGC_PHY_Read32(pPhyBase, 0x14);
	u4Reg &= ~(uint32_t)0x300000;
	u4Reg |= (uint32_t)0x10200000; //bit[21:20]: RG_USB20_DISCD[1:0]=2'b10. bit[28]: RG_USB20_DISC_FIT_EN=1'b1
	MGC_PHY_Write32(pPhyBase, 0x14, u4Reg);
#endif

    // This is important: TM1_EN
    // speed up OTG setting for checking device connect event after MUC_Initial().
    u4Reg = MGC_Read32(pBase, 0x604);
    u4Reg &= ~0x01;
    MGC_Write32(pBase,0x604, u4Reg);
	
    return 0;
}

static void MGC_IntrLevel1En(uint8_t bPortNum)
{
    uint8_t *pBase = NULL;
    uint32_t u4Reg = 0;	
    unsigned long flags=0;
	
    local_irq_save(flags);
	
	switch((uint32_t)bPortNum)
		{
			case 0:
					pBase = (uint8_t*)MUSB_BASE;
					break;
			case 1:
					pBase = (uint8_t*)MUSB_BASE1;
					break;
			case 2:
					pBase = (uint8_t*)MUSB_BASE2;
					break;
			default: 
				#ifdef MUSB_BASE3
					pBase = (uint8_t*)MUSB_BASE3;
				#endif
					break;
		}
	
	if(pBase == NULL)
	{
		local_irq_restore(flags);
		return;
	}
	u4Reg = MGC_Read32(pBase, M_REG_INTRLEVEL1EN);
	#ifdef CONFIG_USB_QMU_SUPPORT
	u4Reg |= 0x2f;
	#else
	u4Reg |= 0x0f;
	#endif
	MGC_Write32(pBase, M_REG_INTRLEVEL1EN, u4Reg);
	
    local_irq_restore(flags);

}

#ifdef CONFIG_MT53XX_NATIVE_GPIO 
void MGC_VBusControl(uint8_t bPortNum, uint8_t bOn)
{
    unsigned long flags=0;

	#ifdef MT7662_POWER_ALWAYS_ENABLE
    if (MUC_aPwrGpio[bPortNum] == 210)
    {
        bOn = 1;
    }
	#endif
	
    local_irq_save(flags);
    if(MUC_aPwrGpio[bPortNum] != -1)
    {
        mtk_gpio_set_value(MUC_aPwrGpio[bPortNum], 
                ((bOn) ? (MUC_aPwrPolarity[bPortNum]) : (1-MUC_aPwrPolarity[bPortNum])));
    }
	if(bOn)
		bPowerStatus[bPortNum]=1;
	else 
		bPowerStatus[bPortNum]=0;
	
	#ifndef DEMO_BOARD
	#ifdef CONFIG_USB_OC_SUPPORT
	if(bOn)
	  {
		u1MUSBVbusEanble = TRUE;
	  }
	  else
	  {
		u1MUSBVbusEanble = FALSE;
	  }
	#endif
	#endif

    local_irq_restore(flags); 
	
	if(bOn)
		MGC_IntrLevel1En(bPortNum);
}
#else //#ifdef CONFIG_MT53XX_NATIVE_GPIO 
#if 0
static void MGC_VBusControl(uint8_t bPortNum, uint8_t bOn)
{
#ifdef USB_PWR_SWITCH_GPIO_CUST
    printk("\n");
    INFO("[USB0]: 5V On, Set GPIO none.\n");
    INFO("[USB1]: 5V On, Set GPIO none.\n");
    INFO("[USB2]: 5V On, Set GPIO none.\n");
    INFO("[USB3]: 5V On, Set GPIO none.\n");
#else //#ifdef USB_PWR_SWITCH_GPIO_CUST
    unsigned long flags=0;

    INFO("[USB%d] Vbus %s.\n", bPortNum, ((bOn) ? "On": "Off"));

    local_irq_save(flags);

    if (bOn)
    {
        switch (bPortNum)
        {
            case 0:
                // GPIO 51
                *((volatile uint32_t *)0xF000D724) |= 0x00080000;
                *((volatile uint32_t *)0xF000D704) |= 0x00080000;
                break;
            case 1:
                // GPIO 43            
                *((volatile uint32_t *)0xF000D724) |= 0x00000800;
                *((volatile uint32_t *)0xF000D704) |= 0x00000800;                            
                break;
            case 2:
                // GPIO 44            
                *((volatile uint32_t *)0xF000D724) |= 0x00001000;
                *((volatile uint32_t *)0xF000D704) |= 0x00001000;
                break;
            case 3:
                // OPTCTRL 10  
                *((volatile uint32_t *)0xF002807C) |= 0x00000400;
                *((volatile uint32_t *)0xF0028074) |= 0x00000400;
                break;

            default:
                break;
        }
    }
    else
    {
        switch (bPortNum)
        {
            case 0:
                // GPIO 51
                *((volatile uint32_t *)0xF000D724) |= 0x00080000;
                *((volatile uint32_t *)0xF000D704) &= ~0x00080000;
                break;
            case 1:
                // GPIO 43            
                *((volatile uint32_t *)0xF000D724) |= 0x00000800;
                *((volatile uint32_t *)0xF000D704) &= ~0x00000800;                            
                break;
            case 2:
                // GPIO 44            
                *((volatile uint32_t *)0xF000D724) |= 0x00001000;
                *((volatile uint32_t *)0xF000D704) &= ~0x00001000;
                break;
            case 3:
                // OPTCTRL 10  
                *((volatile uint32_t *)0xF002807C) |= 0x00000400;
                *((volatile uint32_t *)0xF0028074) &= ~0x00000400;
                break;

            default:
                break;
        }
    }
	if(bOn)
		bPowerStatus[bPortNum]=1;
	else 
		bPowerStatus[bPortNum]=0;
	
	#ifndef DEMO_BOARD
	#ifdef CONFIG_USB_OC_SUPPORT
	if(bOn)
	  {
		u1MUSBVbusEanble = TRUE;
	  }
	  else
	  {
		u1MUSBVbusEanble = FALSE;
	  }
	#endif
	#endif
            
    local_irq_restore(flags);            
          
    #endif //#ifdef USB_PWR_SWITCH_GPIO_CUST

	if(bOn)
		MGC_IntrLevel1En(bPortNum);
}
#else
	#error CONFIG_MT53XX_NATIVE_GPIO not defined !
#endif
#endif //#ifdef CONFIG_MT53XX_NATIVE_GPIO 



#if defined(CONFIG_ARCH_MT5399) || defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5861) || defined(CONFIG_ARCH_MT5882)
MUSB_LinuxController MUC_aLinuxController[] = 
{
    {
         /* Port 0 information. */
        .pBase = (void*)MUSB_BASE, 
        .pPhyBase = (void*)((MUSB_BASE + MUSB_PHYBASE)+ (MUSB_PORT0_PHYBASE)), 
        .dwIrq = VECTOR_USB0, 
        .bSupportCmdQ = TRUE, 
        .bEndpoint_Tx_num = 9,	    /* Endpoint Tx Number : 8+1(end0) */
        .bEndpoint_Rx_num = 9,      /* Endpoint Rx Number : 8+1(end0) */
        .bHub_support = TRUE,
        .MGC_pfVbusControl = MGC_VBusControl, 
        .MGC_pfPhyReset = MGC_PhyReset,          
    },
    {
         /* Port 1 information. */
        .pBase = (void*)MUSB_BASE1, 
        .pPhyBase = (void*)((MUSB_BASE + MUSB_PHYBASE)+ (MUSB_PORT1_PHYBASE)), 
        .dwIrq = VECTOR_USB1, 
        .bSupportCmdQ = TRUE, 
		.bEndpoint_Tx_num = 9,    /* Endpoint Tx Number : 8+1 */
	 	.bEndpoint_Rx_num = 9,    /* Endpoint Rx Number : 8+1 */
        .bHub_support = TRUE,
        .MGC_pfVbusControl = MGC_VBusControl,
        .MGC_pfPhyReset = MGC_PhyReset,        
    }, 
        {
         /* Port 2 information. */
        .pBase = (void*)MUSB_BASE2, 
        .pPhyBase = (void*)((MUSB_BASE + MUSB_PHYBASE)+ (MUSB_PORT2_PHYBASE)), 
        .dwIrq = VECTOR_USB2, 
        .bSupportCmdQ = TRUE, 
	 	.bEndpoint_Tx_num = 9,    /* Endpoint Tx Number : 8+1 */
	 	.bEndpoint_Rx_num = 9,    /* Endpoint Rx Number : 8+1 */
        .bHub_support = TRUE,
        .MGC_pfVbusControl = MGC_VBusControl,
        .MGC_pfPhyReset = MGC_PhyReset,        
    },
    {
         /* Port 3 information. */
        .pBase = (void*)MUSB_BASE3, 
        .pPhyBase = (void*)((MUSB_BASE + MUSB_PHYBASE)+ (MUSB_PORT3_PHYBASE)), 
        .dwIrq = VECTOR_USB3, 
        .bSupportCmdQ = FALSE, 
	 	.bEndpoint_Tx_num = 9,    /* Endpoint Tx Number : 8+1 */
	 	.bEndpoint_Rx_num = 9,    /* Endpoint Rx Number : 8+1 */
        .bHub_support = TRUE,
        .MGC_pfVbusControl = MGC_VBusControl,
        .MGC_pfPhyReset = MGC_PhyReset,        
    }
};
#else
#error CONFIG_ARCH_MTXXXX not defined !
#endif 


static int MGC_VBUS_Monitor_ThreadMain(void * pvArg)
{

	uint8_t bPortNum;
        uint counter_usb_reset_retry_limit = 0;
        uint fg_usb_reset_retry_limit = 0;

	// Need to Re-TurnOn VBUS under three condition
	// CaseB && indicate Portx VBUS ON
	do{
		for(bPortNum=0; bPortNum< MUC_NUM_MAX_CONTROLLER; bPortNum++)
		{
		if((usb_HDDREGFlag & (1<<bPortNum)) && (usb_HDDREGFlag & (1<<(bPortNum+8))))
				{
				if((MUC_aPwrGpio[bPortNum] != -1)  )
					MGC_VBusControl(bPortNum, 1);
				DBG(2,"[USB] Re-TurnOn VBUS Port[%d] GPIO[%d] usb_HDDREGFlag[0x%x]\n",bPortNum,MUC_aPwrGpio[bPortNum],usb_HDDREGFlag);

                		if((usb_HDDWDFlag == 1) && (fg_usb_reset_retry_limit==0)){
					if((MUC_aPwrGpio[bPortNum] != -1)  )
					{
						MGC_VBusControl(2, 0);
                                		msleep(1000);
						MGC_VBusControl(2, 1);
						usb_HDDWDFlag = 0;
                                                fg_usb_reset_retry_limit = 1;
						printk("set usb_HDDWDFlag = 0\n");
						printk("[USB] Reset VBUS for maxpacket = 9 issue DM15GN1-38623\n");
					}
				}
                                
			}
		}
                if(fg_usb_reset_retry_limit == 1){
               		if(counter_usb_reset_retry_limit > 31){ 
				counter_usb_reset_retry_limit = 0;
                                fg_usb_reset_retry_limit = 0;
                                printk("[USB] Reset Limit cnt = %d, fg = %d\n", counter_usb_reset_retry_limit, fg_usb_reset_retry_limit);
			} else {
                		counter_usb_reset_retry_limit++;
                                printk("[USB] Reset Linit cnt = %d, fg = %d\n", counter_usb_reset_retry_limit, fg_usb_reset_retry_limit);
			}
		}
		msleep(1000);
	}while(1);
	
	return 0;
}


void MUT_VBUS_Monitor()
{

	struct task_struct *tThreadStruct;
	char ThreadName[10]="VBUS_CTL";

	printk("VBUS_Monitor thread.\n");

	tThreadStruct = kthread_create(MGC_VBUS_Monitor_ThreadMain, NULL, ThreadName);

	if ( (unsigned long)tThreadStruct == -ENOMEM )
	{
		printk("[USB]MGC_VBUS_Monitor_ThreadMain Failed\n");
		return;
	}

	wake_up_process(tThreadStruct);
	return;

}

