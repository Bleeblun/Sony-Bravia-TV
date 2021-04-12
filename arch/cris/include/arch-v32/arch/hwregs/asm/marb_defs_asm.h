#ifndef __marb_defs_asm_h
#define __marb_defs_asm_h

/*
 * This file is autogenerated from
 *   file:           ../../inst/memarb/rtl/guinness/marb_top.r
 *     id:           <not found>
 *     last modfied: Mon Apr 11 16:12:16 2005
 *
 *   by /n/asic/design/tools/rdesc/src/rdes2c -asm --outfile asm/marb_defs_asm.h ../../inst/memarb/rtl/guinness/marb_top.r
 *      id: $Id: //DTV/MP_BR/DTV_X_IDTV1501_002288_8_001_800_002/apollo/linux_core/kernel/linux-3.10/arch/cris/include/arch-v32/arch/hwregs/asm/marb_defs_asm.h#1 $
 * Any changes here will be lost.
 *
 * -*- buffer-read-only: t -*-
 */

#ifndef REG_FIELD
#define REG_FIELD( scope, reg, field, value ) \
  REG_FIELD_X_( value, reg_##scope##_##reg##___##field##___lsb )
#define REG_FIELD_X_( value, shift ) ((value) << shift)
#endif

#ifndef REG_STATE
#define REG_STATE( scope, reg, field, symbolic_value ) \
  REG_STATE_X_( regk_##scope##_##symbolic_value, reg_##scope##_##reg##___##field##___lsb )
#define REG_STATE_X_( k, shift ) (k << shift)
#endif

#ifndef REG_MASK
#define REG_MASK( scope, reg, field ) \
  REG_MASK_X_( reg_##scope##_##reg##___##field##___width, reg_##scope##_##reg##___##field##___lsb )
#define REG_MASK_X_( width, lsb ) (((1 << width)-1) << lsb)
#endif

#ifndef REG_LSB
#define REG_LSB( scope, reg, field ) reg_##scope##_##reg##___##field##___lsb
#endif

#ifndef REG_BIT
#define REG_BIT( scope, reg, field ) reg_##scope##_##reg##___##field##___bit
#endif

#ifndef REG_ADDR
#define REG_ADDR( scope, inst, reg ) REG_ADDR_X_(inst, reg_##scope##_##reg##_offset)
#define REG_ADDR_X_( inst, offs ) ((inst) + offs)
#endif

#ifndef REG_ADDR_VECT
#define REG_ADDR_VECT( scope, inst, reg, index ) \
         REG_ADDR_VECT_X_(inst, reg_##scope##_##reg##_offset, index, \
			 STRIDE_##scope##_##reg )
#define REG_ADDR_VECT_X_( inst, offs, index, stride ) \
                          ((inst) + offs + (index) * stride)
#endif

#define STRIDE_marb_rw_int_slots 4
/* Register rw_int_slots, scope marb, type rw */
#define reg_marb_rw_int_slots___owner___lsb 0
#define reg_marb_rw_int_slots___owner___width 4
#define reg_marb_rw_int_slots_offset 0

#define STRIDE_marb_rw_ext_slots 4
/* Register rw_ext_slots, scope marb, type rw */
#define reg_marb_rw_ext_slots___owner___lsb 0
#define reg_marb_rw_ext_slots___owner___width 4
#define reg_marb_rw_ext_slots_offset 256

#define STRIDE_marb_rw_regs_slots 4
/* Register rw_regs_slots, scope marb, type rw */
#define reg_marb_rw_regs_slots___owner___lsb 0
#define reg_marb_rw_regs_slots___owner___width 4
#define reg_marb_rw_regs_slots_offset 512

/* Register rw_intr_mask, scope marb, type rw */
#define reg_marb_rw_intr_mask___bp0___lsb 0
#define reg_marb_rw_intr_mask___bp0___width 1
#define reg_marb_rw_intr_mask___bp0___bit 0
#define reg_marb_rw_intr_mask___bp1___lsb 1
#define reg_marb_rw_intr_mask___bp1___width 1
#define reg_marb_rw_intr_mask___bp1___bit 1
#define reg_marb_rw_intr_mask___bp2___lsb 2
#define reg_marb_rw_intr_mask___bp2___width 1
#define reg_marb_rw_intr_mask___bp2___bit 2
#define reg_marb_rw_intr_mask___bp3___lsb 3
#define reg_marb_rw_intr_mask___bp3___width 1
#define reg_marb_rw_intr_mask___bp3___bit 3
#define reg_marb_rw_intr_mask_offset 528

/* Register rw_ack_intr, scope marb, type rw */
#define reg_marb_rw_ack_intr___bp0___lsb 0
#define reg_marb_rw_ack_intr___bp0___width 1
#define reg_marb_rw_ack_intr___bp0___bit 0
#define reg_marb_rw_ack_intr___bp1___lsb 1
#define reg_marb_rw_ack_intr___bp1___width 1
#define reg_marb_rw_ack_intr___bp1___bit 1
#define reg_marb_rw_ack_intr___bp2___lsb 2
#define reg_marb_rw_ack_intr___bp2___width 1
#define reg_marb_rw_ack_intr___bp2___bit 2
#define reg_marb_rw_ack_intr___bp3___lsb 3
#define reg_marb_rw_ack_intr___bp3___width 1
#define reg_marb_rw_ack_intr___bp3___bit 3
#define reg_marb_rw_ack_intr_offset 532

/* Register r_intr, scope marb, type r */
#define reg_marb_r_intr___bp0___lsb 0
#define reg_marb_r_intr___bp0___width 1
#define reg_marb_r_intr___bp0___bit 0
#define reg_marb_r_intr___bp1___lsb 1
#define reg_marb_r_intr___bp1___width 1
#define reg_marb_r_intr___bp1___bit 1
#define reg_marb_r_intr___bp2___lsb 2
#define reg_marb_r_intr___bp2___width 1
#define reg_marb_r_intr___bp2___bit 2
#define reg_marb_r_intr___bp3___lsb 3
#define reg_marb_r_intr___bp3___width 1
#define reg_marb_r_intr___bp3___bit 3
#define reg_marb_r_intr_offset 536

/* Register r_masked_intr, scope marb, type r */
#define reg_marb_r_masked_intr___bp0___lsb 0
#define reg_marb_r_masked_intr___bp0___width 1
#define reg_marb_r_masked_intr___bp0___bit 0
#define reg_marb_r_masked_intr___bp1___lsb 1
#define reg_marb_r_masked_intr___bp1___width 1
#define reg_marb_r_masked_intr___bp1___bit 1
#define reg_marb_r_masked_intr___bp2___lsb 2
#define reg_marb_r_masked_intr___bp2___width 1
#define reg_marb_r_masked_intr___bp2___bit 2
#define reg_marb_r_masked_intr___bp3___lsb 3
#define reg_marb_r_masked_intr___bp3___width 1
#define reg_marb_r_masked_intr___bp3___bit 3
#define reg_marb_r_masked_intr_offset 540

/* Register rw_stop_mask, scope marb, type rw */
#define reg_marb_rw_stop_mask___dma0___lsb 0
#define reg_marb_rw_stop_mask___dma0___width 1
#define reg_marb_rw_stop_mask___dma0___bit 0
#define reg_marb_rw_stop_mask___dma1___lsb 1
#define reg_marb_rw_stop_mask___dma1___width 1
#define reg_marb_rw_stop_mask___dma1___bit 1
#define reg_marb_rw_stop_mask___dma2___lsb 2
#define reg_marb_rw_stop_mask___dma2___width 1
#define reg_marb_rw_stop_mask___dma2___bit 2
#define reg_marb_rw_stop_mask___dma3___lsb 3
#define reg_marb_rw_stop_mask___dma3___width 1
#define reg_marb_rw_stop_mask___dma3___bit 3
#define reg_marb_rw_stop_mask___dma4___lsb 4
#define reg_marb_rw_stop_mask___dma4___width 1
#define reg_marb_rw_stop_mask___dma4___bit 4
#define reg_marb_rw_stop_mask___dma5___lsb 5
#define reg_marb_rw_stop_mask___dma5___width 1
#define reg_marb_rw_stop_mask___dma5___bit 5
#define reg_marb_rw_stop_mask___dma6___lsb 6
#define reg_marb_rw_stop_mask___dma6___width 1
#define reg_marb_rw_stop_mask___dma6___bit 6
#define reg_marb_rw_stop_mask___dma7___lsb 7
#define reg_marb_rw_stop_mask___dma7___width 1
#define reg_marb_rw_stop_mask___dma7___bit 7
#define reg_marb_rw_stop_mask___dma8___lsb 8
#define reg_marb_rw_stop_mask___dma8___width 1
#define reg_marb_rw_stop_mask___dma8___bit 8
#define reg_marb_rw_stop_mask___dma9___lsb 9
#define reg_marb_rw_stop_mask___dma9___width 1
#define reg_marb_rw_stop_mask___dma9___bit 9
#define reg_marb_rw_stop_mask___cpui___lsb 10
#define reg_marb_rw_stop_mask___cpui___width 1
#define reg_marb_rw_stop_mask___cpui___bit 10
#define reg_marb_rw_stop_mask___cpud___lsb 11
#define reg_marb_rw_stop_mask___cpud___width 1
#define reg_marb_rw_stop_mask___cpud___bit 11
#define reg_marb_rw_stop_mask___iop___lsb 12
#define reg_marb_rw_stop_mask___iop___width 1
#define reg_marb_rw_stop_mask___iop___bit 12
#define reg_marb_rw_stop_mask___slave___lsb 13
#define reg_marb_rw_stop_mask___slave___width 1
#define reg_marb_rw_stop_mask___slave___bit 13
#define reg_marb_rw_stop_mask_offset 544

/* Register r_stopped, scope marb, type r */
#define reg_marb_r_stopped___dma0___lsb 0
#define reg_marb_r_stopped___dma0___width 1
#define reg_marb_r_stopped___dma0___bit 0
#define reg_marb_r_stopped___dma1___lsb 1
#define reg_marb_r_stopped___dma1___width 1
#define reg_marb_r_stopped___dma1___bit 1
#define reg_marb_r_stopped___dma2___lsb 2
#define reg_marb_r_stopped___dma2___width 1
#define reg_marb_r_stopped___dma2___bit 2
#define reg_marb_r_stopped___dma3___lsb 3
#define reg_marb_r_stopped___dma3___width 1
#define reg_marb_r_stopped___dma3___bit 3
#define reg_marb_r_stopped___dma4___lsb 4
#define reg_marb_r_stopped___dma4___width 1
#define reg_marb_r_stopped___dma4___bit 4
#define reg_marb_r_stopped___dma5___lsb 5
#define reg_marb_r_stopped___dma5___width 1
#define reg_marb_r_stopped___dma5___bit 5
#define reg_marb_r_stopped___dma6___lsb 6
#define reg_marb_r_stopped___dma6___width 1
#define reg_marb_r_stopped___dma6___bit 6
#define reg_marb_r_stopped___dma7___lsb 7
#define reg_marb_r_stopped___dma7___width 1
#define reg_marb_r_stopped___dma7___bit 7
#define reg_marb_r_stopped___dma8___lsb 8
#define reg_marb_r_stopped___dma8___width 1
#define reg_marb_r_stopped___dma8___bit 8
#define reg_marb_r_stopped___dma9___lsb 9
#define reg_marb_r_stopped___dma9___width 1
#define reg_marb_r_stopped___dma9___bit 9
#define reg_marb_r_stopped___cpui___lsb 10
#define reg_marb_r_stopped___cpui___width 1
#define reg_marb_r_stopped___cpui___bit 10
#define reg_marb_r_stopped___cpud___lsb 11
#define reg_marb_r_stopped___cpud___width 1
#define reg_marb_r_stopped___cpud___bit 11
#define reg_marb_r_stopped___iop___lsb 12
#define reg_marb_r_stopped___iop___width 1
#define reg_marb_r_stopped___iop___bit 12
#define reg_marb_r_stopped___slave___lsb 13
#define reg_marb_r_stopped___slave___width 1
#define reg_marb_r_stopped___slave___bit 13
#define reg_marb_r_stopped_offset 548

/* Register rw_no_snoop, scope marb, type rw */
#define reg_marb_rw_no_snoop___dma0___lsb 0
#define reg_marb_rw_no_snoop___dma0___width 1
#define reg_marb_rw_no_snoop___dma0___bit 0
#define reg_marb_rw_no_snoop___dma1___lsb 1
#define reg_marb_rw_no_snoop___dma1___width 1
#define reg_marb_rw_no_snoop___dma1___bit 1
#define reg_marb_rw_no_snoop___dma2___lsb 2
#define reg_marb_rw_no_snoop___dma2___width 1
#define reg_marb_rw_no_snoop___dma2___bit 2
#define reg_marb_rw_no_snoop___dma3___lsb 3
#define reg_marb_rw_no_snoop___dma3___width 1
#define reg_marb_rw_no_snoop___dma3___bit 3
#define reg_marb_rw_no_snoop___dma4___lsb 4
#define reg_marb_rw_no_snoop___dma4___width 1
#define reg_marb_rw_no_snoop___dma4___bit 4
#define reg_marb_rw_no_snoop___dma5___lsb 5
#define reg_marb_rw_no_snoop___dma5___width 1
#define reg_marb_rw_no_snoop___dma5___bit 5
#define reg_marb_rw_no_snoop___dma6___lsb 6
#define reg_marb_rw_no_snoop___dma6___width 1
#define reg_marb_rw_no_snoop___dma6___bit 6
#define reg_marb_rw_no_snoop___dma7___lsb 7
#define reg_marb_rw_no_snoop___dma7___width 1
#define reg_marb_rw_no_snoop___dma7___bit 7
#define reg_marb_rw_no_snoop___dma8___lsb 8
#define reg_marb_rw_no_snoop___dma8___width 1
#define reg_marb_rw_no_snoop___dma8___bit 8
#define reg_marb_rw_no_snoop___dma9___lsb 9
#define reg_marb_rw_no_snoop___dma9___width 1
#define reg_marb_rw_no_snoop___dma9___bit 9
#define reg_marb_rw_no_snoop___cpui___lsb 10
#define reg_marb_rw_no_snoop___cpui___width 1
#define reg_marb_rw_no_snoop___cpui___bit 10
#define reg_marb_rw_no_snoop___cpud___lsb 11
#define reg_marb_rw_no_snoop___cpud___width 1
#define reg_marb_rw_no_snoop___cpud___bit 11
#define reg_marb_rw_no_snoop___iop___lsb 12
#define reg_marb_rw_no_snoop___iop___width 1
#define reg_marb_rw_no_snoop___iop___bit 12
#define reg_marb_rw_no_snoop___slave___lsb 13
#define reg_marb_rw_no_snoop___slave___width 1
#define reg_marb_rw_no_snoop___slave___bit 13
#define reg_marb_rw_no_snoop_offset 832

/* Register rw_no_snoop_rq, scope marb, type rw */
#define reg_marb_rw_no_snoop_rq___cpui___lsb 10
#define reg_marb_rw_no_snoop_rq___cpui___width 1
#define reg_marb_rw_no_snoop_rq___cpui___bit 10
#define reg_marb_rw_no_snoop_rq___cpud___lsb 11
#define reg_marb_rw_no_snoop_rq___cpud___width 1
#define reg_marb_rw_no_snoop_rq___cpud___bit 11
#define reg_marb_rw_no_snoop_rq_offset 836


/* Constants */
#define regk_marb_cpud                            0x0000000b
#define regk_marb_cpui                            0x0000000a
#define regk_marb_dma0                            0x00000000
#define regk_marb_dma1                            0x00000001
#define regk_marb_dma2                            0x00000002
#define regk_marb_dma3                            0x00000003
#define regk_marb_dma4                            0x00000004
#define regk_marb_dma5                            0x00000005
#define regk_marb_dma6                            0x00000006
#define regk_marb_dma7                            0x00000007
#define regk_marb_dma8                            0x00000008
#define regk_marb_dma9                            0x00000009
#define regk_marb_iop                             0x0000000c
#define regk_marb_no                              0x00000000
#define regk_marb_r_stopped_default               0x00000000
#define regk_marb_rw_ext_slots_default            0x00000000
#define regk_marb_rw_ext_slots_size               0x00000040
#define regk_marb_rw_int_slots_default            0x00000000
#define regk_marb_rw_int_slots_size               0x00000040
#define regk_marb_rw_intr_mask_default            0x00000000
#define regk_marb_rw_no_snoop_default             0x00000000
#define regk_marb_rw_no_snoop_rq_default          0x00000000
#define regk_marb_rw_regs_slots_default           0x00000000
#define regk_marb_rw_regs_slots_size              0x00000004
#define regk_marb_rw_stop_mask_default            0x00000000
#define regk_marb_slave                           0x0000000d
#define regk_marb_yes                             0x00000001
#endif /* __marb_defs_asm_h */
#ifndef __marb_bp_defs_asm_h
#define __marb_bp_defs_asm_h

/*
 * This file is autogenerated from
 *   file:           ../../inst/memarb/rtl/guinness/marb_top.r
 *     id:           <not found>
 *     last modfied: Mon Apr 11 16:12:16 2005
 *
 *   by /n/asic/design/tools/rdesc/src/rdes2c -asm --outfile asm/marb_defs_asm.h ../../inst/memarb/rtl/guinness/marb_top.r
 *      id: $Id: //DTV/MP_BR/DTV_X_IDTV1501_002288_8_001_800_002/apollo/linux_core/kernel/linux-3.10/arch/cris/include/arch-v32/arch/hwregs/asm/marb_defs_asm.h#1 $
 * Any changes here will be lost.
 *
 * -*- buffer-read-only: t -*-
 */

#ifndef REG_FIELD
#define REG_FIELD( scope, reg, field, value ) \
  REG_FIELD_X_( value, reg_##scope##_##reg##___##field##___lsb )
#define REG_FIELD_X_( value, shift ) ((value) << shift)
#endif

#ifndef REG_STATE
#define REG_STATE( scope, reg, field, symbolic_value ) \
  REG_STATE_X_( regk_##scope##_##symbolic_value, reg_##scope##_##reg##___##field##___lsb )
#define REG_STATE_X_( k, shift ) (k << shift)
#endif

#ifndef REG_MASK
#define REG_MASK( scope, reg, field ) \
  REG_MASK_X_( reg_##scope##_##reg##___##field##___width, reg_##scope##_##reg##___##field##___lsb )
#define REG_MASK_X_( width, lsb ) (((1 << width)-1) << lsb)
#endif

#ifndef REG_LSB
#define REG_LSB( scope, reg, field ) reg_##scope##_##reg##___##field##___lsb
#endif

#ifndef REG_BIT
#define REG_BIT( scope, reg, field ) reg_##scope##_##reg##___##field##___bit
#endif

#ifndef REG_ADDR
#define REG_ADDR( scope, inst, reg ) REG_ADDR_X_(inst, reg_##scope##_##reg##_offset)
#define REG_ADDR_X_( inst, offs ) ((inst) + offs)
#endif

#ifndef REG_ADDR_VECT
#define REG_ADDR_VECT( scope, inst, reg, index ) \
         REG_ADDR_VECT_X_(inst, reg_##scope##_##reg##_offset, index, \
			 STRIDE_##scope##_##reg )
#define REG_ADDR_VECT_X_( inst, offs, index, stride ) \
                          ((inst) + offs + (index) * stride)
#endif

/* Register rw_first_addr, scope marb_bp, type rw */
#define reg_marb_bp_rw_first_addr_offset 0

/* Register rw_last_addr, scope marb_bp, type rw */
#define reg_marb_bp_rw_last_addr_offset 4

/* Register rw_op, scope marb_bp, type rw */
#define reg_marb_bp_rw_op___rd___lsb 0
#define reg_marb_bp_rw_op___rd___width 1
#define reg_marb_bp_rw_op___rd___bit 0
#define reg_marb_bp_rw_op___wr___lsb 1
#define reg_marb_bp_rw_op___wr___width 1
#define reg_marb_bp_rw_op___wr___bit 1
#define reg_marb_bp_rw_op___rd_excl___lsb 2
#define reg_marb_bp_rw_op___rd_excl___width 1
#define reg_marb_bp_rw_op___rd_excl___bit 2
#define reg_marb_bp_rw_op___pri_wr___lsb 3
#define reg_marb_bp_rw_op___pri_wr___width 1
#define reg_marb_bp_rw_op___pri_wr___bit 3
#define reg_marb_bp_rw_op___us_rd___lsb 4
#define reg_marb_bp_rw_op___us_rd___width 1
#define reg_marb_bp_rw_op___us_rd___bit 4
#define reg_marb_bp_rw_op___us_wr___lsb 5
#define reg_marb_bp_rw_op___us_wr___width 1
#define reg_marb_bp_rw_op___us_wr___bit 5
#define reg_marb_bp_rw_op___us_rd_excl___lsb 6
#define reg_marb_bp_rw_op___us_rd_excl___width 1
#define reg_marb_bp_rw_op___us_rd_excl___bit 6
#define reg_marb_bp_rw_op___us_pri_wr___lsb 7
#define reg_marb_bp_rw_op___us_pri_wr___width 1
#define reg_marb_bp_rw_op___us_pri_wr___bit 7
#define reg_marb_bp_rw_op_offset 8

/* Register rw_clients, scope marb_bp, type rw */
#define reg_marb_bp_rw_clients___dma0___lsb 0
#define reg_marb_bp_rw_clients___dma0___width 1
#define reg_marb_bp_rw_clients___dma0___bit 0
#define reg_marb_bp_rw_clients___dma1___lsb 1
#define reg_marb_bp_rw_clients___dma1___width 1
#define reg_marb_bp_rw_clients___dma1___bit 1
#define reg_marb_bp_rw_clients___dma2___lsb 2
#define reg_marb_bp_rw_clients___dma2___width 1
#define reg_marb_bp_rw_clients___dma2___bit 2
#define reg_marb_bp_rw_clients___dma3___lsb 3
#define reg_marb_bp_rw_clients___dma3___width 1
#define reg_marb_bp_rw_clients___dma3___bit 3
#define reg_marb_bp_rw_clients___dma4___lsb 4
#define reg_marb_bp_rw_clients___dma4___width 1
#define reg_marb_bp_rw_clients___dma4___bit 4
#define reg_marb_bp_rw_clients___dma5___lsb 5
#define reg_marb_bp_rw_clients___dma5___width 1
#define reg_marb_bp_rw_clients___dma5___bit 5
#define reg_marb_bp_rw_clients___dma6___lsb 6
#define reg_marb_bp_rw_clients___dma6___width 1
#define reg_marb_bp_rw_clients___dma6___bit 6
#define reg_marb_bp_rw_clients___dma7___lsb 7
#define reg_marb_bp_rw_clients___dma7___width 1
#define reg_marb_bp_rw_clients___dma7___bit 7
#define reg_marb_bp_rw_clients___dma8___lsb 8
#define reg_marb_bp_rw_clients___dma8___width 1
#define reg_marb_bp_rw_clients___dma8___bit 8
#define reg_marb_bp_rw_clients___dma9___lsb 9
#define reg_marb_bp_rw_clients___dma9___width 1
#define reg_marb_bp_rw_clients___dma9___bit 9
#define reg_marb_bp_rw_clients___cpui___lsb 10
#define reg_marb_bp_rw_clients___cpui___width 1
#define reg_marb_bp_rw_clients___cpui___bit 10
#define reg_marb_bp_rw_clients___cpud___lsb 11
#define reg_marb_bp_rw_clients___cpud___width 1
#define reg_marb_bp_rw_clients___cpud___bit 11
#define reg_marb_bp_rw_clients___iop___lsb 12
#define reg_marb_bp_rw_clients___iop___width 1
#define reg_marb_bp_rw_clients___iop___bit 12
#define reg_marb_bp_rw_clients___slave___lsb 13
#define reg_marb_bp_rw_clients___slave___width 1
#define reg_marb_bp_rw_clients___slave___bit 13
#define reg_marb_bp_rw_clients_offset 12

/* Register rw_options, scope marb_bp, type rw */
#define reg_marb_bp_rw_options___wrap___lsb 0
#define reg_marb_bp_rw_options___wrap___width 1
#define reg_marb_bp_rw_options___wrap___bit 0
#define reg_marb_bp_rw_options_offset 16

/* Register r_brk_addr, scope marb_bp, type r */
#define reg_marb_bp_r_brk_addr_offset 20

/* Register r_brk_op, scope marb_bp, type r */
#define reg_marb_bp_r_brk_op___rd___lsb 0
#define reg_marb_bp_r_brk_op___rd___width 1
#define reg_marb_bp_r_brk_op___rd___bit 0
#define reg_marb_bp_r_brk_op___wr___lsb 1
#define reg_marb_bp_r_brk_op___wr___width 1
#define reg_marb_bp_r_brk_op___wr___bit 1
#define reg_marb_bp_r_brk_op___rd_excl___lsb 2
#define reg_marb_bp_r_brk_op___rd_excl___width 1
#define reg_marb_bp_r_brk_op___rd_excl___bit 2
#define reg_marb_bp_r_brk_op___pri_wr___lsb 3
#define reg_marb_bp_r_brk_op___pri_wr___width 1
#define reg_marb_bp_r_brk_op___pri_wr___bit 3
#define reg_marb_bp_r_brk_op___us_rd___lsb 4
#define reg_marb_bp_r_brk_op___us_rd___width 1
#define reg_marb_bp_r_brk_op___us_rd___bit 4
#define reg_marb_bp_r_brk_op___us_wr___lsb 5
#define reg_marb_bp_r_brk_op___us_wr___width 1
#define reg_marb_bp_r_brk_op___us_wr___bit 5
#define reg_marb_bp_r_brk_op___us_rd_excl___lsb 6
#define reg_marb_bp_r_brk_op___us_rd_excl___width 1
#define reg_marb_bp_r_brk_op___us_rd_excl___bit 6
#define reg_marb_bp_r_brk_op___us_pri_wr___lsb 7
#define reg_marb_bp_r_brk_op___us_pri_wr___width 1
#define reg_marb_bp_r_brk_op___us_pri_wr___bit 7
#define reg_marb_bp_r_brk_op_offset 24

/* Register r_brk_clients, scope marb_bp, type r */
#define reg_marb_bp_r_brk_clients___dma0___lsb 0
#define reg_marb_bp_r_brk_clients___dma0___width 1
#define reg_marb_bp_r_brk_clients___dma0___bit 0
#define reg_marb_bp_r_brk_clients___dma1___lsb 1
#define reg_marb_bp_r_brk_clients___dma1___width 1
#define reg_marb_bp_r_brk_clients___dma1___bit 1
#define reg_marb_bp_r_brk_clients___dma2___lsb 2
#define reg_marb_bp_r_brk_clients___dma2___width 1
#define reg_marb_bp_r_brk_clients___dma2___bit 2
#define reg_marb_bp_r_brk_clients___dma3___lsb 3
#define reg_marb_bp_r_brk_clients___dma3___width 1
#define reg_marb_bp_r_brk_clients___dma3___bit 3
#define reg_marb_bp_r_brk_clients___dma4___lsb 4
#define reg_marb_bp_r_brk_clients___dma4___width 1
#define reg_marb_bp_r_brk_clients___dma4___bit 4
#define reg_marb_bp_r_brk_clients___dma5___lsb 5
#define reg_marb_bp_r_brk_clients___dma5___width 1
#define reg_marb_bp_r_brk_clients___dma5___bit 5
#define reg_marb_bp_r_brk_clients___dma6___lsb 6
#define reg_marb_bp_r_brk_clients___dma6___width 1
#define reg_marb_bp_r_brk_clients___dma6___bit 6
#define reg_marb_bp_r_brk_clients___dma7___lsb 7
#define reg_marb_bp_r_brk_clients___dma7___width 1
#define reg_marb_bp_r_brk_clients___dma7___bit 7
#define reg_marb_bp_r_brk_clients___dma8___lsb 8
#define reg_marb_bp_r_brk_clients___dma8___width 1
#define reg_marb_bp_r_brk_clients___dma8___bit 8
#define reg_marb_bp_r_brk_clients___dma9___lsb 9
#define reg_marb_bp_r_brk_clients___dma9___width 1
#define reg_marb_bp_r_brk_clients___dma9___bit 9
#define reg_marb_bp_r_brk_clients___cpui___lsb 10
#define reg_marb_bp_r_brk_clients___cpui___width 1
#define reg_marb_bp_r_brk_clients___cpui___bit 10
#define reg_marb_bp_r_brk_clients___cpud___lsb 11
#define reg_marb_bp_r_brk_clients___cpud___width 1
#define reg_marb_bp_r_brk_clients___cpud___bit 11
#define reg_marb_bp_r_brk_clients___iop___lsb 12
#define reg_marb_bp_r_brk_clients___iop___width 1
#define reg_marb_bp_r_brk_clients___iop___bit 12
#define reg_marb_bp_r_brk_clients___slave___lsb 13
#define reg_marb_bp_r_brk_clients___slave___width 1
#define reg_marb_bp_r_brk_clients___slave___bit 13
#define reg_marb_bp_r_brk_clients_offset 28

/* Register r_brk_first_client, scope marb_bp, type r */
#define reg_marb_bp_r_brk_first_client___dma0___lsb 0
#define reg_marb_bp_r_brk_first_client___dma0___width 1
#define reg_marb_bp_r_brk_first_client___dma0___bit 0
#define reg_marb_bp_r_brk_first_client___dma1___lsb 1
#define reg_marb_bp_r_brk_first_client___dma1___width 1
#define reg_marb_bp_r_brk_first_client___dma1___bit 1
#define reg_marb_bp_r_brk_first_client___dma2___lsb 2
#define reg_marb_bp_r_brk_first_client___dma2___width 1
#define reg_marb_bp_r_brk_first_client___dma2___bit 2
#define reg_marb_bp_r_brk_first_client___dma3___lsb 3
#define reg_marb_bp_r_brk_first_client___dma3___width 1
#define reg_marb_bp_r_brk_first_client___dma3___bit 3
#define reg_marb_bp_r_brk_first_client___dma4___lsb 4
#define reg_marb_bp_r_brk_first_client___dma4___width 1
#define reg_marb_bp_r_brk_first_client___dma4___bit 4
#define reg_marb_bp_r_brk_first_client___dma5___lsb 5
#define reg_marb_bp_r_brk_first_client___dma5___width 1
#define reg_marb_bp_r_brk_first_client___dma5___bit 5
#define reg_marb_bp_r_brk_first_client___dma6___lsb 6
#define reg_marb_bp_r_brk_first_client___dma6___width 1
#define reg_marb_bp_r_brk_first_client___dma6___bit 6
#define reg_marb_bp_r_brk_first_client___dma7___lsb 7
#define reg_marb_bp_r_brk_first_client___dma7___width 1
#define reg_marb_bp_r_brk_first_client___dma7___bit 7
#define reg_marb_bp_r_brk_first_client___dma8___lsb 8
#define reg_marb_bp_r_brk_first_client___dma8___width 1
#define reg_marb_bp_r_brk_first_client___dma8___bit 8
#define reg_marb_bp_r_brk_first_client___dma9___lsb 9
#define reg_marb_bp_r_brk_first_client___dma9___width 1
#define reg_marb_bp_r_brk_first_client___dma9___bit 9
#define reg_marb_bp_r_brk_first_client___cpui___lsb 10
#define reg_marb_bp_r_brk_first_client___cpui___width 1
#define reg_marb_bp_r_brk_first_client___cpui___bit 10
#define reg_marb_bp_r_brk_first_client___cpud___lsb 11
#define reg_marb_bp_r_brk_first_client___cpud___width 1
#define reg_marb_bp_r_brk_first_client___cpud___bit 11
#define reg_marb_bp_r_brk_first_client___iop___lsb 12
#define reg_marb_bp_r_brk_first_client___iop___width 1
#define reg_marb_bp_r_brk_first_client___iop___bit 12
#define reg_marb_bp_r_brk_first_client___slave___lsb 13
#define reg_marb_bp_r_brk_first_client___slave___width 1
#define reg_marb_bp_r_brk_first_client___slave___bit 13
#define reg_marb_bp_r_brk_first_client_offset 32

/* Register r_brk_size, scope marb_bp, type r */
#define reg_marb_bp_r_brk_size_offset 36

/* Register rw_ack, scope marb_bp, type rw */
#define reg_marb_bp_rw_ack_offset 40


/* Constants */
#define regk_marb_bp_no                           0x00000000
#define regk_marb_bp_rw_op_default                0x00000000
#define regk_marb_bp_rw_options_default           0x00000000
#define regk_marb_bp_yes                          0x00000001
#endif /* __marb_bp_defs_asm_h */