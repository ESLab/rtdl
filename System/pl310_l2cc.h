/*
** Copyright (c) 2009 ARM Ltd. All rights reserved.
*/

#ifndef PL310_L2CC_H
#define PL310_L2CC_H

void init_l2cc(void);

// Accessor functions

// Register r0
unsigned get_pl310_l2cc_CacheID(void);
unsigned get_pl310_l2cc_CacheType(void);

// Register r1
void pl310_l2cc_enable(unsigned set);

unsigned get_pl310_l2cc_AuxCtrl(void);
void set_pl310_l2cc_AuxCtrl(unsigned data);
unsigned get_pl310_l2cc_TagRAMLatencyCtrl(void);
void set_pl310_l2cc_TagRAMLatencyCtrl(unsigned data);
unsigned get_pl310_l2cc_DataRAMLatencyCtrl(void);
void set_pl310_l2cc_DataRAMLatencyCtrl(unsigned data);

// Register r2
unsigned get_pl310_l2cc_EvtCtrCtrl(void);
void set_pl310_l2cc_EvtCtrCtrl(unsigned data);
unsigned get_pl310_l2cc_EvtCtr1Cnfg(void);
void set_pl310_l2cc_EvtCtr1Cnfg(unsigned data);
unsigned get_pl310_l2cc_EvtCtr0Cnfg(void);
void set_pl310_l2cc_EvtCtr0Cnfg(unsigned data);
unsigned get_pl310_l2cc_EvtCtr1Val(void);
void set_pl310_l2cc_EvtCtr1Val(unsigned data);
unsigned get_pl310_l2cc_EvtCtr0Val(void);
void set_pl310_l2cc_EvtCtr0Val(unsigned data);
unsigned get_pl310_l2cc_IntrMask(void);
void set_pl310_l2cc_IntrMask(unsigned data);
unsigned get_pl310_l2cc_MaskIntrStatus(void);
unsigned get_pl310_l2cc_RawIntrStatus(void);
void set_pl310_l2cc_IntrClr(unsigned data);

// Register r7
unsigned get_pl310_l2cc_CacheSync(void);
void set_pl310_l2cc_CacheSync(unsigned data);
unsigned get_pl310_l2cc_InvalLineByPA(void);
void set_pl310_l2cc_InvalLineByPA(unsigned data);
unsigned get_pl310_l2cc_InvalByWay(void);
void set_pl310_l2cc_InvalByWay(unsigned data);
unsigned get_pl310_l2cc_CleanLineByPA(void);
void set_pl310_l2cc_CleanLineByPA(unsigned data);
unsigned get_pl310_l2cc_CleanLineByIndexWay(void);
void set_pl310_l2cc_CleanLineByIndexWay(unsigned data);
unsigned get_pl310_l2cc_CleanByWay(void);
void set_pl310_l2cc_CleanByWay(unsigned data);
unsigned get_pl310_l2cc_CleanInvalByPA(void);
void set_pl310_l2cc_CleanInvalByPA(unsigned data);
unsigned get_pl310_l2cc_CleanInvalByIndexWay(void);
void set_pl310_l2cc_CleanInvalByIndexWay(unsigned data);
unsigned get_pl310_l2cc_CleanInvalByWay(void);
void set_pl310_l2cc_CleanInvalByWay(unsigned data);

// Register r9
unsigned get_pl310_l2cc_DataLockdownByWay(unsigned master_id);
void set_pl310_l2cc_DataLockdownByWay(unsigned master_id, unsigned data);
unsigned get_pl310_l2cc_InstrLockdownByWay(unsigned master_id);
void set_pl310_l2cc_InstrLockdownByWay(unsigned master_id, unsigned data);
unsigned get_pl310_l2cc_LockdownByLineEnable(void);
void set_pl310_l2cc_LockdownByLineEnable(unsigned data);
unsigned get_pl310_l2cc_UnlockAllLinesByWay(void);
void set_pl310_l2cc_UnlockAllLinesByWay(unsigned data);

// Register r12
unsigned get_pl310_l2cc_AddrFilteringStart(void);
void set_pl310_l2cc_AddrFilteringStart(unsigned data);
unsigned get_pl310_l2cc_AddrFilteringEnd(void);
void set_pl310_l2cc_AddrFilteringEnd(unsigned data);

// Register r15
unsigned get_pl310_l2cc_DebugCtrl(void);
void set_pl310_l2cc_DebugCtrl(unsigned data);

#endif
