#include <stdarg.h>
#include <time.h>
#include "driver.h"
#include "cpu/m6800/m6800.h"
#include "machine/6821pia.h"
#include "core.h"
#include "sndbrd.h"
#include "by35snd.h"
#include "stsnd.h"
#include "hnks.h"
#include "by35.h"

static char debugms[] = "01234567";
// ok
static READ_HANDLER(snd300_r) {
  logerror("%04x: snd300_r adress A%c data latch %02x give %02x \n", activecpu_get_previouspc(), debugms[offset],snddatst300.ax[offset],snddatst300.axb[offset]);
  snddatst300.axb[2] = snddatst300.timer1 / 256;
  snddatst300.axb[3] = snddatst300.timer1 -  (snddatst300.axb[2] * 256);
  snddatst300.axb[4] = snddatst300.timer2 / 256;
  snddatst300.axb[5] = snddatst300.timer2 -  (snddatst300.axb[4] * 256);
  snddatst300.axb[6] = snddatst300.timer3 / 256;
  snddatst300.axb[7] = snddatst300.timer3 -  (snddatst300.axb[6] * 256);

  return snddatst300.axb[offset];
}
 
static WRITE_HANDLER(snd300_w) {
  snddatst300.ax[offset]=data;
  sndbrd_0_data_w(0,offset);
}

static WRITE_HANDLER(snd300_wex) {
  sndbrd_0_ctrl_w(0,data);
}


#define BY35_PIA0 0
#define BY35_PIA1 1

#define BY35_VBLANKFREQ    60 /* VBLANK frequency */

#define BY35_SOLSMOOTH       2 /* Smooth the Solenoids over this numer of VBLANKS */

#define BY35_LAMPSMOOTH      2 /* Smooth the lamps over this number of VBLANKS */

#define BY35_DISPLAYSMOOTH   4 /* Smooth the display over this number of VBLANKS */
/*--------------------------------------------------
/ There are a few variants on the BY35 hardware
/ PIA1:A1  - SOUNDE (BY35), 7th display digit (Stern)
/ PIA1:A0  - credit/ball strobe. Positive edge (BY17,Stern), Negative edge (BY35)
/ PIA1:CA2 - sound control (HNK), lamp strobe 2 (BY17, BY35, stern)
/ 4th DIP bank (not HNK)
/ 9 segment display (HNK)
----------------------------------------------------*/
#define BY35HW_SOUNDE    0x01 // supports 5th sound line (BY35 only)
#define BY35HW_INVDISP4  0x02 // inverted strobe to credit/ball (BY17+stern)
#define BY35HW_DIP4      0x04 // got 4th DIP bank (not HNK)
#define BY35HW_REVSW     0x08 // reversed switches (HNK only)
#define BY35HW_SCTRL     0x20 // uses lamp2 as sound ctrl (HNK)

static struct {
  int a0, a1, b1, ca20, ca21, cb20, cb21;
  int bcd[7], lastbcd;
  const int *bcd2seg;
  int lampadr1, lampadr2;
  UINT32 solenoids;
  core_tSeg segments,pseg;
  int diagnosticLed;
  int vblankCount;
  int hw;
  int phaseA;
} locals;

static void piaIrq(int state) {
  cpu_set_irq_line(0, M6800_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}

static void by35_dispStrobe(int mask) {
  int digit = locals.a1 & 0xfe;
  int ii,jj;

  /* This handles O. Kaegi's 7-digit mod wiring */
  if (locals.hw & BY35HW_SOUNDE && !(core_gameData->hw.gameSpecific1 & BY35GD_PHASE))
    digit = (digit & 0xf0) | ((digit & 0x0c) == 0x0c ? 0x02 : (digit & 0x0d));

  for (ii = 0; digit; ii++, digit>>=1)
    if (digit & 0x01) {
      UINT8 dispMask = mask;
      for (jj = 0; dispMask; jj++, dispMask>>=1)
        if (dispMask & 0x01)
          locals.segments[jj*8+ii].w |= locals.pseg[jj*8+ii].w = locals.bcd2seg[locals.bcd[jj] & 0x0f];
    }

  /* This handles the fake zero for Nuova Bell games */
  if (core_gameData->hw.gameSpecific1 & BY35GD_FAKEZERO) {
	if (locals.segments[7].w) locals.segments[8].w = locals.pseg[8].w = locals.bcd2seg[0];
	if (locals.segments[15].w) locals.segments[16].w = locals.pseg[16].w = locals.bcd2seg[0];
	if (locals.segments[23].w) locals.segments[24].w = locals.pseg[24].w = locals.bcd2seg[0];
	if (locals.segments[31].w) locals.segments[32].w = locals.pseg[32].w = locals.bcd2seg[0];
  }
}

static void by35_lampStrobe(int board, int lampadr) {
  if (lampadr != 0x0f) {
    int lampdata = (locals.a0>>4)^0x0f;
    UINT8 *matrix = &coreGlobals.tmpLampMatrix[(lampadr>>3)+8*board];
    int bit = 1<<(lampadr & 0x07);

    while (lampdata) {
      if (lampdata & 0x01) *matrix |= bit;
      lampdata >>= 1; matrix += 2;
    }
  }
}

/* PIA0:A-W  Control what is read from PIA0:B */
static WRITE_HANDLER(pia0a_w) {
  if (!locals.ca20) {
    int bcdLoad = locals.lastbcd & ~data & 0x0f;
    int ii;

    for (ii = 0; bcdLoad; ii++, bcdLoad>>=1)
      if (bcdLoad & 0x01) locals.bcd[ii] = data>>4;
    locals.lastbcd = (locals.lastbcd & 0x10) | (data & 0x0f);
  }
  locals.a0 = data;
  if (core_gameData->hw.gameSpecific1 & BY35GD_PHASE)
    by35_lampStrobe(locals.phaseA, locals.lampadr1);
  else {
    by35_lampStrobe(0, locals.lampadr1);
    if (core_gameData->hw.lampCol > 0) by35_lampStrobe(1,locals.lampadr2);
  }
}

/* PIA1:A-W  0,2-7 Display handling */
/*        W  1     Sound E */
static WRITE_HANDLER(pia1a_w) {
  if (locals.hw & BY35HW_SOUNDE) sndbrd_0_ctrl_w(1, (locals.cb21 ? 1 : 0) | (data & 0x02));

  if (!locals.ca20) {
    if (locals.hw & BY35HW_INVDISP4) {
      if (core_gameData->gen & GEN_BOWLING) {
        if (data & 0x02) {
          locals.bcd[5] = locals.a0>>4;
          by35_dispStrobe(0x20);
        }
        if (data & 0x04) {
          locals.bcd[6] = locals.a0>>4;
          by35_dispStrobe(0x40);
        }
      }
      if (~(locals.lastbcd>>4) & data & 0x01) { // positive edge
        locals.bcd[4] = locals.a0>>4;
        by35_dispStrobe(0x10);
      }
    }
    else if ((locals.lastbcd>>4) & ~data & 0x01) { // negative edge
      locals.bcd[4] = locals.a0>>4;
      by35_dispStrobe(0x10);
    }
    locals.lastbcd = (locals.lastbcd & 0x0f) | ((data & 0x01)<<4);
  }
  locals.a1 = data;
}

/* PIA0:B-R  Get Data depending on PIA0:A */
static READ_HANDLER(pia0b_r) {
  if (locals.a0 & 0x20) return core_getDip(0); // DIP#1 1-8
  if (locals.a0 & 0x40) return core_getDip(1); // DIP#2 9-16
  if (locals.a0 & 0x80) return core_getDip(2); // DIP#3 17-24
  if ((locals.hw & BY35HW_DIP4) && locals.cb20) return core_getDip(3); // DIP#4 25-32
  {
    UINT8 sw = core_getSwCol((locals.a0 & 0x1f) | ((locals.b1 & 0x80)>>2));
    return (locals.hw & BY35HW_REVSW) ? core_revbyte(sw) : sw;
  }
}

/* PIA0:CB2-W Lamp Strobe #1, DIPBank3 STROBE */
static WRITE_HANDLER(pia0cb2_w) {
  int sb = core_gameData->hw.soundBoard;		// ok
  if (locals.cb20 & ~data) locals.lampadr1 = locals.a0 & 0x0f;
  locals.cb20 = data;
//

   if (sb == SNDBRD_ST300V) {
      if (snddatst300.sampleisplaying) {
   	  pia_set_input_cb1(BY35_PIA1,0); }
       else {
   	  pia_set_input_cb1(BY35_PIA1,data);}
   }
}

/* PIA1:CA2-W Lamp Strobe #2 */
static WRITE_HANDLER(pia1ca2_w) {
  int sb = core_gameData->hw.soundBoard;		// ok
  if (locals.ca21 & ~data) {
    locals.lampadr2 = locals.a0 & 0x0f;
    if (core_gameData->hw.display & 0x01)
      { locals.bcd[6] = locals.a0>>4; by35_dispStrobe(0x40); }
  }
  if (locals.hw & BY35HW_SCTRL) sndbrd_0_ctrl_w(0, data);
  if ((sb == SNDBRD_ST300V) && (data)) {
    sndbrd_1_ctrl_w(0, locals.a0); // ok
  }
  locals.ca21 = locals.diagnosticLed = data;
}

/* PIA0:CA2-W Display Strobe */
static WRITE_HANDLER(pia0ca2_w) {
  locals.ca20 = data;
  if (data) by35_dispStrobe(0x0f);
}

/* PIA1:B-W Solenoid/Sound output */
static WRITE_HANDLER(pia1b_w) {
  int sb = core_gameData->hw.soundBoard;		// ok
  // check for extra display connected to solenoids
  if (~locals.b1 & data & core_gameData->hw.display & 0xf0)
    { locals.bcd[5] = locals.a0>>4; by35_dispStrobe(0x20); }
  locals.b1 = data;
  if ((sb & 0xff00) != SNDBRD_ST300 && sb != SNDBRD_ASTRO && (sb & 0xff00) != SNDBRD_ST100)  sndbrd_0_data_w(0, data & 0x0f); 	// ok
  coreGlobals.pulsedSolState = 0;
  if (!locals.cb21)
    locals.solenoids |= coreGlobals.pulsedSolState = (1<<(data & 0x0f)) & 0x7fff;
  data ^= 0xf0;
  coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xfff0ffff) | ((data & 0xf0)<<12);
  locals.solenoids |= (data & 0xf0)<<12;
}

/* PIA1:CB2-W Solenoid/Sound select */
static WRITE_HANDLER(pia1cb2_w) {
  int sb = core_gameData->hw.soundBoard;		// ok
  locals.cb21 = data;
  if (((locals.hw & BY35HW_SCTRL) == 0) && ((sb & 0xff00) != SNDBRD_ST300) && (sb != SNDBRD_ASTRO) && (sb & 0xff00) != SNDBRD_ST100)
   	// ok
    sndbrd_0_ctrl_w(1, (data ? 1 : 0) | (locals.a1 & 0x02));
}

static INTERRUPT_GEN(by35_vblank) {
  /*-------------------------------
  /  copy local data to interface
  /--------------------------------*/
  locals.vblankCount += 1;

  /*-- lamps --*/
  if ((locals.vblankCount % BY35_LAMPSMOOTH) == 0) {
    memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
    memset(coreGlobals.tmpLampMatrix, 0, sizeof(coreGlobals.tmpLampMatrix));
  }

  /*-- solenoids --*/
  if ((locals.vblankCount % BY35_SOLSMOOTH) == 0) {
    coreGlobals.solenoids = locals.solenoids;
    locals.solenoids = coreGlobals.pulsedSolState;
  }
  /*-- display --*/
  if ((locals.vblankCount % BY35_DISPLAYSMOOTH) == 0) {
    memcpy(coreGlobals.segments, locals.segments, sizeof(coreGlobals.segments));
    memcpy(locals.segments, locals.pseg, sizeof(locals.segments));
    memset(locals.pseg,0,sizeof(locals.pseg));
    coreGlobals.diagnosticLed = locals.diagnosticLed;
    locals.diagnosticLed = 0;
  }
  core_updateSw(core_getSol(core_gameData->gen & GEN_BYPROTO ? 18 : 19));
}

static SWITCH_UPDATE(by35) {
  if (inports) {
    if (core_gameData->gen & (GEN_BY17|GEN_BY35|GEN_STMPU100)) {
      CORE_SETKEYSW(inports[BY35_COMINPORT],   0x07,0);
      CORE_SETKEYSW(inports[BY35_COMINPORT],   0x60,1);
      CORE_SETKEYSW(inports[BY35_COMINPORT]>>8,0x87,2);
    }
    else if (core_gameData->gen & GEN_STMPU200) {
      CORE_SETKEYSW(inports[BY35_COMINPORT],   0x07,0);
      CORE_SETKEYSW(inports[BY35_COMINPORT],   0x60,1);
      CORE_SETKEYSW(inports[BY35_COMINPORT]>>8,0x87,1);
    }
    else if (core_gameData->gen & GEN_HNK) {
      CORE_SETKEYSW(inports[BY35_COMINPORT]>>8,0x03,0);
      CORE_SETKEYSW(inports[BY35_COMINPORT],   0x06,1);
      CORE_SETKEYSW(inports[BY35_COMINPORT],   0x81,2);
    }
    else if (core_gameData->gen & GEN_BYPROTO) {
      CORE_SETKEYSW(inports[BY35_COMINPORT]>>8,0x01,3);
      CORE_SETKEYSW(inports[BY35_COMINPORT],   0x1f,4);
    }
    else if (core_gameData->gen & GEN_ASTRO) {
      CORE_SETKEYSW(inports[BY35_COMINPORT],   0x07,0);
      CORE_SETKEYSW(inports[BY35_COMINPORT]>>8,0x03,1);
      CORE_SETKEYSW((inports[BY35_COMINPORT]&0x20)<<2,0x80,5);
    }
    else if (core_gameData->gen & GEN_BOWLING) {
      CORE_SETKEYSW(inports[BY35_COMINPORT],   0x07,0);
      CORE_SETKEYSW(inports[BY35_COMINPORT],   0x20,5);
      CORE_SETKEYSW(inports[BY35_COMINPORT]>>7,0x0e,5);
      CORE_SETKEYSW(inports[BY35_COMINPORT]>>15,0x01,5);
    }
  }
  if ((core_gameData->gen & GEN_BYPROTO) == 0) {
    /*-- Diagnostic buttons on CPU board --*/
    cpu_set_nmi_line(0, core_getSw(BY35_SWCPUDIAG) ? ASSERT_LINE : CLEAR_LINE);
    sndbrd_0_diag(core_getSw(BY35_SWSOUNDDIAG));
    /*-- coin door switches --*/
    pia_set_input_ca1(BY35_PIA0, !core_getSw(BY35_SWSELFTEST));
  }
}

/* PIA 0 (U10)
PA0-1: (o) Cabinet Switches Strobe
PA0-4: (o) Switch Strobe(Columns)
PA0-3: (o) Lamp Address (Shared with Switch Strobe)
PA0-3: (o) Display Latch Strobe
PA4-7: (o) Lamp Data & BCD Display Data
PA5:   (o) S01-S08 Dips
PA6:   (o) S09-S16 Dips
PA7:   (o) S17-S24 Dips
PB0-7: (i) Switch Returns/Rows and Cabinet Switch Returns/Rows
PB0-7: (i) Dips returns
CA1:   (i) Self Test Switch
CB1:   (i) Zero Cross Detection
CA2:   (o) Display blanking
CB2:   (o) S25-S32 Dips, Lamp strobe #1
*/
/* PIA 1 (U11)
PA0:   (o) 5th display strobe
PA1:   (o) Sound Module Address E / 7th display digit
PA2-7: (o) Display digit select
PB0-3: (o) Momentary Solenoid/Sound Data
PB4-7: (o) Continuous Solenoid
CA1:   (i) Display Interrupt Generator
CA2:   (o) Diag LED + Lamp Strobe #2/Sound select
CB1:   ?
CB2:   (o) Solenoid/Sound Bank Select
*/
static struct pia6821_interface by35_pia[] = {{
/* I:  A/B,CA1/B1,CA2/B2 */  0, pia0b_r, PIA_UNUSED_VAL(1),PIA_UNUSED_VAL(1), 0,0,
/* O:  A/B,CA2/B2        */  pia0a_w,0, pia0ca2_w,pia0cb2_w,
/* IRQ: A/B              */  piaIrq,piaIrq
},{
/* I:  A/B,CA1/B1,CA2/B2 */  0,0, 0,0, 0,0,
/* O:  A/B,CA2/B2        */  pia1a_w,pia1b_w,pia1ca2_w,pia1cb2_w,
/* IRQ: A/B              */  piaIrq,piaIrq
}};

static INTERRUPT_GEN(by35_irq) {
  static int last = 0; pia_set_input_ca1(BY35_PIA1, last = !last);
}

static void by35_zeroCross(int data) {
    pia_set_input_cb1(BY35_PIA0, locals.phaseA = !locals.phaseA);
}


/* Bally Prototype changes below.
   Note there is no extra display for ball in play,
   they just used 5 lights on the backglass.
   Also the lamps are accessed along with the displays!
   Since this required a different lamp strobing,
   I introduced a new way of arranging the lamps which
   makes it easier to map the lights, and saves a row.
 */

static void by35p_lampStrobe(void) {
  int strobe = locals.a1 >> 2;
  int ii,jj;
  for (ii = 0; strobe; ii++, strobe>>=1) {
    if (strobe & 0x01)
      for (jj = 0; jj < 5; jj++) {
        int lampdata = (locals.bcd[jj]>>4)^0x0f;
        int lampadr = ii*5 + jj;
        coreGlobals.tmpLampMatrix[lampadr/2] |= (lampadr%2 ? lampdata << 4 : lampdata);
      }
  }
}
// buffer lamps & display digits
static WRITE_HANDLER(piap0a_w) {
  locals.a0 = data;
  if (!locals.ca20 && locals.lastbcd)
    locals.bcd[--locals.lastbcd] = data;

  if (locals.ca21) logerror("%04x: speechboard active ! data  %02x \n", activecpu_get_previouspc(),data);

}
// switches & dips (inverted)
static READ_HANDLER(piap0b_r) {
  UINT8 sw = 0;
  if (locals.a0 & 0x10) sw = core_getDip(0); // DIP#1 1-8
  else if (locals.a0 & 0x20) sw = core_getDip(1); // DIP#2 9-16
  else if (locals.a0 & 0x40) sw = core_getDip(2); // DIP#3 17-24
  else if (locals.a0 & 0x80) sw = core_getDip(3); // DIP#4 25-32
  else sw = core_getSwCol(locals.a0 & 0x0f);
  return core_revbyte(sw);
}
// display strobe
static WRITE_HANDLER(piap0ca2_w) {
  if (data & ~locals.ca20) {
    locals.lastbcd = 5;
  } else if (~data & locals.ca20) {
    by35_dispStrobe(0x1f);
    by35p_lampStrobe();
  }
  locals.ca20 = data;
}
// set display row
static WRITE_HANDLER(piap1a_w) {
  locals.a1 = data;
  if (data & 0x01) logerror("PIA#1 Port A = %02X\n", data);
}
// solenoids
static WRITE_HANDLER(piap1b_w) {
  locals.b1 = data;
  coreGlobals.pulsedSolState = 0;
  if (locals.cb21)
    locals.solenoids |= coreGlobals.pulsedSolState = (1<<(data & 0x0f)) & 0x7fff;
  data ^= 0xf0;
  coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xfff87fff) | ((data & 0xf0)<<11);
  locals.solenoids |= (data & 0xf0)<<11;
}
//diag. LED
static WRITE_HANDLER(piap1ca2_w) {
  locals.ca21 = locals.diagnosticLed = data;
}
// solenoid control?
static WRITE_HANDLER(piap1cb2_w) {
  locals.cb21 = data;
}

static struct pia6821_interface by35Proto_pia[] = {{
/* I:  A/B,CA1/B1,CA2/B2 */  0, piap0b_r, 0,0, 0,0,
/* O:  A/B,CA2/B2        */  piap0a_w,0, piap0ca2_w,0,
/* IRQ: A/B              */  piaIrq,0
},{
/* I:  A/B,CA1/B1,CA2/B2 */  0, 0, 0,0, 0,0,
/* O:  A/B,CA2/B2        */  piap1a_w,piap1b_w, piap1ca2_w,piap1cb2_w,
/* IRQ: A/B              */  piaIrq,0
}};

static INTERRUPT_GEN(byProto_irq) {
  pia_set_input_ca1(BY35_PIA0, 0); pia_set_input_ca1(BY35_PIA0, 1);
}

static void by35p_zeroCross(int data) {
  pia_set_input_ca1(BY35_PIA1, 0); pia_set_input_ca1(BY35_PIA1, 1);
}


/*-----------------------------------------------
/ Load/Save static ram
/-------------------------------------------------*/
static UINT8 *by35_CMOS;

static NVRAM_HANDLER(by35) {
  core_nvram(file, read_or_write, by35_CMOS, 0x100, (core_gameData->gen & (GEN_STMPU100|GEN_STMPU200|GEN_BYPROTO))?0x00:0xff);
}
// Bally only uses top 4 bits
static WRITE_HANDLER(by35_CMOS_w) { by35_CMOS[offset] = data | 0x0f; }

// These games use the A0 memory address for extra sound solenoids only.
WRITE_HANDLER(extra_sol_w) {
  logerror("%04x: extra sol w (a0)  data  %02x \n", activecpu_get_previouspc(),data);
  sndbrd_0_data_w(0,data);
//if (data != 0)
//  coreGlobals.pulsedSolState = (data << 24);
//locals.solenoids = (locals.solenoids & 0x00ffffff) | coreGlobals.pulsedSolState;
}

WRITE_HANDLER(stern100_sol_w) {
  logerror("%04x: stern100_sol (c0) data  %02x \n", activecpu_get_previouspc(),data);
//extra_sol_w(offset, data ^ 0xff);
}

static MACHINE_INIT(by35) {
  int sb = core_gameData->hw.soundBoard;
  memset(&locals, 0, sizeof(locals));

  pia_config(BY35_PIA0, PIA_STANDARD_ORDERING, &by35_pia[0]);
  pia_config(BY35_PIA1, PIA_STANDARD_ORDERING, &by35_pia[1]);
//   if ((sb & 0xff00) != SNDBRD_ST300)		// ok
  sndbrd_0_init(sb, 1, memory_region(REGION_SOUND1), NULL, NULL);
// do the voice boards...
  if (sb == SNDBRD_ST300V)   sndbrd_1_init(sb, 1, memory_region(REGION_SOUND1), NULL, NULL);
//
  locals.vblankCount = 1;
  // set up hardware
  if (core_gameData->gen & (GEN_BY17|GEN_BOWLING)) {
    locals.hw = BY35HW_INVDISP4|BY35HW_DIP4;
    install_mem_write_handler(0,0x0200, 0x02ff, by35_CMOS_w);
    locals.bcd2seg = core_bcd2seg;
  }
  else if (core_gameData->gen & GEN_BY35) {
    locals.hw = BY35HW_DIP4;
    if ((core_gameData->hw.gameSpecific1 & BY35GD_NOSOUNDE) == 0)
      locals.hw |= BY35HW_SOUNDE;
    install_mem_write_handler(0,0x0200, 0x02ff, by35_CMOS_w);
    locals.bcd2seg = core_bcd2seg;
  }
  else if (core_gameData->gen & (GEN_STMPU100|GEN_STMPU200|GEN_ASTRO)) {
    locals.hw = BY35HW_INVDISP4|BY35HW_DIP4;
    locals.bcd2seg = core_bcd2seg;
  }
  else if (core_gameData->gen & GEN_HNK) {
    locals.hw = BY35HW_REVSW|BY35HW_SCTRL|BY35HW_INVDISP4;
    install_mem_write_handler(0,0x0200, 0x02ff, by35_CMOS_w);
    locals.bcd2seg = core_bcd2seg9;
  }


  if ((sb & 0xff00) == SNDBRD_ST300 || sb == SNDBRD_ASTRO) {
    install_mem_write_handler(0,0x00a0, 0x00a7, snd300_w);	// ok
    install_mem_read_handler (0,0x00a0, 0x00a7, snd300_r);    	// ok
    install_mem_write_handler(0,0x00c0, 0x00c0, snd300_wex);	// ok
  } else if (sb == SNDBRD_ST100) {
    install_mem_write_handler(0,0x00a0, 0x00a0, extra_sol_w); // sounds on (DIP 23 = 1)
    install_mem_write_handler(0,0x00c0, 0x00c0, stern100_sol_w); // chimes on (DIP 23 = 0)
  } else if (sb == SNDBRD_ST100B) {
    install_mem_write_handler(0,0x00a0, 0x00a0, extra_sol_w);
  }
}

static MACHINE_INIT(by35Proto) {
  memset(&locals, 0, sizeof(locals));

  pia_config(BY35_PIA0, PIA_STANDARD_ORDERING, &by35Proto_pia[0]);
  pia_config(BY35_PIA1, PIA_STANDARD_ORDERING, &by35Proto_pia[1]);
  locals.vblankCount = 1;
  // set up hardware
  locals.hw = BY35HW_REVSW|BY35HW_INVDISP4|BY35HW_DIP4;
  install_mem_write_handler(0,0x0200, 0x02ff, by35_CMOS_w);
  locals.bcd2seg = core_bcd2seg;
}

static MACHINE_RESET(by35) { pia_reset(); }
static MACHINE_STOP(by35) {
  if ((core_gameData->hw.soundBoard & 0xff00) != SNDBRD_ST300 && core_gameData->hw.soundBoard != SNDBRD_ASTRO)
    sndbrd_0_exit();
}

/*-----------------------------------
/  Memory map for CPU board
/------------------------------------*/
/* Roms: U1: 0x1000-0x1800
         U3: 0x1800-0x2000??
	 U4: 0x2000-0x2800??
	 U5: 0x2800-0x3000??
	 U2: 0x5000-0x5800
	 U6: 0x5800-0x6000
*/
static MEMORY_READ_START(by35_readmem)
  { 0x0000, 0x0080, MRA_RAM }, /* U7 128 Byte Ram*/
  { 0x0200, 0x02ff, MRA_RAM }, /* CMOS Battery Backed*/
  { 0x0088, 0x008b, pia_r(BY35_PIA0) }, /* U10 PIA: Switchs + Display + Lamps*/
  { 0x0090, 0x0093, pia_r(BY35_PIA1) }, /* U11 PIA: Solenoids/Sounds + Display Strobe */
  { 0x1000, 0x1fff, MRA_ROM },
  { 0x5000, 0x5fff, MRA_ROM },
  { 0xf000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(by35_writemem)
  { 0x0000, 0x0080, MWA_RAM }, /* U7 128 Byte Ram*/
  { 0x0200, 0x02ff, MWA_RAM, &by35_CMOS }, /* CMOS Battery Backed*/
  { 0x0088, 0x008b, pia_w(BY35_PIA0) }, /* U10 PIA: Switchs + Display + Lamps*/
  { 0x0090, 0x0093, pia_w(BY35_PIA1) }, /* U11 PIA: Solenoids/Sounds + Display Strobe */
  { 0x1000, 0x1fff, MWA_ROM },
  { 0x5000, 0x5fff, MWA_ROM },
  { 0xf000, 0xffff, MWA_ROM },
MEMORY_END
 
MACHINE_DRIVER_START(by35)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(by35,by35,by35)
  MDRV_CPU_ADD_TAG("mcpu", M6800, 450000)
  MDRV_CPU_MEMORY(by35_readmem, by35_writemem)
  MDRV_CPU_VBLANK_INT(by35_vblank, 1)
  MDRV_CPU_PERIODIC_INT(by35_irq, BY35_IRQFREQ)
  MDRV_NVRAM_HANDLER(by35)
  MDRV_DIPS(32)
  MDRV_SWITCH_UPDATE(by35)
  MDRV_DIAGNOSTIC_LEDH(1)
  MDRV_TIMER_ADD(by35_zeroCross,BY35_ZCFREQ*2)
MACHINE_DRIVER_END
  
MACHINE_DRIVER_START(byProto)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(by35Proto,by35,NULL)
  MDRV_CPU_ADD_TAG("mcpu", M6800, 560000)
  MDRV_CPU_MEMORY(by35_readmem, by35_writemem)
  MDRV_CPU_VBLANK_INT(by35_vblank, 1)
  MDRV_CPU_PERIODIC_INT(byProto_irq, BY35_IRQFREQ)
  MDRV_NVRAM_HANDLER(by35)
  MDRV_DIPS(32)
  MDRV_SWITCH_UPDATE(by35)
  MDRV_DIAGNOSTIC_LEDH(1)
  MDRV_TIMER_ADD(by35p_zeroCross, 120) // won't work with 100Hz, ie. not in Europe!
MACHINE_DRIVER_END

MACHINE_DRIVER_START(st100s)
  MDRV_IMPORT_FROM(by35)
  MDRV_IMPORT_FROM(st100)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(by35_32S)
  MDRV_IMPORT_FROM(by35)
  MDRV_IMPORT_FROM(by32)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(by35_51S)
  MDRV_IMPORT_FROM(by35)
  MDRV_IMPORT_FROM(by51)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(by35_56S)
  MDRV_IMPORT_FROM(by35)
  MDRV_IMPORT_FROM(by56)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(by35_61S)
  MDRV_IMPORT_FROM(by35)
  MDRV_IMPORT_FROM(by61)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(by35_45S)
  MDRV_IMPORT_FROM(by35)
  MDRV_IMPORT_FROM(by45)
MACHINE_DRIVER_END

#ifdef MAME_DEBUG
MACHINE_DRIVER_START(by6802_61S)
  MDRV_IMPORT_FROM(by35)
  MDRV_CPU_REPLACE("mcpu",M6802, 375000)
  MDRV_CPU_PERIODIC_INT(by35_irq, BY35_6802IRQFREQ)
  MDRV_IMPORT_FROM(by61)
MACHINE_DRIVER_END
#endif

MACHINE_DRIVER_START(by6802_45S)
  MDRV_IMPORT_FROM(by35)
  MDRV_CPU_REPLACE("mcpu",M6802, 375000)
  MDRV_CPU_PERIODIC_INT(by35_irq, BY35_6802IRQFREQ)
  MDRV_IMPORT_FROM(by45)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START(st200NS)
  MDRV_IMPORT_FROM(by35)
  MDRV_CPU_REPLACE("mcpu",M6800, 1000000)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(st200)
  MDRV_IMPORT_FROM(st200NS)
  MDRV_IMPORT_FROM(st300)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(st200f2k)
  MDRV_IMPORT_FROM(st200NS)
  MDRV_IMPORT_FROM(st300f2k)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(st200fal)
  MDRV_IMPORT_FROM(st200NS)
  MDRV_IMPORT_FROM(st300fal)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(st200lit)
  MDRV_IMPORT_FROM(st200NS)
  MDRV_IMPORT_FROM(st300lit)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(st200sec)
  MDRV_IMPORT_FROM(st200NS)
  MDRV_IMPORT_FROM(st300sec)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(st200cat)
  MDRV_IMPORT_FROM(st200NS)
  MDRV_IMPORT_FROM(st300cat)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(st200orb)
  MDRV_IMPORT_FROM(st200NS)
  MDRV_IMPORT_FROM(st300orb)
MACHINE_DRIVER_END


MACHINE_DRIVER_START(hnk)
  MDRV_IMPORT_FROM(by35)
  MDRV_CPU_MODIFY("mcpu")
  MDRV_CPU_PERIODIC_INT(NULL, 0) // no irq
  MDRV_DIPS(24)
  MDRV_IMPORT_FROM(hnks)
MACHINE_DRIVER_END
