/************************************************************************************************
 Videodens (Spain)
 ------------------------

   Hardware:
   ---------
		CPU:     Z80 @ 4? MHz
			INT: IRQ @ 1000? Hz
		IO:      CPU ports
		DISPLAY: 7-digit 8-segment panels, direct segment access
		SOUND:	 ?
 ************************************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "core.h"
#include "sound/ay8910.h"
#include "sndbrd.h"

#define JP_VBLANKFREQ   60 /* VBLANK frequency */
#define JP_CPUFREQ 4000000 /* CPU clock frequency */

/*----------------
/  Local variables
/-----------------*/
static struct {
  int    vblankCount, solCount;
  UINT32 solenoids;
  core_tSeg segments;
  int    col;
} locals;

static INTERRUPT_GEN(VD_irq) {
  cpu_set_irq_line(0, 0, PULSE_LINE);
}

/*-------------------------------
/  copy local data to interface
/--------------------------------*/
static INTERRUPT_GEN(VD_vblank) {
  locals.vblankCount++;

  /*-- lamps --*/
  memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
  /*-- solenoids --*/
  coreGlobals.solenoids = locals.solenoids;
  /*-- display --*/
  memcpy(coreGlobals.segments, locals.segments, sizeof(locals.segments));

  core_updateSw(TRUE);
}

static SWITCH_UPDATE(VD) {
  if (inports) {
    CORE_SETKEYSW(inports[CORE_COREINPORT], 0x07, 0);
    CORE_SETKEYSW(inports[CORE_COREINPORT]>>4, 0x01, 2);
    CORE_SETKEYSW(inports[CORE_COREINPORT]>>5, 0x01, 3);
    CORE_SETKEYSW(inports[CORE_COREINPORT]>>6, 0x01, 4);
    CORE_SETKEYSW(inports[CORE_COREINPORT]>>7, 0x01, 5);
  }
}

struct AY8910interface VD_ay8910Int = {
  2,         /* 2 chips ? */
  2500000,   /* 2.5 MHz ? */
  { 30, 30 } /* Volume */
};

static READ_HANDLER(sw_r) {
  return coreGlobals.swMatrix[1+offset];
}

static READ_HANDLER(sw0_r) {
  return ~coreGlobals.swMatrix[0];
}

static WRITE_HANDLER(col_w) {
  if ((data & 0x0f) < 0x08) locals.col = data & 0x07;
  else logerror("col_w:%02x\n", data);
}

static WRITE_HANDLER(lamp_w) {
  coreGlobals.tmpLampMatrix[offset] = data;
}

static WRITE_HANDLER(disp_w) {
  int digit = offset*8 + locals.col;
  locals.segments[digit].w = (locals.segments[digit].w & 0x80) | (core_revbyte(data) & 0x7f);
  // comma segments are carried by the following digit!
  if (digit) locals.segments[digit-1].w = (locals.segments[digit-1].w & 0x7f) | ((data & 1) << 7);
}

static WRITE_HANDLER(p60_w) {
  logerror("p60_w:%02x\n", data);
}

static WRITE_HANDLER(p62_w) {
  logerror("p62_w:%02x\n", data);
}

static WRITE_HANDLER(p80_w) {
  logerror("p80_w:%02x\n", data);
}

static WRITE_HANDLER(p82_w) {
  logerror("p82_w:%02x\n", data);
}

static READ_HANDLER(pa0_r) {
  logerror("pa0_r:\n");
  return 0xff;
}

static WRITE_HANDLER(sol_w) {
  if (data) locals.solenoids = (1 << (data-1));
  else locals.solenoids = 0;
}

static MEMORY_READ_START(VD_readmem)
  {0x0000,0x5fff, MRA_ROM},
  {0x6000,0x67ff, MRA_RAM},
MEMORY_END

static MEMORY_WRITE_START(VD_writemem)
  {0x6000,0x67ff, MWA_RAM, &generic_nvram, &generic_nvram_size},
MEMORY_END

static PORT_READ_START(VD_readport)
  {0x00,0x05, sw_r},
  {0x61,0x61, sw0_r},
  {0xa0,0xa0, pa0_r},
PORT_END

static PORT_WRITE_START(VD_writeport)
  {0x20,0x27, lamp_w},
  {0x28,0x28, sol_w},
  {0x40,0x44, disp_w},
  {0x60,0x60, p60_w},
  {0x62,0x62, p62_w},
  {0x80,0x80, p80_w},
  {0x82,0x82, p82_w},
  {0xc0,0xc0, col_w},
PORT_END

static MACHINE_INIT(VD) {
  memset(&locals, 0, sizeof locals);
}

#define VD_INPUT_PORTS_START(name,balls) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls) \
    PORT_START /* 0 */ \
    COREPORT_BITDEF(0x0010, IPT_START1,      IP_KEY_DEFAULT) \
    COREPORT_BITDEF(0x0040, IPT_COIN1,       IP_KEY_DEFAULT) \
    COREPORT_BITDEF(0x0080, IPT_COIN2,       IP_KEY_DEFAULT) \
    COREPORT_BIT(   0x0020, "Ball Tilt",     KEYCODE_INSERT) \
    COREPORT_BIT(   0x0001, "Accounting #1", KEYCODE_7)  \
    COREPORT_BIT(   0x0002, "Accounting #2", KEYCODE_8)  \
    COREPORT_BIT(   0x0004, "Accounting #3", KEYCODE_9)

MACHINE_DRIVER_START(VD)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CPU_ADD_TAG("mcpu", Z80, JP_CPUFREQ)
  MDRV_CPU_MEMORY(VD_readmem, VD_writemem)
  MDRV_CPU_PORTS(VD_readport, VD_writeport)
  MDRV_CPU_VBLANK_INT(VD_vblank, 1)
  MDRV_CPU_PERIODIC_INT(VD_irq, 1000)
  MDRV_CORE_INIT_RESET_STOP(VD,NULL,NULL)
  MDRV_NVRAM_HANDLER(generic_0fill)
  MDRV_SWITCH_UPDATE(VD)
//  MDRV_SOUND_ADD(AY8910, VD_ay8910Int)
MACHINE_DRIVER_END

static const core_tLCDLayout dispVD[] = {
  { 0, 0, 0, 7, CORE_SEG87 },
  { 3, 0, 8, 7, CORE_SEG87 },
  { 6, 0,16, 7, CORE_SEG87 },
  { 9, 0,24, 7, CORE_SEG87 },
  {12, 1,32, 1, CORE_SEG7S}, {12, 4,33, 1, CORE_SEG7S}, {12, 7,34, 1, CORE_SEG7S}, {12,10,35, 2, CORE_SEG7S}, {12,15,37, 1, CORE_SEG7S},
  {0}
};

// Break (1986)

static core_tGameData breakGameData = {GEN_ZAC1,dispVD,{FLIP_SW(FLIP_L),0,0,0}}; \
static void init_break(void) { core_gameData = &breakGameData; }

ROM_START(break) \
  NORMALREGION(0x10000, REGION_CPU1) \
    ROM_LOAD("break1.cpu", 0x0000, 0x2000, CRC(c187d263) SHA1(1790566799ccc41cd5445936e86f945150e24e8a)) \
    ROM_LOAD("break2.cpu", 0x2000, 0x2000, CRC(ed8f84ab) SHA1(ff5d7e3c373ca345205e8b92c6ce7b02f36a3d95)) \
    ROM_LOAD("break3.cpu", 0x4000, 0x2000, CRC(3cdfedc2) SHA1(309fd04c81b8facdf705e6297c0f4d507957ae1f))
ROM_END
VD_INPUT_PORTS_START(break, 1) INPUT_PORTS_END
CORE_GAMEDEFNV(break, "Break", 1986, "Videodens", VD, GAME_NO_SOUND)

// Papillon (1986)
