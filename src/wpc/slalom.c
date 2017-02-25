/******************************************************************************************
  Stargame: Slalom Code 0.3
*******************************************************************************************/

#include "driver.h"
#include "core.h"
#include "sim.h"
#include "cpu/z80/z80.h"
#include "machine/z80fmly.h"

static struct {
  UINT8 sndCmd, msmData;
  int swCol, dispCol, dispCnt, f2;
} locals;

static INTERRUPT_GEN(slalom_vblank) {
  core_updateSw(TRUE); // game enables flippers directly
}

static INTERRUPT_GEN(slalom_zc) {
  static int zc;
  z80ctc_0_trg3_w(0, zc);
  if (zc) {
    locals.f2 = !locals.f2;
  }
  zc = !zc;
}

static void ctc_interrupt(int state) {
  cpu_set_irq_line_and_vector(0, 0, state, Z80_VECTOR(0, state));
}

static WRITE_HANDLER(to0_w) {
  // SINT - not used
}

static z80ctc_interface ctc_intf = {
  1,				/* 1 chip */
  { 0 },		/* clock (0 = CPU 0 clock) */
  { 0 },		/* timer disables (none) */
  { ctc_interrupt },
  { to0_w },
  { 0 },
  { 0 }
};

static Z80_DaisyChain slalom_DaisyChain[] = {
  {z80ctc_reset, z80ctc_interrupt, z80ctc_reti, 0},
  {0,0,0,-1}
};

static MACHINE_INIT(slalom) {
  ctc_intf.baseclock[0] = Machine->drv->cpu[0].cpu_clock;
  z80ctc_init(&ctc_intf);
}

static MACHINE_RESET(slalom) {
  memset(&locals, 0x00, sizeof(locals));
}

#ifdef MAME_DEBUG
static void adjust_snd(int offset) {
  static char s[4];
  locals.sndCmd += offset;
  sprintf(s, "%02x", locals.sndCmd);
  core_textOut(s, 4, 40, 2, 5);
}
#endif /* MAME_DEBUG */

static SWITCH_UPDATE(slalom) {
  if (inports) {
    CORE_SETKEYSW(inports[CORE_COREINPORT], 0x3f, 9);
  }
#ifdef MAME_DEBUG
  if (keyboard_pressed_memory_repeat(KEYCODE_B, 2)) {
    adjust_snd(-1);
  }
  if (keyboard_pressed_memory_repeat(KEYCODE_N, 2)) {
    adjust_snd(1);
  }
  if (keyboard_pressed_memory_repeat(KEYCODE_M, 2)) {
    cpu_set_nmi_line(1, PULSE_LINE);
  }
#endif /* MAME_DEBUG */
}

static READ_HANDLER(sw_r) {
  return ~coreGlobals.swMatrix[1 + locals.swCol];
}

static READ_HANDLER(sw0_r) {
  return ~coreGlobals.swMatrix[9];
}

static READ_HANDLER(int_r) {
  return locals.f2;
}

static WRITE_HANDLER(swCol_w) {
  locals.swCol = data;
}

static WRITE_HANDLER(sol1_w) {
  coreGlobals.solenoids = (coreGlobals.solenoids & 0xbffff) | ((data & 1) << 18); // coin counter
}

static WRITE_HANDLER(sol2_w) {
  switch (offset) {
    case 0: coreGlobals.solenoids = (coreGlobals.solenoids & 0xeffff) | ((data & 1) << 16); break; // control fijas
    case 1: coreGlobals.solenoids = (coreGlobals.solenoids & 0xdffff) | ((data & 1) << 17); break; // reset
  }
}

// solenoids are part of the lamp matrix, as usual on Spanish manufacturers, but this time very heavily so!
static WRITE_HANDLER(lamp_w) {
  coreGlobals.lampMatrix[offset + (locals.f2 ? 0 : 8)] = data;
  switch (offset) {
    case 0: coreGlobals.solenoids = (coreGlobals.solenoids & 0xffefe) | ((data >> 3) & 1) | (((data >> 5) & 1) << 8); break;
    case 1: coreGlobals.solenoids = (coreGlobals.solenoids & 0xffdfd) | (((data >> 3) & 1) << 1) | (((data >> 5) & 1) << 9); break;
    case 2: coreGlobals.solenoids = (coreGlobals.solenoids & 0xffbfb) | (((data >> 3) & 1) << 2) | (((data >> 5) & 1) << 10); break;
    case 3: coreGlobals.solenoids = (coreGlobals.solenoids & 0xff7f7) | (((data >> 3) & 1) << 3) | (((data >> 5) & 1) << 11); break;
    case 4: coreGlobals.solenoids = (coreGlobals.solenoids & 0xfefff) | (((data >> 5) & 1) << 12); break;
    case 5: coreGlobals.solenoids = (coreGlobals.solenoids & 0xfdfff) | (((data >> 5) & 1) << 13); break;
    case 6: coreGlobals.solenoids = (coreGlobals.solenoids & 0xfbfff) | (((data >> 5) & 1) << 14); break;
    case 7: coreGlobals.solenoids = (coreGlobals.solenoids & 0xf7f7f) | ((data >> 3) & 1) << 7 | (((data >> 5) & 1) << 15); break;
  }
}

static WRITE_HANDLER(dispCol_w) {
  static int pos[7] = { 5, 4, 3, 0, 6, 1, 2 };
  locals.dispCol = pos[core_BitColToNum(data) - 1];
  locals.dispCnt = 0;
}

static UINT8 convBits(UINT8 data) {
  return ((data & 0x01) << 7) | ((data & 0x06) << 3) | ((data & 0x28) >> 3) | ((data & 0x10) >> 1) | (data & 0x40) | ((data & 0x80) >> 6);
}

static WRITE_HANDLER(dispData_w) {
  switch (locals.dispCnt) {
    case 23: coreGlobals.segments[6 - locals.dispCol].w = convBits(data ^ 0xff); break;
    case 31: coreGlobals.segments[13 - locals.dispCol].w = convBits(data ^ 0xff); break;
    case 7: coreGlobals.segments[20 - locals.dispCol].w = convBits(data ^ 0xff); break;
    case 15: coreGlobals.segments[27 - locals.dispCol].w = convBits(data ^ 0xff); break;
    case 39: coreGlobals.segments[34 - locals.dispCol].w = convBits(data ^ 0xff); break;
    case 47: coreGlobals.segments[41 - locals.dispCol].w = convBits(data ^ 0xff); break;
  }
  locals.dispCnt++;
}

static WRITE_HANDLER(snd_w) {
  locals.sndCmd = data;
  cpu_set_nmi_line(1, PULSE_LINE);
}

static MEMORY_READ_START(cpu_readmem)
  {0x0000, 0x7fff, MRA_ROM},
  {0x8000, 0x87ff, MRA_RAM},
  {0xa020, 0xa020, sw_r},
  {0xa028, 0xa030, MRA_NOP},
  {0xa038, 0xa038, sw0_r},
  {0xe000, 0xe000, int_r},
MEMORY_END

static MEMORY_WRITE_START(cpu_writemem)
  {0x8000, 0x87ff, MWA_RAM, &generic_nvram, &generic_nvram_size},
  {0x9000, 0x9000, sol1_w},
  {0x9003, 0x9003, swCol_w},
  {0x9004, 0x9005, MWA_NOP},
  {0x9006, 0x9007, sol2_w},
  {0xa008, 0xa008, dispCol_w},
  {0xa010, 0xa017, lamp_w},
  {0xa018, 0xa018, dispData_w},
  {0xf000, 0xf000, snd_w},
MEMORY_END

static PORT_READ_START(cpu_readport)
  {0x00, 0x03, z80ctc_0_r},
PORT_END

static PORT_WRITE_START(cpu_writeport)
  {0x00, 0x03, z80ctc_0_w},
PORT_END

static READ_HANDLER(csport_r) {
  return locals.sndCmd;
}

static WRITE_HANDLER(resint_w) {
  cpu_set_irq_line(1, 0, CLEAR_LINE);
}

static WRITE_HANDLER(ay_0_w) {
  AY8910Write(0, offset, data);
}

static WRITE_HANDLER(ay_1_w) {
  AY8910Write(1, offset, data);
}

static WRITE_HANDLER(msm_ctrl_w) {
  if (GET_BIT6) {
    cpu_setbank(1, memory_region(REGION_SOUND1));
  } else if (GET_BIT7) {
    cpu_setbank(1, memory_region(REGION_SOUND1) + 0x4000);
  } else {
    cpu_setbank(1, memory_region(REGION_SOUND1) + 0x8000);
  }
  MSM5205_playmode_w(0, GET_BIT1 ? MSM5205_S48_4B : MSM5205_S96_4B);
  MSM5205_reset_w(0, GET_BIT0);
}

static WRITE_HANDLER(msm_data_w) {
  locals.msmData = data;
}

static void msmIrq(int data) {
  static int intr;
  cpu_set_irq_line(1, 0, intr ? ASSERT_LINE : CLEAR_LINE);
  MSM5205_data_w(0, intr ? locals.msmData & 0x0f : locals.msmData >> 4);
  intr = !intr;
}

static struct AY8910interface slalom_ay8910Int = {
  2,
  1500000,
  { MIXER(30,MIXER_PAN_LEFT), MIXER(30,MIXER_PAN_RIGHT) },
  { 0 }, { 0 },
  { DAC_0_data_w, msm_ctrl_w },
  { 0, msm_data_w }
};

static struct MSM5205interface slalom_msm5205Int = {
  1,
  375000,
  { msmIrq },
  { MSM5205_S48_4B },
  { 100 }
};

static struct DACinterface slalom_dacInt = {
  1,
  { 25 }
};

static MEMORY_READ_START(snd_readmem)
  {0x0000, 0x7fff, MRA_ROM},
  {0x8000, 0xbfff, MRA_BANKNO(1)},
  {0xc000, 0xc3ff, MRA_RAM},
MEMORY_END

static MEMORY_WRITE_START(snd_writemem)
  {0xc000, 0xc3ff, MWA_RAM},
MEMORY_END

static PORT_READ_START(snd_readport)
  {0x04, 0x04, csport_r},
PORT_END

static PORT_WRITE_START(snd_writeport)
  {0x00, 0x01, ay_0_w},
  {0x02, 0x03, ay_1_w},
  {0x06, 0x06, resint_w},
PORT_END

static MACHINE_DRIVER_START(slalom)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_FRAMES_PER_SECOND(50)

  MDRV_CPU_ADD_TAG("mcpu", Z80, 3000000)
  MDRV_CPU_CONFIG(slalom_DaisyChain)
  MDRV_CPU_MEMORY(cpu_readmem, cpu_writemem)
  MDRV_CPU_PORTS(cpu_readport, cpu_writeport)
  MDRV_CPU_VBLANK_INT(slalom_vblank, 1)
  MDRV_CPU_PERIODIC_INT(slalom_zc, 200)

  MDRV_CPU_ADD_TAG("scpu", Z80, 3000000)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(snd_readmem, snd_writemem)
  MDRV_CPU_PORTS(snd_readport, snd_writeport)

  MDRV_CORE_INIT_RESET_STOP(slalom,slalom,NULL)
  MDRV_SWITCH_UPDATE(slalom)
  MDRV_NVRAM_HANDLER(generic_0fill)
  MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
  MDRV_SOUND_ADD(AY8910, slalom_ay8910Int)
  MDRV_SOUND_ADD(MSM5205, slalom_msm5205Int)
  MDRV_SOUND_ADD(DAC, slalom_dacInt)
MACHINE_DRIVER_END

static core_tLCDLayout dispSlalom[] = {
  {0, 0, 0,7,CORE_SEG8D}, {3, 0, 7,7,CORE_SEG8D},
  {0,30,14,7,CORE_SEG8D}, {3,30,21,7,CORE_SEG8D},
  {3,16,28,2,CORE_SEG8D}, {3,21,30,1,CORE_SEG8D}, {3,24,31,2,CORE_SEG8D},
  {0}
};
static core_tGameData slalom03GameData = { 0, dispSlalom, { FLIP_SWNO(72,71), 0, 8 }};
static void init_slalom03(void) { core_gameData = &slalom03GameData; }
ROM_START(slalom03)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("scode03.rom", 0x0000, 0x8000, CRC(a0263129) SHA1(2f3fe3e91c351cb67fe156d19703eb654388d920))
  NORMALREGION(0x10000, REGION_CPU2)
    ROM_LOAD("2.bin", 0x0000, 0x8000, CRC(ac2d66ab) SHA1(6bdab76373c58ae176b0615c9e44f28d624fc43f))
  NORMALREGION(0x10000, REGION_SOUND1)
    ROM_LOAD("3.bin", 0x0000, 0x8000, CRC(79054b5f) SHA1(f0d704545735cdf7fd0431679c0809cdb1bbfa35))
    ROM_COPY(REGION_CPU2, 0x0000, 0x8000, 0x4000)
ROM_END
INPUT_PORTS_START(slalom03)
  CORE_PORTS
  SIM_PORTS(1)
  PORT_START /* 0 */
    COREPORT_BIT   (0x0008, "Game start", KEYCODE_1)
    COREPORT_BIT   (0x0001, "Coin 1",     KEYCODE_3)
    COREPORT_BIT   (0x0002, "Coin 2",     KEYCODE_4)
    COREPORT_BIT   (0x0004, "Coin 3",     KEYCODE_5)
    COREPORT_BIT   (0x0020, "Tilt",       KEYCODE_INSERT)
    COREPORT_BIT   (0x0010, "Test",       KEYCODE_7)
INPUT_PORTS_END
CORE_GAMEDEFNV(slalom03, "Slalom Code 0.3", 1988, "Stargame", slalom, 0)
