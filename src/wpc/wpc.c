#include <stdarg.h>
#include <time.h>
#include "driver.h"
#include "cpu/m6809/m6809.h"
#include "wpcsound.h"
#include "dcs.h"
#include "snd_cmd.h"
#include "sim.h"
#include "core.h"
#include "wpc.h"

#define WPC_VBLANKDIV      4 /* How often to check the DMD FIRQ interrupt */

/*-- IRQ frequence, most WPC functions are performed at 1/16 of this frequency --*/
#define WPC_IRQFREQ      976 /* IRQ Frequency-Timed by JD*/

#define GEN_HASWPCSOUND
#define GEN_FLIPTRONFLIP (GEN_ALLWPC & ~(GEN_WPCALPHA_1|GEN_WPCALPHA_2|GEN_WPCDMD))
#define GEN_HASDMD

/*---------------------
/  local WPC functions
/----------------------*/
/*-- interrupt handling --*/
static int wpc_vblank(void);
static int wpc_irq(void);

/*-- PIC security chip emulation --*/
static int wpc_pic_r(void);
static void wpc_pic_w(int data);
static void wpc_serialCnv(char no[21], UINT8 schip[16],
                          UINT8 code[3]);
/*-- DMD --*/
static int  dmd_start(void);
static void dmd_refresh(struct mame_bitmap *bitmap, int full_refresh);

/*-- misc. --*/
static void wpc_init(void);
static void wpc_exit(void);
static void wpc_nvram(void *file, int write);

/*------------------------
/  DMD display registers
/-------------------------*/
#define DMD_PAGE3A00    (0x3fbc - WPC_BASE)
#define DMD_PAGE3800    (0x3fbe - WPC_BASE)
#define DMD_VISIBLEPAGE (0x3fbf - WPC_BASE)
#define DMD_FIRQLINE    (0x3fbd - WPC_BASE)

/*---------------------
/  Global variables
/---------------------*/
UINT8 *wpc_data;     /* WPC registers */
int WPC_gWPC95;      /* dcs95 sound, used in ADSP2100 patch ? */
core_tLCDLayout wpc_dispAlpha[] = {
  DISP_SEG_16(0,CORE_SEG16),DISP_SEG_16(1,CORE_SEG16),{0}
};
core_tLCDLayout wpc_dispDMD[] = {
  {0,0,32,128,CORE_DMD}, {0}
};

/*------------------
/  Local variables
/------------------*/
static struct {
  UINT32  solData;        /* current value of solenoids 1-28 */
  UINT8   solFlip, solFlipPulse;  /* current value of flipper solenoids */
  UINT8   nonFlipBits;    /* flipper solenoids not used for flipper (smoothed) */
  int  vblankCount;                   /* vblank interrupt counter */
  core_tSeg alphaSeg;
  struct {
    UINT8 sData[16];
    UINT8 codeNo[3];
    UINT8 lastW;             /* last written command */
    UINT8 sNoS;              /* serial number scrambler */
    UINT8 count;
    int           codeW;
  } pic;
  int time, ltime, ticks;
  int pageMask;            /* page handling */
  int firqSrc;             /* source of last firq */
  int initDone;            /* TRUE if glocals initialised, used to handle restarts */
  int diagnostic;
} wpclocals;

/*-- pointers --*/
static void *wpc_printfile = NULL;

/*---------------------------
/  Memory map for CPU board
/----------------------------*/
static MEMORY_READ_START(wpc_readmem)
  { 0x0000, 0x37ff, MRA_RAM },
  { 0x3800, 0x39ff, MRA_BANK2 },  /* DMD */
  { 0x3A00, 0x3bff, MRA_BANK3 },  /* DMD */
  { 0x3c00, 0x3faf, MRA_RAM },
  { 0x3fb0, 0x3fff, wpc_r },
  { 0x4000, 0x7fff, MRA_BANK1 },
  { 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(wpc_writemem)
  { 0x0000, 0x37ff, MWA_RAM },
  { 0x3800, 0x39ff, MWA_BANK2 },
  { 0x3A00, 0x3bff, MWA_BANK3 },
  { 0x3c00, 0x3faf, MWA_RAM },
  { 0x3fb0, 0x3fff, wpc_w, &wpc_data },
  { 0x8000, 0xffff, MWA_ROM },
MEMORY_END

/*-----------------
/  Machine drivers
/------------------*/
struct MachineDriver machine_driver_wpcDMD = {
  {{  CPU_M6809, 2000000, /* 2 Mhz? */
      wpc_readmem, wpc_writemem, NULL, NULL,
      wpc_vblank, WPC_VBLANKDIV,
      wpc_irq, WPC_IRQFREQ
  }},
  WPC_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  100, wpc_init, CORE_EXITFUNC(wpc_exit)
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_SUPPORTS_DIRTY | VIDEO_TYPE_RASTER, 0,
  dmd_start, NULL, dmd_refresh,
  0,0,0,0, {{ 0,0 }},
  wpc_nvram
};

struct MachineDriver machine_driver_wpcAlpha = {
  {{  CPU_M6809, 2000000, /* 2 Mhz? */
      wpc_readmem, wpc_writemem, NULL, NULL,
      wpc_vblank, WPC_VBLANKDIV,
      wpc_irq, WPC_IRQFREQ
  }},
  WPC_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  100, wpc_init, CORE_EXITFUNC(wpc_exit)
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_SUPPORTS_DIRTY | VIDEO_TYPE_RASTER, 0,
  NULL, NULL, gen_refresh,
  0,0,0,0, {{ 0,0 }},
  wpc_nvram
};

struct MachineDriver machine_driver_wpcDMD_s = {
  {{  CPU_M6809, 2000000, /* 2 Mhz? */
      wpc_readmem, wpc_writemem, NULL, NULL,
      wpc_vblank, WPC_VBLANKDIV, wpc_irq, WPC_IRQFREQ
  } WPCS_SOUNDCPU },
  WPC_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  100, wpc_init, CORE_EXITFUNC(wpc_exit)
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_SUPPORTS_DIRTY | VIDEO_TYPE_RASTER, 0,
  dmd_start, NULL, dmd_refresh,
  0, 0, 0, 0, { WPCS_SOUND },
  wpc_nvram
};
struct MachineDriver machine_driver_wpcAlpha_s = {
  {{  CPU_M6809, 2000000, /* 2 Mhz? */
      wpc_readmem, wpc_writemem, NULL, NULL,
      wpc_vblank, WPC_VBLANKDIV, wpc_irq, WPC_IRQFREQ
  }   WPCS_SOUNDCPU },
  WPC_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  100, wpc_init, CORE_EXITFUNC(wpc_exit)
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_SUPPORTS_DIRTY | VIDEO_TYPE_RASTER, 0,
  NULL, NULL, gen_refresh,
  0, 0, 0, 0, { WPCS_SOUND },
  wpc_nvram
};

struct MachineDriver machine_driver_wpcDCS_s = {
  {{  CPU_M6809, 2000000, /* 2 Mhz? */
      wpc_readmem, wpc_writemem, NULL, NULL,
      wpc_vblank, WPC_VBLANKDIV, wpc_irq, WPC_IRQFREQ
  } DCS1_SOUNDCPU },
  WPC_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  100, wpc_init, CORE_EXITFUNC(wpc_exit)
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_SUPPORTS_DIRTY | VIDEO_TYPE_RASTER, 0,
  dmd_start, NULL, dmd_refresh,
  0, 0, 0, 0, { DCS_SOUND },
  wpc_nvram
};

struct MachineDriver machine_driver_wpcDCS95_s = {
  {{  CPU_M6809, 2000000, /* 2 Mhz? */
      wpc_readmem, wpc_writemem, NULL, NULL,
      wpc_vblank, WPC_VBLANKDIV, wpc_irq, WPC_IRQFREQ
  } DCS2_SOUNDCPU },
  WPC_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  100, wpc_init, CORE_EXITFUNC(wpc_exit)
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_SUPPORTS_DIRTY | VIDEO_TYPE_RASTER, 0,
  dmd_start, NULL, dmd_refresh,
  0, 0, 0, 0, { DCS_SOUND },
  wpc_nvram
};

/*--------------------------------------------------------------
/ This is generated WPC_VBLANKDIV times per frame
/ = every 32/WPC_VBLANKDIV lines.
/ Generate a FIRQ if it matches the DMD line
/ Also do the smoothing of the solenoids and lamps
/--------------------------------------------------------------*/
static int wpc_vblank(void) {
  wpclocals.vblankCount = (wpclocals.vblankCount+1) % 16;

  if (core_gameData->gen & (GEN_ALLWPC & ~(GEN_WPCALPHA_1|GEN_WPCALPHA_2))) {
    /*-- check if the DMD line matches the requested interrupt line */
    if ((wpclocals.vblankCount % WPC_VBLANKDIV) == (wpc_data[DMD_FIRQLINE]*WPC_VBLANKDIV/32))
      wpc_firq(TRUE, WPC_FIRQ_DMD);
    if ((wpclocals.vblankCount % WPC_VBLANKDIV) == 0) {
      /*-- This is the real VBLANK interrupt --*/
      coreGlobals_dmd.DMDFrames[coreGlobals_dmd.nextDMDFrame] = memory_region(WPC_MEMREG_DMD)+ (wpc_data[DMD_VISIBLEPAGE] & 0x0f) * 0x200;
      coreGlobals_dmd.nextDMDFrame = (coreGlobals_dmd.nextDMDFrame + 1) % DMD_FRAMES;
    }
  }

  /*--------------------------------------------------------
  /  Most solonoids don't have a holding coil so the software
  /  simulates it by pulsing the power to the main (and only) coil.
  /  (I assume this is why the Auto Fire diverter in TZ seems to flicker.)
  /  I simulate the coil position by looking over WPC_SOLSMOOTH vblanks
  /  and only turns the solenoid off if it has not been pulsed
  /  during that time.
  /-------------------------------------------------------------*/
  if ((wpclocals.vblankCount % (WPC_VBLANKDIV*WPC_SOLSMOOTH)) == 0) {
    coreGlobals.solenoids = (wpc_data[WPC_SOLENOID1] << 24) |
                            (wpc_data[WPC_SOLENOID3] << 16) |
                            (wpc_data[WPC_SOLENOID4] <<  8) |
                             wpc_data[WPC_SOLENOID2];

    wpc_data[WPC_SOLENOID1] = wpc_data[WPC_SOLENOID2] = 0;
    wpc_data[WPC_SOLENOID3] = wpc_data[WPC_SOLENOID4] = 0;
    if (core_gameData->gen & GEN_FLIPTRONFLIP) {
      coreGlobals.solenoids2 = wpclocals.solFlip;
      wpclocals.solFlip = wpclocals.solFlipPulse;
    }
  }
  else if (((wpclocals.vblankCount % WPC_VBLANKDIV) == 0) &&
           (core_gameData->gen & GEN_FLIPTRONFLIP)) {
    coreGlobals.solenoids2 = (coreGlobals.solenoids2 & wpclocals.nonFlipBits) | wpclocals.solFlip;
    wpclocals.solFlip = (wpclocals.solFlip & wpclocals.nonFlipBits) | wpclocals.solFlipPulse;
  }

  /*--------------------------------------------------------
  / The lamp circuit pulses the power to the lamps one column
  / at a time. By only reading the value every WPC_LAMPSMOOTH
  / vblank we get a steady light.
  /
  / Williams changed the software for the lamphandling at some stage
  / earlier code (TZ 9.2)	newer code (TZ 9.4)
  / ------------		            -----------
  / Activate Column             Activate rows
  / Activate rows               Activate column
  / wait                        wait
  / Deactivate rows             repeat for next column
  / repeat for next column
  / ..
  / For the game it doesn't really matter but it confused me when
  / the lamp code here worked for 9.2 but not in 9.4.
  /-------------------------------------------------------------*/
  if ((wpclocals.vblankCount % (WPC_VBLANKDIV*WPC_LAMPSMOOTH)) == 0) {
    memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
    memset(coreGlobals.tmpLampMatrix, 0, sizeof(coreGlobals.tmpLampMatrix));
  }
  if ((wpclocals.vblankCount % (WPC_VBLANKDIV*WPC_DISPLAYSMOOTH)) == 0) {
    if (core_gameData->gen & (GEN_WPCALPHA_1|GEN_WPCALPHA_2)) {
      memcpy(coreGlobals.segments, wpclocals.alphaSeg, sizeof(coreGlobals.segments));
      memset(wpclocals.alphaSeg, 0, sizeof(wpclocals.alphaSeg));
    }
    coreGlobals.diagnosticLed = wpclocals.diagnostic;
    wpclocals.diagnostic = 0;
  }

  /*------------------------------
  /  Update switches every vblank
  /-------------------------------*/
  if ((wpclocals.vblankCount % WPC_VBLANKDIV) == 0) /*-- update switches --*/
	core_updateSw((core_gameData->gen & GEN_FLIPTRONFLIP) ? TRUE : (wpc_data[WPC_GILAMPS] & 0x80));
  return 0;
}

/* The FIRQ line is wired between the WPC chip and all external I/Os (sound) */
/* The DMD firq must be generated via the WPC but I don't how. */
extern void wpc_firq(int set, int src) {
  if (set)
    wpclocals.firqSrc |= src;
  else
    wpclocals.firqSrc &= ~src;
  cpu_set_irq_line(WPC_CPUNO, M6809_FIRQ_LINE, wpclocals.firqSrc ? HOLD_LINE : CLEAR_LINE);
}

/*----------------------
/ Emulate the WPC chip
/-----------------------*/
READ_HANDLER(wpc_r) {
  switch (offset) {
    case WPC_FLIPPERS: /* Flipper switches */
      if (core_gameData->gen & (GEN_ALLWPC & ~(GEN_WPC95|GEN_WPC95DCS)))
        return ~coreGlobals.swMatrix[WPC_CABINETSWCOL];
      break;
    case WPC_FLIPPERSW95:
      if (core_gameData->gen & (GEN_WPC95|GEN_WPC95DCS))
        return ~coreGlobals.swMatrix[WPC_CABINETSWCOL];
      break;
    case WPC_SWCOINDOOR: /* cabinet switches */
      return coreGlobals.swMatrix[CORE_COINDOORSWCOL];
    case WPC_SWROWREAD: /* read row */
      if (core_gameData->gen & (GEN_WPCSECURITY | GEN_WPC95 | GEN_WPC95DCS))
        return wpc_pic_r();
      return core_getSwCol(wpc_data[WPC_SWCOLSELECT]);
    case WPC_SHIFTADRH:
      return wpc_data[WPC_SHIFTADRH] +
             ((wpc_data[WPC_SHIFTADRL] + (wpc_data[WPC_SHIFTBIT]>>3))>>8);
      break;
    case WPC_SHIFTADRL:
      return (wpc_data[WPC_SHIFTADRL] + (wpc_data[WPC_SHIFTBIT]>>3)) & 0xff;
    case WPC_SHIFTBIT:
    case WPC_SHIFTBIT2:
      return 1<<(wpc_data[offset] & 0x07);
    case WPC_FIRQSRC: /* FIRQ source */
      //DBGLOG(("R:FIRQSRC\n"));
      return (wpclocals.firqSrc & WPC_FIRQ_DMD) ? 0x00 : 0x80;
    case WPC_DIPSWITCH:
      return (core_getDip(0) & 0xfc) + 3;
    case WPC_RTCHOUR: {
      UINT8 *timeMem = memory_region(WPC_MEMREG_CPU) + 0x1800;
      UINT16 checksum = 0;
      time_t now;
      struct tm *systime;

      time(&now);
      systime = localtime(&now);
      checksum += *timeMem++ = (systime->tm_year + 1900)>>8;
      checksum += *timeMem++ = (systime->tm_year + 1900)&0xff;
      checksum += *timeMem++ = systime->tm_mon + 1;
      checksum += *timeMem++ = systime->tm_mday;
      checksum += *timeMem++ = systime->tm_wday + 1;
      checksum += *timeMem++ = 0;
      checksum += *timeMem++ = 1;
      checksum = 0xffff - checksum;
      *timeMem++ = checksum>>8;
      *timeMem   = checksum & 0xff;
      return systime->tm_hour;
    }
    case WPC_RTCMIN: {
      time_t now;
      struct tm *systime;
      time(&now);
      systime = localtime(&now);

      return (systime->tm_min);
    }
    case WPC_WATCHDOG:
      break;
    case WPC_SOUNDIF:
      if (coreGlobals.soundEn) {
        DBGLOG(("soundif_r: (PC=%4x)\n",cpu_get_pc()));
        return (core_gameData->gen &
                (GEN_WPCALPHA_2 | GEN_WPCDMD | GEN_WPCFLIPTRON)) ? wpcs_data_r(0) :
                                                                   dcs_data_r(0);
      }
      return 0;
    case WPC_SOUNDBACK:
      if (coreGlobals.soundEn) {
        DBGLOG(("soundback_r: (PC=%4x)\n",cpu_get_pc()));
        return (core_gameData->gen &
                (GEN_WPCALPHA_2 | GEN_WPCDMD | GEN_WPCFLIPTRON)) ? wpcs_ctrl_r(0) :
                                                                   dcs_ctrl_r(0);
      }
      return 0;
    case WPC_PRINTBUSY:
      return 0;
    case DMD_VISIBLEPAGE:
      break;
    case DMD_PAGE3800:
    case DMD_PAGE3A00:
      return 0; /* these can't be read */
    case DMD_FIRQLINE:
      return (wpclocals.firqSrc & WPC_FIRQ_DMD) ? 0x80 : 0x00;
    default:
      DBGLOG(("wpc_r %4x\n", offset+WPC_BASE));
      break;
  }
  return wpc_data[offset];
}

WRITE_HANDLER(wpc_w) {
  switch (offset) {
    case WPC_ROMBANK: { /* change rom bank */
      int bank = data & wpclocals.pageMask;
      cpu_setbank(1, memory_region(WPC_MEMREG_ROM)+ bank * 0x4000);
      break;
    }
    case WPC_FLIPPERS: /* Flipper coils */
      if (core_gameData->gen & (GEN_ALLWPC & ~(GEN_WPC95|GEN_WPC95DCS))) {
        wpclocals.solFlip &= wpclocals.nonFlipBits;
        wpclocals.solFlip |= wpclocals.solFlipPulse = ~data;
      }
      break;
    case WPC_FLIPPERCOIL95:
      if (core_gameData->gen & (GEN_WPC95|GEN_WPC95DCS)) {
        wpclocals.solFlip &= wpclocals.nonFlipBits;
        wpclocals.solFlip |= wpclocals.solFlipPulse = data;
      }
      else if (core_gameData->gen & (GEN_WPCALPHA_1|GEN_WPCALPHA_2))
        wpclocals.alphaSeg[1][wpc_data[WPC_ALPHAPOS]].hi |= data;
      break;
    /*-----------------------------
    /  Lamp handling
    /  Lamp values are accumulated
    /  see vblank for description
    /------------------------------*/
    case WPC_LAMPROW: /* row and column can be written in any order */
      core_setLamp(coreGlobals.tmpLampMatrix,wpc_data[WPC_LAMPCOLUMN],data);
      break;
    case WPC_LAMPCOLUMN: /* row and column can be written in any order */
      core_setLamp(coreGlobals.tmpLampMatrix,data,wpc_data[WPC_LAMPROW]);
      break;
    case WPC_SWCOLSELECT:
      if (core_gameData->gen & (GEN_WPCSECURITY | GEN_WPC95 | GEN_WPC95DCS))
        wpc_pic_w(data);
      break;
    case WPC_GILAMPS: { /* For now, we simply catch if GI String is on or off*/
      int ii, tmp = data;
      for (ii = 0; ii < CORE_MAXGI; ii++, tmp >>= 1)
        coreGlobals.gi[ii] = (tmp & 0x01);
      break;
    }
    case WPC_EXTBOARD1: /* WPC_ALPHAPOS */
      break; /* just save position */
    case WPC_EXTBOARD2: /* WPC_ALPHA1 */
      if (core_gameData->gen & (GEN_WPCALPHA_1|GEN_WPCALPHA_2))
        wpclocals.alphaSeg[0][wpc_data[WPC_ALPHAPOS]].hi |= data;
      break;
    case WPC_EXTBOARD3:
      if (core_gameData->gen & (GEN_WPCALPHA_1|GEN_WPCALPHA_2))
        wpclocals.alphaSeg[0][wpc_data[WPC_ALPHAPOS]].lo |= data;
      break;
    /* case WPC_EXTBOARD4: */
    case WPC_EXTBOARD5:
      if (core_gameData->gen & (GEN_WPCALPHA_1|GEN_WPCALPHA_2))
        wpclocals.alphaSeg[1][wpc_data[WPC_ALPHAPOS]].lo |= data;
      break;
    case WPC_SHIFTADRH:
    case WPC_SHIFTADRL:
    case WPC_SHIFTBIT:
    case WPC_SHIFTBIT2:
      break; /* just save value */
    case WPC_DIPSWITCH:
      //DBGLOG(("W:DIPSWITCH %x\n",data));
      break; /* just save value */
    /*-------------------------------------------
    /  The solenoids are accumulated over time.
    /  See vblank interrupt for explanation.
    /--------------------------------------------*/
    case WPC_SOLENOID1:
      coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0x00FFFFFF) | (data<<24);
      data |= wpc_data[offset];
      break;
    case WPC_SOLENOID2:
      coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xFFFFFF00) | data;
      data |= wpc_data[offset];
      break;
    case WPC_SOLENOID3:
      coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xFF00FFFF) | (data<<16);
      data |= wpc_data[offset];
      break;
    case WPC_SOLENOID4:
      coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xFFFF00FF) | (data<<8);
      data |= wpc_data[offset];
      break;
    case WPC_SOUNDIF:
      if (coreGlobals.soundEn) {
        //DBGLOG(("Sound cmd: %2x\n",data));
        snd_cmd_log(data);
        if (core_gameData->gen &
            (GEN_WPCALPHA_2 | GEN_WPCDMD | GEN_WPCFLIPTRON)) wpcs_data_w(0,data);
        else                                                 dcs_data_w(0,data);
      }
      break;
    case WPC_SOUNDBACK:
      if (coreGlobals.soundEn) {
        if (core_gameData->gen &
            (GEN_WPCALPHA_2 | GEN_WPCDMD | GEN_WPCFLIPTRON)) wpcs_ctrl_w(0,data);
        else                                                 dcs_ctrl_w(0,data);
      }
      break;
    case WPC_WATCHDOG:
      break; /* Just ignore for now */
    case WPC_FIRQSRC:
      /* CPU writes here after a non-dmd firq. Don't know what happens */
      break;
    case WPC_IRQACK:
      cpu_set_irq_line(WPC_CPUNO, M6809_IRQ_LINE, CLEAR_LINE);
      break;
    case DMD_PAGE3800: /* set the page that is visible at 0x3800 */
      cpu_setbank(2, memory_region(WPC_MEMREG_DMD) + (data & 0x0f) * 0x200); break;
    case DMD_PAGE3A00: /* set the page that is visible at 0x3A00 */
      cpu_setbank(3, memory_region(WPC_MEMREG_DMD) + (data & 0x0f) * 0x200); break;
    case DMD_FIRQLINE: /* set the line to generate FIRQ at. */
      wpc_firq(FALSE, WPC_FIRQ_DMD);
      break;
    case DMD_VISIBLEPAGE: /* set the visible page */
      break;
    case WPC_RTCHOUR:
      wpclocals.ltime = (wpclocals.ltime % 60) + data*60;
      break;
    case WPC_RTCMIN:
      wpclocals.ltime = (wpclocals.ltime / 60) * 60 + data;
      break;
    case WPC_RTCLOAD:
      wpclocals.time = wpclocals.ltime;
      break;
    case WPC_LED:
      wpclocals.diagnostic |= (data>>7);
      break;
    case WPC_PRINTDATA:
      break;
    case WPC_PRINTDATAX:
      if (data == 0) {
        if (wpc_printfile == NULL) {
          char filename[13];

          sprintf(filename,"%s.prt", Machine->gamedrv->name);
          wpc_printfile = osd_fopen(Machine->gamedrv->name,filename,OSD_FILETYPE_MEMCARD,1);
          if (wpc_printfile == NULL) break;
        }
        osd_fwrite(wpc_printfile, &wpc_data[WPC_PRINTDATA], 1);
      }
      break;
    default:
      DBGLOG(("wpc_w %4x %2x\n", offset+WPC_BASE, data));
      break;
  }
  wpc_data[offset] = data;
}

/*--------------------------
/ Security chip simulation
/---------------------------*/
static int wpc_pic_r(void) {
  int ret = 0;
  if (wpclocals.pic.lastW == 0x0d)
    ret = wpclocals.pic.count;
  else if ((wpclocals.pic.lastW >= 0x16) && (wpclocals.pic.lastW <= 0x1f))
    ret = coreGlobals.swMatrix[wpclocals.pic.lastW-0x15];
  else if ((wpclocals.pic.lastW & 0xf0) == 0x70) {
    ret = wpclocals.pic.sData[wpclocals.pic.lastW & 0x0f];
    /* update serial number scrambler */
    wpclocals.pic.sNoS = ((wpclocals.pic.sNoS>>4) | (wpclocals.pic.lastW <<4)) & 0xff;
    wpclocals.pic.sData[5]  = (wpclocals.pic.sData[5]  ^ wpclocals.pic.sNoS) + wpclocals.pic.sData[13];
    wpclocals.pic.sData[13] = (wpclocals.pic.sData[13] + wpclocals.pic.sNoS) ^ wpclocals.pic.sData[5];
  }
  return ret;
}

static void wpc_pic_w(int data) {
  if (wpclocals.pic.codeW > 0) {
    if (wpclocals.pic.codeNo[3 - wpclocals.pic.codeW] != data)
      DBGLOG(("Wrong code %2x (expected %2x) sent to pic.",
               data, wpclocals.pic.codeNo[3 - wpclocals.pic.codeW]));
    wpclocals.pic.codeW -= 1;
  }
  else if (data == 0) {
    wpclocals.pic.sNoS = 0xa5;
    wpclocals.pic.sData[5]  = wpclocals.pic.sData[0] ^ wpclocals.pic.sData[15];
    wpclocals.pic.sData[13] = wpclocals.pic.sData[2] ^ wpclocals.pic.sData[12];
    wpclocals.pic.count = 0x20;
  }
  else if (data == 0x20) {
    wpclocals.pic.codeW = 3;
  }
  else if (data == 0x0d) {
    wpclocals.pic.count = (wpclocals.pic.count - 1) & 0x1f;
  }

  wpclocals.pic.lastW = data;
}

/*-------------------------
/  Generate IRQ interrupt
/--------------------------*/
static int wpc_irq(void) {
  /*-- update rtc clock --*/
  if (++wpclocals.ticks >= 60*WPC_IRQFREQ)
    { wpclocals.time += 1; wpclocals.ticks = 0; }
  cpu_set_irq_line(WPC_CPUNO, M6809_IRQ_LINE, HOLD_LINE);
  return 0;
}

static void wpc_updSw(int *inports) {
  if (inports) {
    coreGlobals.swMatrix[CORE_COINDOORSWCOL] = inports[WPC_COMINPORT] & 0xff;
    /*-- check standard keys --*/
    if (core_gameData->wpc.comSw.start)
      core_setSw(core_gameData->wpc.comSw.start,    inports[WPC_COMINPORT] & WPC_COMSTARTKEY);
    if (core_gameData->wpc.comSw.tilt)
      core_setSw(core_gameData->wpc.comSw.tilt,     inports[WPC_COMINPORT] & WPC_COMTILTKEY);
    if (core_gameData->wpc.comSw.sTilt)
      core_setSw(core_gameData->wpc.comSw.sTilt,    inports[WPC_COMINPORT] & WPC_COMSTILTKEY);
    if (core_gameData->wpc.comSw.coinDoor)
      core_setSw(core_gameData->wpc.comSw.coinDoor, inports[WPC_COMINPORT] & WPC_COMCOINDOORKEY);
    if (core_gameData->wpc.comSw.shooter)
      core_setSw(core_gameData->wpc.comSw.shooter,  inports[CORE_SIMINPORT] & SIM_SHOOTERKEY);
  }
}

static void wpc_sndCmd(int cmd) {
  if (core_gameData->gen & (GEN_WPCDCS|GEN_WPCSECURITY|GEN_WPC95|GEN_WPC95DCS))
    dcs_data_w(0,cmd);
  else
    wpcs_data_w(0,cmd);
}

static core_tData dcs_coreData = {
  8, /* 8 DIPs */
  wpc_updSw,
  1,
  dcs_data_w,
  "dcs"
};
static core_tData wpcs_coreData = {
  8, /* 8 DIPs */
  wpc_updSw,
  1,
  wpcs_data_w,
  "wpcs"
};
static void wpc_init(void) {
                              /*128K  256K        512K        768K       1024K*/
  static const int romLengthMask[] = {0x07, 0x0f, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x3f};
  int romLength = memory_region_length(WPC_MEMREG_ROM);

  /* for some strange reason is there no game_exit or machine_exit functions. */
  /* Don't know why the mame32 or MacMame people doesn't complain */
  /* fake an exit */
  if (wpclocals.initDone) CORE_DOEXIT(wpc_exit);
  if (core_gameData == NULL)  return;

  if (core_gameData->gen & (GEN_WPCDCS|GEN_WPCSECURITY|GEN_WPC95|GEN_WPC95DCS))
    { if (core_init(&dcs_coreData))  return; }
  else
    { if (core_init(&wpcs_coreData)) return; }

  memset(&wpclocals, 0, sizeof(wpclocals));
  wpclocals.initDone = TRUE;

  wpclocals.pageMask = romLengthMask[((romLength>>17)-1)&0x07];

  /* the non-paged ROM is at the end of the image. move it into the CPU region */
  memcpy(memory_region(WPC_MEMREG_CPU) + 0x8000,
         memory_region(WPC_MEMREG_ROM) + romLength - 0x8000, 0x8000);

  /*-- sync counter with vblank --*/
  wpclocals.vblankCount = 1;

  /*-- init sound (if present) --*/
  if (coreGlobals.soundEn) {
    WPC_gWPC95 = (core_gameData->gen & GEN_WPC95) > 0;
    if (core_gameData->gen &
        (GEN_WPCALPHA_2 | GEN_WPCFLIPTRON | GEN_WPCDMD)) wpcs_init();
    else                                                 dcs_init();
  }

  coreGlobals.swMatrix[2] |= 0x08; /* Always closed switch */

  /*-- init security chip (if present) --*/
  if ((core_gameData->gen & (GEN_WPCSECURITY | GEN_WPC95|GEN_WPC95DCS)) &&
      (core_gameData->wpc.serialNo))
    wpc_serialCnv(core_gameData->wpc.serialNo, wpclocals.pic.sData,
                  wpclocals.pic.codeNo);
  /* check flippers we have */
  if ((core_gameData->hw.flippers & FLIP_SOL(FLIP_UR)) == 0) /* No upper right flipper */
    wpclocals.nonFlipBits |= CORE_URFLIPSOLBITS;
  if ((core_gameData->hw.flippers & FLIP_SOL(FLIP_UL)) == 0) /* No upper left flipper */
    wpclocals.nonFlipBits |= CORE_ULFLIPSOLBITS;

  if (options.cheat && !(core_gameData->gen & GEN_WPCALPHA_1)) {
    /*-- speed up startup by disable checksum --*/
    *(memory_region(WPC_MEMREG_CPU) + 0xffec) = 0x00;
    *(memory_region(WPC_MEMREG_CPU) + 0xffed) = 0xff;
  }
}

static void wpc_exit(void) {
  if (wpc_printfile)
    { osd_fclose(wpc_printfile); wpc_printfile = NULL; }
  core_exit();
}

/*-----------------------------------------------
/ Load/Save static ram
/ Most of the SRAM is cleared on startup but the
/ non-cleared part differs from game to game.
/ Better save it all
/ The RAM size was changed from 8K to 16K starting with
/ DCS generation
/-------------------------------------------------*/
void wpc_nvram(void *file, int write) {
  core_nvram(file, write, memory_region(WPC_MEMREG_CPU),
	         (core_gameData->gen & (GEN_WPCDCS | GEN_WPCSECURITY | GEN_WPC95 | GEN_WPC95DCS)) ? 0x3800 : 0x2000);
}

static void wpc_serialCnv(char no[21], UINT8 pic[16], UINT8 code[3]) {
  int x;

  pic[10] = 0x12; /* whatever */
  pic[2]  = 0x34; /* whatever */
  x = (no[5] - '0') + 10*(no[8]-'0') + 100*(no[1]-'0');
  x += pic[10]*5;
  x *= 0x001bcd; /*   7117 = 11*647         */
  x += 0x01f3f0; /* 127984 = 2*2*2*2*19*421 */
  pic[1]  = x >> 16;
  pic[11] = x >> 8;
  pic[9]  = x;
  x = (no[7]-'0') + 10*(no[9]-'0') + 100*(no[0]-'0') + 1000*(no[18]-'0') + 10000*(no[2]-'0');
  x += 2*pic[10] + pic[2];
  x *= 0x0000107f; /*    4223 = 41*103     */
  x += 0x0071e259; /* 7463513 = 53*53*2657 */
  pic[7]  = x >> 24;
  pic[12] = x >> 16;
  pic[0]  = x >> 8;
  pic[8]  = x;
  x = (no[17]-'0') + 10*(no[6]-'0') + 100*(no[4]-'0') + 1000*(no[19]-'0');
  x += pic[2];
  x *= 0x000245; /*   581 = 7*83         */
  x += 0x003d74; /* 15732 =2*2*3*3*19*23 */
  pic[3]  = x >> 16;
  pic[14] = x >> 8;
  pic[6]  = x;
  x = ('9'-no[11]) + 10*('9'-no[12]) + 100*('9'-no[13]) + 1000*('9'-no[14]) + 10000*('9'-no[15]);
  pic[15] = x >> 8;
  pic[4]  = x;

  x = 100*(no[0]-'0') + 10*(no[1]-'0') + (no[2]-'0');
  x = (x >> 8) * (0x100*no[17] + no[19]) + (x & 0xff) * (0x100*no[18] + no[17]);
  code[0] = x >> 16;
  code[1] = x >> 8;
  code[2] = x;
}

/*----------------------------------*/
/* Williams WPC 128x32 DMD Handling */
/*----------------------------------*/
static int dmd_start(void) {
  UINT8 *RAM = memory_region(WPC_MEMREG_DMD);
  int ii;

  for (ii = 0; ii < DMD_FRAMES; ii++)
    coreGlobals_dmd.DMDFrames[ii] = RAM;
  coreGlobals_dmd.nextDMDFrame = 0;
  return 0;
}

static void dmd_refresh(struct mame_bitmap *bitmap, int fullRefresh) {
  tDMDDot dotCol;
  int ii,jj,kk;

  /* Drawing is not optimised so just clear everything */
  if (fullRefresh) fillbitmap(bitmap,Machine->pens[0],NULL);

  /* Create a temporary buffer with all pixels */
  for (kk = 0, ii = 1; ii < 33; ii++) {
    UINT8 *line = &dotCol[ii][0];
    for (jj = 0; jj < 16; jj++) {
      /* Intensity depends on how many times the pixel */
      /* been on in the last 3 frames                  */
      unsigned int intens1 = ((coreGlobals_dmd.DMDFrames[0][kk] & 0x55) +
                              (coreGlobals_dmd.DMDFrames[1][kk] & 0x55) +
                              (coreGlobals_dmd.DMDFrames[2][kk] & 0x55));
      unsigned int intens2 = ((coreGlobals_dmd.DMDFrames[0][kk] & 0xaa) +
                              (coreGlobals_dmd.DMDFrames[1][kk] & 0xaa) +
                              (coreGlobals_dmd.DMDFrames[2][kk] & 0xaa));

      *line++ = (intens1)    & 0x03;
      *line++ = (intens2>>1) & 0x03;
      *line++ = (intens1>>2) & 0x03;
      *line++ = (intens2>>3) & 0x03;
      *line++ = (intens1>>4) & 0x03;
      *line++ = (intens2>>5) & 0x03;
      *line++ = (intens1>>6) & 0x03;
      *line++ = (intens2>>7) & 0x03;
      kk +=1;
    }
    *line = 0; /* to simplify antialiasing */
  }
  dmd_draw(bitmap, dotCol, core_gameData->lcdLayout ? core_gameData->lcdLayout : &wpc_dispDMD[0]);

  drawStatus(bitmap,fullRefresh);
}

