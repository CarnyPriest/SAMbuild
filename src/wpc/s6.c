#include "driver.h"
#include "cpu/m6800/m6800.h"
#include "machine/6821pia.h"
#include "core.h"
#include "sndbrd.h"
#include "wmssnd.h"
#include "s6.h"

#define S6_PIA0 0
#define S6_PIA1 1
#define S6_PIA2 2
#define S6_PIA3 3

#define S6_VBLANKFREQ    60 /* VBLANK frequency */
#define S6_IRQFREQ     1000 /* IRQ Frequency*/

/*
  6 Digit Structure, Alpha Position, and Layout
  ----------------------------------------------
  (00)(01)(02)(03)(04)(05) (08)(09)(10)(11)(12)(13)

  (16)(17)(18((19)(20)(21) (24)(25)(26)(27)(28)(29)

	   (14)(15)   (06)(07)

  0-5 =		Player 1
  6-7 =		Right Side
  8-13 =	Player 2
  14-15 =	Left side
  16-21 =	Player 3
  24-29 =	Player 4
*/
const core_tLCDLayout s6_6digit_disp[] = {
  // Player 1            Player 2
  {0, 0, 0,6,CORE_SEG7}, {0,18, 8,6,CORE_SEG7},
  // Player 3            Player 4
  {2, 0,20,6,CORE_SEG7}, {2,18,28,6,CORE_SEG7},
  // Right Side          Left Side
  {4, 9,14,2,CORE_SEG7}, {4,14, 6,2,CORE_SEG7}, {0}
};

/*
  7 Digit Structure, Alpha Position, and Layout
  ----------------------------------------------
  (01)(02)(03)(04)(05)(06)(07)  (09)(10)(11)(12)(13)(14)(15)

  (17)(18)(19)(20)(21)(22)(23)  (25)(26)(27)(28)(29)(30)(31)

         (00)(08)   (16)(24)

  0 =		Left Side Left Digit
  1-7		Player 1 (Digits 1-7)
  8 =		Left Side Right Digit
  9-15 =	Player 2 (Digits 1-7)
  16 =		Right Side Left Digit
  17-23 =	Player 3 (Digits 1-7)
  24 =		Right Side Right Digit
  25-31 =	Player 4 (Digits 1-7) */

const core_tLCDLayout s6_7digit_disp[] = {
  // Player 1 Segment    Player 2 Segment
  {0, 0, 1,7,CORE_SEG7},{0,18, 9,7,CORE_SEG7},
  // Player 3 Segment    Player 4 Segment
  {2, 0,21,7,CORE_SEG7},{2,18,29,7,CORE_SEG7},
  // Credits (Left Side)
  {4,14, 0,1,CORE_SEG7},{4,16, 8,1,CORE_SEG7},
  // Balls (Right Side)
  {4, 9,20,1,CORE_SEG7},{4,11,28,1,CORE_SEG7}, {0}
};

static void s6_exit(void);

static struct {
  int	 alphapos;
  int    vblankCount;
  UINT32 solenoids;
  core_tSeg segments,pseg;
  int    lampRow, lampColumn;
  int    diagnosticLed;
  int    swCol;
  int    ssEn;
  int    mainIrq;
  int    ca20,a0;
} s6locals;
static data8_t *s6_CMOS;

static void s6_nvram(void *file, int write);
static void s6_piaIrq(int state);

static READ_HANDLER(s6_pia0ca1_r) {
  return activecpu_get_reg(M6800_IRQ_STATE) && core_getSw(S6_SWADVANCE);
}
static READ_HANDLER(s6_pia0cb1_r) {
  return activecpu_get_reg(M6800_IRQ_STATE) && core_getSw(S6_SWUPDN);
}

/*-- Dips: alphapos selects one 4 bank --*/
static READ_HANDLER(s6_dips_r) {
  if ((s6locals.ca20) && (s6locals.alphapos < 0x04)) { /* 0x01=0, 0x02=1, 0x04=2, 0x08=3 */
    return (core_getDip(s6locals.alphapos/2+1)<<(4*(1-(s6locals.alphapos&0x01)))) & 0xf0;
  }
  return 0xff;
}
/********************/
/*SWITCH MATRIX     */
/********************/
static READ_HANDLER(s6_swrow_r) { return core_getSwCol(s6locals.swCol); }
static WRITE_HANDLER(s6_swcol_w) { s6locals.swCol = data; }

/*****************/
/*DISPLAY ALPHA  */
/*****************/
static WRITE_HANDLER(s6_alpha_w) {
  s6locals.segments[0][s6locals.alphapos].lo |=
      s6locals.pseg[0][s6locals.alphapos].lo = core_bcd2seg[data>>4];
  s6locals.segments[1][s6locals.alphapos].lo |=
      s6locals.pseg[1][s6locals.alphapos].lo = core_bcd2seg[data&0x0f];
}

static WRITE_HANDLER(s6_pa_w) {
  s6locals.a0 = data;
  if (s6locals.ca20) s6locals.diagnosticLed = ((data>>3)&0x02)|((data>>5)&0x01);
  s6locals.alphapos = data & 0x0f;
}

static WRITE_HANDLER(s6_pia0ca2_w) {
  s6locals.ca20 = data;
  if (data) {
    s6locals.diagnosticLed = ((s6locals.a0>>3)&0x02)|((s6locals.a0>>5)&0x01);
  }
}

/*-- Lamp handling --*/
static WRITE_HANDLER(s6_lampcol_w)
{ core_setLamp(coreGlobals.tmpLampMatrix, s6locals.lampColumn = data, s6locals.lampRow); }
static WRITE_HANDLER(s6_lamprow_w)
{ core_setLamp(coreGlobals.tmpLampMatrix, s6locals.lampColumn, s6locals.lampRow = ~data); }

/*-- Solenoid 1-8 --*/
static WRITE_HANDLER(s6_sol1_8_w) {
  coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xffffff00) | data;
  s6locals.solenoids |= data;
}

/*-- Solenoid 9-16 (9-13 used for sound) --*/
static WRITE_HANDLER(s6_sol9_16_w) {
  sndbrd_0_data_w(0, ~data); data &= 0xe0; /* mask of sound command bits */
  s6locals.solenoids |= data<<8;
  coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xffff00ff) | (data<<8);
}

/*-- Special solenoids */
static void setSSSol(int data, int solNo) {
  int bit = CORE_SOLBIT(CORE_FIRSTSSSOL + solNo);
  if (s6locals.ssEn & (~data & 1)) { coreGlobals.pulsedSolState |= bit;  s6locals.solenoids |= bit; }
  else                               coreGlobals.pulsedSolState &= ~bit;
}
static WRITE_HANDLER(s6_gameon_w) { s6locals.ssEn = data; }
static WRITE_HANDLER(s6_specsol1_w) { setSSSol(data,0); }
static WRITE_HANDLER(s6_specsol2_w) { setSSSol(data,1); }
static WRITE_HANDLER(s6_specsol3_w) { setSSSol(data,2); }
static WRITE_HANDLER(s6_specsol4_w) { setSSSol(data,3); }
static WRITE_HANDLER(s6_specsol5_w) { setSSSol(data,4); }
static WRITE_HANDLER(s6_specsol6_w) { setSSSol(data,5); }

static const struct pia6821_interface s6_pia[] = {
{ /* PIA 1 - Display and other
  o  PA0-3:  16 Digit Strobing.. 1 # activates a digit.
  o  PA4-5:  Output LED's - When CA2 Output = 1
  i  PA4-7:  16 Functional Switches (Dips?)
  o  PB0-7:  BCD1&BCD2 Outputs..
  i  CA1:    Diagnostic Switch - On interrupt: Advance Switch
  i  CB1:    Diagnostic Switch - On interrupt: Auto/Manual Switch
  i  CA2:    Read Master Command Enter Switch
  o  CB2:    Special Solenoid #6 */
 /* in  : A/B,CA1/B1,CA2/B2 */ s6_dips_r, 0, s6_pia0ca1_r, s6_pia0cb1_r, 0, 0,
 /* out : A/B,CA2/B2        */ s6_pa_w, s6_alpha_w, s6_pia0ca2_w, s6_specsol6_w,
 /* irq : A/B               */ s6_piaIrq, s6_piaIrq
},{/*PIA 2 - Switch Matrix
  i  PA0-7:  Switch Rows
  i  PB0-7:  Switch Columns??
  o  PB0-7:  Switch Column Enable
  o  CA2:    Special Solenoid #4
  o  CB2:    Special Solenoid #3
     CA1,CB1 NC */
 /* in  : A/B,CA1/B1,CA2/B2 */ s6_swrow_r, 0, 0, 0, 0, 0,
 /* out : A/B,CA2/B2        */ 0, s6_swcol_w, s6_specsol4_w,s6_specsol3_w,
 /* irq : A/B               */ s6_piaIrq, s6_piaIrq
},{/*PIA 3 - Lamp Matrix
  o  PA0-7:  Lamp Rows
  o  PB0-7:  Lamp Columns
  i  PB0-7:  Lamp Column (input????!!)
  o  CA2:    Special Solenoid #2
  o  CB2:    Special Solenoid #1
     CA1,CB1 NC */
 /* in  : A/B,CA1/B1,CA2/B2 */ 0, 0, 0, 0, 0, 0,
 /* out : A/B,CA2/B2        */ s6_lamprow_w, s6_lampcol_w, s6_specsol2_w,s6_specsol1_w,
 /* irq : A/B               */ s6_piaIrq, s6_piaIrq
},{/*PIA 4 - Standard Solenoids
     PA0-7:  Solenoids 1-8
     PB0-7:  Solenoids 9-16 - Sound Commands (#9-#13 only)!
     CA2:    Special Solenoid #5
     CB2:    Game On Signal
     CA1,CB1 NC */
 /* in  : A/B,CA1/B1,CA2/B2 */ 0, 0, 0, 0, 0, 0,
 /* out : A/B,CA2/B2        */ s6_sol1_8_w, s6_sol9_16_w,s6_specsol5_w, s6_gameon_w,
 /* irq : A/B               */ s6_piaIrq, s6_piaIrq
}};

static void s6_piaIrq(int state) {
  s6locals.mainIrq = state;
  cpu_set_irq_line(0, M6808_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}

static int s6_irq(void) {
  if (s6locals.mainIrq == 0) /* Don't send IRQ if already active */
    cpu_set_irq_line(0, M6808_IRQ_LINE, HOLD_LINE);
  return 0;
}

static int s6_vblank(void) {
  /*-------------------------------
  /  copy local data to interface
  /--------------------------------*/
  s6locals.vblankCount += 1;

  /*-- lamps --*/
  if ((s6locals.vblankCount % S6_LAMPSMOOTH) == 0) {
    memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
    memset(coreGlobals.tmpLampMatrix, 0, sizeof(coreGlobals.tmpLampMatrix));
  }

  /*-- solenoids --*/
  if ((s6locals.vblankCount % S6_SOLSMOOTH) == 0) {
    coreGlobals.solenoids = s6locals.solenoids;
    if (s6locals.ssEn) {
      int ii;
      coreGlobals.solenoids |= CORE_SOLBIT(CORE_SSFLIPENSOL);
      /*-- special solenoids updated based on switches --*/
      for (ii = 0; ii < 6; ii++)
        if (core_gameData->sxx.ssSw[ii] && core_getSw(core_gameData->sxx.ssSw[ii]))
          coreGlobals.solenoids |= CORE_SOLBIT(CORE_FIRSTSSSOL+ii);
    }
    s6locals.solenoids = coreGlobals.pulsedSolState;
  }
  /*-- display --*/
  if ((s6locals.vblankCount % S6_DISPLAYSMOOTH) == 0) {
    memcpy(coreGlobals.segments, s6locals.segments, sizeof(coreGlobals.segments));
    memcpy(s6locals.segments, s6locals.pseg, sizeof(s6locals.segments));

    /*update leds*/
    coreGlobals.diagnosticLed = s6locals.diagnosticLed;
    s6locals.diagnosticLed = 0;

  }
  core_updateSw(s6locals.ssEn);
  return 0;
}

static void s6_updSw(int *inports) {
  if (inports) {
    coreGlobals.swMatrix[1] = inports[S6_COMINPORT] & 0x00ff;
    coreGlobals.swMatrix[0] = (inports[S6_COMINPORT] & 0xff00)>>8;
  }
  /*-- Diagnostic buttons on CPU board --*/
  cpu_set_nmi_line(0, core_getSw(S6_SWCPUDIAG) ? ASSERT_LINE : CLEAR_LINE);
  sndbrd_0_diag(core_getSw(S6_SWSOUNDDIAG));

  /*-- coin door switches --*/
  pia_set_input_ca1(S6_PIA0, core_getSw(S6_SWADVANCE));
  pia_set_input_cb1(S6_PIA0, core_getSw(S6_SWUPDN));
  /* Show Status of Auto/Manual Switch */
  core_textOutf(40, 30, BLACK, core_getSw(S6_SWUPDN) ? "Auto  " : "Manual");
}

static const core_tData s6Data = {
  8+16, /* 16 DIPs */
  s6_updSw,
  2 | DIAGLED_VERTICAL,
  sndbrd_0_data_w, "s6",
  core_swSeq2m, core_swSeq2m,core_m2swSeq,core_m2swSeq
};

static void s6_init(void) {
  memset(&s6locals, 0, sizeof(s6locals));
  if (core_init(&s6Data)) return;
  pia_config(S6_PIA0, PIA_STANDARD_ORDERING, &s6_pia[0]);
  pia_config(S6_PIA1, PIA_STANDARD_ORDERING, &s6_pia[1]);
  pia_config(S6_PIA2, PIA_STANDARD_ORDERING, &s6_pia[2]);
  pia_config(S6_PIA3, PIA_STANDARD_ORDERING, &s6_pia[3]);
  sndbrd_0_init(SNDBRD_S67S, 1, NULL, NULL, NULL);
  pia_reset();
  s6locals.vblankCount = 1;
}

static void s6_exit(void) {
  sndbrd_0_exit(); core_exit();
}

static WRITE_HANDLER(s6_CMOS_w) { s6_CMOS[offset] = data | 0xf0; }

/*-----------------------------------
/  Memory map for Sys 6 CPU board
/------------------------------------*/
static MEMORY_READ_START(s6_readmem)
  { 0x0000, 0x01ff, MRA_RAM},
  { 0x2200, 0x2203, pia_r(S6_PIA3) },		/*Solenoids + Sound Commands*/
  { 0x2400, 0x2403, pia_r(S6_PIA2) }, 		/*Lamps*/
  { 0x2800, 0x2803, pia_r(S6_PIA0) },		/*Display + Other*/
  { 0x3000, 0x3003, pia_r(S6_PIA1) },		/*Switches*/
  { 0x6000, 0x8000, MRA_ROM },
  { 0x8000, 0xffff, MRA_ROM },		/*Doubled ROM region since only 15 address pins used!*/
MEMORY_END

static MEMORY_WRITE_START(s6_writemem)
  { 0x0000, 0x0100, MWA_RAM },
  { 0x0100, 0x01ff, s6_CMOS_w, &s6_CMOS },
  { 0x2200, 0x2203, pia_w(S6_PIA3) },		/*Solenoids + Sound Commands*/
  { 0x2400, 0x2403, pia_w(S6_PIA2) }, 		/*Lamps*/
  { 0x2800, 0x2803, pia_w(S6_PIA0) },		/*Display + Other*/
  { 0x3000, 0x3003, pia_w(S6_PIA1) },		/*Switches*/
  { 0x6000, 0x8000, MWA_ROM },
  { 0x8000, 0xffff, MWA_ROM },		/*Doubled ROM region since only 15 address pins used!*/
MEMORY_END

const struct MachineDriver machine_driver_s6 = {
  {{  CPU_M6808, 3580000/4, /* 3.58/4 = 900hz */
      s6_readmem, s6_writemem, NULL, NULL,
      s6_vblank, 1, s6_irq, S6_IRQFREQ
  }},
  S6_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50, s6_init, s6_exit,
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_TYPE_RASTER, 0, NULL, NULL, gen_refresh,
  0,0,0,0, {{0}},
  s6_nvram
};

const struct MachineDriver machine_driver_s6s= {
  {{  CPU_M6808, 3580000/4, /* 3.58/4 = 900hz */
      s6_readmem, s6_writemem, NULL, NULL,
      s6_vblank, 1, s6_irq, S6_IRQFREQ
  }, S67S_SOUNDCPU
  },
  S6_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50, s6_init, s6_exit,
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_TYPE_RASTER, 0, NULL, NULL, gen_refresh,
  0,0,0,0, { S67S_SOUND },
  s6_nvram
};

/*-----------------------------------------------
/ Load/Save static ram
/-------------------------------------------------*/
static void s6_nvram(void *file, int write) {
  core_nvram(file, write, s6_CMOS, 0x0100, 0xff);
}

