/* Zaccaria Prototype Solid State machines */
/* CPU: National Semiconductor SC/MP ISP-8A 500D */

#include "driver.h"
#include "cpu/scamp/scamp.h"
#include "core.h"
#include "sim.h"

static struct {
  core_tSeg segments;
  UINT32 solenoids;
  UINT16 sols2;
  int vblankCount;
  int dispCount;
} locals;

static INTERRUPT_GEN(vblank) {
  locals.vblankCount++;

  /*-------------------------------
  /  copy local data to interface
  /--------------------------------*/
  memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
  memcpy(coreGlobals.segments, locals.segments, sizeof(coreGlobals.segments));
  if (++locals.dispCount > 11) {
    locals.dispCount = 0;
    memset(locals.segments, 0, sizeof(locals.segments));
  }
  if (locals.solenoids & 0xffff) {
    coreGlobals.solenoids = locals.solenoids;
    locals.vblankCount = 1;
  }
  if ((locals.vblankCount % 4) == 0)
    coreGlobals.solenoids = locals.solenoids;

  core_updateSw(core_getSol(17));
}

static MACHINE_INIT(zacProto) {
  memset(&locals, 0, sizeof(locals));
}

static MACHINE_STOP(zacProto) {
  memset(&locals, 0, sizeof(locals));
}

static SWITCH_UPDATE(zacProto) {
  if (inports) {
    CORE_SETKEYSW(inports[CORE_COREINPORT],     0x03,0);
    CORE_SETKEYSW(inports[CORE_COREINPORT] >> 8,0x0f,1);
  }
}

static READ_HANDLER(sw_r) {
  SCAMP_set_sense_a(coreGlobals.swMatrix[0] & 0x01);
  SCAMP_set_sense_b(coreGlobals.swMatrix[0] & 0x02);
  return coreGlobals.swMatrix[offset+1];
}

static READ_HANDLER(dip_r) {
  return core_getDip(offset);
}

static WRITE_HANDLER(disp_w) {
  locals.dispCount = 0;
  locals.segments[9-offset*2].w = core_bcd2seg7e[data & 0x0f];
  locals.segments[8-offset*2].w = core_bcd2seg7e[data >> 4];
  // fake single 0 digit
  locals.segments[10].w = locals.segments[9].w ? core_bcd2seg7e[0] : 0;
}

static WRITE_HANDLER(sol_w) {
  if (!offset)
    locals.solenoids = (locals.solenoids & 0xffffff00) | data;
  else
    locals.solenoids = (locals.solenoids & 0xffff00ff) | (data << 8);
}

static WRITE_HANDLER(sound_w) {
  if (data) logerror("sound byte #%d: %02x, %02x inversed\n", offset, data, data^0xff);
}

static WRITE_HANDLER(lamp_w) {
  coreGlobals.tmpLampMatrix[offset] = data;
  if (!offset)
    locals.solenoids = (locals.solenoids & 0xfffeffff) | ((data & 0x01) << 16);
  if (offset > 7) logerror("lamp write to col. %d: %02x\n", offset, data);
}

static MEMORY_READ_START(readmem)
  { 0x0000, 0x0bff, MRA_ROM },
  { 0x0d00, 0x0dff, MRA_RAM },
  { 0x0e00, 0x0e04, sw_r },
  { 0x0e05, 0x0e07, dip_r },
  { 0x13ff, 0x17ff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(writemem)
  { 0x0d00, 0x0dff, MWA_RAM, &generic_nvram, &generic_nvram_size },
  { 0x0e00, 0x0e01, sol_w },
  { 0x0e02, 0x0e06, disp_w },
  { 0x0e07, 0x0e08, sound_w },
  { 0x0e09, 0x0e16, lamp_w },
MEMORY_END

static core_tLCDLayout disp[] = {
  {0, 0, 4, 7,CORE_SEG87FD},
  {3, 2, 0, 2,CORE_SEG7S},
  {3,16, 2, 2,CORE_SEG7S},
  {0}
};
static core_tGameData strikeGameData = {GEN_ZAC1,disp};
static void init_strike(void) {
  core_gameData = &strikeGameData;
}

MACHINE_DRIVER_START(zacProto)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(zacProto,NULL,zacProto)
  MDRV_CPU_ADD_TAG("mcpu", SCAMP, 1000000)
  MDRV_SWITCH_UPDATE(zacProto)
  MDRV_CPU_MEMORY(readmem, writemem)
  MDRV_CPU_VBLANK_INT(vblank, 1)
  MDRV_NVRAM_HANDLER(generic_0fill)
MACHINE_DRIVER_END

INPUT_PORTS_START(strike) \
  CORE_PORTS \
  SIM_PORTS(1) \
  PORT_START /* 0 */ \
    COREPORT_BIT   (0x0400, "Start Button",  KEYCODE_1) \
    COREPORT_BIT   (0x0100, "Coin Slot #1",  KEYCODE_3) \
    COREPORT_BIT   (0x0200, "Coin Slot #2",  KEYCODE_5) \
    COREPORT_BITTOG(0x0002, "Coin Door",     KEYCODE_END) \
    COREPORT_BIT   (0x0800, "Tilt/Advance",  KEYCODE_INSERT) \
    COREPORT_BITTOG(0x0001, "Sense Input A", KEYCODE_PGDN) \
  PORT_START /* 1 */ \
    COREPORT_DIPNAME( 0x000f, 0x0002, "Coin slot #1 plays") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0001, "1/2" ) \
      COREPORT_DIPSET(0x0002, "1" ) \
      COREPORT_DIPSET(0x0003, "1 1/2" ) \
      COREPORT_DIPSET(0x0004, "2" ) \
      COREPORT_DIPSET(0x0005, "2 1/2" ) \
      COREPORT_DIPSET(0x0006, "3" ) \
      COREPORT_DIPSET(0x0007, "3 1/2" ) \
      COREPORT_DIPSET(0x0008, "4" ) \
      COREPORT_DIPSET(0x0009, "4 1/2" ) \
      COREPORT_DIPSET(0x000a, "5" ) \
      COREPORT_DIPSET(0x000b, "5 1/2" ) \
      COREPORT_DIPSET(0x000c, "6" ) \
      COREPORT_DIPSET(0x000d, "6 1/2" ) \
      COREPORT_DIPSET(0x000e, "7" ) \
      COREPORT_DIPSET(0x000f, "7 1/2" ) \
    COREPORT_DIPNAME( 0x00f0, 0x0040, "Coin slot #1 plays") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0010, "1/2" ) \
      COREPORT_DIPSET(0x0020, "1" ) \
      COREPORT_DIPSET(0x0030, "1 1/2" ) \
      COREPORT_DIPSET(0x0040, "2" ) \
      COREPORT_DIPSET(0x0050, "2 1/2" ) \
      COREPORT_DIPSET(0x0060, "3" ) \
      COREPORT_DIPSET(0x0070, "3 1/2" ) \
      COREPORT_DIPSET(0x0080, "4" ) \
      COREPORT_DIPSET(0x0090, "4 1/2" ) \
      COREPORT_DIPSET(0x00a0, "5" ) \
      COREPORT_DIPSET(0x00b0, "5 1/2" ) \
      COREPORT_DIPSET(0x00c0, "6" ) \
      COREPORT_DIPSET(0x00d0, "6 1/2" ) \
      COREPORT_DIPSET(0x00e0, "7" ) \
      COREPORT_DIPSET(0x00f0, "7 1/2" ) \
    COREPORT_DIPNAME( 0x0300, 0x0200, "Replays for beating HSTD") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0100, "1" ) \
      COREPORT_DIPSET(0x0200, "2" ) \
      COREPORT_DIPSET(0x0300, "3" ) \
    COREPORT_DIPNAME( 0x0400, 0x0000, "HSTD/Match award") \
      COREPORT_DIPSET(0x0000, "Replay" ) \
      COREPORT_DIPSET(0x0400, "Super Bonus" ) \
    COREPORT_DIPNAME( 0x0800, 0x0800, "Match feature") \
      COREPORT_DIPSET(0x0000, " off" ) \
      COREPORT_DIPSET(0x0800, " on" ) \
    COREPORT_DIPNAME( 0x3000, 0x2000, "HSTD/Random award") \
      COREPORT_DIPSET(0x0000, "500,000 Pts" ) \
      COREPORT_DIPSET(0x1000, "Extra Ball" ) \
      COREPORT_DIPSET(0x2000, "Replay" ) \
      COREPORT_DIPSET(0x3000, "Super Bonus" ) \
    COREPORT_DIPNAME( 0xc000, 0x8000, "Special award") \
      COREPORT_DIPSET(0x0000, "500,000 Pts" ) \
      COREPORT_DIPSET(0x4000, "Extra Ball" ) \
      COREPORT_DIPSET(0x8000, "Replay" ) \
      COREPORT_DIPSET(0xc000, "Super Bonus" ) \
  PORT_START /* 2 */ \
    COREPORT_DIPNAME( 0x0001, 0x0000, "Random feature") \
      COREPORT_DIPSET(0x0000, " on" ) \
      COREPORT_DIPSET(0x0001, " off" ) \
    COREPORT_DIPNAME( 0x0006, 0x0004, "Balls per game") \
      COREPORT_DIPSET(0x0000, "1" ) \
      COREPORT_DIPSET(0x0002, "3" ) \
      COREPORT_DIPSET(0x0004, "5" ) \
      COREPORT_DIPSET(0x0006, "7" ) \
    COREPORT_DIPNAME( 0x0018, 0x0018, "Strikes needed for Special") \
      COREPORT_DIPSET(0x0000, "1" ) \
      COREPORT_DIPSET(0x0008, "2" ) \
      COREPORT_DIPSET(0x0010, "3" ) \
      COREPORT_DIPSET(0x0018, "4" ) \
    COREPORT_DIPNAME( 0x0020, 0x0000, "Unlimited Specials") \
      COREPORT_DIPSET(0x0000, " off" ) \
      COREPORT_DIPSET(0x0020, " on" ) \
    COREPORT_DIPNAME( 0x0040, 0x0040, "Bonus Ball Award") \
      COREPORT_DIPSET(0x0000, "200,000 Pts" ) \
      COREPORT_DIPSET(0x0040, "Bonus Ball" ) \
    COREPORT_DIPNAME( 0x0080, 0x0000, "Unknown ") \
      COREPORT_DIPSET(0x0000, " off" ) \
      COREPORT_DIPSET(0x0080, " on" )
INPUT_PORTS_END

ROM_START(strike) \
  NORMALREGION(0x8000, REGION_CPU1) \
    ROM_LOAD("strike1.bin", 0x0000, 0x0400, CRC(650abc54) SHA1(6a4f83016a38338ba6a04271532f0880264e61a7)) \
    ROM_LOAD("strike2.bin", 0x0400, 0x0400, CRC(13c5a168) SHA1(2da3a5bc0c28a2aacd8c1396dac95cf35f8797cd)) \
    ROM_LOAD("strike3.bin", 0x0800, 0x0400, CRC(ebbbf315) SHA1(c87e961c8e5e99b0672cd632c5e104ea52088b5d)) \
    ROM_LOAD("strike4.bin", 0x1400, 0x0400, CRC(ca0eddd0) SHA1(52f9faf791c56b68b1806e685d0479ea67aba019))
ROM_END

CORE_GAMEDEFNV(strike, "Strike", 1978, "Zaccaria", zacProto, GAME_NO_SOUND)
