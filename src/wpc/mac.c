/******************************************************************************************
  MAC / CICPlay (Spain)
*******************************************************************************************/

#include "driver.h"
#include "core.h"
#include "sim.h"
#include "cpu/z80/z80.h"

static struct {
  int    i8279cmd;
  int    i8279reg;
  UINT8  i8279ram[16];
  UINT8  i8279data;
  int strobe;
  int ed;
  int el;
  int isCic;
} locals;

static INTERRUPT_GEN(mac_vblank) {
  core_updateSw(TRUE);
}

static void mac_nmi(int data) {
  if (!(coreGlobals.swMatrix[0] & 0x40))
    cpu_set_nmi_line(0, PULSE_LINE);
}

static MACHINE_INIT(MAC) {
}

static MACHINE_RESET(MAC) {
  memset(&locals, 0x00, sizeof(locals));
}

static MACHINE_RESET(CIC) {
  memset(&locals, 0x00, sizeof(locals));
  locals.isCic = 1;
  coreGlobals.segments[32].w = core_bcd2seg9[0];
}

static MACHINE_STOP(MAC) {
}

static SWITCH_UPDATE(MAC) {
  int i;
  if (inports) {
    CORE_SETKEYSW(inports[CORE_COREINPORT] >> 8, 0xc0, 0);
    CORE_SETKEYSW(inports[CORE_COREINPORT], 0x7f, 1);
  }
  for (i = 1; i < 9; i++) {
    if (coreGlobals.swMatrix[i]) {
      cpu_set_irq_line(0, 0, ASSERT_LINE);
      return;
    }
  }
}

static WRITE_HANDLER(ay8910_0_porta_w)	{
  locals.strobe = data;
}
static WRITE_HANDLER(ay8910_0_portb_w)	{
  coreGlobals.solenoids = data ^ 0xff;
}

static READ_HANDLER(ay8910_1_portb_r)   { return coreGlobals.swMatrix[0] & 0x80; }
static WRITE_HANDLER(ay8910_1_porta_w)	{
  if (locals.strobe == 0xfd)
    coreGlobals.lampMatrix[0] = data;
  else if (locals.strobe > 0xfd)
    coreGlobals.lampMatrix[6 + locals.strobe - 0xfe] = data;
  else if (locals.strobe > 0xf7)
    coreGlobals.lampMatrix[1 + locals.strobe - 0xf8] = data;
  else if (data)
    printf("s %02x=%02x\n", locals.strobe, data);
}
static WRITE_HANDLER(ay8910_1_portb_w)	{
  if (GET_BIT4) {
    locals.ed = GET_BIT1;
    locals.el = GET_BIT2;
    cpu_set_irq_line(0, 0, ASSERT_LINE);
  }
}

struct AY8910interface MAC_ay8910Int = {
	2,			/* 1 chip */
	2000000,	/* 2 MHz */
	{ 30, 30 },		/* Volume */
	{ 0, 0 },
	{ 0, ay8910_1_portb_r },
	{ ay8910_0_porta_w, ay8910_1_porta_w },
	{ ay8910_0_portb_w, ay8910_1_portb_w },
};
static WRITE_HANDLER(ay8910_0_ctrl_w)   { AY8910Write(0,0,data); }
static WRITE_HANDLER(ay8910_0_data_w)   { AY8910Write(0,1,data); }
static READ_HANDLER (ay8910_0_r)        { return AY8910Read(0); }
static WRITE_HANDLER(ay8910_1_ctrl_w)   { AY8910Write(1,0,data); }
static WRITE_HANDLER(ay8910_1_data_w)   { AY8910Write(1,1,data); }
static READ_HANDLER (ay8910_1_r)        { return AY8910Read(1); }

static UINT16 mac_bcd2seg(UINT8 data) {
  switch (data & 0x0f) {
    case 0x0a: return 0x79; // E
    case 0x0b: return 0x50; // r
    case 0x0c: return 0x5c; // o
  }
  return locals.isCic ? core_bcd2seg9[data & 0x0f] : core_bcd2seg7[data & 0x0f];
}

// handles the 8279 keyboard / display interface chip
static READ_HANDLER(i8279_r) {
  int row;
  logerror("i8279 r%d (cmd %02x, reg %02x)\n", offset, locals.i8279cmd, locals.i8279reg);
  if ((locals.i8279cmd & 0xe0) == 0x40) {
    row = locals.i8279reg & 0x07;
    if (row < 2) locals.i8279data = core_getDip(row); // read dips
    else locals.i8279data = coreGlobals.swMatrix[row - 1]; // read switches
  } else if ((locals.i8279cmd & 0xe0) == 0x60)
    locals.i8279data = locals.i8279ram[locals.i8279reg]; // read display ram
  else logerror("i8279 r:%02x\n", locals.i8279cmd);
  if (locals.i8279cmd & 0x10) locals.i8279reg = (locals.i8279reg+1) % 16; // auto-increase if register is set
  return locals.i8279data;
}
static WRITE_HANDLER(i8279_w) {
  if (offset) { // command
    locals.i8279cmd = data;
    if ((locals.i8279cmd & 0xe0) == 0x40)
      logerror("I8279 read switches: %x\n", data & 0x07);
    else if ((locals.i8279cmd & 0xe0) == 0x80)
      logerror("I8279 write display: %x\n", data & 0x0f);
    else if ((locals.i8279cmd & 0xe0) == 0x60)
      logerror("I8279 read display: %x\n", data & 0x0f);
    else if ((locals.i8279cmd & 0xe0) == 0x20)
      logerror("I8279 scan rate: %02x\n", data & 0x1f);
    else if ((locals.i8279cmd & 0xe0) == 0xa0)
      logerror("I8279 blank: %x\n", data & 0x0f);
    else if ((locals.i8279cmd & 0xe0) == 0xc0) {
      logerror("I8279 clear: %x, %x\n", (data & 0x1c) >> 2, data & 0x03);
      if (data & 0x03) {
        locals.i8279data = 0;
        cpu_set_irq_line(0, 0, CLEAR_LINE);
      }
    } else if ((locals.i8279cmd & 0xe0) == 0xe0) {
      logerror("I8279 end interrupt\n");
      locals.i8279data = 0;
      cpu_set_irq_line(0, 0, CLEAR_LINE);
    } else if ((locals.i8279cmd & 0xe0) == 0)
      logerror("I8279 set modes: display %x, keyboard %x\n", (data >> 3) & 0x03, data & 0x07);
    else printf("i8279 w%d:%02x\n", offset, data);
    if (locals.i8279cmd & 0x10) locals.i8279reg = data & 0x0f; // reset data for auto-increment
  } else { // data
    if ((locals.i8279cmd & 0xe0) == 0x80) { // write display ram
      if (locals.i8279reg < 12) {
        if (locals.ed || locals.el)
          coreGlobals.lampMatrix[8] = data;
        else {
          coreGlobals.segments[locals.i8279reg].w = mac_bcd2seg(data);
          coreGlobals.segments[16 + locals.i8279reg].w = mac_bcd2seg(data >> 4);
        }
      } else {
        if (locals.ed || locals.el)
          coreGlobals.lampMatrix[9] = data;
        else {
          coreGlobals.segments[locals.i8279reg].w = core_bcd2seg7[data & 0x0f];
          coreGlobals.segments[16 + locals.i8279reg].w = core_bcd2seg7[data >> 4];
        }
      }
    } else printf("i8279 w%d:%02x\n", offset, data);
    if (locals.i8279cmd & 0x10) locals.i8279reg = (locals.i8279reg+1) % 16; // auto-increase if register is set
  }
}

static MEMORY_READ_START(cpu_readmem)
  {0x0000, 0x3fff, MRA_ROM},
  {0xc000, 0xc7ff, MRA_RAM},
MEMORY_END

static MEMORY_WRITE_START(cpu_writemem)
  {0x0000, 0x3fff, MWA_NOP},
  {0xc000, 0xc7ff, MWA_RAM, &generic_nvram, &generic_nvram_size},
MEMORY_END

static PORT_READ_START(cpu_readport)
  {0x09,0x09, ay8910_0_r},
  {0x29,0x29, ay8910_1_r},
  {0x40,0x40, i8279_r},
PORT_END

static PORT_WRITE_START(cpu_writeport)
  {0x08,0x08, ay8910_0_ctrl_w},
  {0x0a,0x0a, ay8910_0_data_w},
  {0x28,0x28, ay8910_1_ctrl_w},
  {0x2a,0x2a, ay8910_1_data_w},
  {0x40,0x41, i8279_w},
PORT_END

MACHINE_DRIVER_START(mac)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CPU_ADD_TAG("mcpu", Z80, 4000000)
  MDRV_CPU_MEMORY(cpu_readmem, cpu_writemem)
  MDRV_CPU_PORTS(cpu_readport, cpu_writeport)
  MDRV_CPU_VBLANK_INT(mac_vblank, 1)
  MDRV_TIMER_ADD(mac_nmi, 120)
  MDRV_CORE_INIT_RESET_STOP(MAC, MAC, MAC)
  MDRV_DIPS(16)
  MDRV_SWITCH_UPDATE(MAC)
  MDRV_NVRAM_HANDLER(generic_0fill)
  MDRV_SOUND_ADD(AY8910, MAC_ay8910Int)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(cic)
  MDRV_IMPORT_FROM(mac)
  MDRV_CORE_INIT_RESET_STOP(MAC, CIC, MAC)
MACHINE_DRIVER_END

#define INITGAME(name, disp, flip) \
static core_tGameData name##GameData = {0,disp,{flip,0,2}}; \
static void init_##name(void) { core_gameData = &name##GameData; }

#define MAC_COMPORTS(game, balls) \
  INPUT_PORTS_START(game) \
  CORE_PORTS \
  SIM_PORTS(balls) \
  PORT_START /* 0 */ \
    COREPORT_BIT   (0x0001, "Start",     KEYCODE_1) \
    COREPORT_BITIMP(0x0002, "Coin 1",    KEYCODE_3) \
    COREPORT_BITIMP(0x0004, "Coin 2",    KEYCODE_5) \
    COREPORT_BIT   (0x0008, "Tilt",      KEYCODE_INSERT) \
    COREPORT_BIT   (0x0010, "Slam Tilt", KEYCODE_HOME) \
    COREPORT_BIT   (0x0020, "Coin 3",    KEYCODE_9) \
    COREPORT_BIT   (0x0040, "Reset RAM", KEYCODE_0) \
    COREPORT_BIT   (0x4000, "SW1",       KEYCODE_DEL) \
    COREPORT_BITTOG(0x8000, "40V Line",  KEYCODE_END) \
  PORT_START /* 1 */ \
    COREPORT_DIPNAME( 0x0001, 0x0001, "Balls per Game") \
      COREPORT_DIPSET(0x0001, "3" ) \
      COREPORT_DIPSET(0x0000, "5" ) \
    COREPORT_DIPNAME( 0x0006, 0x0006, "Credits f. sm. / big coins") \
      COREPORT_DIPSET(0x0000, "1/2 / 3" ) \
      COREPORT_DIPSET(0x0002, "1/2 / 4" ) \
      COREPORT_DIPSET(0x0006, "5/4 / 5" ) \
      COREPORT_DIPSET(0x0004, "3/2 / 6" ) \
    COREPORT_DIPNAME( 0x0008, 0x0000, "S4") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0008, "1" ) \
    COREPORT_DIPNAME( 0x0010, 0x0000, "S5") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0010, "1" ) \
    COREPORT_DIPNAME( 0x0020, 0x0000, "S6") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0020, "1" ) \
    COREPORT_DIPNAME( 0x0040, 0x0000, "End game when idle") \
      COREPORT_DIPSET(0x0000, DEF_STR(No) ) \
      COREPORT_DIPSET(0x0040, DEF_STR(Yes) ) \
    COREPORT_DIPNAME( 0x0080, 0x0080, "Unused, always on") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0080, "1" ) \
    COREPORT_DIPNAME( 0x0f00, 0x0600, "Ball/RP1/2/HSTD") \
      COREPORT_DIPSET(0x0000, "300K/450K/630K/1M" ) \
      COREPORT_DIPSET(0x0100, "325K/500K/690K/1.1M" ) \
      COREPORT_DIPSET(0x0200, "350K/570K/740K/1.15M" ) \
      COREPORT_DIPSET(0x0300, "375K/610K/790K/1.25M" ) \
      COREPORT_DIPSET(0x0400, "400K/650K/850K/1.35M" ) \
      COREPORT_DIPSET(0x0500, "425K/690K/900K/1.4M" ) \
      COREPORT_DIPSET(0x0600, "450K/730K/950K/1.5M" ) \
      COREPORT_DIPSET(0x0700, "500K/810K/1.05M/1.65M" ) \
      COREPORT_DIPSET(0x0800, "550K/890K/1.15M/1.8M" ) \
      COREPORT_DIPSET(0x0900, "600K/950K/1.25M/2M" ) \
      COREPORT_DIPSET(0x0a00, "650K/1.05M/1.35M/2.15M" ) \
      COREPORT_DIPSET(0x0b00, "700K/1.15M/1.5M/2.3M" ) \
      COREPORT_DIPSET(0x0c00, "750K/1.25M/1.6M/2.5M" ) \
      COREPORT_DIPSET(0x0d00, "800K/1.3M/1.7M/2.7M" ) \
      COREPORT_DIPSET(0x0e00, "900K/1.45M/1.9M/3M" ) \
      COREPORT_DIPSET(0x0f00, "1M/1.65M/2.15M/3.3M" ) \
    COREPORT_DIPNAME( 0x1000, 0x0000, "Auto-adjust") \
      COREPORT_DIPSET(0x0000, DEF_STR(Off) ) \
      COREPORT_DIPSET(0x1000, DEF_STR(On) ) \
    COREPORT_DIPNAME( 0x2000, 0x0000, "Auto-adjust Ball") \
      COREPORT_DIPSET(0x0000, DEF_STR(Off) ) \
      COREPORT_DIPSET(0x2000, DEF_STR(On) ) \
    COREPORT_DIPNAME( 0x4000, 0x0000, "Auto-adjust Replay") \
      COREPORT_DIPSET(0x0000, DEF_STR(Off) ) \
      COREPORT_DIPSET(0x4000, DEF_STR(On) ) \
    COREPORT_DIPNAME( 0x8000, 0x8000, "Unused, always on") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x8000, "1" ) \
  INPUT_PORTS_END

static core_tLCDLayout dispMAC[] = {
  {0, 0, 0,6,CORE_SEG8},
  {0,16, 6,6,CORE_SEG8},
  {3, 0,16,6,CORE_SEG8},
  {3,16,22,6,CORE_SEG8},
  {6, 0,15,1,CORE_SEG8}, {6, 4,14,1,CORE_SEG8}, {6, 8,13,1,CORE_SEG8}, {6,10,12,1,CORE_SEG8},
  {6,16,28,4,CORE_SEG8}, {6,24,31,1,CORE_SEG8}, {6,26,31,1,CORE_SEG8},
  {6, 2,32,1,CORE_SEG8}, {6, 6,32,1,CORE_SEG8},
  {0}
};

ROM_START(spctrain)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("mbm27128.25", 0x0000, 0x4000, CRC(d65c5c36) SHA1(6f350b48daaecd36b3086e682ec6ee174f297a34))
ROM_END

INITGAME(spctrain,dispMAC,FLIP_SW(FLIP_L))
MAC_COMPORTS(spctrain, 1)
CORE_GAMEDEFNV(spctrain, "Space Train", 1987, "MAC", mac, 0)

static core_tLCDLayout dispCIC[] = {
  {0, 0, 0,1,CORE_SEG10},{0, 2, 1,2,CORE_SEG9}, {0, 6, 3,1,CORE_SEG10},{0, 8, 4,2,CORE_SEG9}, {0,12,32,1,CORE_SEG9},
  {0,16, 6,1,CORE_SEG10},{0,18, 7,2,CORE_SEG9}, {0,22, 9,1,CORE_SEG10},{0,24,10,2,CORE_SEG9}, {0,28,32,1,CORE_SEG9},
  {2, 0,16,1,CORE_SEG10},{2, 2,17,2,CORE_SEG9}, {2, 6,19,1,CORE_SEG10},{2, 8,20,2,CORE_SEG9}, {2,12,32,1,CORE_SEG9},
  {2,16,22,1,CORE_SEG10},{2,18,23,2,CORE_SEG9}, {2,22,25,1,CORE_SEG10},{2,24,26,2,CORE_SEG9}, {2,28,32,1,CORE_SEG9},
  {4, 8,13,1,CORE_SEG7S},{4,10,12,1,CORE_SEG7S},{4,14,14,1,CORE_SEG7S},{4,18,15,1,CORE_SEG7S},{4,22,28,2,CORE_SEG7S},{4,28,30,2,CORE_SEG7S},
  {0}
};

ROM_START(glxplay2)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("gp_27128.bin", 0x0000, 0x4000, NO_DUMP)
ROM_END

INITGAME(glxplay2,dispCIC,FLIP_SW(FLIP_L))
MAC_COMPORTS(glxplay2, 1)
CORE_GAMEDEFNV(glxplay2, "Galaxy Play 2", 1987, "CICPlay", cic, 0)
