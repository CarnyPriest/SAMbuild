#ifndef INC_BY35
#define INC_BY35

/*-- Common Inports for BY35 Games --*/
#define BY35_COMPORTS \
  PORT_START /* 0 */ \
    /* Switch Column 1 (Switches #6 & #7) */ \
    COREPORT_BITDEF(  0x0020, IPT_START1,         IP_KEY_DEFAULT)  \
    COREPORT_BIT(     0x0040, "Ball Tilt",        KEYCODE_2)  \
    /* Switch Column 2 (Switches #9 - #16) */ \
    /* For Stern MPU-200 (Switches #1-3, and #8) */ \
    COREPORT_BITDEF(  0x0100, IPT_COIN1,          IP_KEY_DEFAULT) \
    COREPORT_BITDEF(  0x0200, IPT_COIN2,          IP_KEY_DEFAULT) \
    COREPORT_BITDEF(  0x0400, IPT_COIN3,          KEYCODE_3) \
    COREPORT_BIT(     0x8000, "Slam Tilt",        KEYCODE_HOME)  \
    /* These are put in switch column 0 */ \
    COREPORT_BIT(     0x0001, "Self Test",        KEYCODE_7) \
    COREPORT_BIT(     0x0002, "CPU Diagnostic",   KEYCODE_9) \
    COREPORT_BIT(     0x0004, "Sound Diagnostic", KEYCODE_0) \
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

#define BY35PROTO_COMPORTS \
  PORT_START /* 0 */ \
    /* Switch Column 3 */ \
    COREPORT_BIT(     0x0100, "Ball Tilt",        KEYCODE_INSERT) \
    /* Switch Column 4 */ \
    COREPORT_BITDEF(  0x0001, IPT_START1,         IP_KEY_DEFAULT)  \
    COREPORT_BIT(     0x0002, "Self Test",        KEYCODE_7) \
    COREPORT_BITDEF(  0x0004, IPT_COIN1,          IP_KEY_DEFAULT) \
    COREPORT_BITDEF(  0x0008, IPT_COIN2,          IP_KEY_DEFAULT) \
    COREPORT_BIT(     0x0010, "Slam Tilt",        KEYCODE_HOME)  \
  PORT_START /* 1 */ \
    COREPORT_DIPNAME( 0x0003, 0x0000, "Chute #1 Coins") \
      COREPORT_DIPSET(0x0000, "1" ) \
      COREPORT_DIPSET(0x0002, "2" ) \
      COREPORT_DIPSET(0x0001, "3" ) \
      COREPORT_DIPSET(0x0003, "4" ) \
    COREPORT_DIPNAME( 0x000c, 0x0000, "Chute #1 Credits") \
      COREPORT_DIPSET(0x0000, "1" ) \
      COREPORT_DIPSET(0x0008, "2" ) \
      COREPORT_DIPSET(0x0004, "3" ) \
      COREPORT_DIPSET(0x000c, "4" ) \
    COREPORT_DIPNAME( 0x00f0, 0x00f0, "Replay #2/#1+") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0080, "4k" ) \
      COREPORT_DIPSET(0x0040, "8k" ) \
      COREPORT_DIPSET(0x00c0, "12k" ) \
      COREPORT_DIPSET(0x0020, "16k" ) \
      COREPORT_DIPSET(0x00a0, "20k" ) \
      COREPORT_DIPSET(0x0060, "24k" ) \
      COREPORT_DIPSET(0x00e0, "28k" ) \
      COREPORT_DIPSET(0x0010, "32k" ) \
      COREPORT_DIPSET(0x0090, "36k" ) \
      COREPORT_DIPSET(0x0050, "40k" ) \
      COREPORT_DIPSET(0x00d0, "44k" ) \
      COREPORT_DIPSET(0x0030, "48k" ) \
      COREPORT_DIPSET(0x00b0, "52k" ) \
      COREPORT_DIPSET(0x0070, "56k" ) \
      COREPORT_DIPSET(0x00f0, "60k" ) \
    COREPORT_DIPNAME( 0x0300, 0x0000, "Chute #2 Coins") \
      COREPORT_DIPSET(0x0000, "1" ) \
      COREPORT_DIPSET(0x0200, "2" ) \
      COREPORT_DIPSET(0x0100, "3" ) \
      COREPORT_DIPSET(0x0300, "4" ) \
    COREPORT_DIPNAME( 0x0c00, 0x0000, "Chute #2 Credits") \
      COREPORT_DIPSET(0x0000, "1" ) \
      COREPORT_DIPSET(0x0800, "2" ) \
      COREPORT_DIPSET(0x0400, "3" ) \
      COREPORT_DIPSET(0x0c00, "4" ) \
    COREPORT_DIPNAME( 0xf000, 0xf000, "Replay #3/#2+") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x8000, "4k" ) \
      COREPORT_DIPSET(0x4000, "8k" ) \
      COREPORT_DIPSET(0xc000, "12k" ) \
      COREPORT_DIPSET(0x2000, "16k" ) \
      COREPORT_DIPSET(0xa000, "20k" ) \
      COREPORT_DIPSET(0x6000, "24k" ) \
      COREPORT_DIPSET(0xe000, "28k" ) \
      COREPORT_DIPSET(0x1000, "32k" ) \
      COREPORT_DIPSET(0x9000, "36k" ) \
      COREPORT_DIPSET(0x5000, "40k" ) \
      COREPORT_DIPSET(0xd000, "44k" ) \
      COREPORT_DIPSET(0x3000, "48k" ) \
      COREPORT_DIPSET(0xb000, "52k" ) \
      COREPORT_DIPSET(0x7000, "56k" ) \
      COREPORT_DIPSET(0xf000, "60k" ) \
  PORT_START /* 2 */ \
    COREPORT_DIPNAME( 0x0001, 0x0000, "Blink display") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0001, "1" ) \
    COREPORT_DIPNAME( 0x0002, 0x0002, "Melody") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0002, "1" ) \
    COREPORT_DIPNAME( 0x0004, 0x0004, "Incentive") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0004, "1" ) \
    COREPORT_DIPNAME( 0x0008, 0x0008, "Score award") \
      COREPORT_DIPSET(0x0000, "Extra Ball" ) \
      COREPORT_DIPSET(0x0008, "Credit" ) \
    COREPORT_DIPNAME( 0x0010, 0x0010, "Special award") \
      COREPORT_DIPSET(0x0000, "Extra Ball" ) \
      COREPORT_DIPSET(0x0010, "Credit" ) \
    COREPORT_DIPNAME( 0x00e0, 0x0020, "Balls per game") \
      COREPORT_DIPSET(0x0000, "1" ) \
      COREPORT_DIPSET(0x0080, "2" ) \
      COREPORT_DIPSET(0x0040, "3" ) \
      COREPORT_DIPSET(0x00c0, "4" ) \
      COREPORT_DIPSET(0x0020, "5" ) \
      COREPORT_DIPSET(0x00a0, "6" ) \
      COREPORT_DIPSET(0x0060, "7" ) \
      COREPORT_DIPSET(0x00e0, "8" ) \
    COREPORT_DIPNAME( 0x0100, 0x0100, "Match feature") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0100, "1" ) \
    COREPORT_DIPNAME( 0x0200, 0x0200, "Replay #1+128k") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0200, "1" ) \
    COREPORT_DIPNAME( 0x0400, 0x0000, "Replay #1+64k") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0400, "1" ) \
    COREPORT_DIPNAME( 0x0800, 0x0000, "Replay #1+32k") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0800, "1" ) \
    COREPORT_DIPNAME( 0x1000, 0x0000, "Replay #1+16k") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x1000, "1" ) \
    COREPORT_DIPNAME( 0x2000, 0x0000, "Replay #1+8k") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x2000, "1" ) \
    COREPORT_DIPNAME( 0x4000, 0x0000, "Replay #1+4k") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x4000, "1" ) \
    COREPORT_DIPNAME( 0x8000, 0x8000, "Replay #1+2k") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x8000, "1" )

#define HNK_COMPORTS \
  PORT_START /* 0 */ \
    /* Switch Column 1 (Switches #2 & #3) */ \
    COREPORT_BIT(     0x0002, "Ball Tilt",        KEYCODE_INSERT)  \
    COREPORT_BITDEF(  0x0004, IPT_START1,         IP_KEY_DEFAULT)  \
    /* Switch Column 2 (Switches #9 - #10) */ \
    COREPORT_BIT(     0x0001, "Slam Tilt",        KEYCODE_HOME)  \
    COREPORT_BITDEF(  0x0080, IPT_COIN1,          IP_KEY_DEFAULT)  \
    /* These are put in switch column 0 */ \
    COREPORT_BIT(     0x0100, "Self Test",        KEYCODE_8) \
    COREPORT_BIT(     0x0200, "CPU Diagnostic",   KEYCODE_9) \
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
    COREPORT_DIPNAME( 0x0010, 0x0010, "S5") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0010, "1" ) \
    COREPORT_DIPNAME( 0x0020, 0x0000, "S6") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0020, "1" ) \
    COREPORT_DIPNAME( 0x0040, 0x0000, "S7") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0040, "1" ) \
    COREPORT_DIPNAME( 0x0080, 0x0080, "S8") \
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
      COREPORT_DIPSET(0x0080, "1" )

/*-- Standard input ports --*/
#define BY35_INPUT_PORTS_START(name,balls) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls) \
    BY35_COMPORTS

#define BY35PROTO_INPUT_PORTS_START(name,balls) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls) \
    BY35PROTO_COMPORTS

#define HNK_INPUT_PORTS_START(name,balls) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls) \
    HNK_COMPORTS

#define BY35_INPUT_PORTS_END INPUT_PORTS_END
#define HNK_INPUT_PORTS_END INPUT_PORTS_END

#define BY35_COMINPORT       CORE_COREINPORT

/*-- By35 switch numbers --*/
#define BY35_SWSELFTEST   -7
#define BY35_SWCPUDIAG    -6
#define BY35_SWSOUNDDIAG  -5

/*-------------------------
/ Machine driver constants
/--------------------------*/
/*-- Memory regions --*/
#define BY35_CPUREGION	REGION_CPU1
#define HNK_MEMREG_SCPU	REGION_CPU2

/*-- Main CPU regions and ROM --*/
#define ASTRO_ROMSTART88(name,n1,chk1,n2,chk2) \
  ROM_START(name) \
    NORMALREGION(0x10000, BY35_CPUREGION) \
      ROM_LOAD( n1, 0x1000, 0x0800, chk1) \
        ROM_RELOAD( 0x5000, 0x0800) \
      ROM_LOAD( n2, 0x1800, 0x0800, chk2) \
        ROM_RELOAD( 0x5800, 0x0800) \
        ROM_RELOAD( 0xf800, 0x0800) \

#define BYPROTO_ROMSTART(name,n1,chk1,n2,chk2,n3,chk3,n4,chk4,n5,chk5,n6,chk6) \
  ROM_START(name) \
    NORMALREGION(0x10000, BY35_CPUREGION) \
      ROM_LOAD( n1, 0x1400, 0x0200, chk1) \
      ROM_LOAD( n2, 0x1600, 0x0200, chk2) \
      ROM_LOAD( n3, 0x1800, 0x0200, chk3) \
      ROM_LOAD( n4, 0x1a00, 0x0200, chk4) \
      ROM_LOAD( n5, 0x1c00, 0x0200, chk5) \
      ROM_LOAD( n6, 0x1e00, 0x0200, chk6) \
        ROM_RELOAD( 0xfe00, 0x0200)

#define BY17_ROMSTART228(name,n1,chk1,n2,chk2,n3,chk3) \
  ROM_START(name) \
    NORMALREGION(0x10000, BY35_CPUREGION) \
      ROM_LOAD( n1, 0x1400, 0x0200, chk1) \
      ROM_LOAD( n2, 0x1600, 0x0200, chk2) \
      ROM_LOAD( n3, 0x1800, 0x0800, chk3) \
        ROM_RELOAD( 0xf800, 0x0800)

#define BY17_ROMSTART8x8(name,n1,chk1,n3,chk3)\
  ROM_START(name) \
    NORMALREGION(0x10000, BY35_CPUREGION) \
      ROM_LOAD( n1, 0x1000, 0x0800, chk1) \
      ROM_LOAD( n3, 0x1800, 0x0800, chk3) \
        ROM_RELOAD( 0xf800, 0x0800)

#define BY17_ROMSTARTx88(name,n2,chk2,n3,chk3)\
  ROM_START(name) \
    NORMALREGION(0x10000, BY35_CPUREGION) \
      ROM_LOAD( n2, 0x1000, 0x0800, chk2) \
      ROM_LOAD( n3, 0x1800, 0x0800, chk3) \
        ROM_RELOAD( 0xf800, 0x0800)

#define BY35_ROMSTART888(name,n1,chk1,n2,chk2,n3,chk3)\
  ROM_START(name) \
    NORMALREGION(0x10000, BY35_CPUREGION) \
      ROM_LOAD( n1, 0x1000, 0x0800, chk1) \
      ROM_LOAD( n2, 0x5000, 0x0800, chk2) \
      ROM_LOAD( n3, 0x5800, 0x0800, chk3) \
        ROM_RELOAD( 0xf800, 0x0800)

#define BY35_ROMSTART880(name,n1,chk1,n2,chk2,n3,chk3)\
  ROM_START(name) \
    NORMALREGION(0x10000, BY35_CPUREGION) \
      ROM_LOAD( n1, 0x1000, 0x0800, chk1) \
      ROM_LOAD( n2, 0x5000, 0x0800, chk2) \
      ROM_LOAD( n3, 0x1800, 0x0800, chk3) \
      ROM_CONTINUE( 0x5800, 0x0800 ) \
        ROM_RELOAD( 0xf000, 0x1000 )

#define BY35_ROMSTARTx00(name,n2,chk2,n3,chk3)\
  ROM_START(name) \
    NORMALREGION(0x10000, BY35_CPUREGION) \
      ROM_LOAD( n2, 0x1000, 0x0800, chk2) \
      ROM_CONTINUE( 0x5000, 0x0800) /* ?? */ \
      ROM_LOAD( n3, 0x1800, 0x0800, chk3 ) \
      ROM_CONTINUE( 0x5800, 0x0800) \
        ROM_RELOAD( 0xf000, 0x1000)

#define ST200_ROMSTART8888(name,n1,chk1,n2,chk2,n3,chk3,n4,chk4)\
  ROM_START(name) \
    NORMALREGION(0x10000, BY35_CPUREGION) \
      ROM_LOAD( n1, 0x1000, 0x0800, chk1) \
      ROM_LOAD( n2, 0x1800, 0x0800, chk2) \
      ROM_LOAD( n3, 0x5000, 0x0800, chk3) \
      ROM_LOAD( n4, 0x5800, 0x0800, chk4) \
        ROM_RELOAD( 0xf800, 0x0800)

#define HNK_ROMSTART(name,n1,chk1,n2,chk2) \
  ROM_START(name) \
    NORMALREGION(0x10000, BY35_CPUREGION) \
      ROM_LOAD(  n1, 0x1000, 0x0800, chk1) \
      ROM_LOAD(  n2, 0x1800, 0x0800, chk2) \
	    ROM_RELOAD(  0xf800, 0x0800)

#define BY35_ROMEND ROM_END
#define BY17_ROMEND ROM_END
#define ST200_ROMEND ROM_END
#define HNK_ROMEND ROM_END

extern MACHINE_DRIVER_EXTERN(byProto);
extern MACHINE_DRIVER_EXTERN(by35);
extern MACHINE_DRIVER_EXTERN(by35_32S);
extern MACHINE_DRIVER_EXTERN(by35_51S);
extern MACHINE_DRIVER_EXTERN(by35_56S);
extern MACHINE_DRIVER_EXTERN(by35_61S);
extern MACHINE_DRIVER_EXTERN(by35_45S);
extern MACHINE_DRIVER_EXTERN(st200);
extern MACHINE_DRIVER_EXTERN(hnk);

#define by35_mBYPROTO   byProto
#define by35_mBY17      by35
#define by35_mBY35_32   by35
#define by35_mBY35_50   by35
#define by35_mBY35_51   by35
#define by35_mBY35_61   by35
#define by35_mBY35_61B  by35
#define by35_mBY35_81   by35
#define by35_mBY35_54   by35
#define by35_mBY35_56   by35 // XENON
#define by35_mBY17S     by35
#define by35_mBY35_32S  by35_32S
#define by35_mBY35_50S  by35_32S
#define by35_mBY35_51S  by35_51S
#define by35_mBY35_61S  by35_61S
#define by35_mBY35_61BS by35_61S
#define by35_mBY35_81S  by35_61S
#define by35_mBY35_56S  by35_56S // XENON
#define by35_mBY35_45S  by35_45S
#define by35_mBowling   by35
#define by35_mAstro     st200
#define by35_mHNK       hnk
#define by35_mST100     by35
#define by35_mST200     st200

/* gameSpecific1 values */
#define BY35GD_NOSOUNDE 0x01 // doesn't use SOUNDE
#define BY35GD_PHASE    0x02 // phased lamps
#define BY35GD_FAKEZERO 0x04 // fake some zero digits for Nuova Bell games

#endif /* INC_BY35 */

