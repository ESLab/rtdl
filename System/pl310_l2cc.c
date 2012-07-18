/*
** Copyright (c) 2009 ARM Ltd. All rights reserved.
*/

#include "pl310_l2cc.h"

struct pl310_l2cc
{
  // Register r0
  const volatile unsigned CacheID;              // 0x000
  const volatile unsigned CacheType;            // 0x004
  volatile unsigned Reserved0[(0x100-0x08)/4];  // 0x008-0x0FC

  // Register r1
  volatile unsigned Ctrl;                       // 0x100
  volatile unsigned AuxCtrl;                    // 0x104
  volatile unsigned TagRAMLatencyCtrl;          // 0x108
  volatile unsigned DataRAMLatencyCtrl;         // 0x10C
  volatile unsigned Reserved1[(0x200-0x110)/4]; // 0x110-0x1FC

  // Register r2
  volatile unsigned EvtCtrCtrl;                 // 0x200
  volatile unsigned EvtCtr1Cnfg;                // 0x204
  volatile unsigned EvtCtr0Cnfg;                // 0x208
  volatile unsigned EvtCtr1Val;                 // 0x20C
  volatile unsigned EvtCtr0Val;                 // 0x210
  volatile unsigned IntrMask;                   // 0x214
  const volatile unsigned MaskIntrStatus;       // 0x218
  const volatile unsigned RawIntrStatus;        // 0x21C
  volatile unsigned IntrClr;                    // 0x220 (WO)
  volatile unsigned Reserved2[(0x730-0x224)/4]; // 0x224-0x72c

  // Register r7
  volatile unsigned CacheSync;                   // 0x730
  volatile unsigned Reserved71[(0x770-0x734)/4]; // 0x734-0x76C
  volatile unsigned InvalLineByPA;               // 0x770
  volatile unsigned Reserved72[(0x77C-0x774)/4]; // 0x774-0x778
  volatile unsigned InvalByWay;                  // 0x77C
  volatile unsigned Reserved73[(0x7B0-0x780)/4]; // 0x780-0x7AC
  volatile unsigned CleanLineByPA;               // 0x7B0
  volatile unsigned Reserved74;                  // 0x7B4
  volatile unsigned CleanLineByIndexWay;         // 0x7B8
  volatile unsigned CleanByWay;                  // 0x7BC
  volatile unsigned Reserved75[(0x7F0-0x7C0)/4]; // 0x7C0-0x7EC
  volatile unsigned CleanInvalByPA;              // 0x7F0
  volatile unsigned Reserved76;                  // 0x7F4
  volatile unsigned CleanInvalByIndexWay;        // 0x7F8
  volatile unsigned CleanInvalByWay;             // 0x7FC
  volatile unsigned Reserved77[(0x900-0x800)/4]; // 0x800-0x8FC

  // Register r9
  volatile unsigned DataLockdown0ByWay;          // 0x900
  volatile unsigned InstrLockdown0ByWay;         // 0x904
  volatile unsigned DataLockdown1ByWay;          // 0x908
  volatile unsigned InstrLockdown1ByWay;         // 0x90C
  volatile unsigned DataLockdown2ByWay;          // 0x910
  volatile unsigned InstrLockdown2ByWay;         // 0x914
  volatile unsigned DataLockdown3ByWay;          // 0x918
  volatile unsigned InstrLockdown3ByWay;         // 0x91C
  volatile unsigned DataLockdown4ByWay;          // 0x920
  volatile unsigned InstrLockdown4ByWay;         // 0x924
  volatile unsigned DataLockdown5ByWay;          // 0x928
  volatile unsigned InstrLockdown5ByWay;         // 0x92C
  volatile unsigned DataLockdown6ByWay;          // 0x930
  volatile unsigned InstrLockdown6ByWay;         // 0x934
  volatile unsigned DataLockdown7ByWay;          // 0x938
  volatile unsigned InstrLockdown7ByWay;         // 0x93C
  volatile unsigned Reserved90[(0x950-0x940)/4]; // 0x940-0x94C
  volatile unsigned LockdownByLineEnable;        // 0x950
  volatile unsigned UnlockAllLinesByWay;         // 0x954
  volatile unsigned Reserved91[(0xC00-0x958)/4]; // 0x958-0x9FC

  // Register r12
  volatile unsigned AddressFilteringStart;       // 0xC00
  volatile unsigned AddressFilteringEnd;         // 0xC04
  volatile unsigned Reserved12[(0xF40-0xC08)/4]; // 0xC08-0xF3C

  // Register r15
  volatile unsigned DebugCtrl;                   // 0xF40
};


struct pl310_l2cc PL310_L2CC_1;


void init_l2cc(void)
{
  pl310_l2cc_enable(0x00000000);

  // Versatile Express settings: 8-way, 64kb way-size, event monitor bus enable,
  // shared override, tag/data RAM latency = 0
  set_pl310_l2cc_AuxCtrl(0x00060000);
  set_pl310_l2cc_TagRAMLatencyCtrl(0x000);
  set_pl310_l2cc_DataRAMLatencyCtrl(0x000);

  set_pl310_l2cc_IntrClr(0x000001ff);

  set_pl310_l2cc_IntrMask(0x000001ff);

  set_pl310_l2cc_InvalByWay(0x000000ff);
  while(get_pl310_l2cc_InvalByWay())
  {};

  pl310_l2cc_enable(0x00000001);
}

// Accessor functions

// Register r0
unsigned get_pl310_l2cc_CacheID(void)
{
  return PL310_L2CC_1.CacheID;
}

unsigned get_pl310_l2cc_CacheType(void)
{
  return PL310_L2CC_1.CacheType;
}

// Register r1
void pl310_l2cc_enable(unsigned set)
{
    PL310_L2CC_1.Ctrl = set;
}

unsigned get_pl310_l2cc_AuxCtrl(void)
{
  return PL310_L2CC_1.AuxCtrl;
}

void set_pl310_l2cc_AuxCtrl(unsigned data)
{
  PL310_L2CC_1.AuxCtrl = data;
}

unsigned get_pl310_l2cc_TagRAMLatencyCtrl(void)
{
  return PL310_L2CC_1.TagRAMLatencyCtrl;
}

void set_pl310_l2cc_TagRAMLatencyCtrl(unsigned data)
{
  PL310_L2CC_1.TagRAMLatencyCtrl = data;
}

unsigned get_pl310_l2cc_DataRAMLatencyCtrl(void)
{
  return PL310_L2CC_1.DataRAMLatencyCtrl;
}

void set_pl310_l2cc_DataRAMLatencyCtrl(unsigned data)
{
  PL310_L2CC_1.DataRAMLatencyCtrl = data;
}

// Register r2
unsigned get_pl310_l2cc_EvtCtrCtrl(void)
{
  return PL310_L2CC_1.EvtCtrCtrl;
}

void set_pl310_l2cc_EvtCtrCtrl(unsigned data)
{
  PL310_L2CC_1.EvtCtrCtrl = data;
}

unsigned get_pl310_l2cc_EvtCtr1Cnfg(void)
{
  return PL310_L2CC_1.EvtCtr1Cnfg;
}

void set_pl310_l2cc_EvtCtr1Cnfg(unsigned data)
{
  PL310_L2CC_1.EvtCtr1Cnfg = data;
}

unsigned get_pl310_l2cc_EvtCtr0Cnfg(void)
{
  return PL310_L2CC_1.EvtCtr0Cnfg;
}

void set_pl310_l2cc_EvtCtr0Cnfg(unsigned data)
{
  PL310_L2CC_1.EvtCtr0Cnfg = data;
}

unsigned get_pl310_l2cc_EvtCtr1Val(void)
{
  return PL310_L2CC_1.EvtCtr1Val;
}

void set_pl310_l2cc_EvtCtr1Val(unsigned data)
{
  PL310_L2CC_1.EvtCtr1Val = data;
}

unsigned get_pl310_l2cc_EvtCtr0Val(void)
{
  return PL310_L2CC_1.EvtCtr0Val;
}

void set_pl310_l2cc_EvtCtr0Val(unsigned data)
{
  PL310_L2CC_1.EvtCtr0Val = data;
}

unsigned get_pl310_l2cc_IntrMask(void)
{
  return PL310_L2CC_1.IntrMask;
}

void set_pl310_l2cc_IntrMask(unsigned data)
{
  PL310_L2CC_1.IntrMask = data;
}

unsigned get_pl310_l2cc_MaskIntrStatus(void)
{
  return PL310_L2CC_1.MaskIntrStatus;
}

unsigned get_pl310_l2cc_RawIntrStatus(void)
{
  return PL310_L2CC_1.RawIntrStatus;
}

void set_pl310_l2cc_IntrClr(unsigned data)
{
  PL310_L2CC_1.IntrClr = data;
}

// Register r7
unsigned get_pl310_l2cc_CacheSync(void)
{
  return PL310_L2CC_1.CacheSync;
}

void set_pl310_l2cc_CacheSync(unsigned data)
{
  PL310_L2CC_1.CacheSync = data;
}

unsigned get_pl310_l2cc_InvalLineByPA(void)
{
  return PL310_L2CC_1.InvalLineByPA;
}

void set_pl310_l2cc_InvalLineByPA(unsigned data)
{
  PL310_L2CC_1.InvalLineByPA = data;
}

unsigned get_pl310_l2cc_InvalByWay(void)
{
  return PL310_L2CC_1.InvalByWay;
}

void set_pl310_l2cc_InvalByWay(unsigned data)
{
  PL310_L2CC_1.InvalByWay = data;
}

unsigned get_pl310_l2cc_CleanLineByPA(void)
{
  return PL310_L2CC_1.CleanLineByPA;
}

void set_pl310_l2cc_CleanLineByPA(unsigned data)
{
  PL310_L2CC_1.CleanLineByPA = data;
}

unsigned get_pl310_l2cc_CleanLineByIndexWay(void)
{
  return PL310_L2CC_1.CleanLineByIndexWay;
}

void set_pl310_l2cc_CleanLineByIndexWay(unsigned data)
{
  PL310_L2CC_1.CleanLineByIndexWay = data;
}

unsigned get_pl310_l2cc_CleanByWay(void)
{
  return PL310_L2CC_1.CleanByWay;
}

void set_pl310_l2cc_CleanByWay(unsigned data)
{
  PL310_L2CC_1.CleanByWay = data;
}

unsigned get_pl310_l2cc_CleanInvalByPA(void)
{
  return PL310_L2CC_1.CleanInvalByPA;
}

void set_pl310_l2cc_CleanInvalByPA(unsigned data)
{
  PL310_L2CC_1.CleanInvalByPA = data;
}

unsigned get_pl310_l2cc_CleanInvalByIndexWay(void)
{
  return PL310_L2CC_1.CleanInvalByIndexWay;
}

void set_pl310_l2cc_CleanInvalByIndexWay(unsigned data)
{
  PL310_L2CC_1.CleanInvalByIndexWay = data;
}

unsigned get_pl310_l2cc_CleanInvalByWay(void)
{
  return PL310_L2CC_1.CleanInvalByWay;
}

void set_pl310_l2cc_CleanInvalByWay(unsigned data)
{
  PL310_L2CC_1.CleanInvalByWay = data;
}

// Register r9
unsigned get_pl310_l2cc_DataLockdownByWay(unsigned master_id)
{
  switch(master_id) {
    case 0:
      return PL310_L2CC_1.DataLockdown0ByWay;
    case 1:
      return PL310_L2CC_1.DataLockdown1ByWay;
    case 2:
      return PL310_L2CC_1.DataLockdown2ByWay;
    case 3:
      return PL310_L2CC_1.DataLockdown3ByWay;
    case 4:
      return PL310_L2CC_1.DataLockdown4ByWay;
    case 5:
      return PL310_L2CC_1.DataLockdown5ByWay;
    case 6:
      return PL310_L2CC_1.DataLockdown6ByWay;
    case 7:
      return PL310_L2CC_1.DataLockdown7ByWay;
    default:
      return 0;
  }
}

void set_pl310_l2cc_DataLockdownByWay(unsigned master_id, unsigned data)
{
  switch(master_id) {
    case 0:
      PL310_L2CC_1.DataLockdown0ByWay = data;
    case 1:
      PL310_L2CC_1.DataLockdown1ByWay = data;
    case 2:
      PL310_L2CC_1.DataLockdown2ByWay = data;
    case 3:
      PL310_L2CC_1.DataLockdown3ByWay = data;
    case 4:
      PL310_L2CC_1.DataLockdown4ByWay = data;
    case 5:
      PL310_L2CC_1.DataLockdown5ByWay = data;
    case 6:
      PL310_L2CC_1.DataLockdown6ByWay = data;
    case 7:
      PL310_L2CC_1.DataLockdown7ByWay = data;
  }
}

unsigned get_pl310_l2cc_InstrLockdownByWay(unsigned master_id)
{
  switch(master_id) {
    case 0:
      return PL310_L2CC_1.InstrLockdown0ByWay;
    case 1:
      return PL310_L2CC_1.InstrLockdown1ByWay;
    case 2:
      return PL310_L2CC_1.InstrLockdown2ByWay;
    case 3:
      return PL310_L2CC_1.InstrLockdown3ByWay;
    case 4:
      return PL310_L2CC_1.InstrLockdown4ByWay;
    case 5:
      return PL310_L2CC_1.InstrLockdown5ByWay;
    case 6:
      return PL310_L2CC_1.InstrLockdown6ByWay;
    case 7:
      return PL310_L2CC_1.InstrLockdown7ByWay;
    default:
      return 0;
  }
}

void set_pl310_l2cc_InstrLockdownByWay(unsigned master_id, unsigned data)
{
  switch(master_id) {
    case 0:
      PL310_L2CC_1.InstrLockdown0ByWay = data;
    case 1:
      PL310_L2CC_1.InstrLockdown1ByWay = data;
    case 2:
      PL310_L2CC_1.InstrLockdown2ByWay = data;
    case 3:
      PL310_L2CC_1.InstrLockdown3ByWay = data;
    case 4:
      PL310_L2CC_1.InstrLockdown4ByWay = data;
    case 5:
      PL310_L2CC_1.InstrLockdown5ByWay = data;
    case 6:
      PL310_L2CC_1.InstrLockdown6ByWay = data;
    case 7:
      PL310_L2CC_1.InstrLockdown7ByWay = data;
  }
}

unsigned get_pl310_l2cc_LockdownByLineEnable(void)
{
  return PL310_L2CC_1.LockdownByLineEnable;
}

void set_pl310_l2cc_LockdownByLineEnable(unsigned data)
{
  PL310_L2CC_1.LockdownByLineEnable = data;
}

unsigned get_pl310_l2cc_UnlockAllLinesByWay(void)
{
  return PL310_L2CC_1.UnlockAllLinesByWay;
}

void set_pl310_l2cc_UnlockAllLinesByWay(unsigned data)
{
  PL310_L2CC_1.UnlockAllLinesByWay = data;
}

// Register r12
unsigned get_pl310_l2cc_AddrFilteringStart(void)
{
  return PL310_L2CC_1.AddressFilteringStart;
}

void set_pl310_l2cc_AddrFilteringStart(unsigned data)
{
  PL310_L2CC_1.AddressFilteringStart = data;
}

unsigned get_pl310_l2cc_AddrFilteringEnd(void)
{
  return PL310_L2CC_1.AddressFilteringEnd;
}

void set_pl310_l2cc_AddrFilteringEnd(unsigned data)
{
  PL310_L2CC_1.AddressFilteringEnd = data;
}

// Register r15
unsigned get_pl310_l2cc_DebugCtrl(void)
{
  return PL310_L2CC_1.DebugCtrl;
}

void set_pl310_l2cc_DebugCtrl(unsigned data)
{
  PL310_L2CC_1.DebugCtrl = data;
}
