#ifndef INC_GTS1
#define INC_GTS1

#include "core.h"
#include "wpcsam.h"
#include "sim.h"

/*-------------------------
/ Machine driver constants
/--------------------------*/
#define GTS1_ROMEND	ROM_END

/*-- Common Inports for GTS1 Games --*/
#define GTS1_COMPORTS \
  PORT_START /* 0 */ \
    /* switch column 0 */ \
    COREPORT_BIT(     0x0001, "Key 1", KEYCODE_1) \
    COREPORT_BIT(     0x0002, "Key 2", KEYCODE_2) \
    COREPORT_BIT(     0x0004, "Key 3", KEYCODE_3) \
    COREPORT_BIT(     0x0008, "Key 4", KEYCODE_4) \
    COREPORT_BIT(     0x0010, "Key 5", KEYCODE_5) \
    COREPORT_BIT(     0x0020, "Key 6", KEYCODE_6) \
    COREPORT_BIT(     0x0040, "Key 7", KEYCODE_7) \
    COREPORT_BIT(     0x0080, "Key 8", KEYCODE_8) \
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
    COREPORT_DIPNAME( 0x0008, 0x0000, "S20") \
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
      COREPORT_DIPSET(0x0080, "1" )

/*-- Standard input ports --*/
#define GTS1_INPUT_PORTS_START(name,balls) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls) \
    GTS1_COMPORTS

#define GTS1_INPUT_PORTS_END INPUT_PORTS_END

#define GTS1_COMINPORT       CORE_COREINPORT

#define GTS1_SOLSMOOTH       1 /* Smooth the Solenoids over this number of VBLANKS */
#define GTS1_LAMPSMOOTH      1 /* Smooth the lamps over this number of VBLANKS */
#define GTS1_DISPLAYSMOOTH   1 /* Smooth the display over this number of VBLANKS */

/*-- To access C-side multiplexed solenoid/flasher --*/
#define GTS1_CSOL(x) ((x)+24)

/*-- GTS1 switch numbers --*/

/*-- Memory regions --*/
#define GTS1_MEMREG_CPU	REGION_CPU1

/* CPUs */
#define GTS1_CPU	0

/*-- GTS1 CPU regions and ROM --*/
#define GTS1_1_ROMSTART(name, n1, chk1) \
   ROM_START(name) \
     NORMALREGION(0x10000, GTS1_MEMREG_CPU) \
       ROM_LOAD(n1, 0x0000, 0x0400, chk1) \
         ROM_RELOAD(0x0400, 0x0400) \
         ROM_RELOAD(0x0800, 0x0400) \
         ROM_RELOAD(0x0c00, 0x0400)

/*-- These are only here so the game structure can be in the game file --*/
extern MACHINE_DRIVER_EXTERN(GTS1);

#define gl_mGTS1		GTS1

#endif /* INC_GTS1 */
