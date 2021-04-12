/*
 * linux/arch/arm/mach-mt53xx/include/mach/irqs.h
 *
 * Copyright (c) 2010-2012 MediaTek Inc.
 * $Author: dtvbm11 $
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
 */

#if !defined(IRQS_H)
#define IRQS_H

#ifndef CC_NO_KRL_UART_DRV
#define CC_NO_KRL_UART_DRV
#endif

#ifdef CONFIG_ARM_GIC
#define IRQ_LOCALTIMER                  29
#define IRQ_LOCALWDOG                   30

#define SPI_OFFSET	32
#if defined(CONFIG_ARCH_MT5880)
#define NR_IRQS         160
#elif defined(CONFIG_ARCH_MT5890)
#define NR_IRQS         288
#define CC_VECTOR_HAS_MISC3
#else
#define NR_IRQS         256
#endif
#else /* not CONFIG_ARM_GIC part*/
#define SPI_OFFSET	0
#define NR_IRQS         128
#endif //CONFIG_ARM_GIC

#ifndef __USE_XBIM_IRQS         // UGLY HACK! make sure people using x_bim.h get desired define.
#define IRQ_TIMER0              (3+SPI_OFFSET)
#define ARCH_TIMER_IRQ          (5+SPI_OFFSET)    // system tick timer irq

// Interrupt vectors
#ifndef VECTOR_PDWNC
#define VECTOR_PDWNC    (0+SPI_OFFSET)           // Power Down module Interrupt
#endif
#ifndef VECTOR_SERIAL
#define VECTOR_SERIAL   (1+SPI_OFFSET)           // Serial Interface Interrupt
#endif
#ifndef VECTOR_NAND
#define VECTOR_NAND     (2+SPI_OFFSET)           // NAND-Flash interface interrupt
#endif
#ifndef VECTOR_T0
#define VECTOR_T0       (3+SPI_OFFSET)           // Timer 0
#endif
#ifndef VECTOR_T1
#define VECTOR_T1       (4+SPI_OFFSET)           // Timer 1
#endif
#ifndef VECTOR_T2
#define VECTOR_T2       (5+SPI_OFFSET)           // Timer 2
#endif
#ifndef VECTOR_SMARTCARD
#define VECTOR_SMARTCARD (6+SPI_OFFSET)         // Smart Card Interrupt
#endif
#ifndef VECTOR_WDT
#define VECTOR_WDT      (7+SPI_OFFSET)           // watchdog
#endif
#ifndef VECTOR_ENET
#define VECTOR_ENET     (8+SPI_OFFSET)           // Ethernet interrupt
#endif
#ifndef VECTOR_DTCP
#define VECTOR_DTCP     (9+SPI_OFFSET)           // DCTP interrupt enable bit
#endif
#ifndef VECTOR_MJC
#define VECTOR_MJC      (9+SPI_OFFSET)           // MJC interrupt
#endif
#ifndef VECTOR_PSCAN
#define VECTOR_PSCAN    (10+SPI_OFFSET)          // PSCAN interrupt
#endif
#ifndef VECTOR_USB1
#define VECTOR_USB1     (11+SPI_OFFSET)          // USB1 Interrupt
#endif
#ifndef VECTOR_IMGRZ
#define VECTOR_IMGRZ    (12+SPI_OFFSET)          // IMGRZ interrupt
#endif
#ifndef VECTOR_GFXSC
#define VECTOR_GFXSC    (12+SPI_OFFSET)          // IMGRZ interrupt
#endif
#ifndef VECTOR_VENC
#define VECTOR_VENC     (13+SPI_OFFSET)          // VENC interrupt
#endif
#ifndef VECTOR_SPDIF
#define VECTOR_SPDIF    (14+SPI_OFFSET)          // Audio PSR/SPDIF interrupt
#endif
#ifndef VECTOR_PSR
#define VECTOR_PSR      (14+SPI_OFFSET)          // Audio PSR/SPDIF interrupt
#endif
#ifndef VECTOR_USB
#define VECTOR_USB      (15+SPI_OFFSET)          // USB0 Interrupt
#endif
#ifndef VECTOR_USB0
#define VECTOR_USB0     (15+SPI_OFFSET)          // USB0 Interrupt
#endif
#ifndef VECTOR_AUDIO
#define VECTOR_AUDIO    (16+SPI_OFFSET)          // Audio DSP interrupt
#endif
#ifndef VECTOR_DSP
#define VECTOR_DSP      (16+SPI_OFFSET)          // Audio DSP Interrupt
#endif
#ifndef VECTOR_RS232
#define VECTOR_RS232    (17+SPI_OFFSET)          // RS232 Interrupt 1
#endif
#ifndef VECTOR_LED
#define VECTOR_LED      (18+SPI_OFFSET)          // LED interrupt
#endif
#ifndef VECTOR_OSD
#define VECTOR_OSD      (19+SPI_OFFSET)          // OSD interrupt
#endif
#ifndef VECTOR_VDOIN
#define VECTOR_VDOIN    (20+SPI_OFFSET)          // Video In interrupt (8202 side)
#endif
#ifndef VECTOR_BLK2RS
#define VECTOR_BLK2RS   (21+SPI_OFFSET)          // Block to Raster Interrupt
#endif
#ifndef VECTOR_DISPLAY
#define VECTOR_DISPLAY  (21+SPI_OFFSET)          // Block to Raster Interrupt
#endif
#ifndef VECTOR_FLASH
#define VECTOR_FLASH    (22+SPI_OFFSET)          // Serial Flash interrupt
#endif
#ifndef VECTOR_POST_PROC
#define VECTOR_POST_PROC (23+SPI_OFFSET)         // POST process interrupt
#endif
#ifndef VECTOR_VDEC
#define VECTOR_VDEC     (24+SPI_OFFSET)          // VDEC interrupt
#endif
#ifndef VECTOR_GFX
#define VECTOR_GFX      (25+SPI_OFFSET)          // Graphic Interrupt
#endif
#ifndef VECTOR_DEMUX
#define VECTOR_DEMUX    (26+SPI_OFFSET)          // Transport demuxer interrupt
#endif
#ifndef VECTOR_DEMOD
#define VECTOR_DEMOD    (27+SPI_OFFSET)          // Demod interrupt enable bit
#endif
#ifndef VECTOR_FCI
#define VECTOR_FCI      (28+SPI_OFFSET)          // FCI interrupt
#endif
#ifndef VECTOR_MSDC
#define VECTOR_MSDC     (28+SPI_OFFSET)          // MSDC interrupt
#endif
#ifndef VECTOR_APWM
#define VECTOR_APWM     (29+SPI_OFFSET)          // Audio PWM Interrupt
#endif
#ifndef VECTOR_PCMCIA
#define VECTOR_PCMCIA   (29+SPI_OFFSET)          // PCMCIA interrupt
#endif
#ifndef VECTOR_MISC2
#define VECTOR_MISC2    (30+SPI_OFFSET)          // MISC2 interrupt
#endif
#ifndef VECTOR_MISC
#define VECTOR_MISC     (31+SPI_OFFSET)          // MISC interrupt
#endif

#ifndef VECTOR_MISC_BASE
#define VECTOR_MISC_BASE  (32+SPI_OFFSET)
#endif
#ifndef VECTOR_MISC2_BASE
#define VECTOR_MISC2_BASE (64+SPI_OFFSET)
#endif
#ifndef VECTOR_MISC3_BASE
#define VECTOR_MISC3_BASE (96+SPI_OFFSET)
#endif

//misc
#ifndef VECTOR_DRAMC
#define VECTOR_DRAMC        (0+VECTOR_MISC_BASE)          // DRAM Controller interrupt
#endif
#ifndef VECTOR_EXT1
#define VECTOR_EXT1         (1+VECTOR_MISC_BASE)          // Dedicated GPIO edge interrupt
#endif
#ifndef VECTOR_POD
#define VECTOR_POD          (2+VECTOR_MISC_BASE)
#endif
#if defined(CONFIG_ARCH_MT5396) || defined(CONFIG_ARCH_MT5389)
#ifndef VECTOR_PMU1
#define VECTOR_PMU1         (3+VECTOR_MISC_BASE)
#endif
#ifndef VECTOR_PMU0
#define VECTOR_PMU0         (4+VECTOR_MISC_BASE)
#endif
#elif defined(CONFIG_ARCH_MT5399) || defined(CONFIG_ARCH_MT5398) || defined(CONFIG_ARCH_MT5880) || defined(CONFIG_ARCH_MT5881) || defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5861) || defined(CONFIG_ARCH_MT5882)
#ifndef VECTOR_PMU1
#define VECTOR_PMU1         (4+VECTOR_MISC_BASE)
#endif
#ifndef VECTOR_PMU0
#define VECTOR_PMU0         (3+VECTOR_MISC_BASE)
#endif
#endif
#ifndef VECTOR_PMU
#define VECTOR_PMU          VECTOR_PMU0                   // ARM performance monitor interrupt
#endif
#ifndef VECTOR_DRAMC_CHB
#define VECTOR_DRAMC_CHB    (5+VECTOR_MISC_BASE)          // 5. DRAM Controller 1 interrupt
#endif
#ifndef VECTOR_TVE
#define VECTOR_TVE          (6+VECTOR_MISC_BASE)          // 6. TVE interrupt
#endif
#if defined(CONFIG_ARCH_MT5399) || defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5861) || defined(CONFIG_ARCH_MT5882)
#ifndef VECTOR_SCALER
#define VECTOR_SCALER       (7+VECTOR_MISC_BASE)
#endif
#else
#ifndef VECTOR_PCIE
#define VECTOR_PCIE         (7+VECTOR_MISC_BASE)
#endif
#endif
#ifndef VECTOR_APWM2
#define VECTOR_APWM2        (8+VECTOR_MISC_BASE)          // 8. APWM2 interrupt
#endif
#ifndef VECTOR_SPI
#define VECTOR_SPI          (9+VECTOR_MISC_BASE)          // 9. SPI interrupt
#endif
#ifndef VECTOR_JPGDEC
#define VECTOR_JPGDEC       (10+VECTOR_MISC_BASE)          // 10. Jpeg Decoder interrupt
#endif
#ifndef VECTOR_P
#define VECTOR_P            (11+VECTOR_MISC_BASE)          // 11. P interrupt   //MT5395 Parser DMA is 43
#endif
#ifndef VECTOR_AES
#define VECTOR_AES          (12+VECTOR_MISC_BASE)          // 12. AES interrupt
#endif
#ifndef VECTOR_UP0
#define VECTOR_UP0          (12+VECTOR_MISC_BASE)          // 12. AES 8032 interrupt 0
#endif
#if defined(CONFIG_ARCH_MT5399) || defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5861) || defined(CONFIG_ARCH_MT5882)
#ifndef VECTOR_TCON
#define VECTOR_TCON         (14+VECTOR_MISC_BASE)          // 14. TCON
#endif
#ifndef VECTOR_TCON_TCH
#define VECTOR_TCON_TCH     (14+VECTOR_MISC_BASE)          // 14. TCON timing control
#endif
#ifndef VECTOR_TCON_EH
#define VECTOR_TCON_EH      (13+VECTOR_MISC_BASE)          // 13. TCON error
#endif
#else
#define VECTOR_TCON         (13+VECTOR_MISC_BASE)          // 13. TCON
#define VECTOR_TCON_TCH     (13+VECTOR_MISC_BASE)          // 13. TCON timing control
#define VECTOR_TCON_EH      (14+VECTOR_MISC_BASE)          // 14. TCON error
#endif
#ifndef VECTOR_OD
#define VECTOR_OD           (15+VECTOR_MISC_BASE)          // 15. OD
#endif
#ifndef VECTOR_USB2
#define VECTOR_USB2         (16+VECTOR_MISC_BASE)          // 16. USB2 Interrupt
#endif
#ifndef VECTOR_USB3
#define VECTOR_USB3         (17+VECTOR_MISC_BASE)          // 17. USB3 Interrupt
#endif
#ifndef VECTOR_GDMA
#define VECTOR_GDMA         (18+VECTOR_MISC_BASE)          // 18. GDMA
#endif
#ifndef VECTOR_EPHY
#define VECTOR_EPHY         (19+VECTOR_MISC_BASE)          // 19. ethernet PHY
#endif
#ifndef VECTOR_TCPIP
#define VECTOR_TCPIP        (20+VECTOR_MISC_BASE)          // 20. TCPIP checksum
#endif
#ifndef VECTOR_GFX3D_GP
#define VECTOR_GFX3D_GP     (21+VECTOR_MISC_BASE)
#endif
#if defined(CONFIG_ARCH_MT5890)
#define VECTOR_SSUSB_DEVICE (24+VECTOR_MISC_BASE)
#endif
#if !defined(CONFIG_ARCH_MT5399) && !defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5861) || defined(CONFIG_ARCH_MT5882)
#define VECTOR_GFX3D_PP     (22+VECTOR_MISC_BASE)
#define VECTOR_GFX3D_PMU    (23+VECTOR_MISC_BASE)
#define VECTOR_GFX3D_GPMMU  (24+VECTOR_MISC_BASE)
#define VECTOR_GFX3D_PPMMU  (25+VECTOR_MISC_BASE)
#endif
#ifndef VECTOR_MMU_GFX
#define VECTOR_MMU_GFX      (26+VECTOR_MISC_BASE)
#endif
#ifndef VECTOR_MMU_GCPU
#define VECTOR_MMU_GCPU     (27+VECTOR_MISC_BASE)
#endif
#ifndef VECTOR_MMU_IMGRZ
#define VECTOR_MMU_IMGRZ    (28+VECTOR_MISC_BASE)
#endif
#ifndef VECTOR_MMU_JPG
#define VECTOR_MMU_JPG      (29+VECTOR_MISC_BASE)
#endif
#ifndef VECTOR_MMU_GDMA
#define VECTOR_MMU_GDMA     (30+VECTOR_MISC_BASE)
#endif
#ifndef VECTOR_DDI
#define VECTOR_DDI          (31+VECTOR_MISC_BASE)
#endif
//misc2
#ifndef VECTOR_LZHS
#define VECTOR_LZHS         (0+VECTOR_MISC2_BASE)
#endif
#ifndef VECTOR_L2C
#define VECTOR_L2C          (1+VECTOR_MISC2_BASE)
#endif
#ifndef VECTOR_IMGRZ2
#define VECTOR_IMGRZ2       (2+VECTOR_MISC2_BASE)
#endif
#ifndef VECTOR_MMU_IMGRZ2
#define VECTOR_MMU_IMGRZ2   (3+VECTOR_MISC2_BASE)
#endif
#ifndef VECTOR_MSDC2
#define VECTOR_MSDC2        (4+VECTOR_MISC2_BASE)
#endif
#ifndef VECTOR_PNG1
#define VECTOR_PNG1         (5+VECTOR_MISC2_BASE)
#endif
#ifndef VECTOR_PNG2
#define VECTOR_PNG2         (6+VECTOR_MISC2_BASE)
#endif
#ifndef VECTOR_PNG3
#define VECTOR_PNG3         (7+VECTOR_MISC2_BASE)
#endif
#ifndef VECTOR_MMU_PNG1
#define VECTOR_MMU_PNG1     (8+VECTOR_MISC2_BASE)
#endif
#if defined (CONFIG_ARCH_MT5399) || defined (CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5861) || defined(CONFIG_ARCH_MT5882)
#ifndef VECTOR_SMCARD1
#define VECTOR_SMCARD1      (9+VECTOR_MISC2_BASE)
#endif
#ifndef VECTOR_TDDC
#define VECTOR_TDDC         (10+VECTOR_MISC2_BASE)
#endif
#ifndef VECTOR_APWM3
#define VECTOR_APWM3        (11+VECTOR_MISC2_BASE)
#endif
#ifndef VECTOR_MMU_VDEC
#define VECTOR_MMU_VDEC     (12+VECTOR_MISC2_BASE)
#endif
#ifndef VECTOR_MMU_DDI
#define VECTOR_MMU_DDI      (13+VECTOR_MISC2_BASE)
#endif
#ifndef VECTOR_MMU_DEMUX
#define VECTOR_MMU_DEMUX    (14+VECTOR_MISC2_BASE)
#endif
#ifndef VECTOR_SSUSB
#define VECTOR_SSUSB        (15+VECTOR_MISC2_BASE)
#endif
#endif
#if defined (CONFIG_ARCH_MT5890)
#define VECTOR_RS232_1_     (16+VECTOR_MISC2_BASE)
#ifndef VECTOR_MMU_IMGRZ3
#define VECTOR_MMU_IMGRZ3   (17+VECTOR_MISC2_BASE)
#endif
#ifndef VECTOR_IMGRZ3
#define VECTOR_IMGRZ3       (18+VECTOR_MISC2_BASE)
#endif
#ifndef VECTOR_ZZ
#define VECTOR_ZZ           (19+VECTOR_MISC2_BASE)
#endif
#ifndef VECTOR_DRAMC_C
#define VECTOR_DRAMC_C      (20+VECTOR_MISC2_BASE)
#endif
#ifndef VECTOR_VDEC_LAE0
#define VECTOR_VDEC_LAE0    (21+VECTOR_MISC2_BASE)
#endif
#ifndef VECTOR_VDEC_LAE1
#define VECTOR_VDEC_LAE1    (22+VECTOR_MISC2_BASE)
#endif
#ifndef VECTOR_TRNG
#define VECTOR_TRNG         (23+VECTOR_MISC2_BASE)
#endif
#ifndef VECTOR_P_B
#define VECTOR_P_B          (24+VECTOR_MISC2_BASE)
#endif
#ifndef VECTOR_VDEC_CORE1
#define VECTOR_VDEC_CORE1   (25+VECTOR_MISC2_BASE)
#endif
#ifndef VECTOR_ABIM0
#define VECTOR_ABIM0        (26+VECTOR_MISC2_BASE)
#endif
#ifndef VECTOR_ABIM1
#define VECTOR_ABIM1        (27+VECTOR_MISC2_BASE)
#endif
#ifndef VECTOR_ABIM2
#define VECTOR_ABIM2        (28+VECTOR_MISC2_BASE)
#endif
#ifndef VECTOR_ABIM3
#define VECTOR_ABIM3        (29+VECTOR_MISC2_BASE)
#endif
#ifndef VECTOR_CI_FMR
#define VECTOR_CI_FMR       (30+VECTOR_MISC2_BASE)
#endif
#ifndef VECTOR_CI_FMR2
#define VECTOR_CI_FMR2      (31+VECTOR_MISC2_BASE)
#endif
#endif // defined (CONFIG_ARCH_MT5890)
#ifndef VECTOR_UART_DMA
#define VECTOR_UART_DMA     (16+VECTOR_MISC2_BASE)
#endif
#if defined (CONFIG_ARCH_MT5399)
#define VECTOR_APOLLO       (17+VECTOR_MISC2_BASE)
#endif
#ifdef CC_NO_KRL_UART_DRV
#define VECTOR_INVALID       (31+VECTOR_MISC2_BASE)
#endif

#if defined (CONFIG_ARCH_MT5890)
#ifndef VECTOR_PTP_THERM
#define VECTOR_PTP_THERM    (0+VECTOR_MISC3_BASE)
#endif
#ifndef VECTOR_PTP_FSM
#define VECTOR_PTP_FSM      (1+VECTOR_MISC3_BASE)
#endif
#ifndef VECTOR_SC_TIMER
#define VECTOR_SC_TIMER     (2+VECTOR_MISC3_BASE)
#endif
#ifndef VECTOR_HDMIRX_20
#define VECTOR_HDMIRX_20    (3+VECTOR_MISC3_BASE)
#endif
#ifndef VECTOR_UHD
#define VECTOR_UHD          (4+VECTOR_MISC3_BASE)
#endif
#endif // defined (CONFIG_ARCH_MT5890)
#ifdef CONFIG_PCI
/* Put the all logical pci-express interrupts behind the normal interrupt. */
#define IRQ_PCIE_INTA	(50+SPI_OFFSET)
#define IRQ_PCIE_INTB	(51+SPI_OFFSET)
#define IRQ_PCIE_INTC	(52+SPI_OFFSET)
#define IRQ_PCIE_INTD	(53+SPI_OFFSET)
#define IRQ_PCIE_AER	(54+SPI_OFFSET)
#ifdef CONFIG_PCI_MSI
#define IRQ_PCIE_MSI	        (IRQ_PCIE_AER + 1)
#define IRQ_PCIE_MSI_END	(IRQ_PCIE_AER + 32)
#endif
#endif

#ifndef _IRQ
#define _IRQ(v)         (1U << (v-SPI_OFFSET)) // IRQ bit by vector
#endif
#ifndef _MISCIRQ
#define _MISCIRQ(v)     (1U << (v-VECTOR_MISC_BASE))    // MISC IRQ bit by vector
#endif
#ifndef _MISC2IRQ
#define _MISC2IRQ(v)     (1U << (v-VECTOR_MISC2_BASE))    // MISC2 IRQ bit by vector
#endif
#ifndef _MISC3IRQ
#define _MISC3IRQ(v)     (1U << (v-VECTOR_MISC3_BASE))    // MISC3 IRQ bit by vector
#endif
#endif /* __USE_XBIM_IRQS */

#endif /*IRQS_H*/
