#ifndef INC_GTS80
#define INC_GTS80

#include "core.h"
#include "wpcsam.h"
#include "sim.h"

/*-------------------------
/ Machine driver constants
/--------------------------*/
#define GTS80_CPUNO	0

#define GTS80_ROMEND	ROM_END

/*-- Common Inports for GTS80Games --*/
#define GTS80_COMPORTS \
  PORT_START /* 0 */ \
    /* switch column 8 */ \
    COREPORT_BIT(     0x0001, "Test",             KEYCODE_8)  \
    COREPORT_BITDEF(  0x0002, IPT_COIN1,          IP_KEY_DEFAULT)  \
    COREPORT_BITDEF(  0x0004, IPT_COIN2,          IP_KEY_DEFAULT)  \
    COREPORT_BITDEF(  0x0008, IPT_COIN3,          KEYCODE_3)  \
    COREPORT_BITDEF(  0x0010, IPT_START1,         IP_KEY_DEFAULT)  \
    COREPORT_BITDEF(  0x0020, IPT_TILT,           KEYCODE_INSERT)  \
    /* These are put in switch column 0 */ \
    COREPORT_BIT(     0x0100, "Slam Tilt",        KEYCODE_HOME)  \
  PORT_START /* 1 */ \
    COREPORT_DIPNAME( 0x0001, 0x0000, "S1") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0001, "1" ) \
    COREPORT_DIPNAME( 0x0002, 0x0002, "S2") \
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
    COREPORT_DIPNAME( 0x0100, 0x0100, "S9") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0100, "1" ) \
    COREPORT_DIPNAME( 0x0200, 0x0200, "S10") \
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
    COREPORT_DIPNAME( 0x8000, 0x8000, "S16") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x8000, "1" ) \
  PORT_START /* 2 */ \
    COREPORT_DIPNAME( 0x0001, 0x0001, "S17") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0001, "1" ) \
    COREPORT_DIPNAME( 0x0002, 0x0002, "S18") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0002, "1" ) \
    COREPORT_DIPNAME( 0x0004, 0x0000, "S19") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0004, "1" ) \
    COREPORT_DIPNAME( 0x0008, 0x0008, "S20") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0008, "1" ) \
    COREPORT_DIPNAME( 0x0010, 0x0010, "S21") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0010, "1" ) \
    COREPORT_DIPNAME( 0x0020, 0x0000, "S22") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0020, "1" ) \
    COREPORT_DIPNAME( 0x0040, 0x0040, "S23") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0040, "1" ) \
    COREPORT_DIPNAME( 0x0080, 0x0080, "S24") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0080, "1" ) \
    COREPORT_DIPNAME( 0x0100, 0x0100, "S25") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0100, "1" ) \
    COREPORT_DIPNAME( 0x0200, 0x0200, "S26") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0200, "1" ) \
    COREPORT_DIPNAME( 0x0400, 0x0400, "S27") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0400, "1" ) \
    COREPORT_DIPNAME( 0x0800, 0x0800, "S28") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0800, "1" ) \
    COREPORT_DIPNAME( 0x1000, 0x1000, "S29") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x1000, "1" ) \
    COREPORT_DIPNAME( 0x2000, 0x2000, "S30") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x2000, "1" ) \
    COREPORT_DIPNAME( 0x4000, 0x4000, "S31") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x4000, "1" ) \
    COREPORT_DIPNAME( 0x8000, 0x8000, "S32") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x8000, "1" )

// 07:Test, 17:Coin1, etc.

/*-- Standard input ports --*/
#define GTS80_INPUT_PORTS_START(name,balls) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls) \
    GTS80_COMPORTS

#define GTS80_INPUT_PORTS_END INPUT_PORTS_END

#define GTS80_COMINPORT       CORE_COREINPORT

#define GTS80_SOLSMOOTH       4 /* Smooth the Solenoids over this numer of VBLANKS */
#define GTS80_LAMPSMOOTH      2 /* Smooth the lamps over this number of VBLANKS */
#define GTS80_DISPLAYSMOOTH   2 /* Smooth the display over this number of VBLANKS */

/*-- GTS80 switches are numbers 0-7/0-7 but row and column reversed--*/
#define GTS80_SWNO(x) (x)

/*-- To access C-side multiplexed solenoid/flasher --*/
#define GTS80_CSOL(x) ((x)+24)

/*-- GTS80 switch numbers --*/
#define GTS80_SWSLAMTILT	  -1

/*-- Memory regions --*/
#define GTS80_MEMREG_CPU		REGION_CPU1
#define GTS80_MEMREG_SCPU1	REGION_CPU2
#define GTS80_MEMREG_SCPU2	REGION_CPU3

/*-- GTS80/GTS80A Main CPU regions and ROM, 2 game PROM version --*/
#define GTS80_2_ROMSTART(name, n1, chk1, n2, chk2, n3, chk3, n4, chk4) \
   ROM_START(name) \
     NORMALREGION(0x10000, GTS80_MEMREG_CPU) \
       ROM_LOAD(n1, 0x1000, 0x0200, chk1) \
	   ROM_RELOAD(  0x1400, 0x0200)       \
       ROM_LOAD(n2, 0x1200, 0x0200, chk2) \
	   ROM_RELOAD(  0x1600, 0x0200)       \
       ROM_LOAD(n3, 0x2000, 0x1000, chk3) \
       ROM_LOAD(n4, 0x3000, 0x1000, chk4)

/*-- GTS80/GTS80A Main CPU regions and ROM, 2 game PROM version --*/
#define GTS80_1_ROMSTART(name, n1, chk1, n2, chk2, n3, chk3) \
   ROM_START(name) \
     NORMALREGION(0x10000, GTS80_MEMREG_CPU) \
       ROM_LOAD(n1, 0x1000, 0x0800, chk1) \
       ROM_LOAD(n2, 0x2000, 0x1000, chk2) \
       ROM_LOAD(n3, 0x3000, 0x1000, chk3)

/*-- GTS80B Main CPU regions and ROM, 8K single game PROM --*/
#define GTS80B_8K_ROMSTART(name, n1, chk1) \
   ROM_START(name) \
     NORMALREGION(0x10000, GTS80_MEMREG_CPU) \
       ROM_LOAD(n1, 0x2000, 0x2000, chk1)

/*-- GTS80B Main CPU regions and ROM, 8K & 2K game PROM --*/
#define GTS80B_2K_ROMSTART(name, n1, chk1, n2, chk2) \
   ROM_START(name) \
     NORMALREGION(0x10000, GTS80_MEMREG_CPU) \
       ROM_LOAD(n1, 0x1000, 0x0800, chk1) \
       ROM_LOAD(n2, 0x2000, 0x2000, chk2)

/*-- GTS80B Main CPU regions and ROM, 8K & 4K game PROM --*/
/*-- the second half of PROM2 is later copied to the right location */
#define GTS80B_4K_ROMSTART(name, n1, chk1, n2, chk2) \
   ROM_START(name) \
     NORMALREGION(0x10000, GTS80_MEMREG_CPU) \
       ROM_LOAD(n1, 0x1000, 0x1000, chk1) \
       ROM_LOAD(n2, 0x2000, 0x2000, chk2)

/*-- TheGTS80 are only here so the game structure can be in the game file --*/
extern struct MachineDriver machine_driver_GTS80S;
extern struct MachineDriver machine_driver_GTS80SS;
extern struct MachineDriver machine_driver_GTS80B;
extern struct MachineDriver machine_driver_GTS80BS1;
extern struct MachineDriver machine_driver_GTS80BS2;
extern struct MachineDriver machine_driver_GTS80BS3;

#define gl_mGTS80S		GTS80S
#define gl_mGTS80SS		GTS80SS
#define gl_mGTS80B		GTS80B
#define gl_mGTS80BS1	GTS80BS1
#define gl_mGTS80BS2	GTS80BS2
#define gl_mGTS80BS3	GTS80BS3

#endif /* INC_GTS80 */
