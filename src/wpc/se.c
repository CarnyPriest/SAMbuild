/************************************************************************************************
  Sega/Stern Pinball
  by Steve Ellenoff

  Additional work by Martin Adrian, Gerrit Volkenborn, Tom Behrens
************************************************************************************************

Hardware from 1994-present

  CPU Boards: Whitestar System

  Display Boards:
    ??

  Sound Board Revisions:
    Integrated with CPU. Similar cap. as 520-5126-xx Series: (Baywatch to Batman Forever)

    11/2003 -New CPU/Sound board (520-5300-00) with an Atmel AT91 (ARM7DMI Variant) CPU for sound & Xilinx FPGA
             emulating the BSMT2000 as well as added 16 bit sample playback and ADPCM compression.

Issues:
DMD Timing is still wrong.. FIRQ rate is variable, and it's not fully understood.

*************************************************************************************************/
#include <stdarg.h>
#include "driver.h"
#include "cpu/m6809/m6809.h"
#include "core.h"
#include "sndbrd.h"
#include "dedmd.h"
#include "se.h"
#include "desound.h"
#include "cpu/at91/at91.h"
#include "sound/wavwrite.h"

#define SE_VBLANKFREQ      60 /* VBLANK frequency */
#define SE_IRQFREQ        976 /* FIRQ Frequency according to Theory of Operation */
#define SE_ROMBANK0         1

#define SE_SOLSMOOTH       4 /* Smooth the Solenoids over this numer of VBLANKS */
#define SE_LAMPSMOOTH      2 /* Smooth the lamps over this number of VBLANKS */
#define SE_DISPLAYSMOOTH   2 /* Smooth the display over this number of VBLANKS */

#define SUPPORT_TRACERAM 0

static NVRAM_HANDLER(se);
static WRITE_HANDLER(mcpu_ram8000_w);
static READ_HANDLER(mcpu_ram8000_r);

/*----------------
/ Local variables
/-----------------*/
struct {
  int    vblankCount;
  int    initDone;
  UINT32 solenoids;
  int    lampRow, lampColumn;
  int    diagnosticLed;
  int    swCol;
  int	 flipsol, flipsolPulse;
  int    dmdStatus;
  UINT8 *ram8000;
  int    auxdata;
  /* Mini DMD stuff */
  int    lastgiaux, miniidx, miniframe;
  int    minidata[7], minidmd[4][3][8];
  /* trace ram related */
#if SUPPORT_TRACERAM
  UINT8 *traceRam;
#endif
  UINT8  curBank;                   /* current bank select */
  #define TRACERAM_SELECTED 0x10    /* this bit set maps trace ram to 0x0000-0x1FFF */
} selocals;

static INTERRUPT_GEN(se_vblank) {
  /*-------------------------------
  /  copy local data to interface
  /--------------------------------*/
  selocals.vblankCount = (selocals.vblankCount+1) % 16;

  /*-- lamps --*/
  if ((selocals.vblankCount % SE_LAMPSMOOTH) == 0) {
    memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
    memset(coreGlobals.tmpLampMatrix, 0, 10);
  }
  /*-- solenoids --*/
  coreGlobals.solenoids2 = selocals.flipsol; selocals.flipsol = selocals.flipsolPulse;
  if ((selocals.vblankCount % SE_SOLSMOOTH) == 0) {
    coreGlobals.solenoids = selocals.solenoids;
    selocals.solenoids = coreGlobals.pulsedSolState;
  }
  /*-- display --*/
  if ((selocals.vblankCount % SE_DISPLAYSMOOTH) == 0) {
    coreGlobals.diagnosticLed = selocals.diagnosticLed;
    selocals.diagnosticLed = 0;
  }
  core_updateSw(TRUE); /* flippers are CPU controlled */
}

static SWITCH_UPDATE(se) {
  if (inports) {
    /*Switch Col 0 = Dedicated Switches - Coin Door Only - Begin at 6th Spot*/
    CORE_SETKEYSW(inports[SE_COMINPORT]<<4, 0xe0, 0);
    /*Switch Col 1 = Coin Switches - (Switches begin at 4th Spot)*/
    CORE_SETKEYSW(inports[SE_COMINPORT]>>5, 0x78, 1);
    /*Copy Start, Tilt, and Slam Tilt to proper position in Matrix: Switches 54,55,56*/
    /*Clear bits 6,7,8 first*/
    CORE_SETKEYSW(inports[SE_COMINPORT]<<1, 0xe0, 7);
  }
}

static MACHINE_INIT(se3) {
	sndbrd_0_init(SNDBRD_DEDMD32, 2, memory_region(DE_DMD32ROMREGION),NULL,NULL);
	sndbrd_1_init(SNDBRD_DE3S,    1, memory_region(DE2S_ROMREGION), NULL, NULL);
}

static MACHINE_INIT(se) {
  sndbrd_0_init(SNDBRD_DEDMD32, 2, memory_region(DE_DMD32ROMREGION),NULL,NULL);
  sndbrd_1_init(SNDBRD_DE2S,    1, memory_region(DE2S_ROMREGION), NULL, NULL);

  // Sharkeys got some extra ram
  if (core_gameData->gen & GEN_WS_1) {
    selocals.ram8000 = install_mem_write_handler(0,0x8000,0x81ff,mcpu_ram8000_w);
                       install_mem_read_handler (0,0x8000,0x81ff,mcpu_ram8000_r);
  }
  selocals.miniidx = selocals.lastgiaux = 0;

#if SUPPORT_TRACERAM
  selocals.traceRam = 0;
  if (core_gameData->gen & GEN_WS_1) {
    selocals.traceRam = malloc(0x2000);
    if (selocals.traceRam) memset(selocals.traceRam,0,0x2000);
  }
#endif
}

static MACHINE_STOP(se) {
  sndbrd_0_exit(); sndbrd_1_exit();
#if SUPPORT_TRACERAM
  if (selocals.traceRam) free(selocals.traceRam);
#endif
}

/*-- Main CPU Bank Switch --*/
// D0-D5 Bank Address
// D6    Unused
// D7    Diagnostic LED
static WRITE_HANDLER(mcpu_bank_w) {
  selocals.curBank = data;   /* keep track of current bank select for trace ram */
  // Should be 0x3f but memreg is only 512K */
  cpu_setbank(SE_ROMBANK0, memory_region(SE_ROMREGION) + (data & 0x1f)* 0x4000);
  selocals.diagnosticLed = data>>7;
}

// Ram 0x0000-0x1FFF Read Handler
static READ_HANDLER(ram_r) {
  if (   (core_gameData->gen & GEN_WS_1)
      && (selocals.curBank & TRACERAM_SELECTED)) {
    DBGLOG(("Read access to traceram[%04X] from pc=%04X",offset,activecpu_get_previouspc()));
#if SUPPORT_TRACERAM
    return selocals.traceRam ? selocals.traceRam[offset] : 0;
#else
    return 0;
#endif
  }
  else
    return memory_region(SE_CPUREGION)[offset];
}

// Ram 0x0000-0x1FFF Write Handler
static WRITE_HANDLER(ram_w) {
  if (   (core_gameData->gen & GEN_WS_1)
      && (selocals.curBank & TRACERAM_SELECTED)) {
#if SUPPORT_TRACERAM
    if (selocals.traceRam) selocals.traceRam[offset] = data;
#else
    return;
#endif
  }
  else
    memory_region(SE_CPUREGION)[offset] = data;
}

/* Sharkey's ShootOut got some ram at 0x8000-0x81ff */
static WRITE_HANDLER(mcpu_ram8000_w) { selocals.ram8000[offset] = data; }
static READ_HANDLER(mcpu_ram8000_r) { return selocals.ram8000[offset]; }

/*-- Lamps --*/
static WRITE_HANDLER(lampdriv_w) {
  selocals.lampRow = core_revbyte(data);
  core_setLamp(coreGlobals.tmpLampMatrix, selocals.lampColumn, selocals.lampRow);
}
static WRITE_HANDLER(lampstrb_w) { core_setLamp(coreGlobals.tmpLampMatrix, selocals.lampColumn = (selocals.lampColumn & 0xff00) | data, selocals.lampRow);}
static WRITE_HANDLER(auxlamp_w) { core_setLamp(coreGlobals.tmpLampMatrix, selocals.lampColumn = (selocals.lampColumn & 0x00ff) | (data<<8), selocals.lampRow);}
static WRITE_HANDLER(gilamp_w) {
  logerror("GI lamps %d=%02x\n", offset, data);
  coreGlobals.tmpLampMatrix[10 + offset] = data;
}
static READ_HANDLER(gilamp_r) { return coreGlobals.tmpLampMatrix[10 + offset]; }

/*-- Switches --*/
static READ_HANDLER(switch_r)	{ return ~core_getSwCol(selocals.swCol); }
static WRITE_HANDLER(switch_w)	{ selocals.swCol = data; }

/*-- Dedicated Switches --*/
// Note: active low
// D0 - DED #1 - Left Flipper
// D1 - DED #2 - Left Flipper EOS
// D2 - DED #3 - Right Flipper
// D3 - DED #4 - Right Flipper EOS
// D4 - DED #5 - Not Used (Upper Flipper on some games!)
// D5 - DED #6 - Volume (Red Button)
// D6 - DED #7 - Service Credit (Green Button)
// D7 - DED #8 - Begin Test (Black Button)
static READ_HANDLER(dedswitch_r) {
  /* CORE Defines flippers in order as: RFlipEOS, RFlip, LFlipEOS, LFlip*/
  /* We need to adjust to: LFlip, LFlipEOS, RFlip, RFlipEOS*/
  /* Swap the 4 lowest bits*/
  UINT8 cds = (coreGlobals.swMatrix[0] & 0xe0);        /* BGRx xxxx */
  UINT8 fls = coreGlobals.swMatrix[CORE_FLIPPERSWCOL]; /* Uxxx LxRx */
  fls = core_revnyb(fls & 0x0f) | ((fls & 0x80)>>3);   /* xxxU xRxL */
  return ~(cds | fls);
}

/*-- Dip Switch SW300 - Country Settings --*/
static READ_HANDLER(dip_r) { return ~core_getDip(0); }

/*-- Solenoids --*/
static WRITE_HANDLER(solenoid_w) {
  static const int solmaskno[] = { 8, 0, 16, 24 };
  UINT32 mask = ~(0xff<<solmaskno[offset]);
  UINT32 sols = data<<solmaskno[offset];
  if (offset == 0) { /* move flipper power solenoids (L=15,R=16) to (R=45,L=47) */
    selocals.flipsol |= selocals.flipsolPulse = ((data & 0x80)>>7) | ((data & 0x40)>>4);
    sols &= 0xffff3fff; /* mask off flipper solenoids */
  }
  coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & mask) | sols;
  selocals.solenoids |= sols;
}
/*-- DMD communication --*/
static WRITE_HANDLER(dmdlatch_w) {
  sndbrd_0_data_w(0,data);
  sndbrd_0_ctrl_w(0,0); sndbrd_0_ctrl_w(0,1);
}
static WRITE_HANDLER(dmdreset_w) {
  sndbrd_0_ctrl_w(0,data?0x02:0x00);
}
static READ_HANDLER(dmdstatus_r) {
  return (sndbrd_0_data_r(0) ? 0x80 : 0x00) | (sndbrd_0_ctrl_r(0)<<3);
}

static READ_HANDLER(dmdie_r) { /*What is this for?*/
//DBGLOG(("DMD Input Enable Read PC=%x\n",activecpu_get_previouspc()));
  return 0x00;
}

static READ_HANDLER(auxboard_r) { return selocals.auxdata; }
static WRITE_HANDLER(auxboard_w) { selocals.auxdata = data; }

static WRITE_HANDLER(giaux_w) {
  if (core_gameData->hw.display & SE_MINIDMD) {
    if (data & ~selocals.lastgiaux & 0x80) { /* clock in data to minidmd */
      selocals.minidata[selocals.miniidx] = selocals.auxdata & 0x7f;
      selocals.miniidx = (selocals.miniidx + 1) % 4;
      if (!(selocals.auxdata & 0x80)) { /* enabled column? */
        int tmp = selocals.minidata[selocals.miniidx] & 0x1f;
        if (tmp) {
          int col = 1;
          while (tmp >>= 1) col += 1;
          selocals.minidmd[selocals.miniframe][0][col-1] = selocals.minidata[(selocals.miniidx + 1) % 4];
          selocals.minidmd[selocals.miniframe][1][col-1] = selocals.minidata[(selocals.miniidx + 2) % 4];
          selocals.minidmd[selocals.miniframe][2][col-1] = selocals.minidata[(selocals.miniidx + 3) % 4];
          // Should find a better way to detect different frames but columns seem
          // to always be updated in order 1-5 so this works
          if (col == 5) selocals.miniframe = (selocals.miniframe + 1) % 3;
        }
      }
    }
    selocals.lastgiaux = data;
  }
  else if (core_gameData->hw.display & SE_MINIDMD2) {
    int ii, bits;
    if (data & ~selocals.lastgiaux & 0x80) { /* clock in data to minidmd */
      selocals.miniidx = (selocals.miniidx + 1) % 7;
      selocals.minidata[selocals.miniidx] = selocals.auxdata & 0x7f;
      if ((selocals.auxdata & 0x80) == 0) { /* enabled column? */
        for (ii=0, bits = 0x01; ii < 14; ii++, bits <<= 1) {
          if (bits & ((selocals.minidata[3] << 7) | selocals.minidata[4])) {
            selocals.minidmd[0][ii/7][ii%7] = selocals.minidata[0];
            selocals.minidmd[1][ii/7][ii%7] = selocals.minidata[1];
            selocals.minidmd[2][ii/7][ii%7] = selocals.minidata[5];
            selocals.minidmd[3][ii/7][ii%7] = selocals.minidata[6];
            break;
          }
        }
      }
    }
    selocals.lastgiaux = data;
  }
  else if (core_gameData->hw.display & SE_LED) { // map LEDs as extra lamp columns
    static int order[] = { 6, 2, 4, 5, 1, 3, 0 };
    if (selocals.auxdata == 0x30 && (selocals.lastgiaux & 0x40)) selocals.miniidx = 0;
    if (data == 0x7e) {
      if (order[selocals.miniidx])
        coreGlobals.tmpLampMatrix[9 + order[selocals.miniidx]] = selocals.auxdata;
      if (selocals.miniidx < 6) selocals.miniidx++;
    }
    selocals.lastgiaux = selocals.auxdata;
  }
}

// MINI DMD Type 1 (HRC) (15x7)
PINMAME_VIDEO_UPDATE(seminidmd1_update) {
  tDMDDot dotCol;
  int ii,jj,kk,bits;
  UINT16 *seg = &coreGlobals.drawSeg[0];

  for (ii = 0, bits = 0x40; ii < 7; ii++, bits >>= 1) {
    UINT8 *line = &dotCol[ii+1][0];
    for (jj = 2; jj >= 0; jj--)
      for (kk = 0; kk < 5; kk++) {
        *line++ = ((selocals.minidmd[0][jj][kk] & bits) + (selocals.minidmd[1][jj][kk] & bits) +
                   (selocals.minidmd[2][jj][kk] & bits))/bits;
      }
  }
  for (ii = 0; ii < 15; ii++) {
    bits = 0;
    for (jj = 0; jj < 7; jj++)
      bits = (bits<<2) | dotCol[jj+1][ii];
    *seg++ = bits;
  }
  video_update_core_dmd(bitmap, cliprect, dotCol, layout);
  return 0;
}
// MINI DMD Type 1 (Ripley's) (3 x 5x7)
PINMAME_VIDEO_UPDATE(seminidmd1a_update) {
  tDMDDot dotCol;
  int ii,kk,bits;
  UINT16 *seg = &coreGlobals.drawSeg[0];

  for (ii = 0, bits = 0x40; ii < 7; ii++, bits >>= 1) {
    UINT8 *line = &dotCol[ii+1][0];
    for (kk = 0; kk < 5; kk++)
      *line++ = ((selocals.minidmd[0][2][kk] & bits) + (selocals.minidmd[1][2][kk] & bits) +
                 (selocals.minidmd[2][2][kk] & bits))/bits;
  }
  for (ii = 0; ii < 5; ii++) {
    bits = 0;
    for (kk = 0; kk < 7; kk++)
      bits = (bits<<2) | dotCol[kk+1][ii];
    *seg++ = bits;
  }
  video_update_core_dmd(bitmap, cliprect, dotCol, layout);
  return 0;
}
PINMAME_VIDEO_UPDATE(seminidmd1b_update) {
  tDMDDot dotCol;
  int ii,kk,bits;
  UINT16 *seg = &coreGlobals.drawSeg[5];

  for (ii = 0, bits = 0x40; ii < 7; ii++, bits >>= 1) {
    UINT8 *line = &dotCol[ii+1][0];
    for (kk = 0; kk < 5; kk++)
      *line++ = ((selocals.minidmd[0][1][kk] & bits) + (selocals.minidmd[1][1][kk] & bits) +
                 (selocals.minidmd[2][1][kk] & bits))/bits;
  }
  for (ii = 0; ii < 5; ii++) {
    bits = 0;
    for (kk = 0; kk < 7; kk++)
      bits = (bits<<2) | dotCol[kk+1][ii];
    *seg++ = bits;
  }
  video_update_core_dmd(bitmap, cliprect, dotCol, layout);
  return 0;
}
PINMAME_VIDEO_UPDATE(seminidmd1c_update) {
  tDMDDot dotCol;
  int ii,kk,bits;
  UINT16 *seg = &coreGlobals.drawSeg[10];

  for (ii = 0, bits = 0x40; ii < 7; ii++, bits >>= 1) {
    UINT8 *line = &dotCol[ii+1][0];
    for (kk = 0; kk < 5; kk++)
      *line++ = ((selocals.minidmd[0][0][kk] & bits) + (selocals.minidmd[1][0][kk] & bits) +
                 (selocals.minidmd[2][0][kk] & bits))/bits;
  }
  for (ii = 0; ii < 5; ii++) {
    bits = 0;
    for (kk = 0; kk < 7; kk++)
      bits = (bits<<2) | dotCol[kk+1][ii];
    *seg++ = bits;
  }
  video_update_core_dmd(bitmap, cliprect, dotCol, layout);
  return 0;
}
// MINI DMD Type 2 (Monopoly) (15x7)
PINMAME_VIDEO_UPDATE(seminidmd2_update) {
  tDMDDot dotCol;
  int ii,jj,kk,bits;
  UINT16 *seg = &coreGlobals.drawSeg[0];

  for (ii = 0, bits = 0x01; ii < 7; ii++, bits <<= 1) {
    UINT8 *line = &dotCol[ii+1][0];
    for (jj = 0; jj < 3; jj++)
      for (kk = 4; kk >= 0; kk--)
        *line++ = ((selocals.minidmd[0][jj][kk] & bits) + (selocals.minidmd[1][jj][kk] & bits) +
                   (selocals.minidmd[2][jj][kk] & bits))/bits;
  }
  for (ii = 0; ii < 15; ii++) {
    bits = 0;
    for (jj = 0; jj < 7; jj++)
      bits = (bits<<2) | dotCol[jj+1][ii];
    *seg++ = bits;
  }
  video_update_core_dmd(bitmap, cliprect, dotCol, layout);
  return 0;
}
// MINI DMD Type 3 (RCT) (21x5)
PINMAME_VIDEO_UPDATE(seminidmd3_update) {
  tDMDDot dotCol;
  int ii,jj,kk,bits;
  UINT16 *seg = &coreGlobals.drawSeg[0];

  memset(&dotCol,0,sizeof(dotCol));
  for (kk = 0; kk < 5; kk++) {
    UINT8 *line = &dotCol[kk+1][0];
    for (jj = 0; jj < 3; jj++)
      for (ii = 0, bits = 0x01; ii < 7; ii++, bits <<= 1)
        *line++ = ((selocals.minidmd[0][jj][kk] & bits) + (selocals.minidmd[1][jj][kk] & bits) +
                   (selocals.minidmd[2][jj][kk] & bits))/bits;
  }
  for (ii = 0; ii < 21; ii++) {
    bits = 0;
    for (jj = 0; jj < 5; jj++)
      bits = (bits<<2) | dotCol[jj+1][ii];
    *seg++ = bits;
  }
  video_update_core_dmd(bitmap, cliprect, dotCol, layout);
  return 0;
}
// 3-Color MINI DMD Type 4 (Simpsons) (14x10)
PINMAME_VIDEO_UPDATE(seminidmd4_update) {
  static int color[2][2] = {
    { 0, 6 }, { 7, 9 } // off, green, red, yellow
  };
  int ii, kk, bits, isRed, isGrn;
  UINT8 *line;
  tDMDDot dotCol;
  UINT16 bits1, bits2;
  UINT16 *seg = &coreGlobals.drawSeg[0];

  for (ii=0; ii < 14; ii++) {
    bits1 = 0;
    bits2 = 0;
    for (kk=0, bits = 0x40; kk < 7; kk++, bits >>= 1) {
      isRed = (selocals.minidmd[0][ii/7][ii%7] & bits) > 0;
      isGrn = (selocals.minidmd[1][ii/7][ii%7] & bits) > 0;
      bits1 = (bits1 << 2) | (isGrn << 1) | isRed;
      line = &dotCol[ii+1][kk];
      *line = color[isRed][isGrn];
      isRed = (selocals.minidmd[2][ii/7][ii%7] & bits) > 0;
      isGrn = (selocals.minidmd[3][ii/7][ii%7] & bits) > 0;
      bits2 = (bits2 << 2) | (isGrn << 1) | isRed;
      line = &dotCol[ii+1][7+kk];
      *line = color[isRed][isGrn];
    }
    *seg++ = bits1;
    *seg++ = bits2;
  }
  video_update_core_dmd(bitmap, cliprect, dotCol, layout);
  return 0;
}

/*---------------------------
/  Memory map for main CPU
/----------------------------*/

static MEMORY_READ_START(se_readmem)
  { 0x0000, 0x1fff, ram_r },
  { 0x2007, 0x2007, auxboard_r },
  { 0x3000, 0x3000, dedswitch_r },
  { 0x3100, 0x3100, dip_r },
  { 0x3400, 0x3400, switch_r },
  { 0x3406, 0x3407, gilamp_r }, // GI lamps on SPP?
  { 0x3500, 0x3500, dmdie_r },
  { 0x3700, 0x3700, dmdstatus_r },
  { 0x4000, 0x7fff, MRA_BANK1 },
  { 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(se_writemem)
  { 0x0000, 0x1fff, ram_w },
  { 0x2000, 0x2003, solenoid_w },
  { 0x2006, 0x2007, auxboard_w },
  { 0x2008, 0x2008, lampstrb_w },
  { 0x2009, 0x2009, auxlamp_w },
  { 0x200a, 0x200a, lampdriv_w },
  { 0x200b, 0x200b, giaux_w },
//{ 0x201a, 0x201a, x201a_w },    // Golden Cue unknown device
  { 0x3200, 0x3200, mcpu_bank_w },
  { 0x3300, 0x3300, switch_w },
  { 0x3406, 0x3407, gilamp_w }, // GI lamps on SPP?
  { 0x3600, 0x3600, dmdlatch_w },
  { 0x3601, 0x3601, dmdreset_w },
  { 0x3800, 0x3800, sndbrd_1_data_w },
MEMORY_END

static MACHINE_DRIVER_START(se)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CPU_ADD(M6809, 2000000)
  MDRV_CORE_INIT_RESET_STOP(se,NULL,se)
  MDRV_CPU_MEMORY(se_readmem, se_writemem)
  MDRV_CPU_VBLANK_INT(se_vblank, 1)
  MDRV_CPU_PERIODIC_INT(irq1_line_pulse, SE_IRQFREQ)
  MDRV_NVRAM_HANDLER(se)
  MDRV_DIPS(8)
  MDRV_SWITCH_UPDATE(se)
  MDRV_DIAGNOSTIC_LEDH(1)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(se2aS)
  MDRV_IMPORT_FROM(se)
  MDRV_IMPORT_FROM(de2as)
  MDRV_IMPORT_FROM(de_dmd32)
  MDRV_SOUND_CMD(sndbrd_1_data_w)
  MDRV_SOUND_CMDHEADING("se")
MACHINE_DRIVER_END

MACHINE_DRIVER_START(se2bS)
  MDRV_IMPORT_FROM(se)
  MDRV_IMPORT_FROM(de2bs)
  MDRV_IMPORT_FROM(de_dmd32)
  MDRV_SOUND_CMD(sndbrd_1_data_w)
  MDRV_SOUND_CMDHEADING("se")
MACHINE_DRIVER_END

MACHINE_DRIVER_START(se2cS)
  MDRV_IMPORT_FROM(se)
  MDRV_IMPORT_FROM(de2cs)
  MDRV_IMPORT_FROM(de_dmd32)
  MDRV_SOUND_CMD(sndbrd_1_data_w)
  MDRV_SOUND_CMDHEADING("se")
MACHINE_DRIVER_END

MACHINE_DRIVER_START(se3aS)
//from se driver - but I need a different init routine
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CPU_ADD(M6809, 2000000)
  MDRV_CORE_INIT_RESET_STOP(se3,NULL,se)
  MDRV_CPU_MEMORY(se_readmem, se_writemem)
  MDRV_CPU_VBLANK_INT(se_vblank, 1)
  MDRV_CPU_PERIODIC_INT(irq1_line_pulse, SE_IRQFREQ)
  MDRV_NVRAM_HANDLER(se)
  MDRV_DIPS(8)
  MDRV_SWITCH_UPDATE(se)
  MDRV_DIAGNOSTIC_LEDH(1)
//
  MDRV_IMPORT_FROM(de3as)
  MDRV_IMPORT_FROM(de_dmd32)
  MDRV_SOUND_CMD(sndbrd_1_data_w)
  MDRV_SOUND_CMDHEADING("se")
MACHINE_DRIVER_END

/*-----------------------------------------------
/ Load/Save static ram
/ Save RAM & CMOS Information
/-------------------------------------------------*/
static NVRAM_HANDLER(se) {
  core_nvram(file, read_or_write, memory_region(SE_CPUREGION), 0x2000, 0xff);
}
