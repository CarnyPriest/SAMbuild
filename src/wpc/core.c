/***********************************************/
/* PINMAME - Interface function (Input/Output) */
/***********************************************/
#include <stdarg.h>
#include "driver.h"
#include "sim.h"
#include "snd_cmd.h"
#include "mech.h"
#include "core.h"

/* stuff to test VPINMAME */
#if 0
#define VPINMAME
int g_fHandleKeyboard = 1, g_fHandleMechanics = 1;
void OnSolenoid(int nSolenoid, int IsActive) {}
void OnStateChange(int nChange) {}
UINT64 vp_getSolMask64(void) { return -1; }
void vp_updateMech(void) {};
int vp_getDip(int bank) { return 0; }
void vp_setDIP(int bank, int value) { }
#endif

#ifdef VPINMAME
  #include "vpintf.h"
  extern int g_fHandleKeyboard, g_fHandleMechanics;
  extern void OnSolenoid(int nSolenoid, int IsActive);
  extern void OnStateChange(int nChange);
#else /* VPINMAME */
  #define g_fHandleKeyboard  (TRUE)
  #define g_fHandleMechanics (0xff)
  #define OnSolenoid(nSolenoid, IsActive)
  #define OnStateChange(nChange)
  #define vp_getSolMask64() ((UINT64)(-1))
  #define vp_updateMech()
  #define vp_setDIP(x,y)
#endif /* VPINMAME */

static void drawChar(struct mame_bitmap *bitmap, int row, int col, UINT32 bits, int type);
static UINT32 core_initDisplaySize(const struct core_dispLayout *layout);
static VIDEO_UPDATE(core_status);

/*---------------------------
/    Global variables
/----------------------------*/
tPMoptions            pmoptions; /* PinMAME specific options */
core_tGlobals         coreGlobals;
struct pinMachine    *coreData;
const core_tGameData *core_gameData = NULL;  /* data about the running game */

/*---------------------
/  Global constants
/----------------------*/
const int core_bcd2seg7[16] = {
/* 0    1    2    3    4    5    6    7    8    9  */
  0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f
#ifdef MAME_DEBUG
/* A    B    C    D    E */
 ,0x77,0x7c,0x39,0x5e,0x79
#endif /* MAME_DEBUG */
};
// including the A to E letters (e.g. Taito)
const int core_bcd2seg7e[16] = {
/* 0    1    2    3    4    5    6    7    8    9  */
  0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f,
/* A    B    C    D    E */
  0x77,0x7c,0x39,0x5e,0x79
};
// missing top line for 6 number (e.g. Atari)
const int core_bcd2seg7a[16] = {
/* 0    1    2    3    4    5    6    7    8    9  */
  0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7c,0x07,0x7f,0x6f
#ifdef MAME_DEBUG
/* A    B    C    D    E */
 ,0x77,0x7c,0x39,0x5e,0x79
#endif /* MAME_DEBUG */
};
const int core_bcd2seg9[16] = {
/* 0     1     2    3    4    5    6    7    8    9  */
  0x3f,0x100,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f
#ifdef MAME_DEBUG
/* A    B    C    D    E */
 ,0x77,0x7c,0x39,0x5e,0x79
#endif /* MAME_DEBUG */
};
// missing top line for 6 number (e.g. Gottlieb Sys80/a)
const int core_bcd2seg9a[16] = {
/* 0     1     2    3    4    5    6    7    8    9  */
  0x3f,0x100,0x5b,0x4f,0x66,0x6d,0x7c,0x07,0x7f,0x6f
#ifdef MAME_DEBUG
/* A    B    C    D    E */
 ,0x77,0x7c,0x39,0x5e,0x79
#endif /* MAME_DEBUG */
};

/* makes it easier to swap bits */
                              // 0  1  2  3  4  5  6  7  8  9 10,11,12,13,14,15
const UINT8 core_swapNyb[16] = { 0, 8, 4,12, 2,10, 6,14, 1, 9, 5,13, 3,11, 7,15};
/* Palette */

static const unsigned char core_palette[COL_COUNT][3] = {
{/*  0 */ 0x00,0x00,0x00}, /* Background */
/* -- DMD DOT COLORS-- */
{/*  1 */ 0x30,0x00,0x00}, /* "Black" Dot - DMD Background */
{/*  2 */ 0x00,0x00,0x00}, /* Intensity  33% - Filled in @ Run Time */
{/*  3 */ 0x00,0x00,0x00}, /* Intensity  66% - Filled in @ Run Time */
{/*  4 */ 0xff,0xe0,0x20}, /* Intensity 100% - Changed @ Run Time to match config vars*/
/* -- PLAYFIELD LAMP COLORS -- */
{/*  5 */ 0x00,0x00,0x00}, /* Black */
{/*  6 */ 0xff,0xff,0xff}, /* White */
{/*  7 */ 0x40,0xff,0x00}, /* green */
{/*  8 */ 0xff,0x00,0x00}, /* Red */
{/*  9 */ 0xff,0x80,0x00}, /* orange */
{/* 10 */ 0xff,0xff,0x00}, /* yellow */
{/* 11 */ 0x00,0x80,0xff}, /* lblue */
{/* 12 */ 0x9f,0x40,0xff}  /* lpurple*/
};

/*-------------------
/  local variables
/-------------------*/
typedef UINT32 tSegRow[17];
typedef struct { int rows, cols; tSegRow *segs; } tSegData;

static struct {
  core_tSeg lastSeg;       // previous segments values
  int       displaySize;   // 1=compact 2=normal
  tSegData  *segData;      // segments to use (normal/compact)
  void      *timers[5];    // allocated timers
  int       flipTimer[4];  // time since flipper was activated (used for EOS simulation)
  UINT8     flipMask;      // Flipper bits used for flippers
  int       firstSimRow, maxSimRows; // space available for simulator
  int       solLog[4];
  int       solLogCount;
} locals;

extern tSegData segData[2][15]; // How do you do a forward static declaration?
/*-------------------------------
/  Initialize the game palette
/-------------------------------*/
static PALETTE_INIT(core) {
  unsigned char tmpPalette[sizeof(core_palette)/3][3];
  int rStart = 0xff, gStart = 0xe0, bStart = 0x20;
  int perc66 = 67, perc33 = 33, perc0  = 20;
  int ii;

  if ((pmoptions.dmd_red > 0) || (pmoptions.dmd_green > 0) || (pmoptions.dmd_blue > 0)) {
    rStart = pmoptions.dmd_red; gStart = pmoptions.dmd_green; bStart = pmoptions.dmd_blue;
  }
  if ((pmoptions.dmd_perc0 > 0) || (pmoptions.dmd_perc33 > 0) || (pmoptions.dmd_perc66 > 0)) {
    perc66 = pmoptions.dmd_perc66; perc33 = pmoptions.dmd_perc33; perc0  = pmoptions.dmd_perc0;
  }
  memcpy(tmpPalette, core_palette, sizeof(core_palette));

  /*-- Autogenerate DMD Color Shades--*/
  tmpPalette[COL_DMDOFF][0]   = rStart * perc0 / 100;
  tmpPalette[COL_DMDOFF][1]   = gStart * perc0 / 100;
  tmpPalette[COL_DMDOFF][2]   = bStart * perc0 / 100;
  tmpPalette[COL_DMD33][0]    = rStart * perc33 / 100;
  tmpPalette[COL_DMD33][1]    = gStart * perc33 / 100;
  tmpPalette[COL_DMD33][2]    = bStart * perc33 / 100;
  tmpPalette[COL_DMD66][0]    = rStart * perc66 / 100;
  tmpPalette[COL_DMD66][1]    = gStart * perc66 / 100;
  tmpPalette[COL_DMD66][2]    = bStart * perc66 / 100;
  tmpPalette[COL_DMDON][0]    = rStart;
  tmpPalette[COL_DMDON][1]    = gStart;
  tmpPalette[COL_DMDON][2]    = bStart;
  /*-- segment display antialias colors --*/
  tmpPalette[COL_SEGAAON1][0] = rStart * 72 / 100;
  tmpPalette[COL_SEGAAON1][1] = gStart * 72 / 100;
  tmpPalette[COL_SEGAAON1][2] = bStart * 72 / 100;
  tmpPalette[COL_SEGAAON2][0] = rStart * 33 / 100;
  tmpPalette[COL_SEGAAON2][1] = gStart * 33 / 100;
  tmpPalette[COL_SEGAAON2][2] = bStart * 33 / 100;
  rStart = tmpPalette[COL_DMDOFF][0];
  gStart = tmpPalette[COL_DMDOFF][1];
  bStart = tmpPalette[COL_DMDOFF][2];
  tmpPalette[COL_SEGAAOFF1][0] = rStart * 72 / 100;
  tmpPalette[COL_SEGAAOFF1][1] = gStart * 72 / 100;
  tmpPalette[COL_SEGAAOFF1][2] = bStart * 72 / 100;
  tmpPalette[COL_SEGAAOFF2][0] = rStart * 33 / 100;
  tmpPalette[COL_SEGAAOFF2][1] = gStart * 33 / 100;
  tmpPalette[COL_SEGAAOFF2][2] = bStart * 33 / 100;

  /*-- Autogenerate Dark Playfield Lamp Colors --*/
  for (ii = 0; ii < COL_LAMPCOUNT; ii++) { /* Reduce by 75% */
    tmpPalette[COL_LAMP+COL_LAMPCOUNT+ii][0] = (tmpPalette[COL_LAMP+ii][0] * 25) / 100;
    tmpPalette[COL_LAMP+COL_LAMPCOUNT+ii][1] = (tmpPalette[COL_LAMP+ii][1] * 25) / 100;
    tmpPalette[COL_LAMP+COL_LAMPCOUNT+ii][2] = (tmpPalette[COL_LAMP+ii][2] * 25) / 100;
  }

  { /*-- Autogenerate antialias colours --*/
    int rStep, gStep, bStep;

    rStep = (tmpPalette[COL_DMDON][0] * pmoptions.dmd_antialias / 100 - rStart) / 6;
    gStep = (tmpPalette[COL_DMDON][1] * pmoptions.dmd_antialias / 100 - gStart) / 6;
    bStep = (tmpPalette[COL_DMDON][2] * pmoptions.dmd_antialias / 100 - bStart) / 6;

    for (ii = 1; ii < COL_DMDAACOUNT; ii++) { // first is black
      tmpPalette[COL_DMDAA+ii][0] = rStart;
      tmpPalette[COL_DMDAA+ii][1] = gStart;
      tmpPalette[COL_DMDAA+ii][2] = bStart;
      rStart += rStep; gStart += gStep; bStart += bStep;
    }
  }
#if MAMEVER >= 6100
  for (ii = 0; ii < sizeof(tmpPalette)/3; ii++)
    palette_set_color(ii, tmpPalette[ii][0], tmpPalette[ii][1], tmpPalette[ii][2]);
#else /* MAMEVER */
  memcpy(palette, tmpPalette, sizeof(tmpPalette));
#endif /* MAMEVER */
}

/*-----------------------------------
/    Generic DMD display handler
/------------------------------------*/
void video_update_core_dmd(struct mame_bitmap *bitmap, const struct rectangle *cliprect, tDMDDot dotCol, const struct core_dispLayout *layout) {
  UINT32 *dmdColor = &CORE_COLOR(COL_DMDOFF);
  UINT32 *aaColor  = &CORE_COLOR(COL_DMDAA);
  BMTYPE **lines = ((BMTYPE **)bitmap->line) + (layout->top*locals.displaySize);
  int ii, jj;

  memset(&dotCol[layout->start+1][0], 0, sizeof(dotCol[0][0])*layout->length+1);
  memset(&dotCol[0][0], 0, sizeof(dotCol[0][0])*layout->length+1); // clear above
  for (ii = 0; ii < layout->start+1; ii++) {
    BMTYPE *line = (*lines++) + (layout->left*locals.displaySize);
    dotCol[ii][layout->length] = 0;
    if (ii > 0) {
      for (jj = 0; jj < layout->length; jj++) {
        *line++ = dmdColor[dotCol[ii][jj]];
        if (locals.displaySize > 1)
          *line++ = (layout->type & CORE_DMDNOAA) ? 0 : aaColor[dotCol[ii][jj] + dotCol[ii][jj+1]];
      }
    }
    if (locals.displaySize > 1) {
      int col1 = dotCol[ii][0] + dotCol[ii+1][0];
      line = (*lines++) + (layout->left*locals.displaySize);
      for (jj = 0; jj < layout->length; jj++) {
        int col2 = dotCol[ii][jj+1] + dotCol[ii+1][jj+1];
        *line++ = (layout->type & CORE_DMDNOAA) ? 0 : aaColor[col1];
        *line++ = (layout->type & CORE_DMDNOAA) ? 0 : aaColor[2*(col1 + col2)/5];
        col1 = col2;
      }
    }
  }
  osd_mark_dirty(layout->left*locals.displaySize,layout->top*locals.displaySize,
                 (layout->left+layout->length)*locals.displaySize,(layout->top+layout->start)*locals.displaySize);
}
#ifdef VPINMAME
#  define inRect(r,l,t,w,h) FALSE
#else /* VPINMAME */
INLINE int inRect(const struct rectangle *r, int left, int top, int width, int height) {
  return (r->max_x >= left) && (r->max_y >= top) &&
         (r->min_x <= left + width) && (r->min_y <= top + height);
}
#endif /* VPINMAME */

/*-----------------------------------
/  Generic segement display handler
/------------------------------------*/
static void updateDisplay(struct mame_bitmap *bitmap, const struct rectangle *cliprect,
                          const struct core_dispLayout *layout, int *pos) {
  if (layout == NULL) { DBGLOG(("gen_refresh without LCD layout\n")); return; }
  for (; layout->length; layout += 1) {
    if (layout->type == CORE_IMPORT)
      { updateDisplay(bitmap, cliprect, (const struct core_dispLayout *)layout->ptr, pos); continue; }
    if (layout->ptr)
      if (((ptPinMAMEvidUpdate)(layout->ptr))(bitmap,cliprect,layout) == 0) continue;
    {
      int zeros = layout->type/32; // dummy zeros
      int left  = layout->left * (locals.segData[layout->type & CORE_SEGMASK].cols+1) / 2;
      int top   = layout->top  * (locals.segData[0].rows + 1) / 2;
      int ii    = layout->length + zeros;
      UINT16 *seg     = &coreGlobals.segments[layout->start].w;
      UINT16 *lastSeg = &locals.lastSeg[layout->start].w;
      int step     = (layout->type & CORE_SEGREV) ? -1 : 1;

      if (step < 0) { seg += ii-1; lastSeg += ii-1; }
      while (ii--) {
        UINT16 tmpSeg = (ii < zeros) ? ((core_bcd2seg7[0]<<8) | (core_bcd2seg7[0])) : *seg;
        int tmpType = layout->type & CORE_SEGMASK;

        if ((tmpSeg != *lastSeg) ||
            inRect(cliprect,left,top,locals.segData[layout->type & 0x0f].cols,locals.segData[layout->type & 0x0f].rows)) {
          tmpSeg >>= (layout->type & CORE_SEGHIBIT) ? 8 : 0;

          switch (tmpType) {
          case CORE_SEG87: case CORE_SEG87F:
            if ((ii > 0) && (ii % 3 == 0)) { // Handle Comma
              if ((tmpType == CORE_SEG87F) && tmpSeg) tmpSeg |= 0x80;
              tmpType = CORE_SEG8;
            } else
              tmpType = CORE_SEG7;
            break;
          case CORE_SEG87FD:
            if ((ii > 0) && (ii % 3 == 0)) { // Handle Dot
              if (tmpSeg) tmpSeg |= 0x80;
            } else
              tmpType = CORE_SEG7;
            break;
          case CORE_SEG98: case CORE_SEG98F:
            tmpSeg |= (tmpSeg & 0x100)<<1;
            if ((ii > 0) && (ii % 3 == 0)) { // Handle Comma
              if ((tmpType == CORE_SEG98F) && tmpSeg) tmpSeg |= 0x80;
              tmpType = CORE_SEG10;
            } else
              tmpType = CORE_SEG9;
            break;
          case CORE_SEG9:
            tmpSeg |= (tmpSeg & 0x100)<<1;
            break;
          }
          drawChar(bitmap,  top, left, tmpSeg, tmpType);
          coreGlobals.drawSeg[*pos] = tmpSeg;
        }
		(*pos)++;
        left += locals.segData[layout->type & 0x0f].cols+1;
        seg += step; lastSeg += step;
      }
    }
  }
}

VIDEO_UPDATE(core_gen) {
  int count = 0;
  updateDisplay(bitmap, cliprect, core_gameData->lcdLayout, &count);
  memcpy(locals.lastSeg, coreGlobals.segments, sizeof(locals.lastSeg));
  video_update_core_status(bitmap,cliprect);
}

/*---------------------
/  Update all switches
/----------------------*/
void core_updateSw(int flipEn) {
  /*-- handle flippers--*/
  const int flip = core_gameData->hw.flippers;
  const int flipSwCol = (core_gameData->gen & (GEN_GTS3 | GEN_ALVG)) ? 15 : CORE_FLIPPERSWCOL;
  int inports[CORE_MAXPORTS];
  UINT8 swFlip;
  int ii;

  if (g_fHandleKeyboard) {
    for (ii = 0; ii < CORE_COREINPORT+(coreData->coreDips+31)/16; ii++)
      inports[ii] = readinputport(ii);

    /*-- buttons --*/
    swFlip = 0;
    if (inports[CORE_FLIPINPORT] & CORE_LLFLIPKEY) swFlip |= CORE_SWLLFLIPBUTBIT;
    if (inports[CORE_FLIPINPORT] & CORE_LRFLIPKEY) swFlip |= CORE_SWLRFLIPBUTBIT;
    if (locals.flipMask & CORE_SWULFLIPBUTBIT) {    /* have UL switch */
      if (flip & FLIP_BUT(FLIP_UL))
        { if (inports[CORE_FLIPINPORT] & CORE_ULFLIPKEY) swFlip |= CORE_SWULFLIPBUTBIT; }
      else
        { if (inports[CORE_FLIPINPORT] & CORE_LLFLIPKEY) swFlip |= CORE_SWULFLIPBUTBIT; }
    }
    if (locals.flipMask & CORE_SWURFLIPBUTBIT) {    /* have UR switch */
      if (flip & FLIP_BUT(FLIP_UR))
        { if (inports[CORE_FLIPINPORT] & CORE_URFLIPKEY) swFlip |= CORE_SWURFLIPBUTBIT; }
      else
        { if (inports[CORE_FLIPINPORT] & CORE_LRFLIPKEY) swFlip |= CORE_SWURFLIPBUTBIT; }
    }
  }
  else
    swFlip = (coreGlobals.swMatrix[flipSwCol] ^ coreGlobals.invSw[flipSwCol]) & (CORE_SWULFLIPBUTBIT|CORE_SWURFLIPBUTBIT|CORE_SWLLFLIPBUTBIT|CORE_SWLRFLIPBUTBIT);

  /*-- set switches in matrix for non-fliptronic games --*/
  if (FLIP_SWL(flip)) core_setSw(FLIP_SWL(flip), swFlip & CORE_SWLLFLIPBUTBIT);
  if (FLIP_SWR(flip)) core_setSw(FLIP_SWR(flip), swFlip & CORE_SWLRFLIPBUTBIT);

  /*-- fake solenoids if not CPU controlled --*/
  if ((flip & FLIP_SOL(FLIP_L)) == 0) {
    coreGlobals.solenoids2 &= 0xffffff00;
    if (flipEn) {
      if (swFlip & CORE_SWLLFLIPBUTBIT) coreGlobals.solenoids2 |= CORE_LLFLIPSOLBITS;
      if (swFlip & CORE_SWLRFLIPBUTBIT) coreGlobals.solenoids2 |= CORE_LRFLIPSOLBITS;
    }
  }

  /*-- EOS switches --*/
  if (locals.flipMask & CORE_SWULFLIPEOSBIT) {
    if (core_getSol(sULFlip)) locals.flipTimer[0] += 1;
    else                      locals.flipTimer[0] = 0;
    if (locals.flipTimer[0] >= CORE_FLIPSTROKETIME) swFlip |= CORE_SWULFLIPEOSBIT;
  }
  if (locals.flipMask & CORE_SWURFLIPEOSBIT) {
    if (core_getSol(sURFlip)) locals.flipTimer[1] += 1;
    else                      locals.flipTimer[1] = 0;
    if (locals.flipTimer[1] >= CORE_FLIPSTROKETIME) swFlip |= CORE_SWURFLIPEOSBIT;
  }
  if (locals.flipMask & CORE_SWLLFLIPEOSBIT) {
    if (core_getSol(sLLFlip)) locals.flipTimer[2] += 1;
    else                      locals.flipTimer[2] = 0;
    if (locals.flipTimer[2] >= CORE_FLIPSTROKETIME) swFlip |= CORE_SWLLFLIPEOSBIT;
  }
  if (locals.flipMask & CORE_SWLRFLIPEOSBIT) {
    if (core_getSol(sLRFlip)) locals.flipTimer[3] += 1;
    else                      locals.flipTimer[3] = 0;
    if (locals.flipTimer[3] >= CORE_FLIPSTROKETIME) swFlip |= CORE_SWLRFLIPEOSBIT;
  }
  coreGlobals.swMatrix[flipSwCol] = (coreGlobals.swMatrix[flipSwCol]         & ~locals.flipMask) |
                                    ((swFlip ^ coreGlobals.invSw[flipSwCol]) &  locals.flipMask);

  /*-- update core dependent switches --*/
  if (coreData->updSw)  coreData->updSw(g_fHandleKeyboard ? inports : NULL);

  /*-- update game dependent switches --*/
  if (g_fHandleMechanics) {
    if (core_gameData->hw.handleMech) core_gameData->hw.handleMech(g_fHandleMechanics);
  }
  /*-- Run simulator --*/
  if (coreGlobals.simAvail)
    sim_run(inports, CORE_COREINPORT+(coreData->coreDips+31)/16,
            (inports[CORE_SIMINPORT] & SIM_SWITCHKEY) == 0,
            (SIM_BALLS(inports[CORE_SIMINPORT])));
  { /*-- check changed solenoids --*/
    UINT64 allSol = core_getAllSol();
    UINT64 chgSol = (allSol ^ coreGlobals.lastSol) & vp_getSolMask64();

    if (chgSol) {
      coreGlobals.lastSol = allSol;
      for (ii = 1; ii < CORE_FIRSTCUSTSOL+core_gameData->hw.custSol; ii++) {
        if (chgSol & 0x01) {
          /*-- solenoid has changed state --*/
          OnSolenoid(ii, allSol & 0x01);
          /*-- log solenoid number on the display (except flippers) --*/
          if ((!pmoptions.dmd_only && (allSol & 0x01)) &&
              ((ii < CORE_FIRSTLFLIPSOL) || (ii >= CORE_FIRSTSIMSOL))) {
            locals.solLog[locals.solLogCount] = ii;
	    core_textOutf(Machine->visible_area.max_x - 12*8,0,BLACK,"%2d %2d %2d %2d",
              locals.solLog[(locals.solLogCount+1) & 3],
              locals.solLog[(locals.solLogCount+2) & 3],
              locals.solLog[(locals.solLogCount+3) & 3],
              locals.solLog[(locals.solLogCount+0) & 3]);
            locals.solLogCount = (locals.solLogCount + 1) & 3;
          }
          if (coreGlobals.soundEn)
            proc_mechsounds(ii, allSol & 0x01);
        }
        chgSol >>= 1;
        allSol >>= 1;
      }
    }
  }

  /*-- check if we should use simulator keys --*/
  if (g_fHandleKeyboard &&
      (!coreGlobals.simAvail || inports[CORE_SIMINPORT] & SIM_SWITCHKEY)) {
    /*-- simulator keys disabled, use row+column keys --*/
    static int lastRow = 0, lastCol = 0;
    int row = 0, col = 0;

    if (((inports[CORE_MANSWINPORT] & CORE_MANSWCOLUMNS) == 0) ||
        ((inports[CORE_MANSWINPORT] & CORE_MANSWROWS) == 0))
      lastRow = lastCol = 0;
    else {
      int bit = 0x0101;

      for (ii = 0; ii < 8; ii++) {
        if (inports[CORE_MANSWINPORT] & CORE_MANSWCOLUMNS & bit) col = ii+1;
        if (inports[CORE_MANSWINPORT] & CORE_MANSWROWS    & bit) row = ii+1;
        bit <<= 1;
      }
      if ((col != lastCol) || (row != lastRow)) {
        coreGlobals.swMatrix[col] ^= (1<<(row-1));
        lastCol = col; lastRow = row;
      }
    }
  }

#ifdef MAME_DEBUG /* Press W and E at the same time to insert a mark in logfile */
  if (g_fHandleKeyboard && ((inports[CORE_MANSWINPORT] & 0x06) == 0x06))
    logerror("\nLogfile Mark\n");
#endif /* MAME_DEBUG */
}

/*--------------------------
/ Write text on the screen
/---------------------------*/
void core_textOut(char *buf, int length, int x, int y, int color) {
  if (y < locals.maxSimRows) {
    int ii, l;

    l = strlen(buf);
    for (ii = 0; ii < length; ii++) {
      char c = (ii >= l) ? ' ' : buf[ii];

      drawgfx(Machine->scrbitmap, Machine->uifont, c, color-1, 0, 0,
              x + ii * Machine->uifont->width, y+locals.firstSimRow, 0,
              TRANSPARENCY_NONE, 0);
    }
  }
}

/*-----------------------------------
/ Write formatted text on the screen
/------------------------------------*/
void CLIB_DECL core_textOutf(int x, int y, int color, const char *text, ...) {
  va_list arg;
  if (y < locals.maxSimRows) {
    char buf[100];
    char *bufPtr = buf;

    va_start(arg, text); vsprintf(buf, text, arg); va_end(arg);

    while (*bufPtr) {
      drawgfx(Machine->scrbitmap, Machine->uifont, *bufPtr++, color-1, 0, 0,
              x, y+locals.firstSimRow, 0, TRANSPARENCY_NONE, 0);
      x += Machine->uifont->width;
    }
  }
}

/*--------------------------------------------
/ Draw status display
/ Lamps, Switches, Solenoids, Diagnostic LEDs
/---------------------------------------------*/
static VIDEO_UPDATE(core_status) {
  BMTYPE **lines = (BMTYPE **)bitmap->line;
  int startRow = 0, nextCol = 0, thisCol = 0;
  int ii, jj, bits;
  BMTYPE dotColor[2];

  /*-- anything to do ? --*/
  if ((pmoptions.dmd_only) || (locals.maxSimRows < 16) ||
      (coreGlobals.soundEn && (!manual_sound_commands(bitmap))))
    return;

  dotColor[0] = CORE_COLOR(COL_DMDOFF); dotColor[1] = CORE_COLOR(COL_DMDON);
  /*--  Draw lamps --*/
  if ((core_gameData->hw.lampData) &&
      (startRow + core_gameData->hw.lampData->startpos.x + core_gameData->hw.lampData->size.x < locals.maxSimRows)) {
    core_tLampDisplay *drawData = core_gameData->hw.lampData;
    int startx = drawData->startpos.x;
    int starty = drawData->startpos.y + thisCol;
    BMTYPE **line = &lines[locals.firstSimRow + startRow + startx];
    int num = 0;
    int qq;

    for (ii = 0; ii < CORE_CUSTLAMPCOL+core_gameData->hw.lampCol; ii++) {
      bits = coreGlobals.lampMatrix[ii];

      for (jj = 0; jj < 8; jj++) {
	for (qq = 0; qq < drawData->lamps[num].totnum; qq++) {
	  int color = drawData->lamps[num].lamppos[qq].color;
	  int lampx = drawData->lamps[num].lamppos[qq].x;
	  int lampy = drawData->lamps[num].lamppos[qq].y;
	  line[lampx][starty + lampy] = CORE_COLOR((bits & 0x01) ? color : COL_SHADE(color));
	}
        bits >>= 1;
        num++;
      }
    }
    osd_mark_dirty(starty,  locals.firstSimRow + startRow + startx,
                   starty + drawData->size.y,
                   locals.firstSimRow + startRow + startx + drawData->size.x);
    startRow += startx + drawData->size.x;
    if (starty + drawData->size.y > nextCol) nextCol = starty + drawData->size.y;
  }
  /*-- Defult square lamp matrix layout --*/
  else {
    for (ii = 0; ii < CORE_CUSTLAMPCOL + core_gameData->hw.lampCol; ii++) {
      BMTYPE **line = &lines[locals.firstSimRow + startRow];
      bits = coreGlobals.lampMatrix[ii];

      for (jj = 0; jj < 8; jj++) {
        line[0][thisCol + ii*2] = dotColor[bits & 0x01];
        line += 2; bits >>= 1;
      }
    }
    osd_mark_dirty(thisCol, locals.firstSimRow + startRow, thisCol + ii*2 ,locals.firstSimRow + startRow + 16);
    startRow += 16; if (thisCol + ii*2 > nextCol) nextCol = thisCol + ii*2;
  } /* else */

  /* Draw the switches */
  startRow += 3;
  if (startRow + 16 >= locals.maxSimRows) { startRow = 0; thisCol = nextCol + 5; }

  for (ii = 0; ii < CORE_CUSTSWCOL+core_gameData->hw.swCol; ii++) {
    BMTYPE **line = &lines[locals.firstSimRow + startRow];
    bits = coreGlobals.swMatrix[ii];

    for (jj = 0; jj < 8; jj++) {
      line[0][thisCol + ii*2] = dotColor[bits & 0x01];
      line += 2; bits >>= 1;
    }
  }
  osd_mark_dirty(thisCol, locals.firstSimRow + startRow, thisCol + ii*2, locals.firstSimRow + startRow + 16);
  startRow += 16; if (thisCol + ii*2 > nextCol) nextCol = thisCol + ii*2;

  /* Draw Solenoids and Flashers */
  startRow += 3;

  if (startRow + 16 >= locals.maxSimRows) { startRow = 0; thisCol = nextCol + 5; }

  {
    BMTYPE **line = &lines[locals.firstSimRow + startRow];
    UINT64 allSol = core_getAllSol();
    for (ii = 0; ii < CORE_FIRSTCUSTSOL+core_gameData->hw.custSol; ii++) {
      line[(ii/8)*2][thisCol + (ii%8)*2] = dotColor[allSol & 0x01];
      allSol >>= 1;
    }
    osd_mark_dirty(thisCol, locals.firstSimRow + startRow, thisCol + 16,
        locals.firstSimRow + startRow + 16);
    startRow += 16; if (thisCol + 16 > nextCol) nextCol = thisCol + 16;
  }

  /*-- draw diagnostic LEDs     --*/
  startRow += 3;

  if (startRow + 16 >= locals.maxSimRows) { startRow = 0; thisCol = nextCol; }

  if (coreData->diagLEDs == 0xff) { /* 7 SEG */
    drawChar(bitmap, locals.firstSimRow + startRow, thisCol, coreGlobals.diagnosticLed, 2);
    startRow += 16; if (thisCol + 12 > nextCol) nextCol = thisCol + 12;
  }
  else {
    BMTYPE **line = &lines[locals.firstSimRow + startRow];
    bits = coreGlobals.diagnosticLed;

    // Draw LEDS Vertically
    if (coreData->diagLEDs & DIAGLED_VERTICAL) {
      for (ii = 0; ii < (coreData->diagLEDs & ~DIAGLED_VERTICAL); ii++) {
        line[0][thisCol + 3] = dotColor[bits & 0x01];
	line += 2; bits >>= 1;
      }
      osd_mark_dirty(thisCol + 3, locals.firstSimRow + startRow, thisCol + 4, locals.firstSimRow + startRow + ii*2);
      startRow += ii*2; if (thisCol + 4 > nextCol) nextCol = thisCol + 4;
    }
    else { // Draw LEDS Horizontally
      for (ii = 0; ii < coreData->diagLEDs; ii++) {
	line[0][thisCol + ii*2] = dotColor[bits & 0x01];
        bits >>= 1;
      }
      osd_mark_dirty(thisCol, locals.firstSimRow + startRow, thisCol + ii*2, locals.firstSimRow + startRow + 1);
      startRow += 1; if (thisCol + ii*2 > nextCol) nextCol = thisCol + ii*2;
    }
  }
  /*-- GI Strings --*/
  if (core_gameData->gen & GEN_ALLWPC) {
    startRow += 3;
    if (startRow + 2 >= locals.maxSimRows) { startRow = 0; thisCol = nextCol + 5; }

    for (ii = 0; ii < CORE_MAXGI; ii++)
      lines[locals.firstSimRow + startRow][thisCol + ii*2] = dotColor[coreGlobals.gi[ii]>0];
    osd_mark_dirty(thisCol, locals.firstSimRow + startRow, thisCol + ii*2, locals.firstSimRow + startRow + 1);
  }
  if (coreGlobals.simAvail) sim_draw(locals.firstSimRow);
  /*-- draw game specific mechanics --*/
  if (core_gameData->hw.drawMech) core_gameData->hw.drawMech((void *)&bitmap->line[locals.firstSimRow]);
}

/*-- lamp handling --*/
void core_setLamp(UINT8 *lampMatrix, int col, int row) {
  while (col) {
    if (col & 0x01) *lampMatrix |= row;
    col >>= 1;
    lampMatrix += 1;
  }
}

/*-- "normal" switch/lamp numbering (1-64) --*/
int core_swSeq2m(int no) { return no+7; }
int core_m2swSeq(int col, int row) { return col*8+row-7; }

/*------------------------------------------
/  Read the current switch value
/
/  This function returns TRUE for active
/  switches even if the switch is active low.
/-------------------------------------------*/
int core_getSw(int swNo) {
  if (coreData->sw2m) swNo = coreData->sw2m(swNo); else swNo = (swNo/10)*8+(swNo%10-1);
  return (coreGlobals.swMatrix[swNo/8] ^ coreGlobals.invSw[swNo/8]) & (1<<(swNo%8));
}

int core_getSwCol(int colEn) {
  int ii = 1;
  if (colEn) {
    while ((colEn & 0x01) == 0) {
      colEn >>= 1;
      ii += 1;
    }
  }
  return coreGlobals.swMatrix[ii];
}

/*----------------------
/  Set/reset a switch
/-----------------------*/
void core_setSw(int swNo, int value) {
  if (coreData->sw2m) swNo = coreData->sw2m(swNo); else swNo = (swNo/10)*8+(swNo%10-1);
  coreGlobals.swMatrix[swNo/8] &= ~(1<<(swNo%8)); /* clear the bit first */
  coreGlobals.swMatrix[swNo/8] |=  ((value ? 0xff : 0) ^ coreGlobals.invSw[swNo/8]) & (1<<(swNo%8));
}

/*-------------------------
/  update active low/high
/-------------------------*/
void core_updInvSw(int swNo, int inv) {
  int bit;
  if (coreData->sw2m) swNo = coreData->sw2m(swNo); else swNo = (swNo/10)*8+(swNo%10-1);
  bit = (1 << (swNo%8));

  if (inv)
    inv = bit;
  if ((coreGlobals.invSw[swNo/8] ^ inv) & bit) {
    coreGlobals.invSw[swNo/8] ^= bit;
    coreGlobals.swMatrix[swNo/8] ^= bit;
  }
}

/*-------------------------------------
/  Read the status of a solenoid
/  For the standard solenoids this is
/  the "smoothed" value
/--------------------------------------*/
int core_getSol(int solNo) {
  if (solNo <= 28)
    return coreGlobals.solenoids & CORE_SOLBIT(solNo);
  else if (solNo <= 32) { // 29-32
    if (core_gameData->gen & GEN_ALLS11)
      return coreGlobals.solenoids & CORE_SOLBIT(solNo);
    else if (core_gameData->gen & GEN_ALLWPC) // GI circuits
      return coreGlobals.solenoids2 & (1<<(solNo-29+8)); // GameOn
  }
  else if (solNo <= 36) { // 33-36 Upper flipper (WPC only)
    if (core_gameData->gen & GEN_ALLWPC) {
      int mask;
      /*-- flipper coils --*/
      if      ((solNo == sURFlip) && (core_gameData->hw.flippers & FLIP_SOL(FLIP_UR)))
        mask = CORE_URFLIPSOLBITS;
      else if ((solNo == sULFlip) && (core_gameData->hw.flippers & FLIP_SOL(FLIP_UL)))
        mask = CORE_ULFLIPSOLBITS;
      else
        mask = 1<<(solNo - 33 + 4);
      return coreGlobals.solenoids2 & mask;
    }
  }
  else if (solNo <= 44) { // 37-44 WPC95 & S11 extra
    if (core_gameData->gen & (GEN_WPC95|GEN_WPC95DCS))
      return coreGlobals.solenoids & (1<<((solNo - 13)|4));
    if (core_gameData->gen & GEN_ALLS11)
      return coreGlobals.solenoids2 & (1<<(solNo - 37 + 8));
  }
  else if (solNo <= 48) { // 45-48 Lower flippers
    int mask = 1<<(solNo - 45);
    /*-- Game must have lower flippers but for symmetry we check anyway --*/
    if      ((solNo == sLRFlip) /*&& (core_gameData->hw.flippers & FLIP_SOL(FLIP_LR))*/)
      mask = CORE_LRFLIPSOLBITS;
    else if ((solNo == sLLFlip) /*&& (core_gameData->hw.flippers & FLIP_SOL(FLIP_LL))*/)
      mask = CORE_LLFLIPSOLBITS;
    return coreGlobals.solenoids2 & mask;
  }
  else if (solNo <= 50) // 49-50 simulated
    return sim_getSol(solNo);
  else if (core_gameData->hw.getSol)
    return core_gameData->hw.getSol(solNo);
  return 0;
}

/*-------------------------------------
/  Read the instant status of a solenoid
/--------------------------------------*/
int core_getPulsedSol(int solNo) {
  if (solNo <= 32)
    return coreGlobals.pulsedSolState & CORE_SOLBIT(solNo);
  else if ((core_gameData->gen & (GEN_WPC95|GEN_WPC95DCS)) && (solNo >= 37) && (solNo <= 44))
    // This is a little messy. Pulsed state is in 29-32 but 29-32 non pulsed is GameOn sol
    return coreGlobals.pulsedSolState & (1<<((solNo-13)|4));
  return core_getSol(solNo); /* sol is not smoothed anyway */
}

/*-------------------------------------------------
/  Get the value of all solenoids in one variable
/--------------------------------------------------*/
UINT64 core_getAllSol(void) {
  UINT64 sol = coreGlobals.solenoids;
  if (core_gameData->gen & GEN_ALLWPC) // 29-32 GameOn
    sol = (sol & 0x0fffffff) | ((coreGlobals.solenoids2 & 0x0f00)<<20);
  if (core_gameData->gen & (GEN_WPC95|GEN_WPC95DCS)) { // 37-44 WPC95 extra
    UINT64 tmp = coreGlobals.solenoids & 0xf0000000;
    sol |= (tmp<<12)|(tmp<<8);
  }
  if (core_gameData->gen & GEN_ALLS11) // 37-44 S11 extra
    sol |= ((UINT64)(coreGlobals.solenoids2 & 0xff00))<<28;
  { // 33-36, 45-48 flipper solenoids
    UINT8 lFlip = (coreGlobals.solenoids2 & (CORE_LRFLIPSOLBITS|CORE_LLFLIPSOLBITS));
    UINT8 uFlip = (coreGlobals.solenoids2 & (CORE_URFLIPSOLBITS|CORE_ULFLIPSOLBITS));
    // hold coil is set if either coil is set
    lFlip = lFlip | ((lFlip & 0x05)<<1);
    if (core_gameData->hw.flippers & FLIP_SOL(FLIP_UR))
      uFlip = uFlip | ((uFlip & 0x10)<<1);
    if (core_gameData->hw.flippers & FLIP_SOL(FLIP_UL))
      uFlip = uFlip | ((uFlip & 0x40)<<1);
    sol |= (((UINT64)lFlip)<<44) | (((UINT64)uFlip)<<28);
  }
  /*-- simulated --*/
  sol |= sim_getSol(49) ? (((UINT64)1)<<48) : 0;
  /*-- custom --*/
  if ( core_gameData->hw.getSol ) {
    UINT64 bit = ((UINT64)1)<<(CORE_FIRSTCUSTSOL-1);
    int ii;

    for (ii = 0; ii < core_gameData->hw.custSol; ii++) {
      sol |= core_gameData->hw.getSol(CORE_FIRSTCUSTSOL + ii) ? bit : 0;
      bit <<= 1;
    }
  }
  return sol;
}

/*---------------------------------------
/  Get the status of a DIP bank (8 dips)
/-----------------------------------------*/
int core_getDip(int dipBank) {
#ifdef VPINMAME
  return vp_getDIP(dipBank);
#else /* VPINMAME */
  return (readinputport(CORE_COREINPORT+1+dipBank/2)>>((dipBank & 0x01)*8))&0xff;
#endif /* VPINMAME */
}

/*--------------------
/   Draw a LED digit
/---------------------*/
static void drawChar(struct mame_bitmap *bitmap, int row, int col, UINT32 bits, int type) {
  const tSegData *s = &locals.segData[type];
  UINT32 pixel[21];
  int kk,ll;

  memset(pixel,0,sizeof(pixel));

  for (kk = 1; bits; kk++, bits >>= 1) {
    if (bits & 0x01)
      for (ll = 0; ll < s->rows; ll++)
        pixel[ll] |= s->segs[ll][kk];
  }
  for (kk = 0; kk < s->rows; kk++) {
    static const int pens[4][4] = {{         0,    COL_DMDON, COL_SEGAAON1, COL_SEGAAON2},
                                   {COL_DMDOFF,    COL_DMDON, COL_SEGAAON1, COL_SEGAAON2},
                                   {COL_SEGAAOFF1, COL_DMDON, COL_SEGAAON1, COL_SEGAAON2},
                                   {COL_SEGAAOFF2, COL_DMDON, COL_SEGAAON1, COL_SEGAAON2}};
    BMTYPE *line = &((BMTYPE **)(bitmap->line))[row+kk][col + s->cols];
    // why don't the bitmap use the leftmost bits. i.e. size is limited to 15
    UINT32 p = pixel[kk]>>(30-2*s->cols), np = s->segs[kk][0]>>(30-2*s->cols);

    for (ll = 0; ll < s->cols; ll++, p >>= 2, np >>= 2)
      *(--line) = CORE_COLOR(pens[np & 0x03][p & 0x03]);
  }
  osd_mark_dirty(col,row,col+s->cols,row+s->rows);
}

/*----------------------
/  Initialize PinMAME
/-----------------------*/
static MACHINE_INIT(core) {
  if (!coreData) { // first time
    /*-- init variables --*/
    memset(&coreGlobals, 0, sizeof(coreGlobals));
    memset(&locals, 0, sizeof(locals));
    memset(&locals.lastSeg, -1, sizeof(locals.lastSeg));
    coreData = (struct pinMachine *)&Machine->drv->pinmame;
    //-- initialise timers --
    if (coreData->timers[0].callback) {
      int ii;
      for (ii = 0; ii < 5; ii++) {
        if (coreData->timers[ii].callback) {
          locals.timers[ii] = timer_alloc(coreData->timers[ii].callback);
          timer_adjust(locals.timers[ii], TIME_IN_HZ(coreData->timers[ii].rate), 0, TIME_IN_HZ(coreData->timers[ii].rate));
        }
      }
    }
    /*-- init switch matrix --*/
    memcpy(&coreGlobals.invSw, core_gameData->wpc.invSw, sizeof(core_gameData->wpc.invSw));
    memcpy(coreGlobals.swMatrix, coreGlobals.invSw, sizeof(coreGlobals.invSw));
    /*-- masks bit used by flippers --*/
    {
      const int flip = core_gameData->hw.flippers;
      locals.flipMask = CORE_SWLRFLIPBUTBIT | CORE_SWLLFLIPBUTBIT |
         ((flip & FLIP_SW(FLIP_UL)) ? CORE_SWULFLIPBUTBIT : 0) |
         ((flip & FLIP_SW(FLIP_UR)) ? CORE_SWURFLIPBUTBIT : 0) |
         ((flip & FLIP_EOS(FLIP_UL))? CORE_SWULFLIPEOSBIT : 0) |
         ((flip & FLIP_EOS(FLIP_UR))? CORE_SWURFLIPEOSBIT : 0) |
         ((flip & FLIP_EOS(FLIP_LL))? CORE_SWLLFLIPEOSBIT : 0) |
         ((flip & FLIP_EOS(FLIP_LR))? CORE_SWLRFLIPEOSBIT : 0);
    }
    /*-- command line options --*/
    locals.displaySize = pmoptions.dmd_compact ? 1 : 2;
    {
      UINT32 size = core_initDisplaySize(core_gameData->lcdLayout) >> 16;
      if ((size > Machine->drv->screen_width) && (locals.displaySize > 1)) {
  	/* force small display */
  	locals.displaySize = 1;
  	core_initDisplaySize(core_gameData->lcdLayout);
      }
    }
    /*-- Sound enabled ? */
    if (((Machine->gamedrv->flags & GAME_NO_SOUND) == 0) && Machine->sample_rate)
      coreGlobals.soundEn = TRUE;

    /*-- init simulator --*/
    if (g_fHandleKeyboard && core_gameData->simData) {
      int inports[CORE_MAXPORTS];
      int ii;

      for (ii = 0; ii < CORE_COREINPORT+(coreData->coreDips+31)/16; ii++)
        inports[ii] = readinputport(ii);

      coreGlobals.simAvail = sim_init((sim_tSimData *)core_gameData->simData,
                                         inports,CORE_COREINPORT+(coreData->coreDips+31)/16);
    }
    /*-- finally init the core --*/
    if (coreData->init) coreData->init();
    /*-- init sound commander --*/
    snd_cmd_init();
  }
  /*-- now reset everything --*/
  if (coreData->reset) coreData->reset();
  mech_emuInit();
  OnStateChange(1); /* We have a lift-off */

/* TOM: this causes to draw the static sim text */
  schedule_full_refresh();
}

static MACHINE_STOP(core) {
  int ii;
  mech_emuExit();
  if (coreData->stop) coreData->stop();
  snd_cmd_exit();
  for (ii = 0; ii < 5; ii++) {
    if (locals.timers[ii])
      timer_remove(locals.timers[ii]);
  }
  memset(locals.timers, 0, sizeof(locals.timers));
  coreData = NULL;
}

static void core_findSize(const struct core_dispLayout *layout, int *maxX, int *maxY) {
  if (layout) {
    for (; layout->length; layout += 1) {
      int tmpX = 0, tmpY = 0, type = layout->type & CORE_SEGMASK;
      if (type == CORE_IMPORT)
        { core_findSize(layout->ptr, maxX, maxY); continue; }
      if (type >= CORE_DMD) {
        tmpX = (layout->left + layout->length) * locals.segData[type].cols + 1;
        tmpY = (layout->top  + layout->start)  * locals.segData[type].rows + 1;
      }
      else {
        tmpX = (layout->left + 2*layout->length) * (locals.segData[type & 0x07].cols + 1) / 2;
        tmpY = (layout->top + 2) * (locals.segData[0].rows + 1) / 2;
      }
      if (tmpX > *maxX) *maxX = tmpX;
      if (tmpY > *maxY) *maxY = tmpY;
    }
#ifndef VPINMAME
    if (*maxX < 256) *maxX = 256;
#endif
  }
}

static UINT32 core_initDisplaySize(const struct core_dispLayout *layout) {
  int maxX = 0, maxY = 0;

  locals.segData = &segData[locals.displaySize == 1][0];
  if (layout)
    core_findSize(layout, &maxX, &maxY);
  else if (locals.displaySize > 1)
#ifdef VPINMAME
    { maxX = 257; maxY = 65; }
#else
    { maxX = 256; maxY = 65; }
#endif /* VPINMAME */
  else
    { maxX = 129; maxY = 33; }
  locals.firstSimRow = maxY + 3;
  locals.maxSimRows = Machine->drv->screen_height - locals.firstSimRow;
  if ((!pmoptions.dmd_only) || (maxY >= Machine->drv->screen_height))
    maxY = Machine->drv->screen_height;
#ifndef VPINMAME
  if (maxX == 257) maxX = 256;
#endif /* VPINMAME */
  set_visible_area(0, maxX-1, 0, maxY-1);
  return (maxX<<16) | maxY;
}

void core_nvram(void *file, int write, void *mem, int length, UINT8 init) {
  if (write)     mame_fwrite(file, mem, length); /* save */
  else if (file) mame_fread(file,  mem, length); /* load */
  else           memset(mem, init, length);     /* first time */
  mech_nv(file, write); /* save mech positions */
  { /*-- Load/Save DIP settings --*/
    UINT8 dips[6];
    int   ii;

    if (write) {
      for (ii = 0; ii < 6; ii++) dips[ii] = core_getDip(ii);
      mame_fwrite(file, dips, sizeof(dips));
    }
    else if (file) {
      /* set the defaults (for compabilty with older versions) */
      dips[0] = readinputport(CORE_COREINPORT+1) & 0xff;
      dips[1] = readinputport(CORE_COREINPORT+1)>>8;
      dips[2] = readinputport(CORE_COREINPORT+2) & 0xff;
      dips[3] = readinputport(CORE_COREINPORT+2)>>8;
      dips[4] = readinputport(CORE_COREINPORT+3) & 0xff;
      dips[5] = readinputport(CORE_COREINPORT+3)>>8;

      mame_fread(file, dips, sizeof(dips));
      for (ii = 0; ii < 6; ii++) vp_setDIP(ii, dips[ii]);

    }
    else { // always get the default from the inports
      /* coreData not initialised yet. Don't know exact number of DIPs */
      vp_setDIP(0, readinputport(CORE_COREINPORT+1) & 0xff);
      vp_setDIP(1, readinputport(CORE_COREINPORT+1)>>8);
      vp_setDIP(2, readinputport(CORE_COREINPORT+2) & 0xff);
      vp_setDIP(3, readinputport(CORE_COREINPORT+2)>>8);
      vp_setDIP(4, readinputport(CORE_COREINPORT+3) & 0xff);
      vp_setDIP(5, readinputport(CORE_COREINPORT+3)>>8);
    }
  }
}

/*----------------------------------------------
/  Add a timer when building the machine driver
/-----------------------------------------------*/
void machine_add_timer(struct InternalMachineDriver *machine, void (*func)(int), int rate) {
  int ii;
  for (ii = 0; machine->pinmame.timers[ii].callback; ii++)
    ;
  machine->pinmame.timers[ii].callback = func;
  machine->pinmame.timers[ii].rate = rate;
}

/*---------------------------------------
/  Default machine driver for all games
/----------------------------------------*/
MACHINE_DRIVER_START(PinMAME)
  MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY)
  MDRV_SCREEN_SIZE(CORE_SCREENX, CORE_SCREENY)
  MDRV_VISIBLE_AREA(0, CORE_SCREENX-1, 0, CORE_SCREENY-1)
  MDRV_PALETTE_INIT(core)
  MDRV_PALETTE_LENGTH(sizeof(core_palette)/sizeof(core_palette[0][0])/3)
  MDRV_FRAMES_PER_SECOND(60)
  MDRV_SWITCH_CONV(core_swSeq2m,core_m2swSeq)
  MDRV_LAMP_CONV(core_swSeq2m,core_m2swSeq)
  MDRV_MACHINE_INIT(core) MDRV_MACHINE_STOP(core)
  MDRV_VIDEO_UPDATE(core_gen)
MACHINE_DRIVER_END

/*------------------------------
/ display segment drawing data
/------------------------------*/
static tSegRow segSize1C[5][20] = { /* with commas */
{ /* alphanumeric display characters */
/*                       all        0001       0002       0004       0008       0010       0020       0040       0080       0100       0200       0400       0800       1000       2000       4000       8000 */
/*    11111111111  */{0x00555554,0x00555554,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000},
/*   1133331333311 */{0x017fdff5,0x003fcff0,0x00000001,0x00000000,0x00000000,0x00000000,0x01000000,0x00000000,0x00000000,0x00400000,0x00001000,0x00000004,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000},
/*   11    1   1 1 */{0x01401011,0x00000000,0x00000001,0x00000000,0x00000000,0x00000000,0x01000000,0x00000000,0x00000000,0x00400000,0x00001000,0x00000010,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000},
/*  31 1  31  1 31 */{0x0d10d04d,0x00000000,0x0000000d,0x00000000,0x00000000,0x00000000,0x0d000000,0x00000000,0x00000000,0x00100000,0x0000d000,0x00000040,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000},
/*  32 1  32  1 32 */{0x0e10e04e,0x00000000,0x0000000e,0x00000000,0x00000000,0x00000000,0x0e000000,0x00000000,0x00000000,0x00100000,0x0000e000,0x00000040,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000},
/*  22  1 22 1  22 */{0x0a04a10a,0x00000000,0x0000000a,0x00000000,0x00000000,0x00000000,0x0a000000,0x00000000,0x00000000,0x00040000,0x0000a000,0x00000100,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000},
/*  23  1 231   23 */{0x0b04b40b,0x00000000,0x0000000b,0x00000000,0x00000000,0x00000000,0x0b000000,0x00000000,0x00000000,0x00040000,0x0000b000,0x00000400,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000},
/*  13   1131   13 */{0x07017407,0x00000000,0x00000007,0x00000000,0x00000000,0x00000000,0x07000000,0x00000000,0x00000000,0x00010000,0x00007000,0x00000400,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000},
/*  1333311133331  */{0x07fd5ff4,0x00000000,0x00000004,0x00000000,0x00000000,0x00000000,0x04000000,0x03fc0000,0x00000000,0x00010000,0x00004000,0x00001000,0x00000ff0,0x00000000,0x00000000,0x00000000,0x00000000},
/*  3111113111113  */{0x0d55d55c,0x00000000,0x0000000c,0x0000000c,0x00000000,0x0c000000,0x0c000000,0x0155c000,0x00000000,0x0000c000,0x0000c000,0x0000c000,0x0000d550,0x0000c000,0x0000c000,0x0000c000,0x00000000},
/*  1333311133331  */{0x07fd5ff4,0x00000000,0x00000000,0x00000004,0x00000000,0x04000000,0x00000000,0x03fc0000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000ff0,0x00001000,0x00004000,0x00010000,0x00000000},
/* 31   1311   31  */{0x34075034,0x00000000,0x00000000,0x00000034,0x00000000,0x34000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00001000,0x00034000,0x00040000,0x00000000},
/* 32   132 1  32  */{0x38078438,0x00000000,0x00000000,0x00000038,0x00000000,0x38000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000400,0x00038000,0x00040000,0x00000000},
/* 22  1 22 1  22  */{0x28128428,0x00000000,0x00000000,0x00000028,0x00000000,0x28000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000400,0x00028000,0x00100000,0x00000000},
/* 23 1  23  1 23  */{0x2c42c12c,0x00000000,0x00000000,0x0000002c,0x00000000,0x2c000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000100,0x0002c000,0x00400000,0x00000000},
/* 13 1  13  1 13  */{0x1c41c11c,0x00000000,0x00000000,0x0000001c,0x00000000,0x1c000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000100,0x0001c000,0x00400000,0x00000000},
/* 1 1   1    11   */{0x11010050,0x00000000,0x00000000,0x00000010,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000040,0x00010000,0x01000000,0x00000000},
/* 113333133331132 */{0x17fdff5e,0x00000000,0x00000000,0x00000010,0x03fcff00,0x10000000,0x00000000,0x00000000,0x0000000e,0x00000000,0x00000000,0x00000000,0x00000000,0x00000040,0x00010000,0x04000000,0x0000000e},
/*  11111111111 31 */{0x0555554d,0x00000000,0x00000000,0x00000000,0x05555540,0x00000000,0x00000000,0x00000000,0x0000000d,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x0000000d},
/*              1  */{0x00000004,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000004,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000}
},{ /* 8 segment LED characters */
/*    11111111111  */{0x00555554,0x00555554,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000},
/*   1333333333331 */{0x01fffffd,0x00fffffc,0x00000001,0x00000000,0x00000000,0x00000000,0x01000000,0x00000000,0x00000000},
/*   1           1 */{0x01000001,0x00000000,0x00000001,0x00000000,0x00000000,0x00000000,0x01000000,0x00000000,0x00000000},
/*  31          31 */{0x0d00000d,0x00000000,0x0000000d,0x00000000,0x00000000,0x00000000,0x0d000000,0x00000000,0x00000000},
/*  32          32 */{0x0e00000e,0x00000000,0x0000000e,0x00000000,0x00000000,0x00000000,0x0e000000,0x00000000,0x00000000},
/*  22          22 */{0x0a00000a,0x00000000,0x0000000a,0x00000000,0x00000000,0x00000000,0x0a000000,0x00000000,0x00000000},
/*  23          23 */{0x0b00000b,0x00000000,0x0000000b,0x00000000,0x00000000,0x00000000,0x0b000000,0x00000000,0x00000000},
/*  13          13 */{0x07000007,0x00000000,0x00000007,0x00000000,0x00000000,0x00000000,0x07000000,0x00000000,0x00000000},
/*  1333333333331  */{0x07fffff4,0x00000000,0x00000004,0x00000000,0x00000000,0x00000000,0x04000000,0x03fffff0,0x00000000},
/*  3111111111113  */{0x0d55555c,0x00000000,0x0000000c,0x0000000c,0x00000000,0x0c000000,0x0c000000,0x0d55555c,0x00000000},
/*  1333333333331  */{0x07fffff4,0x00000000,0x00000000,0x00000004,0x00000000,0x04000000,0x00000000,0x03fffff0,0x00000000},
/* 31          31  */{0x34000034,0x00000000,0x00000000,0x00000034,0x00000000,0x34000000,0x00000000,0x00000000,0x00000000},
/* 32          32  */{0x38000038,0x00000000,0x00000000,0x00000038,0x00000000,0x38000000,0x00000000,0x00000000,0x00000000},
/* 22          22  */{0x28000028,0x00000000,0x00000000,0x00000028,0x00000000,0x28000000,0x00000000,0x00000000,0x00000000},
/* 23          23  */{0x2c00002c,0x00000000,0x00000000,0x0000002c,0x00000000,0x2c000000,0x00000000,0x00000000,0x00000000},
/* 13          13  */{0x1c00001c,0x00000000,0x00000000,0x0000001c,0x00000000,0x1c000000,0x00000000,0x00000000,0x00000000},
/* 1           1   */{0x10000010,0x00000000,0x00000000,0x00000010,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000},
/* 133333333333132 */{0x1fffffde,0x00000000,0x00000000,0x00000010,0x0fffffc0,0x10000000,0x00000000,0x00000000,0x0000000e},
/*  11111111111 31 */{0x0555554d,0x00000000,0x00000000,0x00000000,0x05555540,0x00000000,0x00000000,0x00000000,0x0000000d},
/*              1  */{0x00000004,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000004}
},{ /* 10 segment LED characters */
/*    11111111111  */{0x00555554,0x00555554,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000},
/*   1333331333331 */{0x01ffdffd,0x00ffcffc,0x00000001,0x00000000,0x00000000,0x00000000,0x01000000,0x00000000,0x00000000,0x00001000,0x00000000},
/*   1     1     1 */{0x01001001,0x00000000,0x00000001,0x00000000,0x00000000,0x00000000,0x01000000,0x00000000,0x00000000,0x00001000,0x00000000},
/*  31    31    31 */{0x0d00d00d,0x00000000,0x0000000d,0x00000000,0x00000000,0x00000000,0x0d000000,0x00000000,0x00000000,0x0000d000,0x00000000},
/*  32    32    32 */{0x0e00e00e,0x00000000,0x0000000e,0x00000000,0x00000000,0x00000000,0x0e000000,0x00000000,0x00000000,0x0000e000,0x00000000},
/*  22    22    22 */{0x0a00a00a,0x00000000,0x0000000a,0x00000000,0x00000000,0x00000000,0x0a000000,0x00000000,0x00000000,0x0000a000,0x00000000},
/*  23    23    23 */{0x0b00b00b,0x00000000,0x0000000b,0x00000000,0x00000000,0x00000000,0x0b000000,0x00000000,0x00000000,0x0000b000,0x00000000},
/*  13    13    13 */{0x07007007,0x00000000,0x00000007,0x00000000,0x00000000,0x00000000,0x07000000,0x00000000,0x00000000,0x00007000,0x00000000},
/*  1333331333331  */{0x07ff7ff4,0x00000000,0x00000004,0x00000000,0x00000000,0x00000000,0x04000000,0x03ff3ff0,0x00000000,0x00004000,0x00000000},
/*  3111111111113  */{0x0d55555c,0x00000000,0x0000000c,0x0000000c,0x00000000,0x0c000000,0x0c000000,0x0d55555c,0x00000000,0x0000c000,0x0000c000},
/*  1333331333331  */{0x07ff7ff4,0x00000000,0x00000000,0x00000004,0x00000000,0x04000000,0x00000000,0x03ff3ff0,0x00000000,0x00000000,0x00004000},
/* 31    31    31  */{0x34034034,0x00000000,0x00000000,0x00000034,0x00000000,0x34000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00034000},
/* 32    32    32  */{0x38038038,0x00000000,0x00000000,0x00000038,0x00000000,0x38000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00038000},
/* 22    22    22  */{0x28028028,0x00000000,0x00000000,0x00000028,0x00000000,0x28000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00028000},
/* 23    23    23  */{0x2c02c02c,0x00000000,0x00000000,0x0000002c,0x00000000,0x2c000000,0x00000000,0x00000000,0x00000000,0x00000000,0x0002c000},
/* 13    13    13  */{0x1c01c01c,0x00000000,0x00000000,0x0000001c,0x00000000,0x1c000000,0x00000000,0x00000000,0x00000000,0x00000000,0x0001c000},
/* 1     1     1   */{0x10010010,0x00000000,0x00000000,0x00000010,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00010000},
/* 133333133333132 */{0x1ffdffde,0x00000000,0x00000000,0x00000010,0x0ffcffc0,0x10000000,0x00000000,0x00000000,0x0000000e,0x00000000,0x00010000},
/*  11111111111 31 */{0x0555554d,0x00000000,0x00000000,0x00000000,0x05555540,0x00000000,0x00000000,0x00000000,0x0000000d,0x00000000,0x00000000},
/*              1  */{0x00000004,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000004,0x00000000,0x00000000}
},{ /* alphanumeric display characters (reversed comma with period) */
/*    11111111111  */{0x00555554,0x00555554,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000},
/*   1133331333311 */{0x017fdff5,0x003fcff0,0x00000001,0x00000000,0x00000000,0x00000000,0x01000000,0x00000000,0x00000000,0x00400000,0x00001000,0x00000004,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000},
/*   11    1   1 1 */{0x01401011,0x00000000,0x00000001,0x00000000,0x00000000,0x00000000,0x01000000,0x00000000,0x00000000,0x00400000,0x00001000,0x00000010,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000},
/*  31 1  31  1 31 */{0x0d10d04d,0x00000000,0x0000000d,0x00000000,0x00000000,0x00000000,0x0d000000,0x00000000,0x00000000,0x00100000,0x0000d000,0x00000040,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000},
/*  32 1  32  1 32 */{0x0e10e04e,0x00000000,0x0000000e,0x00000000,0x00000000,0x00000000,0x0e000000,0x00000000,0x00000000,0x00100000,0x0000e000,0x00000040,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000},
/*  22  1 22 1  22 */{0x0a04a10a,0x00000000,0x0000000a,0x00000000,0x00000000,0x00000000,0x0a000000,0x00000000,0x00000000,0x00040000,0x0000a000,0x00000100,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000},
/*  23  1 231   23 */{0x0b04b40b,0x00000000,0x0000000b,0x00000000,0x00000000,0x00000000,0x0b000000,0x00000000,0x00000000,0x00040000,0x0000b000,0x00000400,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000},
/*  13   1131   13 */{0x07017407,0x00000000,0x00000007,0x00000000,0x00000000,0x00000000,0x07000000,0x00000000,0x00000000,0x00010000,0x00007000,0x00000400,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000},
/*  1333311133331  */{0x07fd5ff4,0x00000000,0x00000004,0x00000000,0x00000000,0x00000000,0x04000000,0x03fc0000,0x00000000,0x00010000,0x00004000,0x00001000,0x00000ff0,0x00000000,0x00000000,0x00000000,0x00000000},
/*  3111113111113  */{0x0d55d55c,0x00000000,0x0000000c,0x0000000c,0x00000000,0x0c000000,0x0c000000,0x0155c000,0x00000000,0x0000c000,0x0000c000,0x0000c000,0x0000d550,0x0000c000,0x0000c000,0x0000c000,0x00000000},
/*  1333311133331  */{0x07fd5ff4,0x00000000,0x00000000,0x00000004,0x00000000,0x04000000,0x00000000,0x03fc0000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000ff0,0x00001000,0x00004000,0x00010000,0x00000000},
/* 31   1311   31  */{0x34075034,0x00000000,0x00000000,0x00000034,0x00000000,0x34000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00001000,0x00034000,0x00040000,0x00000000},
/* 32   132 1  32  */{0x38078438,0x00000000,0x00000000,0x00000038,0x00000000,0x38000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000400,0x00038000,0x00040000,0x00000000},
/* 22  1 22 1  22  */{0x28128428,0x00000000,0x00000000,0x00000028,0x00000000,0x28000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000400,0x00028000,0x00100000,0x00000000},
/* 23 1  23  1 23  */{0x2c42c12c,0x00000000,0x00000000,0x0000002c,0x00000000,0x2c000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000100,0x0002c000,0x00400000,0x00000000},
/* 13 1  13  1 13  */{0x1c41c11c,0x00000000,0x00000000,0x0000001c,0x00000000,0x1c000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000100,0x0001c000,0x00400000,0x00000000},
/* 1 1   1    11   */{0x11010050,0x00000000,0x00000000,0x00000010,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000040,0x00010000,0x01000000,0x00000000},
/* 113333133331132 */{0x17fdff5e,0x00000000,0x00000000,0x00000010,0x03fcff00,0x10000000,0x00000000,0x00000000,0x0000000e,0x00000000,0x00000000,0x00000000,0x00000000,0x00000040,0x00010000,0x04000000,0x0000000e},
/*  11111111111 31 */{0x0555554d,0x00000000,0x00000000,0x00000000,0x05555540,0x00000000,0x00000000,0x00000000,0x0000000d,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x0000000d},
/*              1  */{0x00000004,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000004}
},{ /* 8 segment LED characters with dots instead of commas */
/*    11111111111  */{0x00555554,0x00555554,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000},
/*   1333333333331 */{0x01fffffd,0x00fffffc,0x00000001,0x00000000,0x00000000,0x00000000,0x01000000,0x00000000,0x00000000},
/*   1           1 */{0x01000001,0x00000000,0x00000001,0x00000000,0x00000000,0x00000000,0x01000000,0x00000000,0x00000000},
/*  31          31 */{0x0d00000d,0x00000000,0x0000000d,0x00000000,0x00000000,0x00000000,0x0d000000,0x00000000,0x00000000},
/*  32          32 */{0x0e00000e,0x00000000,0x0000000e,0x00000000,0x00000000,0x00000000,0x0e000000,0x00000000,0x00000000},
/*  22          22 */{0x0a00000a,0x00000000,0x0000000a,0x00000000,0x00000000,0x00000000,0x0a000000,0x00000000,0x00000000},
/*  23          23 */{0x0b00000b,0x00000000,0x0000000b,0x00000000,0x00000000,0x00000000,0x0b000000,0x00000000,0x00000000},
/*  13          13 */{0x07000007,0x00000000,0x00000007,0x00000000,0x00000000,0x00000000,0x07000000,0x00000000,0x00000000},
/*  1333333333331  */{0x07fffff4,0x00000000,0x00000004,0x00000000,0x00000000,0x00000000,0x04000000,0x03fffff0,0x00000000},
/*  3111111111113  */{0x0d55555c,0x00000000,0x0000000c,0x0000000c,0x00000000,0x0c000000,0x0c000000,0x0d55555c,0x00000000},
/*  1333333333331  */{0x07fffff4,0x00000000,0x00000000,0x00000004,0x00000000,0x04000000,0x00000000,0x03fffff0,0x00000000},
/* 31          31  */{0x34000034,0x00000000,0x00000000,0x00000034,0x00000000,0x34000000,0x00000000,0x00000000,0x00000000},
/* 32          32  */{0x38000038,0x00000000,0x00000000,0x00000038,0x00000000,0x38000000,0x00000000,0x00000000,0x00000000},
/* 22          22  */{0x28000028,0x00000000,0x00000000,0x00000028,0x00000000,0x28000000,0x00000000,0x00000000,0x00000000},
/* 23          23  */{0x2c00002c,0x00000000,0x00000000,0x0000002c,0x00000000,0x2c000000,0x00000000,0x00000000,0x00000000},
/* 13          13  */{0x1c00001c,0x00000000,0x00000000,0x0000001c,0x00000000,0x1c000000,0x00000000,0x00000000,0x00000000},
/* 1           1   */{0x10000010,0x00000000,0x00000000,0x00000010,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000},
/* 133333333333132 */{0x1fffffde,0x00000000,0x00000000,0x00000010,0x0fffffc0,0x10000000,0x00000000,0x00000000,0x0000000e},
/*  11111111111 31 */{0x0555554d,0x00000000,0x00000000,0x00000000,0x05555540,0x00000000,0x00000000,0x00000000,0x0000000d},
/*                 */{0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000}
}};
static tSegRow segSize1[3][20] = { /* without commas */
{ /* alphanumeric display characters */
/*                       all        0001       0002       0004       0008       0010       0020       0040       0080       0100       0200       0400       0800       1000       2000       4000 */
/*    11111111111  */{0x00555554,0x00555554,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000},
/*   1133331333311 */{0x017fdff5,0x003fcff0,0x00000001,0x00000000,0x00000000,0x00000000,0x01000000,0x00000000,0x00000000,0x00400000,0x00001000,0x00000004,0x00000000,0x00000000,0x00000000,0x00000000},
/*   11    1   1 1 */{0x01401011,0x00000000,0x00000001,0x00000000,0x00000000,0x00000000,0x01000000,0x00000000,0x00000000,0x00400000,0x00001000,0x00000010,0x00000000,0x00000000,0x00000000,0x00000000},
/*  31 1  31  1 31 */{0x0d10d04d,0x00000000,0x0000000d,0x00000000,0x00000000,0x00000000,0x0d000000,0x00000000,0x00000000,0x00100000,0x0000d000,0x00000040,0x00000000,0x00000000,0x00000000,0x00000000},
/*  32 1  32  1 32 */{0x0e10e04e,0x00000000,0x0000000e,0x00000000,0x00000000,0x00000000,0x0e000000,0x00000000,0x00000000,0x00100000,0x0000e000,0x00000040,0x00000000,0x00000000,0x00000000,0x00000000},
/*  22  1 22 1  22 */{0x0a04a10a,0x00000000,0x0000000a,0x00000000,0x00000000,0x00000000,0x0a000000,0x00000000,0x00000000,0x00040000,0x0000a000,0x00000100,0x00000000,0x00000000,0x00000000,0x00000000},
/*  23  1 231   23 */{0x0b04b40b,0x00000000,0x0000000b,0x00000000,0x00000000,0x00000000,0x0b000000,0x00000000,0x00000000,0x00040000,0x0000b000,0x00000400,0x00000000,0x00000000,0x00000000,0x00000000},
/*  13   1131   13 */{0x07017407,0x00000000,0x00000007,0x00000000,0x00000000,0x00000000,0x07000000,0x00000000,0x00000000,0x00010000,0x00007000,0x00000400,0x00000000,0x00000000,0x00000000,0x00000000},
/*  1333311133331  */{0x07fd5ff4,0x00000000,0x00000004,0x00000000,0x00000000,0x00000000,0x04000000,0x03fc0000,0x00000000,0x00010000,0x00004000,0x00001000,0x00000ff0,0x00000000,0x00000000,0x00000000},
/*  3111113111113  */{0x0d55d55c,0x00000000,0x0000000c,0x0000000c,0x00000000,0x0c000000,0x0c000000,0x0155c000,0x00000000,0x0000c000,0x0000c000,0x0000c000,0x0000d550,0x0000c000,0x0000c000,0x0000c000},
/*  1333311133331  */{0x07fd5ff4,0x00000000,0x00000000,0x00000004,0x00000000,0x04000000,0x00000000,0x03fc0000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000ff0,0x00001000,0x00004000,0x00010000},
/* 31   1311   31  */{0x34075034,0x00000000,0x00000000,0x00000034,0x00000000,0x34000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00001000,0x00034000,0x00040000},
/* 32   132 1  32  */{0x38078438,0x00000000,0x00000000,0x00000038,0x00000000,0x38000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000400,0x00038000,0x00040000},
/* 22  1 22 1  22  */{0x28128428,0x00000000,0x00000000,0x00000028,0x00000000,0x28000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000400,0x00028000,0x00100000},
/* 23 1  23  1 23  */{0x2c42c12c,0x00000000,0x00000000,0x0000002c,0x00000000,0x2c000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000100,0x0002c000,0x00400000},
/* 13 1  13  1 13  */{0x1c41c11c,0x00000000,0x00000000,0x0000001c,0x00000000,0x1c000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000100,0x0001c000,0x00400000},
/* 1 1   1    11   */{0x11010050,0x00000000,0x00000000,0x00000010,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000040,0x00010000,0x01000000},
/* 1133331333311   */{0x17fdff50,0x00000000,0x00000000,0x00000010,0x03fcff00,0x10000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000040,0x00010000,0x04000000},
/*  11111111111    */{0x05555540,0x00000000,0x00000000,0x00000000,0x05555540,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000},
/*                 */{0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000}
},{ /* 7 segment LED characters */
/*    11111111111  */{0x00555554,0x00555554,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000},
/*   1333333333331 */{0x01fffffd,0x00fffffc,0x00000001,0x00000000,0x00000000,0x00000000,0x01000000,0x00000000},
/*   1           1 */{0x01000001,0x00000000,0x00000001,0x00000000,0x00000000,0x00000000,0x01000000,0x00000000},
/*  31          31 */{0x0d00000d,0x00000000,0x0000000d,0x00000000,0x00000000,0x00000000,0x0d000000,0x00000000},
/*  32          32 */{0x0e00000e,0x00000000,0x0000000e,0x00000000,0x00000000,0x00000000,0x0e000000,0x00000000},
/*  22          22 */{0x0a00000a,0x00000000,0x0000000a,0x00000000,0x00000000,0x00000000,0x0a000000,0x00000000},
/*  23          23 */{0x0b00000b,0x00000000,0x0000000b,0x00000000,0x00000000,0x00000000,0x0b000000,0x00000000},
/*  13          13 */{0x07000007,0x00000000,0x00000007,0x00000000,0x00000000,0x00000000,0x07000000,0x00000000},
/*  1333333333331  */{0x07fffff4,0x00000000,0x00000004,0x00000000,0x00000000,0x00000000,0x04000000,0x03fffff0},
/*  3111111111113  */{0x0d55555c,0x00000000,0x0000000c,0x0000000c,0x00000000,0x0c000000,0x0c000000,0x0d55555c},
/*  1333333333331  */{0x07fffff4,0x00000000,0x00000000,0x00000004,0x00000000,0x04000000,0x00000000,0x03fffff0},
/* 31          31  */{0x34000034,0x00000000,0x00000000,0x00000034,0x00000000,0x34000000,0x00000000,0x00000000},
/* 32          32  */{0x38000038,0x00000000,0x00000000,0x00000038,0x00000000,0x38000000,0x00000000,0x00000000},
/* 22          22  */{0x28000028,0x00000000,0x00000000,0x00000028,0x00000000,0x28000000,0x00000000,0x00000000},
/* 23          23  */{0x2c00002c,0x00000000,0x00000000,0x0000002c,0x00000000,0x2c000000,0x00000000,0x00000000},
/* 13          13  */{0x1c00001c,0x00000000,0x00000000,0x0000001c,0x00000000,0x1c000000,0x00000000,0x00000000},
/* 1           1   */{0x10000010,0x00000000,0x00000000,0x00000010,0x00000000,0x10000000,0x00000000,0x00000000},
/* 1333333333331   */{0x1fffffd0,0x00000000,0x00000000,0x00000010,0x0fffffc0,0x10000000,0x00000000,0x00000000},
/*  11111111111    */{0x05555540,0x00000000,0x00000000,0x00000000,0x05555540,0x00000000,0x00000000,0x00000000},
/*                 */{0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000}
},{ /* 9 segment LED characters */
/*    11111111111  */{0x00555554,0x00555554,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000},
/*   1333331333331 */{0x01ffdffd,0x00ffcffc,0x00000001,0x00000000,0x00000000,0x00000000,0x01000000,0x00000000,0x00000000,0x00001000,0x00000000},
/*   1     1     1 */{0x01001001,0x00000000,0x00000001,0x00000000,0x00000000,0x00000000,0x01000000,0x00000000,0x00000000,0x00001000,0x00000000},
/*  31    31    31 */{0x0d00d00d,0x00000000,0x0000000d,0x00000000,0x00000000,0x00000000,0x0d000000,0x00000000,0x00000000,0x0000d000,0x00000000},
/*  32    32    32 */{0x0e00e00e,0x00000000,0x0000000e,0x00000000,0x00000000,0x00000000,0x0e000000,0x00000000,0x00000000,0x0000e000,0x00000000},
/*  22    22    22 */{0x0a00a00a,0x00000000,0x0000000a,0x00000000,0x00000000,0x00000000,0x0a000000,0x00000000,0x00000000,0x0000a000,0x00000000},
/*  23    23    23 */{0x0b00b00b,0x00000000,0x0000000b,0x00000000,0x00000000,0x00000000,0x0b000000,0x00000000,0x00000000,0x0000b000,0x00000000},
/*  13    13    13 */{0x07007007,0x00000000,0x00000007,0x00000000,0x00000000,0x00000000,0x07000000,0x00000000,0x00000000,0x00007000,0x00000000},
/*  1333331333331  */{0x07ff7ff4,0x00000000,0x00000004,0x00000000,0x00000000,0x00000000,0x04000000,0x03ff3ff0,0x00000000,0x00004000,0x00000000},
/*  3111111111113  */{0x0d55555c,0x00000000,0x0000000c,0x0000000c,0x00000000,0x0c000000,0x0c000000,0x0d55555c,0x00000000,0x0000c000,0x0000c000},
/*  1333331333331  */{0x07ff7ff4,0x00000000,0x00000000,0x00000004,0x00000000,0x04000000,0x00000000,0x03ff3ff0,0x00000000,0x00000000,0x00004000},
/* 31    31    31  */{0x34034034,0x00000000,0x00000000,0x00000034,0x00000000,0x34000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00034000},
/* 32    32    32  */{0x38038038,0x00000000,0x00000000,0x00000038,0x00000000,0x38000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00038000},
/* 22    22    22  */{0x28028028,0x00000000,0x00000000,0x00000028,0x00000000,0x28000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00028000},
/* 23    23    23  */{0x2c02c02c,0x00000000,0x00000000,0x0000002c,0x00000000,0x2c000000,0x00000000,0x00000000,0x00000000,0x00000000,0x0002c000},
/* 13    13    13  */{0x1c01c01c,0x00000000,0x00000000,0x0000001c,0x00000000,0x1c000000,0x00000000,0x00000000,0x00000000,0x00000000,0x0001c000},
/* 1     1     1   */{0x10010010,0x00000000,0x00000000,0x00000010,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00010000},
/* 1333331333331   */{0x1ffdffd0,0x00000000,0x00000000,0x00000010,0x0ffcffc0,0x10000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00010000},
/*  11111111111    */{0x05555540,0x00000000,0x00000000,0x00000000,0x05555540,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000},
/*                 */{0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000}
}};

static tSegRow segSize2C[5][12] = { /* with commas */
{ /* alphanumeric display characters */
/*                   all        0001       0002       0004       0008       0010       0020       0040       0080       0100       0200       0400       0800       1000       2000       4000       8000 */
/*  xxxxxxx    */{0x05554000,0x05554000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000},
/* xx  x  xx   */{0x14105000,0x00000000,0x00001000,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00000000,0x04000000,0x00100000,0x00004000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000},
/* x x x x x   */{0x11111000,0x00000000,0x00001000,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00000000,0x01000000,0x00100000,0x00010000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000},
/* x x x x x   */{0x11111000,0x00000000,0x00001000,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00000000,0x01000000,0x00100000,0x00010000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000},
/* x  xxx  x   */{0x10541000,0x00000000,0x00001000,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00000000,0x00400000,0x00100000,0x00040000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000},
/*  xxx xxx    */{0x05454000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x05400000,0x00000000,0x00000000,0x00000000,0x00000000,0x00054000,0x00000000,0x00000000,0x00000000,0x00000000},
/* x  xxx  x   */{0x10541000,0x00000000,0x00000000,0x00001000,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00040000,0x00100000,0x00400000,0x00000000},
/* x x x x x   */{0x11111000,0x00000000,0x00000000,0x00001000,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00010000,0x00100000,0x01000000,0x00000000},
/* x x x x x   */{0x11111000,0x00000000,0x00000000,0x00001000,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00010000,0x00100000,0x01000000,0x00000000},
/* xx  x  xx x */{0x14105100,0x00000000,0x00000000,0x00001000,0x00000000,0x10000000,0x00000000,0x00000000,0x00000100,0x00000000,0x00000000,0x00000000,0x00000000,0x00004000,0x00100000,0x04000000,0x00000000},
/*  xxxxxxx  x */{0x05554100,0x00000000,0x00000000,0x00000000,0x05554000,0x00000000,0x00000000,0x00000000,0x00000100,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000100},
/*          x  */{0x00000400,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000400,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000}
},{ /* 8 segment LED characters */
/*  xxxxxxx    */{0x05554000,0x05554000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000},
/* x       x   */{0x10001000,0x00000000,0x00001000,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00000000},
/* x       x   */{0x10001000,0x00000000,0x00001000,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00000000},
/* x       x   */{0x10001000,0x00000000,0x00001000,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00000000},
/* x       x   */{0x10001000,0x00000000,0x00001000,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00000000},
/*  xxxxxxx    */{0x05554000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x05554000,0x00000000},
/* x       x   */{0x10001000,0x00000000,0x00000000,0x00001000,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000},
/* x       x   */{0x10001000,0x00000000,0x00000000,0x00001000,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000},
/* x       x   */{0x10001000,0x00000000,0x00000000,0x00001000,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000},
/* x       x x */{0x10001100,0x00000000,0x00000000,0x00001000,0x00000000,0x10000000,0x00000000,0x00000000,0x00000100},
/*  xxxxxxx  x */{0x05554100,0x00000000,0x00000000,0x00000000,0x05554000,0x00000000,0x00000000,0x00000000,0x00000100},
/*          x  */{0x00000400,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000400}
},{ /* 10 segment LED characters */
/*  xxxxxxx    */{0x05554000,0x05554000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000},
/* x   x   x   */{0x10101000,0x00000000,0x00001000,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00000000,0x00100000,0x00000000},
/* x   x   x   */{0x10101000,0x00000000,0x00001000,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00000000,0x00100000,0x00000000},
/* x   x   x   */{0x10101000,0x00000000,0x00001000,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00000000,0x00100000,0x00000000},
/* x   x   x   */{0x10101000,0x00000000,0x00001000,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00000000,0x00100000,0x00000000},
/*  xxxxxxx    */{0x05554000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x05554000,0x00000000,0x00000000,0x00000000},
/* x   x   x   */{0x10101000,0x00000000,0x00000000,0x00001000,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00100000},
/* x   x   x   */{0x10101000,0x00000000,0x00000000,0x00001000,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00100000},
/* x   x   x   */{0x10101000,0x00000000,0x00000000,0x00001000,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00100000},
/* x   x   x x */{0x10101100,0x00000000,0x00000000,0x00001000,0x00000000,0x10000000,0x00000000,0x00000000,0x00000100,0x00000000,0x00100000},
/*  xxxxxxx  x */{0x05554100,0x00000000,0x00000000,0x00000000,0x05554000,0x00000000,0x00000000,0x00000000,0x00000100,0x00000000,0x00000000},
/*          x  */{0x00000400,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000400,0x00000000,0x00000000}
},{ /* alphanumeric display characters (reversed comma with period) */
/*  xxxxxxx    */{0x05554000,0x05554000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000},
/* xx  x  xx   */{0x14105000,0x00000000,0x00001000,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00000000,0x04000000,0x00100000,0x00004000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000},
/* x x x x x   */{0x11111000,0x00000000,0x00001000,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00000000,0x01000000,0x00100000,0x00010000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000},
/* x x x x x   */{0x11111000,0x00000000,0x00001000,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00000000,0x01000000,0x00100000,0x00010000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000},
/* x  xxx  x   */{0x10541000,0x00000000,0x00001000,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00000000,0x00400000,0x00100000,0x00040000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000},
/*  xxx xxx    */{0x05454000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x05400000,0x00000000,0x00000000,0x00000000,0x00000000,0x00054000,0x00000000,0x00000000,0x00000000,0x00000000},
/* x  xxx  x   */{0x10541000,0x00000000,0x00000000,0x00001000,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00040000,0x00100000,0x00400000,0x00000000},
/* x x x x x   */{0x11111000,0x00000000,0x00000000,0x00001000,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00010000,0x00100000,0x01000000,0x00000000},
/* x x x x x   */{0x11111000,0x00000000,0x00000000,0x00001000,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00010000,0x00100000,0x01000000,0x00000000},
/* xx  x  xx x */{0x14105100,0x00000000,0x00000000,0x00001000,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00004000,0x00100000,0x04000000,0x00000100},
/*  xxxxxxx  x */{0x05554100,0x00000000,0x00000000,0x00000000,0x05554000,0x00000000,0x00000000,0x00000000,0x00000100,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000100},
/*          x  */{0x00000400,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000400}
},{ /* 8 segment LED characters with dots instead of commas */
/*  xxxxxxx    */{0x05554000,0x05554000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000},
/* x       x   */{0x10001000,0x00000000,0x00001000,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00000000},
/* x       x   */{0x10001000,0x00000000,0x00001000,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00000000},
/* x       x   */{0x10001000,0x00000000,0x00001000,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00000000},
/* x       x   */{0x10001000,0x00000000,0x00001000,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00000000},
/*  xxxxxxx    */{0x05554000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x05554000,0x00000000},
/* x       x   */{0x10001000,0x00000000,0x00000000,0x00001000,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000},
/* x       x   */{0x10001000,0x00000000,0x00000000,0x00001000,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000},
/* x       x   */{0x10001000,0x00000000,0x00000000,0x00001000,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000},
/* x       x   */{0x10001000,0x00000000,0x00000000,0x00001000,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000},
/*  xxxxxxx  x */{0x05554100,0x00000000,0x00000000,0x00000000,0x05554000,0x00000000,0x00000000,0x00000000,0x00000100},
/*             */{0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000}
}};
static tSegRow segSize2[3][12] = { /* without commas */
{ /* alphanumeric display characters */
/*                   all        0001       0002       0004       0008       0010       0020       0040       0080       0100       0200       0400       0800       1000       2000       4000       8000 */
/*  xxxxxxx    */{0x05554000,0x05554000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000},
/* xx  x  xx   */{0x14105000,0x00000000,0x00001000,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00000000,0x04000000,0x00100000,0x00004000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000},
/* x x x x x   */{0x11111000,0x00000000,0x00001000,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00000000,0x01000000,0x00100000,0x00010000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000},
/* x x x x x   */{0x11111000,0x00000000,0x00001000,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00000000,0x01000000,0x00100000,0x00010000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000},
/* x  xxx  x   */{0x10541000,0x00000000,0x00001000,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00000000,0x00400000,0x00100000,0x00040000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000},
/*  xxx xxx    */{0x05454000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x05400000,0x00000000,0x00000000,0x00000000,0x00000000,0x00054000,0x00000000,0x00000000,0x00000000,0x00000000},
/* x  xxx  x   */{0x10541000,0x00000000,0x00000000,0x00001000,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00040000,0x00100000,0x00400000,0x00000000},
/* x x x x x   */{0x11111000,0x00000000,0x00000000,0x00001000,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00010000,0x00100000,0x01000000,0x00000000},
/* x x x x x   */{0x11111000,0x00000000,0x00000000,0x00001000,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00010000,0x00100000,0x01000000,0x00000000},
/* xx  x  xx   */{0x14105000,0x00000000,0x00000000,0x00001000,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00004000,0x00100000,0x04000000,0x00000000},
/*  xxxxxxx    */{0x05554000,0x00000000,0x00000000,0x00000000,0x05554000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000},
/*             */{0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000}
},{ /* 7 segment LED characters */
/*  xxxxxxx    */{0x05554000,0x05554000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000},
/* x       x   */{0x10001000,0x00000000,0x00001000,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000},
/* x       x   */{0x10001000,0x00000000,0x00001000,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000},
/* x       x   */{0x10001000,0x00000000,0x00001000,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000},
/* x       x   */{0x10001000,0x00000000,0x00001000,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000},
/*  xxxxxxx    */{0x05554000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x05554000},
/* x       x   */{0x10001000,0x00000000,0x00000000,0x00001000,0x00000000,0x10000000,0x00000000,0x00000000},
/* x       x   */{0x10001000,0x00000000,0x00000000,0x00001000,0x00000000,0x10000000,0x00000000,0x00000000},
/* x       x   */{0x10001000,0x00000000,0x00000000,0x00001000,0x00000000,0x10000000,0x00000000,0x00000000},
/* x       x   */{0x10001000,0x00000000,0x00000000,0x00001000,0x00000000,0x10000000,0x00000000,0x00000000},
/*  xxxxxxx    */{0x05554000,0x00000000,0x00000000,0x00000000,0x05554000,0x00000000,0x00000000,0x00000000},
/*             */{0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000}
},{ /* 9 segment LED characters */
/*  xxxxxxx    */{0x05554000,0x05554000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000},
/* x   x   x   */{0x10101000,0x00000000,0x00001000,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00000000,0x00100000,0x00000000},
/* x   x   x   */{0x10101000,0x00000000,0x00001000,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00000000,0x00100000,0x00000000},
/* x   x   x   */{0x10101000,0x00000000,0x00001000,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00000000,0x00100000,0x00000000},
/* x   x   x   */{0x10101000,0x00000000,0x00001000,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00000000,0x00100000,0x00000000},
/*  xxxxxxx    */{0x05554000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x05554000,0x00000000,0x00000000,0x00000000},
/* x   x   x   */{0x10101000,0x00000000,0x00000000,0x00001000,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00100000},
/* x   x   x   */{0x10101000,0x00000000,0x00000000,0x00001000,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00100000},
/* x   x   x   */{0x10101000,0x00000000,0x00000000,0x00001000,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00100000},
/* x   x   x   */{0x10101000,0x00000000,0x00000000,0x00001000,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00100000},
/*  xxxxxxx    */{0x05554000,0x00000000,0x00000000,0x00000000,0x05554000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000},
/*             */{0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000}
}};

static tSegRow segSize3[4][8] = {
{ /* alphanumeric display characters */
{0} /* not possible */
},{ /* 8 segment LED characters */
/*               all        0001       0002       0004       0008       0010       0020       0040       0080       0100       0200 */
/*  xxx    */{0x05400000,0x05400000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000},
/* x   x   */{0x10100000,0x00000000,0x00100000,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00000000},
/* x   x   */{0x10100000,0x00000000,0x00100000,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00000000},
/*  xxx    */{0x05400000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x05400000,0x00000000},
/* x   x   */{0x10100000,0x00000000,0x00000000,0x00100000,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000},
/* x   x   */{0x10100000,0x00000000,0x00000000,0x00100000,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000},
/*  xxx  x */{0x05410000,0x00000000,0x00000000,0x00000000,0x05400000,0x00000000,0x00000000,0x00000000,0x00010000},
/*      x  */{0x00040000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00040000}
},{ /* 10 segment LED characters */
/*  xxx    */{0x05400000,0x05400000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000},
/* x x x   */{0x11100000,0x00000000,0x00100000,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00000000,0x01000000,0x00000000},
/* x x x   */{0x11100000,0x00000000,0x00100000,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00000000,0x01000000,0x00000000},
/*  xxx    */{0x05400000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x05400000,0x00000000,0x00000000,0x00000000},
/* x x x   */{0x11100000,0x00000000,0x00000000,0x00100000,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000,0x00000000,0x01000000},
/* x x x   */{0x11100000,0x00000000,0x00000000,0x00100000,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000,0x00000000,0x01000000},
/*  xxx  x */{0x05410000,0x00000000,0x00000000,0x00000000,0x05400000,0x00000000,0x00000000,0x00000000,0x00010000,0x00000000,0x00000000},
/*      x  */{0x00040000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00040000,0x00000000,0x00000000}
},{ /* 8 segment LED characters with dots instead of commas */
/*  xxx    */{0x05400000,0x05400000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000},
/* x   x   */{0x10100000,0x00000000,0x00100000,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00000000},
/* x   x   */{0x10100000,0x00000000,0x00100000,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00000000},
/*  xxx    */{0x05400000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x05400000,0x00000000},
/* x   x   */{0x10100000,0x00000000,0x00000000,0x00100000,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000},
/* x   x   */{0x10100000,0x00000000,0x00000000,0x00100000,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000},
/*  xxx  x */{0x05410000,0x00000000,0x00000000,0x00000000,0x05400000,0x00000000,0x00000000,0x00000000,0x00010000},
/*         */{0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000}
}};

// This should be static but then you can't do a forward declaration
/*static*/ tSegData segData[2][15] = {{
  {20,15,&segSize1C[0][0]},/* SEG16 */
  {20,15,&segSize1C[3][0]},/* SEG16R*/
  {20,15,&segSize1C[2][0]},/* SEG10 */
  {20,15,&segSize1[2][0]}, /* SEG9 */
  {20,15,&segSize1C[1][0]},/* SEG8 */
  {20,15,&segSize1[1][0]}, /* SEG7 */
  {20,15,&segSize1C[1][0]},/* SEG87 */
  {20,15,&segSize1C[1][0]},/* SEG87F */
  {20,15,&segSize1C[4][0]},/* SEG87FD */
  {20,15,&segSize1C[2][0]},/* SEG98 */
  {20,15,&segSize1C[2][0]},/* SEG98F */
  {12, 9,&segSize2[1][0]}, /* SEG7S */
  { 2, 2,NULL},{2,2,NULL}, /* DMD */
  { 1, 1,NULL}             /* VIDEO */
},{
  {12,11,&segSize2C[0][0]},/* SEG16 */
  {12,11,&segSize2C[3][0]},/* SEG16R*/
  {12,11,&segSize2C[2][0]},/* SEG10 */
  {12,11,&segSize2[2][0]}, /* SEG9 */
  {12,11,&segSize2C[1][0]},/* SEG8 */
  {12,11,&segSize2[1][0]}, /* SEG7 */
  {12,11,&segSize2C[1][0]},/* SEG87 */
  {12,11,&segSize2C[1][0]},/* SEG87F */
  {12,11,&segSize2C[4][0]},/* SEG87FD */
  {12,11,&segSize2C[2][0]},/* SEG98 */
  {12,11,&segSize2C[2][0]},/* SEG98F */
  { 8, 5,&segSize3[1][0]}, /* SEG7S */
  { 1, 1,NULL},{1,1,NULL}, /* DMD */
  { 1, 1,NULL}             /* VIDEO */
}};
