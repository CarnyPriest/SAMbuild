#ifndef INC_S7
#define INC_S7

/*-- Common Inports for S7Games --*/
#define S7_COMPORTS \
  PORT_START /* 0 */ \
    /* Switch Column 1 */ \
    COREPORT_BITDEF(  0x0001, IPT_TILT,           KEYCODE_INSERT)  \
    COREPORT_BIT(     0x0002, "Ball Tilt",        KEYCODE_2)  \
    COREPORT_BITDEF(  0x0004, IPT_START1,         IP_KEY_DEFAULT)  \
    COREPORT_BITDEF(  0x0008, IPT_COIN1,          IP_KEY_DEFAULT) \
    COREPORT_BITDEF(  0x0010, IPT_COIN2,          IP_KEY_DEFAULT) \
    COREPORT_BITDEF(  0x0020, IPT_COIN3,          KEYCODE_3) \
    COREPORT_BIT(     0x0040, "Slam Tilt",        KEYCODE_HOME)  \
    COREPORT_BIT(     0x0080, "Hiscore Reset",    KEYCODE_4) \
    /* These are put in switch column 0 */ \
    COREPORT_BIT(     0x0100, "Advance",          KEYCODE_8) \
    COREPORT_BITTOG(  0x0200, "Auto/Manual",      KEYCODE_7) \
    COREPORT_BIT(     0x0400, "CPU Diagnostic",   KEYCODE_9) \
    COREPORT_BIT(     0x0800, "Sound Diagnostic", KEYCODE_0) \
  PORT_START /* 1 */ \
    COREPORT_DIPNAME( 0x0001, 0x0000, "Sound  Dip 1") \
      COREPORT_DIPSET(0x0001, "1" ) \
      COREPORT_DIPSET(0x0000, "0" ) \
    COREPORT_DIPNAME( 0x0002, 0x0002, "Sound Dip 2") \
      COREPORT_DIPSET(0x0002, "1" ) \
      COREPORT_DIPSET(0x0000, "0" )

/*-- Standard input ports --*/
#define S7_INPUT_PORTS_START(name,balls) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls) \
    S7_COMPORTS

#define S7_INPUT_PORTS_END INPUT_PORTS_END

#define S7_COMINPORT       CORE_COREINPORT

/*-- S7 switch numbers --*/
#define S7_SWADVANCE     -7
#define S7_SWUPDN        -6
#define S7_SWCPUDIAG     -5
#define S7_SWSOUNDDIAG   -4

/* GameOn Solenoid */
#define S7_GAMEONSOL 25
/*-------------------------
/ Machine driver constants
/--------------------------*/
#define S7_CPUNO   0

/*-- Memory regions --*/
#define S7_CPUREGION REGION_CPU1

#define S7_ROMSTART8088(name,ver, ic14,chk14, ic17,chk17, ic20,chk20, ic26,chk26) \
  ROM_START(name##_##ver) \
    NORMALREGION(0x10000, S7_CPUREGION) \
      ROM_LOAD(ic14, 0xe000, 0x0800, chk14 ) \
        ROM_RELOAD(  0x6000, 0x0800) \
      ROM_LOAD(ic17, 0xf000, 0x1000, chk17 ) \
        ROM_RELOAD(  0x7000, 0x1000) \
      ROM_LOAD(ic20, 0xe800, 0x0800, chk20 ) \
        ROM_RELOAD(  0x6800, 0x0800) \
      ROM_LOAD(ic26, 0xd800, 0x0800, chk26 ) \
        ROM_RELOAD(  0x5800, 0x0800)

#define S7_ROMSTART000x(name, ver, ic14,chk14, ic17,chk17, ic20,chk20) \
  ROM_START(name##_##ver) \
    NORMALREGION(0x10000, S7_CPUREGION) \
      ROM_LOAD(ic20, 0xd000, 0x1000, chk20) \
        ROM_RELOAD(  0x5000, 0x1000) \
      ROM_LOAD(ic14, 0xe000, 0x1000, chk14) \
        ROM_RELOAD(  0x6000, 0x1000) \
      ROM_LOAD(ic17, 0xf000, 0x1000, chk17) \
        ROM_RELOAD(  0x7000, 0x1000)

#define S7_ROMEND ROM_END

extern MACHINE_DRIVER_EXTERN(s7);
extern MACHINE_DRIVER_EXTERN(s7S);
#define s7_mS7         s7
#define s7_mS7S        s7S
extern const core_tLCDLayout s7_dispS7[];
#endif /* INC_S7 */

