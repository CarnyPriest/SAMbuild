/************************************************************************************************
  Sega/Stern Pinball

  Hardware from 1994-????

  CPU Boards: Whitestar System

  Display Boards:
    ??

  Sound Board Revisions:
    Integrated with CPU. Similar cap. as 520-5126-xx Series: (Baywatch to Batman Forever)

*************************************************************************************************/
#include <stdarg.h>
#include "driver.h"
#include "cpu/m6809/m6809.h"
#include "core.h"
#include "sndbrd.h"
#include "dedmd.h"
#include "se.h"
#include "desound.h"

#define SE_VBLANKFREQ      60 /* VBLANK frequency */
#define SE_IRQFREQ        976 /* FIRQ Frequency according to Theory of Operation */
#define SE_ROMBANK0         1

static void SE_init(void);
static void SE_exit(void);
static void SE_nvram(void *file, int write);
static WRITE_HANDLER(mcpu_ram8000_w);
/*----------------
/  Local varibles
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
} SElocals;

static int SE_irq(void) {
  cpu_set_irq_line(0, M6809_FIRQ_LINE,PULSE_LINE);
  return ignore_interrupt();	//NO INT OR NMI GENERATED!
}

static int SE_vblank(void) {
  /*-------------------------------
  /  copy local data to interface
  /--------------------------------*/
  SElocals.vblankCount = (SElocals.vblankCount+1) % 16;

  /*-- lamps --*/
  if ((SElocals.vblankCount % SE_LAMPSMOOTH) == 0) {
    memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
    memset(coreGlobals.tmpLampMatrix, 0, sizeof(coreGlobals.tmpLampMatrix));
  }
  /*-- solenoids --*/
  coreGlobals.solenoids2 = SElocals.flipsol; SElocals.flipsol = SElocals.flipsolPulse;
  if ((SElocals.vblankCount % SE_SOLSMOOTH) == 0) {
    coreGlobals.solenoids = SElocals.solenoids;
    SElocals.solenoids = coreGlobals.pulsedSolState;
  }
  /*-- display --*/
  if ((SElocals.vblankCount % SE_DISPLAYSMOOTH) == 0) {
    coreGlobals.diagnosticLed = SElocals.diagnosticLed;
    SElocals.diagnosticLed = 0;
  }
  core_updateSw(TRUE); /* flippers are CPU controlled */
  return 0;
}

static void SE_updSw(int *inports) {
  if (inports) {
    /*Switch Col 0 = Dedicated Switches - Coin Door Only - Begin at 6th Spot*/
    coreGlobals.swMatrix[0] = (inports[SE_COMINPORT] & 0x000f)<<4;
    /*Switch Col 1 = Coin Switches - (Switches begin at 4th Spot)*/
    coreGlobals.swMatrix[1] = ((inports[SE_COMINPORT] & 0x0f00)>>5); //>>5 = >>8 + <<3
    /*Copy Start, Tilt, and Slam Tilt to proper position in Matrix: Switchs 66,67,68*/
    /*Clear bits 6,7,8 first*/
    coreGlobals.swMatrix[7] &= ~0xe0;
    coreGlobals.swMatrix[7] |= (inports[SE_COMINPORT] & 0x00f0)<<1;	//<<1 = >>4 + <<5
  }
}

static core_tData SEData = {
  8, /* 8 DIPs */
  SE_updSw,
  1,
  sndbrd_1_data_w, "SE",
  core_swSeq2m, core_swSeq2m,core_m2swSeq,core_m2swSeq
};

static void SE_init(void) {
  if (core_init(&SEData)) return;
  /* Copy Last 32K into last 32K of CPU space */
  memcpy(memory_region(SE_CPUREGION) + 0x8000,
         memory_region(SE_ROMREGION) +
	 (memory_region_length(SE_ROMREGION) - 0x8000), 0x8000);
  sndbrd_0_init(SNDBRD_DEDMD32, 2, memory_region(DE_DMD32ROMREGION),NULL,NULL);
  sndbrd_1_init(SNDBRD_DE2S,    1, memory_region(DE2S_ROMREGION), NULL, NULL);

  // Sharkeys got some extra ram
  if (core_gameData->gen & GEN_WS_1)
    SElocals.ram8000 = install_mem_write_handler(0,0x8000,0x81ff,mcpu_ram8000_w);
}

static void SE_exit(void) {
  sndbrd_0_exit(); sndbrd_1_exit(); core_exit();
}

/*-- Main CPU Bank Switch --*/
// D0-D5 Bank Address
// D6    Unused
// D7    Diagnostic LED
static WRITE_HANDLER(mcpu_bank_w) {
  // Should be 0x3f but memreg is only 512K */
  cpu_setbank(SE_ROMBANK0, memory_region(SE_ROMREGION) + (data & 0x1f)* 0x4000);
  SElocals.diagnosticLed = data>>7;
}

/* Sharkey's ShootOut got some ram at 0x8000-0x81ff */
static WRITE_HANDLER(mcpu_ram8000_w) { SElocals.ram8000[offset] = data; }

/*-- Lamps --*/
static WRITE_HANDLER(lampdriv_w) {
  SElocals.lampRow = core_revbyte(data);
  core_setLamp(coreGlobals.tmpLampMatrix, SElocals.lampColumn, SElocals.lampRow);
}
static WRITE_HANDLER(lampstrb_w) { core_setLamp(coreGlobals.tmpLampMatrix, SElocals.lampColumn = (SElocals.lampColumn & 0xff00) | data, SElocals.lampRow);}
static WRITE_HANDLER(auxlamp_w) { core_setLamp(coreGlobals.tmpLampMatrix, SElocals.lampColumn = (SElocals.lampColumn & 0x00ff) | (data<<8), SElocals.lampRow);}

/*-- Switches --*/
static READ_HANDLER(switch_r)	{ return ~core_getSwCol(SElocals.swCol); }
static WRITE_HANDLER(switch_w)	{ SElocals.swCol = data; }

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
  return ~((coreGlobals.swMatrix[0]) | core_revnyb(coreGlobals.swMatrix[11] & 0x0f));
}

/*-- Dip Switch SW300 - Country Settings --*/
static READ_HANDLER(dip_r) { return ~core_getDip(0); }

/*-- Solenoids --*/
static WRITE_HANDLER(solenoid_w) {
  static const int solmaskno[] = { 8, 0, 16, 24 };
  UINT32 mask = ~(0xff<<solmaskno[offset]);
  UINT32 sols = data<<solmaskno[offset];

  if (offset == 0) { /* move flipper power solenoids (L=15,R=16) to (R=45,L=47) */
    SElocals.flipsol |= SElocals.flipsolPulse = ((data & 0x80)>>7) | ((data & 0x40)>>4);
    sols &= 0xffff3fff; /* mask off flipper solenoids */
  }
  coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & mask) | sols;
  SElocals.solenoids |= sols;
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
  DBGLOG(("DMD Input Enable Read PC=%x\n",cpu_getpreviouspc())); return 0x00;
}

static WRITE_HANDLER(auxboard_w) { /* logerror("Aux Board Write: Offset: %x Data: %x\n",offset,data); */}
static WRITE_HANDLER(giaux_w)    { /* logerror("GI/Aux Board Write: Offset: %x Data: %x\n",offset,data); */}

/*---------------------------
/  Memory map for main CPU
/----------------------------*/
static MEMORY_READ_START(SE_readmem)
  { 0x0000, 0x1fff, MRA_RAM },
  { 0x3000, 0x3000, dedswitch_r },
  { 0x3100, 0x3100, dip_r },
  { 0x3400, 0x3400, switch_r },
  { 0x3500, 0x3500, dmdie_r },
  { 0x3700, 0x3700, dmdstatus_r },
  { 0x4000, 0x7fff, MRA_BANK1 },
  { 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(SE_writemem)
  { 0x0000, 0x1fff, MWA_RAM },
  { 0x2000, 0x2003, solenoid_w },
  { 0x2006, 0x2007, auxboard_w },
  { 0x2008, 0x2008, lampstrb_w },
  { 0x2009, 0x2009, auxlamp_w },
  { 0x200a, 0x200a, lampdriv_w },
  { 0x200b, 0x200b, giaux_w },
  { 0x3200, 0x3200, mcpu_bank_w },
  { 0x3300, 0x3300, switch_w },
  { 0x3600, 0x3600, dmdlatch_w },
  { 0x3601, 0x3601, dmdreset_w },
  { 0x3800, 0x3800, sndbrd_1_data_w },
  { 0x4000, 0x7fff, MWA_ROM },
  { 0x8000, 0xffff, MWA_ROM },
MEMORY_END

struct MachineDriver machine_driver_SE_1S = {
  {{  CPU_M6809, 2000000, /* 2 Mhz */
      SE_readmem, SE_writemem, NULL, NULL,
      SE_vblank, 1,
      SE_irq, SE_IRQFREQ
  }, DE2S_SOUNDCPU, DE_DMD32CPU },
  SE_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50, SE_init, SE_exit,
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_SUPPORTS_DIRTY | VIDEO_TYPE_RASTER, 0,
  NULL, NULL, de_dmd128x32_refresh,
  SOUND_SUPPORTS_STEREO,0,0,0,{ DE2S_SOUNDA },
  SE_nvram
};

struct MachineDriver machine_driver_SE_2S = {
  {{  CPU_M6809, 2000000, /* 2 Mhz */
      SE_readmem, SE_writemem, NULL, NULL,
      SE_vblank, 1,
      SE_irq, SE_IRQFREQ
  }, DE2S_SOUNDCPU, DE_DMD32CPU },
  SE_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50, SE_init, SE_exit,
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_SUPPORTS_DIRTY | VIDEO_TYPE_RASTER, 0,
  NULL, NULL, de_dmd128x32_refresh,
  SOUND_SUPPORTS_STEREO,0,0,0,{ DE2S_SOUNDB },
  SE_nvram
};

struct MachineDriver machine_driver_SE_3S = {
  {{  CPU_M6809, 2000000, /* 2 Mhz */
      SE_readmem, SE_writemem, NULL, NULL,
      SE_vblank, 1,
      SE_irq, SE_IRQFREQ
  }, DE2S_SOUNDCPU, DE_DMD32CPU },
  SE_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50, SE_init, SE_exit,
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_SUPPORTS_DIRTY | VIDEO_TYPE_RASTER, 0,
  NULL, NULL, de_dmd128x32_refresh,
  SOUND_SUPPORTS_STEREO,0,0,0,{ DE2S_SOUNDC },
  SE_nvram
};


/*-----------------------------------------------
/ Load/Save static ram
/ Save RAM & CMOS Information
/-------------------------------------------------*/
static void SE_nvram(void *file, int write) {
  core_nvram(file, write, memory_region(SE_CPUREGION), 0x2000, 0xff);
}

