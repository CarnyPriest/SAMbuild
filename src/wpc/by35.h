#ifndef INC_BY35
#define INC_BY35

#include "core.h"
#include "wpcsam.h"
#include "sim.h"

#define BY35_SOLSMOOTH       2 /* Smooth the Solenoids over this numer of VBLANKS */
#define BY35_LAMPSMOOTH      2 /* Smooth the lamps over this number of VBLANKS */
#define BY35_DISPLAYSMOOTH   4 /* Smooth the display over this number of VBLANKS */

/*-- Common Inports for BY35 Games --*/
#define BY35_COMPORTS \
  PORT_START /* 0 */ \
    /* Switch Column 1 (Switches #6 & #7) */ \
    COREPORT_BITDEF(  0x0001, IPT_START1,         IP_KEY_DEFAULT)  \
    COREPORT_BIT(     0x0002, "Ball Tilt",        KEYCODE_2)  \
    /* Switch Column 2 (Switches #9 - #16) */ \
	/* For Stern MPU-200 (Switches #1-3, and #8) */ \
    COREPORT_BITDEF(  0x0004, IPT_COIN1,          IP_KEY_DEFAULT) \
    COREPORT_BITDEF(  0x0008, IPT_COIN2,          IP_KEY_DEFAULT) \
    COREPORT_BITDEF(  0x0010, IPT_COIN3,          KEYCODE_3) \
    COREPORT_BIT(     0x0200, "Slam Tilt",        KEYCODE_HOME)  \
    /* These are put in switch column 0 */ \
    COREPORT_BIT(     0x0400, "Self Test",        KEYCODE_7) \
    COREPORT_BIT(     0x0800, "CPU Diagnostic",   KEYCODE_9) \
    COREPORT_BIT(     0x1000, "Sound Diagnostic", KEYCODE_0) \
  PORT_START /* 1 */ \
    COREPORT_DIPNAME( 0x0001, 0x0000, "S1") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0001, "1" ) \
    COREPORT_DIPNAME( 0x0002, 0x0000, "S2") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0002, "1" ) \
    COREPORT_DIPNAME( 0x0004, 0x0000, "S3") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0004, "1" ) \
    COREPORT_DIPNAME( 0x0008, 0x0000, "S4") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0008, "1" ) \
    COREPORT_DIPNAME( 0x0010, 0x0000, "S5") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0010, "1" ) \
    COREPORT_DIPNAME( 0x0020, 0x0000, "S6") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0020, "1" ) \
    COREPORT_DIPNAME( 0x0040, 0x0000, "S7") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0040, "1" ) \
    COREPORT_DIPNAME( 0x0080, 0x0000, "S8") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0080, "1" ) \
    COREPORT_DIPNAME( 0x0100, 0x0000, "S9") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0100, "1" ) \
    COREPORT_DIPNAME( 0x0200, 0x0000, "S10") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0200, "1" ) \
    COREPORT_DIPNAME( 0x0400, 0x0000, "S11") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0400, "1" ) \
    COREPORT_DIPNAME( 0x0800, 0x0000, "S12") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0800, "1" ) \
    COREPORT_DIPNAME( 0x1000, 0x0000, "S13") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x1000, "1" ) \
    COREPORT_DIPNAME( 0x2000, 0x0000, "S14") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x2000, "1" ) \
    COREPORT_DIPNAME( 0x4000, 0x0000, "S15") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x4000, "1" ) \
    COREPORT_DIPNAME( 0x8000, 0x0000, "S16") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x8000, "1" ) \
  PORT_START /* 2 */ \
    COREPORT_DIPNAME( 0x0001, 0x0000, "S17") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0001, "1" ) \
    COREPORT_DIPNAME( 0x0002, 0x0000, "S18") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0002, "1" ) \
    COREPORT_DIPNAME( 0x0004, 0x0000, "S19") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0004, "1" ) \
    COREPORT_DIPNAME( 0x0008, 0x0008, "S20") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0008, "1" ) \
    COREPORT_DIPNAME( 0x0010, 0x0000, "S21") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0010, "1" ) \
    COREPORT_DIPNAME( 0x0020, 0x0000, "S22") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0020, "1" ) \
    COREPORT_DIPNAME( 0x0040, 0x0000, "S23") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0040, "1" ) \
    COREPORT_DIPNAME( 0x0080, 0x0000, "S24") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0080, "1" ) \
    COREPORT_DIPNAME( 0x0100, 0x0000, "S25") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0100, "1" ) \
    COREPORT_DIPNAME( 0x0200, 0x0000, "S26") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0200, "1" ) \
    COREPORT_DIPNAME( 0x0400, 0x0000, "S27") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0400, "1" ) \
    COREPORT_DIPNAME( 0x0800, 0x0000, "S28") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0800, "1" ) \
    COREPORT_DIPNAME( 0x1000, 0x0000, "S29") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x1000, "1" ) \
    COREPORT_DIPNAME( 0x2000, 0x0000, "S30") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x2000, "1" ) \
    COREPORT_DIPNAME( 0x4000, 0x0000, "S31") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x4000, "1" ) \
    COREPORT_DIPNAME( 0x8000, 0x0000, "S32") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x8000, "1" )


/*-- Standard input ports --*/
#define BY35_INPUT_PORTS_START(name,balls) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls) \
    BY35_COMPORTS

#define BY35_INPUT_PORTS_END INPUT_PORTS_END

#define BY35_COMINPORT       CORE_COREINPORT

/*-- By35 switch numbers --*/
#define BY35_SWSELFTEST   -7
#define BY35_SWCPUDIAG    -6
#define BY35_SWSOUNDDIAG  -5

/*-------------------------
/ Machine driver constants
/--------------------------*/
#define BY35_CPUNO   0
#define BY35_SCPU1NO 1
#define BY35_SCPU2NO 2

/*-- Memory regions --*/
#define BY35_MEMREG_CPU		REGION_CPU1
#define BY35_MEMREG_S1CPU	REGION_CPU2
#define BY35_MEMREG_S2CPU	REGION_CPU3
#define BY35_MEMREG_SROM        REGION_SOUND1

/*-- Main CPU regions and ROM --*/
#define BY17_ROMSTART228(name,n1,chk1,n2,chk2,n3,chk3) \
  ROM_START(name) \
    NORMALREGION(0x10000, BY35_MEMREG_CPU) \
      ROM_LOAD( n1, 0x1400, 0x0200, chk1) \
      ROM_LOAD( n2, 0x1600, 0x0200, chk2) \
      ROM_LOAD( n3, 0x1800, 0x0800, chk3) \
      ROM_RELOAD(   0xf800, 0x0800)

#define BY17_ROMSTART8x8(name,n1,chk1,n3,chk3)\
  ROM_START(name) \
    NORMALREGION(0x10000, BY35_MEMREG_CPU) \
      ROM_LOAD( n1, 0x1000, 0x0800, chk1) \
      ROM_LOAD( n3, 0x1800, 0x0800, chk3) \
      ROM_RELOAD(   0xf800, 0x0800)

#define BY17_ROMSTARTx88(name,n2,chk2,n3,chk3)\
  ROM_START(name) \
    NORMALREGION(0x10000, BY35_MEMREG_CPU) \
      ROM_LOAD( n2, 0x1000, 0x0800, chk2) \
      ROM_LOAD( n3, 0x1800, 0x0800, chk3) \
      ROM_RELOAD(   0xf800, 0x0800)

#define BY35_ROMSTART888(name,n1,chk1,n2,chk2,n3,chk3)\
  ROM_START(name) \
    NORMALREGION(0x10000, BY35_MEMREG_CPU) \
      ROM_LOAD( n1, 0x1000, 0x0800, chk1) \
      ROM_LOAD( n2, 0x5000, 0x0800, chk2) \
      ROM_LOAD( n3, 0x5800, 0x0800, chk3) \
      ROM_RELOAD(   0xf800, 0x0800)

#define BY35_ROMSTART880(name,n1,chk1,n2,chk2,n3,chk3)\
  ROM_START(name) \
    NORMALREGION(0x10000, BY35_MEMREG_CPU) \
      ROM_LOAD( n1, 0x1000, 0x0800, chk1) \
      ROM_LOAD( n2, 0x5000, 0x0800, chk2) \
      ROM_LOAD( n3, 0x1800, 0x0800, chk3) \
      ROM_CONTINUE( 0x5800, 0x0800 ) \
      ROM_RELOAD(   0xf000, 0x1000 )


#define BY35_ROMSTARTx00(name,n2,chk2,n3,chk3)\
  ROM_START(name) \
    NORMALREGION(0x10000, BY35_MEMREG_CPU) \
      ROM_LOAD( n2, 0x1000, 0x0800, chk2) \
      ROM_CONTINUE( 0x5000, 0x0800) /* ?? */ \
      ROM_LOAD( n3, 0x1800, 0x0800, chk3 ) \
      ROM_CONTINUE( 0x5800, 0x0800) \
      ROM_RELOAD(   0xf000, 0x1000)

#define ST200_ROMSTART8888(name,n1,chk1,n2,chk2,n3,chk3,n4,chk4)\
  ROM_START(name) \
    NORMALREGION(0x10000, BY35_MEMREG_CPU) \
      ROM_LOAD( n1, 0x1000, 0x0800, chk1) \
      ROM_LOAD( n2, 0x1800, 0x0800, chk2) \
      ROM_LOAD( n3, 0x5000, 0x0800, chk3) \
      ROM_LOAD( n4, 0x5800, 0x0800, chk4) \
      ROM_RELOAD(   0xf800, 0x0800)

#define BY35_ROMEND ROM_END
#define BY17_ROMEND ROM_END

/*-- These are only here so the game structure can be in the game file --*/
extern const struct MachineDriver machine_driver_by35;
extern const struct MachineDriver machine_driver_st200;
#define by35_mBY17      by35
#define by35_mBY35_32   by35
#define by35_mBY35_50   by35
#define by35_mBY35_51   by35
#define by35_mBY35_61   by35
#define by35_mBY35_61B  by35
#define by35_mBY35_81   by35
#define by35_mBY35_54   by35
#define by35_mBY35_56   by35 // XENON
#define by35_mST100     by35
#define by35_mST200     st200

extern const struct MachineDriver machine_driver_by35_32s;
extern const struct MachineDriver machine_driver_by35_51s;
extern const struct MachineDriver machine_driver_by35_61s;
extern const struct MachineDriver machine_driver_by35_56s;
extern const struct MachineDriver machine_driver_by35_45s;
#define by35_mBY17S     by35
#define by35_mBY35_32S  by35_32s
#define by35_mBY35_50S  by35_32s
#define by35_mBY35_51S  by35_51s
#define by35_mBY35_61S  by35_61s
#define by35_mBY35_61BS by35_61s
#define by35_mBY35_81S  by35_61s
#define by35_mBY35_54S  by35
#define by35_mBY35_56S  by35_56s // XENON
#define by35_mBY35_45S  by35_45s
#endif /* INC_BY35 */

