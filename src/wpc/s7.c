#include "driver.h"
#include "cpu/m6800/m6800.h"
#include "machine/6821pia.h"
#include "core.h"
#include "sndbrd.h"
#include "wmssnd.h"
#include "s7.h"

#define S7_VBLANKFREQ    60 /* VBLANK frequency */
#define S7_IRQFREQ     1000
#define S7_PIA0  0
#define S7_PIA1  1
#define S7_PIA2  2
#define S7_PIA3  3
#define S7_PIA4  4
#define S7_PIA5  5
#define S7_BANK0 1

#define S7_SOLSMOOTH       4 /* Smooth the Solenoids over this numer of VBLANKS */
#define S7_LAMPSMOOTH      2 /* Smooth the lamps over this number of VBLANKS */
#define S7_DISPLAYSMOOTH   2 /* Smooth the display over this number of VBLANKS */

static NVRAM_HANDLER(s7);

/*----------------
/ Local variables
/-----------------*/
static struct {
  int    vblankCount;
  UINT32 solenoids;
  core_tSeg segments, pseg;
  UINT16 alphaSegs;
  int    lampRow, lampColumn;
  int    digSel;
  int    diagnosticLed;
  int    swCol;
  int    ssEn; /* Special solenoids and flippers enabled ? */
  int    piaIrq;
  int    s6sound;
} s7locals;
static data8_t *s7_rambankptr, *s7_CMOS;

static void s7_irqline(int state) {
  if (state) {
    cpu_set_irq_line(0, M6808_IRQ_LINE, ASSERT_LINE);
    pia_set_input_ca1(S7_PIA3, core_getSw(S7_SWADVANCE));
    pia_set_input_cb1(S7_PIA3, core_getSw(S7_SWUPDN));
  }
  else if (!s7locals.piaIrq) {
    cpu_set_irq_line(0, M6808_IRQ_LINE, CLEAR_LINE);
    pia_set_input_ca1(S7_PIA3, 0);
    pia_set_input_cb1(S7_PIA3, 0);
  }
}

static void s7_piaIrq(int state) {
  s7_irqline(s7locals.piaIrq = state);
}

static INTERRUPT_GEN(s7_irq) {
  s7_irqline(1);
  timer_set(TIME_IN_CYCLES(32,0),0,s7_irqline);
}

static INTERRUPT_GEN(s7_vblank) {
  /*-------------------------------
  /  copy local data to interface
  /--------------------------------*/
  s7locals.vblankCount += 1;
  /*-- lamps --*/
  if ((s7locals.vblankCount % S7_LAMPSMOOTH) == 0) {
    memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
    memset(coreGlobals.tmpLampMatrix, 0, sizeof(coreGlobals.tmpLampMatrix));
  }
  /*-- solenoids --*/
  if ((s7locals.vblankCount % S7_SOLSMOOTH) == 0) {
    coreGlobals.solenoids = s7locals.solenoids;
    if (s7locals.ssEn) {
      int ii;
      coreGlobals.solenoids |= CORE_SOLBIT(S7_GAMEONSOL);
      /*-- special solenoids updated based on switches --*/
      for (ii = 0; ii < 8; ii++)
        if (core_gameData->sxx.ssSw[ii] && core_getSw(core_gameData->sxx.ssSw[ii]))
          coreGlobals.solenoids |= CORE_SOLBIT(CORE_FIRSTSSSOL+ii);
    }
    s7locals.solenoids = coreGlobals.pulsedSolState;
  }
  /*-- display --*/
  if ((s7locals.vblankCount % S7_DISPLAYSMOOTH) == 0) {
    memcpy(coreGlobals.segments, s7locals.segments, sizeof(coreGlobals.segments));
    memcpy(s7locals.segments, s7locals.pseg, sizeof(s7locals.segments));
    coreGlobals.diagnosticLed = s7locals.diagnosticLed;
    s7locals.diagnosticLed = 0;
  }
  core_updateSw(s7locals.ssEn);
}

/*---------------
/  Lamp handling
/----------------*/
static WRITE_HANDLER(pia2a_w) {
  core_setLamp(coreGlobals.tmpLampMatrix, s7locals.lampColumn, s7locals.lampRow = ~data);
}
static WRITE_HANDLER(pia2b_w) {
  core_setLamp(coreGlobals.tmpLampMatrix, s7locals.lampColumn = data, s7locals.lampRow);
}

/*-----------------
/ Display handling
/-----------------*/
static WRITE_HANDLER(pia3a_w) {
  s7locals.digSel = data & 0x0f;
  s7locals.diagnosticLed |= core_bcd2seg[data>>4];
}
static WRITE_HANDLER(pia3ca2_w) {
  DBGLOG(("pia3ca2_w\n"));
}

static WRITE_HANDLER(pia3b_w) {
  s7locals.segments[s7locals.digSel].w |=
    s7locals.pseg[s7locals.digSel].w = core_bcd2seg[data>>4];
  s7locals.segments[20+s7locals.digSel].w |=
    s7locals.pseg[20+s7locals.digSel].w = core_bcd2seg[data&0x0f];
}
static WRITE_HANDLER(pia0b_w) {
  if (data & 0x40) {
    s7locals.segments[21+s7locals.digSel].w |= 0x80;
    s7locals.pseg[21+s7locals.digSel].w |= 0x80;
  }
  if (data & 0x80) {
    s7locals.segments[1+s7locals.digSel].w |= 0x80;
    s7locals.pseg[1+s7locals.digSel].w |= 0x80;
  }
}

/*---------------------------
/ Alpha Display on Hyperball
/----------------------------*/
static WRITE_HANDLER(pia5a_w) {
  s7locals.alphaSegs = data & 0x3f;
  if (data & 0x40) s7locals.alphaSegs |= 0x100;
  if (data & 0x80) s7locals.alphaSegs |= 0x200;
}
static WRITE_HANDLER(pia5b_w) {
  if (data & 0x01) s7locals.alphaSegs |= 0x400;
  if (data & 0x02) s7locals.alphaSegs |= 0x40;
  if (data & 0x04) s7locals.alphaSegs |= 0x800;
  if (data & 0x08) s7locals.alphaSegs |= 0x4000;
  if (data & 0x10) s7locals.alphaSegs |= 0x2000;
  if (data & 0x20) s7locals.alphaSegs |= 0x1000;
  if (data & 0x40) s7locals.alphaSegs |= 0x80;
  if (data & 0x80) s7locals.alphaSegs |= 0x8000;
  s7locals.segments[40+s7locals.digSel].w |=
    s7locals.pseg[40+s7locals.digSel].w = s7locals.alphaSegs;
}
static WRITE_HANDLER(pia5ca2_w) {
  DBGLOG(("pia5ca2_w\n"));
}
static WRITE_HANDLER(pia5cb2_w) {
  DBGLOG(("pia5cb2_w\n"));
}

/*------------
/  Solenoids
/-------------*/
static void setSSSol(int data, int solNo) {
  int bit = CORE_SOLBIT(CORE_FIRSTSSSOL + solNo);
  if (s7locals.ssEn & (~data & 1))
    { coreGlobals.pulsedSolState |= bit;  s7locals.solenoids |= bit; }
  else
    coreGlobals.pulsedSolState &= ~bit;
}

static WRITE_HANDLER(pia1b_w) {
  if (s7locals.s6sound) {
    sndbrd_0_data_w(0, ~data); data &= 0xe0; /* mask of sound command bits */
  }
  coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xffff00ff) | (((UINT16)data)<<8);
  s7locals.solenoids |= (((UINT16)data)<<8);
}
static WRITE_HANDLER(pia1a_w) {
  coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xffffff00) | data;
  s7locals.solenoids |= data;
  // the following lines draw the extra lamp columns on Hyperball!
  if (s7locals.lampColumn & 0x01) core_setLamp(coreGlobals.tmpLampMatrix, 0x100, ~data & 0x0f);
  if (s7locals.lampColumn & 0x02) core_setLamp(coreGlobals.tmpLampMatrix, 0x100, ~data << 4);
  if (s7locals.lampColumn & 0x04) core_setLamp(coreGlobals.tmpLampMatrix, 0x200, ~data & 0x0f);
  if (s7locals.lampColumn & 0x08) core_setLamp(coreGlobals.tmpLampMatrix, 0x200, ~data << 4);
  if (s7locals.lampColumn & 0x10) core_setLamp(coreGlobals.tmpLampMatrix, 0x400, ~data & 0x0f);
  if (s7locals.lampColumn & 0x20) core_setLamp(coreGlobals.tmpLampMatrix, 0x400, ~data << 4);
  if (s7locals.lampColumn & 0x40) core_setLamp(coreGlobals.tmpLampMatrix, 0x800, ~data & 0x0f);
  if (s7locals.lampColumn & 0x80) core_setLamp(coreGlobals.tmpLampMatrix, 0x800, ~data << 4);
}
static WRITE_HANDLER(pia1cb2_w) { s7locals.ssEn = data;}
static WRITE_HANDLER(pia0ca2_w) { setSSSol(data, 7); }
static WRITE_HANDLER(pia0cb2_w) { setSSSol(data, 6); }
static WRITE_HANDLER(pia1ca2_w) { setSSSol(data, 4); }
static WRITE_HANDLER(pia2ca2_w) { setSSSol(data, 1); }
static WRITE_HANDLER(pia2cb2_w) { setSSSol(data, 0); }
static WRITE_HANDLER(pia3cb2_w) { setSSSol(data, 5); }
static WRITE_HANDLER(pia4ca2_w) { setSSSol(data, 3); }
static WRITE_HANDLER(pia4cb2_w) { setSSSol(data, 2); }

/*---------------
/ Switch reading
/----------------*/
static WRITE_HANDLER(pia4b_w) { s7locals.swCol = data; }
static READ_HANDLER(pia4a_r)  { return core_getSwCol(s7locals.swCol); }

static struct pia6821_interface s7_pia[] = {
{  /* PIA 0 (2100)
    PA0-4 Sound
    PB5   CA1
    PB6   Comma 3+4
    PB7   Comma 1+2
    CA2   SS8
    CB2   SS7
    CA1,CB1 NC */
 /* in  : A/B,CA/B1,CA/B2 */ 0, PIA_UNUSED_VAL(0x3f), PIA_UNUSED_VAL(1), PIA_UNUSED_VAL(0), 0, 0,
 /* out : A/B,CA/B2       */ sndbrd_0_data_w, pia0b_w, pia0ca2_w, pia0cb2_w,
 /* irq : A/B             */ s7_piaIrq, s7_piaIrq
},{/* PIA 1 (2200)
    PA0-7 Sol 1-8 (Extra lamp strobe on Hyperball)
    PB0-7 Sol 9-16
    CA2   SS5
    CB2   GameOn (0)
    CA1,CB1 NC */
 /* in  : A/B,CA/B1,CA/B2 */ 0, 0, 0, PIA_UNUSED_VAL(0), 0, 0,
 /* out : A/B,CA/B2       */ pia1a_w, pia1b_w, pia1ca2_w, pia1cb2_w,
 /* irq : A/B             */ s7_piaIrq, s7_piaIrq
},{/* PIA 2 (2400)
    PA0-7  Lamp return
    PB0-7  Lamp Strobe
    CA2    SS2
    CB2    SS1
    CA1,CB1 NC */
 /* in  : A/B,CA/B1,CA/B2 */ 0, 0, 0, PIA_UNUSED_VAL(0), 0, 0,
 /* out : A/B,CA/B2       */ pia2a_w, pia2b_w, pia2ca2_w, pia2cb2_w,
 /* irq : A/B             */ s7_piaIrq, s7_piaIrq
},{/* PIA 3 (2800)
    PA0-3  Digit Select
    PA4-7  Diagnostic LED
    PB0-7  BCD output
    CA1    Diag In
    CB1    Diag In
    CB2    SS6
    CA2    Diagnostic LED control? */
 /* in  : A/B,CA/B1,CA/B2 */ 0, 0, 0, 0, 0, 0,
 /* out : A/B,CA/B2       */ pia3a_w, pia3b_w, pia3ca2_w, pia3cb2_w,
 /* irq : A/B             */ s7_piaIrq, s7_piaIrq
},{/* PIA 4 (3000)
    PA0-7  Switch return
    PB0-7  Switch drive
    CB2    SS3
    CA2    SS4 */
 /* in  : A/B,CA/B1,CA/B2 */ pia4a_r, 0, 0, 0, 0, 0,
 /* out : A/B,CA/B2       */ 0, pia4b_w, pia4ca2_w, pia4cb2_w,
 /* irq : A/B             */ s7_piaIrq, s7_piaIrq
},{/* PIA 5 (4000)
    PA0-7  Digit Select
    PB0-7  alphanumeric output */
 /* in  : A/B,CA/B1,CA/B2 */ 0, 0, 0, 0, 0, 0,
 /* out : A/B,CA/B2       */ pia5a_w, pia5b_w, pia5ca2_w, pia5cb2_w,
 /* irq : A/B             */ s7_piaIrq, s7_piaIrq
}};

static SWITCH_UPDATE(s7) {
  if (inports) {
    coreGlobals.swMatrix[0] = (inports[S7_COMINPORT] & 0x7f00)>>8;
    coreGlobals.swMatrix[1] = inports[S7_COMINPORT];
  }
  /*-- Generate interupts for diganostic keys --*/
  cpu_set_nmi_line(0, core_getSw(S7_SWCPUDIAG) ? ASSERT_LINE : CLEAR_LINE);
  sndbrd_0_diag(core_getSw(S7_SWSOUNDDIAG));
}

static MACHINE_INIT(s7) {
  if (core_gameData == NULL) return;
  pia_config(S7_PIA0, PIA_STANDARD_ORDERING, &s7_pia[0]);
  pia_config(S7_PIA1, PIA_STANDARD_ORDERING, &s7_pia[1]);
  pia_config(S7_PIA2, PIA_STANDARD_ORDERING, &s7_pia[2]);
  pia_config(S7_PIA3, PIA_STANDARD_ORDERING, &s7_pia[3]);
  pia_config(S7_PIA4, PIA_STANDARD_ORDERING, &s7_pia[4]);
  pia_config(S7_PIA5, PIA_STANDARD_ORDERING, &s7_pia[5]);
  sndbrd_0_init(SNDBRD_S67S, 1, NULL, NULL, NULL);
  cpu_setbank(S7_BANK0, s7_rambankptr);
}

static MACHINE_INIT(s7S6) {
  machine_init_s7();
  s7locals.s6sound = 1;
}

static MACHINE_RESET(s7) {
  pia_reset();
}

static MACHINE_STOP(s7) {
  sndbrd_0_exit();
}

/*---------------------------
/  Memory map for main CPU
/----------------------------*/
static WRITE_HANDLER(s7_CMOS_w) { s7_CMOS[offset] = data | 0xf0; }

/*---------------------------------
  IC13+
  IC16 RAM (0000-007f, 1000-13ff)
  IC19 RAM (0100-01ff)
  IC22 ROM (6200-63ff)
  IC21 ROM (6000-61ff)
  IC14 ROM (6000-67ff)
  IC17 ROM (7000-77ff)
  IC26 ROM (5800-5fff)
  IC20 ROM (6800-6fff)
-----------------------------------*/
static MEMORY_READ_START(s7_readmem)
  { 0x0000, 0x00ff, MRA_BANKNO(S7_BANK0)},
  { 0x0100, 0x01ff, MRA_RAM},
  { 0x1000, 0x13ff, MRA_RAM}, /* CMOS */
  { 0x2100, 0x2103, pia_r(S7_PIA0)},
  { 0x2200, 0x2203, pia_r(S7_PIA1)},
  { 0x2400, 0x2403, pia_r(S7_PIA2)},
  { 0x2800, 0x2803, pia_r(S7_PIA3)},
  { 0x3000, 0x3003, pia_r(S7_PIA4)},
  { 0x4000, 0x4003, pia_r(S7_PIA5)},
  { 0x5000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(s7_writemem)
  { 0x0000, 0x00ff, MWA_BANKNO(S7_BANK0)},
  { 0x0100, 0x01ff, s7_CMOS_w, &s7_CMOS },
  { 0x1000, 0x13ff, MWA_RAM, &s7_rambankptr }, /* CMOS */
  { 0x2100, 0x2103, pia_w(S7_PIA0)},
  { 0x2200, 0x2203, pia_w(S7_PIA1)},
  { 0x2400, 0x2403, pia_w(S7_PIA2)},
  { 0x2800, 0x2803, pia_w(S7_PIA3)},
  { 0x3000, 0x3003, pia_w(S7_PIA4)},
  { 0x4000, 0x4003, pia_w(S7_PIA5)},
  { 0x5000, 0xffff, MWA_ROM },
MEMORY_END

/*-----------------
/  Machine drivers
/------------------*/
MACHINE_DRIVER_START(s7)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(s7,s7,s7)
  MDRV_CPU_ADD(M6808, 3580000/4)
  MDRV_CPU_MEMORY(s7_readmem, s7_writemem)
  MDRV_CPU_VBLANK_INT(s7_vblank, 1)
  MDRV_CPU_PERIODIC_INT(s7_irq, S7_IRQFREQ)
  MDRV_NVRAM_HANDLER(s7)
  MDRV_DIPS(2) /* On sound board */
  MDRV_SWITCH_UPDATE(s7)
  MDRV_DIAGNOSTIC_LED7
MACHINE_DRIVER_END

MACHINE_DRIVER_START(s7S)
  MDRV_IMPORT_FROM(s7)
  MDRV_IMPORT_FROM(wmssnd_s67s)
  MDRV_SOUND_CMD(sndbrd_0_data_w)
  MDRV_SOUND_CMDHEADING("s7")
MACHINE_DRIVER_END

MACHINE_DRIVER_START(s7S6)
  MDRV_IMPORT_FROM(s7)
  MDRV_CORE_INIT_RESET_STOP(s7S6,s7,s7)
  MDRV_IMPORT_FROM(wmssnd_s67s)
  MDRV_SOUND_CMD(sndbrd_0_data_w)
  MDRV_SOUND_CMDHEADING("s7")
MACHINE_DRIVER_END

/*-----------------------------------------------
/ Load/Save static ram
/-------------------------------------------------*/
static NVRAM_HANDLER(s7) {
  core_nvram(file, read_or_write, s7_CMOS, 0x0100, 0xff);
}
