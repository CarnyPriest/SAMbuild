/************************************************************************************************
 Gottlieb System 80 Pinball

 CPU: 6502
 I/O: 6532


 6532:
 ----
 Port A:
 Port B:
************************************************************************************************/
#include <stdarg.h>
#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m6502/m6502.h"
#include "cpu/i86/i86.h"
#include "machine/6532riot.h"
#include "sound/votrax.h"
#include "core.h"
#include "sndbrd.h"
#include "gts80.h"
#include "gts80s.h"

static int core_ascii2seg[] = {
	/* 0x00-0x07 */ 0x0000, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	/* 0x08-0x0f */ 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	/* 0x10-0x17 */ 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	/* 0x18-0x1f */ 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	/* 0x20-0x27 */ 0x0000, 0x0903, 0x2002, 0x4E2A, 0x6D2A, 0x656E, 0x5D13, 0x0004,
	/* 0x28-0x2f */ 0x0014, 0x0041, 0x407F, 0x402A, 0x0000, 0x4008, 0x0000, 0x0044,
	/* 0x30-0x37 */ 0x3f00, 0x0022, 0x5B08, 0x4F08, 0x6608, 0x6D08, 0x7D08, 0x0700,
	/* 0x38-0x3f */ 0x7F08, 0x6F08, 0x0900, 0x0140, 0x0844, 0x4808, 0x0811, 0x0328,
	/* 0x40-0x47 */ 0x5F20, 0x7708, 0x0F2A, 0x3900, 0x0F22, 0x7900, 0x7100, 0x3D08,
	/* 0x48-0x4f */ 0x7608, 0x0922, 0x1E00, 0x7014, 0x3800, 0x3605, 0x3611, 0x3f00,
	/* 0x50-0x57 */ 0x7308, 0x3F10, 0x7318, 0x6D08, 0x0122, 0x3E00, 0x3044, 0x3650,
	/* 0x58-0x5f */ 0x0055, 0x0025, 0x0944, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	/* 0x60-0x67 */ 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	/* 0x68-0x6f */ 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	/* 0x70-0x77 */ 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	/* 0x78-0x7f */ 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
};

/*----------------
/  Local variables
/-----------------*/
static struct {
  int    vblankCount;
  int    initDone;
  UINT32 solenoids;
  UINT8  swMatrix[CORE_MAXSWCOL];
  UINT8  lampMatrix[CORE_MAXLAMPCOL];
  core_tSeg segments, pseg;
  int    swColOp;
  int    swRow;
  int    ssEn;
  int    OpSwitchEnable;
  int    disData;
  int    segSel;
  int    seg1;
  int    seg2;
  int    seg3;
  int    data;
  int    segPos1;
  int    segPos2;
  int    sndCmd;
  int    disCmdMode1;
  int    disCmdMode2;

  int	 sound_data;
  UINT8* pRAM;
} GTS80locals;

static void GTS80_irq(int state) {
  cpu_set_irq_line(GTS80_CPU, M6502_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}

static INTERRUPT_GEN(GTS80_vblank) {
  /*-------------------------------
  /  copy local data to interface
  /--------------------------------*/

  GTS80locals.vblankCount += 1;
  /*-- lamps --*/
  if ((GTS80locals.vblankCount % GTS80_LAMPSMOOTH) == 0) {
    memcpy(coreGlobals.lampMatrix, GTS80locals.lampMatrix, sizeof(GTS80locals.lampMatrix));
//    memset(GTS80locals.lampMatrix, 0, sizeof(GTS80locals.lampMatrix));
  }
  /*-- solenoids --*/
  if ((GTS80locals.vblankCount % GTS80_SOLSMOOTH) == 0) {
    coreGlobals.solenoids = GTS80locals.solenoids;
    if (GTS80locals.ssEn) {
      int ii;
      coreGlobals.solenoids |= CORE_SOLBIT(CORE_SSFLIPENSOL);
      /*-- special solenoids updated based on switches --*/
      for (ii = 0; ii < 6; ii++)
        if (core_gameData->sxx.ssSw[ii] && core_getSw(core_gameData->sxx.ssSw[ii]))
          coreGlobals.solenoids |= CORE_SOLBIT(CORE_FIRSTSSSOL+ii);
    }
    GTS80locals.solenoids = coreGlobals.pulsedSolState;
  }
  /*-- display --*/
  if ((GTS80locals.vblankCount % GTS80_DISPLAYSMOOTH) == 0)
  {
    memcpy(coreGlobals.segments, GTS80locals.segments, sizeof(coreGlobals.segments));
	memset(GTS80locals.segments, 0x00, sizeof GTS80locals.segments);
//    memcpy(GTS80locals.segments, GTS80locals.pseg, sizeof(GTS80locals.segments));

    /*update leds*/
    coreGlobals.diagnosticLed = 0;
  }
  core_updateSw(TRUE); /* assume flipper enabled */
}

static SWITCH_UPDATE(GTS80) {
	if (inports) {
		coreGlobals.swMatrix[0] = ((inports[GTS80_COMINPORT] & 0xff00)>>8);
        coreGlobals.swMatrix[8] = (coreGlobals.swMatrix[8]&0xc0) | (inports[GTS80_COMINPORT] & 0x003f);
	}

  /*-- slam tilt --*/
	riot6532_set_input_a(1, (core_gameData->gen&GEN_GTS80B4K) ? (core_getSw(GTS80_SWSLAMTILT)?0x80:0x00) : (core_getSw(GTS80_SWSLAMTILT)?0x00:0x80));
}

static WRITE_HANDLER(GTS80_sndCmd_w) {
	// logerror("sound cmd: 0x%02x\n", data);
	if ( Machine->gamedrv->flags & GAME_NO_SOUND )
		return;

	sndbrd_0_data_w(0, data|((GTS80locals.lampMatrix[1]&0x02)?0x10:0x00));
}

/* GTS80 switch numbering, row and column is swapped */
static int GTS80_sw2m(int no) {
	if ( no>=96 )
		return (no/10)*8+(no%10-1);
	else {
		no += 1;
		return (no%10)*8 + no/10;
	}
}

static int GTS80_m2sw(int col, int row) {
	if (col > 9 || (col == 9 && row >= 6))
		return col*8+row;
	else
		return row*10+col-1;
}
static int GTS80_m2lamp(int no) { return no+8; }
static int GTS80_lamp2m(int col, int row) { return (col-1)*8+row; }


static int GTS80_getSwRow(int row) {
	int value = 0;
	int ii;

	for (ii=8; ii>=1; ii--)
		value = (value<<1) | (coreGlobals.swMatrix[ii]&row?0x01:0x00);

	return value;
}

static int revertByte(int value) {
	int retVal = 0;
	int ii;

	for (ii=0; ii<8; ii++) {
		retVal = (retVal<<1) | (value&0x01);
		value >>= 1;
	}

	return retVal;
}

/*---------------
/ Switch reading
/----------------*/
static READ_HANDLER(riot6532_0a_r)  { return (GTS80locals.OpSwitchEnable) ? core_revbyte(core_getDip(GTS80locals.swColOp)) : (GTS80_getSwRow(GTS80locals.swRow)&0xff);}
static WRITE_HANDLER(riot6532_0a_w) { logerror("riot6532_0a_w: 0x%02x\n", data); }

static READ_HANDLER(riot6532_0b_r)  { /* logerror("riot6532_0b_r\n"); */ return 0x7f; }
static WRITE_HANDLER(riot6532_0b_w) { /* logerror("riot6532_0b_w: 0x%02x\n", data); */  GTS80locals.swRow = data; }

/*---------------------------
/  Display
/----------------------------*/

static READ_HANDLER(riot6532_1a_r)  { /* logerror("riot6532_1a_r\n"); */ return core_gameData->gen&GEN_GTS80B4K ? (core_getSw(GTS80_SWSLAMTILT)?0x80:0x00) : (core_getSw(GTS80_SWSLAMTILT)?0x00:0x80); }
static WRITE_HANDLER(riot6532_1a_w)
{
	int segSel = ((data>>4)&0x07);
	if ( core_gameData->gen & (GEN_GTS80B2K|GEN_GTS80B4K|GEN_GTS80B8K) ) {
		if ( (segSel&0x01) && !(GTS80locals.segSel&0x01) )
			GTS80locals.data = (GTS80locals.data&0xf0) | (GTS80locals.disData&0x0f);
		if ( (segSel&0x02) && !(GTS80locals.segSel&0x02) )
			GTS80locals.data = (GTS80locals.data&0x0f) | ((GTS80locals.disData&0x0f)<<4);
	}
	else {
		int strobe = (data&0x0f);

		if ( (segSel&0x01) && !(GTS80locals.segSel&0x01) )
			GTS80locals.seg1 = core_bcd2seg9[GTS80locals.disData&0x0f];
		if ( !(GTS80locals.disData&0x10) )
			GTS80locals.seg1 = core_bcd2seg9[0x01];

		if ( (segSel&0x02) && !(GTS80locals.segSel&0x02) )
			GTS80locals.seg2 = core_bcd2seg9[GTS80locals.disData&0x0f];
		if ( !(GTS80locals.disData&0x20) )
			GTS80locals.seg2 = core_bcd2seg9[0x01];

		if ( (segSel&0x04) && !(GTS80locals.segSel&0x04) )
			GTS80locals.seg3 = core_bcd2seg9[GTS80locals.disData&0x0f];
		if ( !(GTS80locals.disData&0x40) )
			GTS80locals.seg3 = core_bcd2seg9[0x01];

		if ( strobe<=5 ) {
			if ( GTS80locals.seg1 ) {
				GTS80locals.segments[0][7-strobe].lo = GTS80locals.seg1 & 0x00ff;
				GTS80locals.segments[0][7-strobe].hi = GTS80locals.seg1 >> 8;
			}
			if ( GTS80locals.seg2 ) {
				GTS80locals.segments[1][7-strobe].lo = GTS80locals.seg2 & 0x00ff;
				GTS80locals.segments[1][7-strobe].hi = GTS80locals.seg2 >> 8;
			}
			if ( GTS80locals.seg3 ) {
				GTS80locals.segments[2][7-strobe].lo = GTS80locals.seg3 & 0x00ff;
				GTS80locals.segments[2][7-strobe].hi = GTS80locals.seg3 >> 8;
			}
		}
		else if ( strobe<=11 ) {
			if ( GTS80locals.seg1 ) {
				GTS80locals.segments[0][21-strobe].lo = GTS80locals.seg1 & 0x00ff;
				GTS80locals.segments[0][21-strobe].hi = GTS80locals.seg1 >> 8;
			}
			if ( GTS80locals.seg2 ) {
				GTS80locals.segments[1][21-strobe].lo = GTS80locals.seg2 & 0x00ff;
				GTS80locals.segments[1][21-strobe].hi = GTS80locals.seg2 >> 8;
			}
			if ( GTS80locals.seg3 ) {
				GTS80locals.segments[2][21-strobe].lo = GTS80locals.seg3 & 0x00ff;
				GTS80locals.segments[2][21-strobe].hi = GTS80locals.seg3 >> 8;
			}
		}
		else {
			// 7th digits in score display for GTS80a
			if ( GTS80locals.seg1 ) {
				if ( strobe==12 ) {
					GTS80locals.segments[0][9].lo = GTS80locals.seg1 & 0x00ff;
					GTS80locals.segments[0][9].hi = GTS80locals.seg1 >> 8;
				}
				else if ( strobe==15 ) {
					GTS80locals.segments[0][1].lo = GTS80locals.seg1 & 0x00ff;
					GTS80locals.segments[0][1].hi = GTS80locals.seg1 >> 8;
				}
			}
			if ( GTS80locals.seg2 ) {
				if ( strobe==12 ) {
					GTS80locals.segments[1][9].lo = GTS80locals.seg2 & 0x00ff;
					GTS80locals.segments[1][9].hi = GTS80locals.seg2 >> 8;
				}
				else if ( strobe==15 ) {
					GTS80locals.segments[1][1].lo = GTS80locals.seg2 & 0x00ff;
					GTS80locals.segments[1][1].hi = GTS80locals.seg2 >> 8;
				}
			}
			if ( GTS80locals.seg3 ) {
				switch ( strobe ) {
				case 12:
					GTS80locals.segments[0][8].lo = GTS80locals.seg3 & 0x00ff;
					GTS80locals.segments[0][8].hi = GTS80locals.seg3 >> 8;
					break;

				case 13:
					GTS80locals.segments[0][0].lo = GTS80locals.seg3 & 0x00ff;
					GTS80locals.segments[0][0].hi = GTS80locals.seg3 >> 8;
					break;

				case 14:
					GTS80locals.segments[1][8].lo = GTS80locals.seg3 & 0x00ff;
					GTS80locals.segments[1][8].hi = GTS80locals.seg3 >> 8;
					break;

				case 15:
					GTS80locals.segments[1][0].lo = GTS80locals.seg3 & 0x00ff;
					GTS80locals.segments[1][0].hi = GTS80locals.seg3 >> 8;
					break;
				}
			}
		}
	}

	GTS80locals.segSel = segSel;
	return;
}

static READ_HANDLER(riot6532_1b_r)  { /* logerror("riot6532_1b_r\n"); */ return 0x00; }
static WRITE_HANDLER(riot6532_1b_w)
{
//	logerror("riot6532_1b_w: 0x%02x\n", data);
	GTS80locals.OpSwitchEnable = (data&0x80);
	if ( core_gameData->gen & (GEN_GTS80B2K|GEN_GTS80B4K|GEN_GTS80B8K) ) {
		int value;

		GTS80locals.disData = (data&0x3f);
		if ( data&0x40 ) {
			GTS80locals.segPos1 = 0;
			GTS80locals.segPos2 = 0;
			/* logerror("display reset\n"); */
			GTS80locals.disCmdMode1 = 0;
			GTS80locals.disCmdMode2 = 0;
		}
		/* LD1 */
		if ( !(GTS80locals.disData&0x10) ) {
			if ( GTS80locals.disCmdMode1 ) {
				GTS80locals.disCmdMode1 = 0;
				GTS80locals.segPos1 = 0;
			}
			else if ( GTS80locals.data==0x01 ) {
				GTS80locals.disCmdMode1 = 1;
			}
			else {
				value = core_ascii2seg[GTS80locals.data&0x7f] | (GTS80locals.data&0x80?0x8080:0x0000);
				GTS80locals.segments[0][GTS80locals.segPos1].lo = value&0x00ff;
				GTS80locals.segments[0][GTS80locals.segPos1].hi = value>>8;
				GTS80locals.segPos1 = (GTS80locals.segPos1+1)%20;
			}
		}
		/* LD2 */
		if ( !(GTS80locals.disData&0x20) ) {
			if ( GTS80locals.disCmdMode2 ) {
				GTS80locals.disCmdMode2 = 0;
				GTS80locals.segPos2 = 0;
			}
			else if ( GTS80locals.data==0x01 ) {
				GTS80locals.disCmdMode2 = 1;
			}
			else {
				value = core_ascii2seg[GTS80locals.data&0x7f] | (GTS80locals.data&0x80?0x8080:0x0000);
				GTS80locals.segments[1][GTS80locals.segPos2].lo = value&0x00ff;
				GTS80locals.segments[1][GTS80locals.segPos2].hi = value>>8;
				GTS80locals.segPos2 = (GTS80locals.segPos2+1)%20;
			}
		}
	}
	else
		GTS80locals.disData = (data&0x7f);

	return;
}

/*---------------------------
/  Solenoids, Lamps and Sound
/----------------------------*/

static int GTS80_bitsLo[] = { 0x01, 0x02, 0x04, 0x08 };
static int GTS80_bitsHi[] = { 0x10, 0x20, 0x40, 0x80 };

/* solenoids */
static READ_HANDLER(riot6532_2a_r)  { /* logerror("riot6532_2a_r\n"); */ return 0xff; }
static WRITE_HANDLER(riot6532_2a_w) {
	/* solenoids 1-4 */
	if ( !(data&0x20) ) GTS80locals.solenoids |= GTS80_bitsLo[~data&0x03];
	/* solenoids 1-4 */
	if ( !(data&0x40) ) GTS80locals.solenoids |= GTS80_bitsHi[(~data>>2)&0x03];
	/* solenoid 9    */
	GTS80locals.solenoids |= ((~data&0x80)<<1);

	/* sound */
	GTS80_sndCmd_w(0,!(data&0x10)?(~data)&0x0f:0x00);

//	logerror("riot6532_2a_w: 0x%02x\n", data);
}

static READ_HANDLER(riot6532_2b_r)  { logerror("riot6532_2b_r\n"); return 0xff; }
static WRITE_HANDLER(riot6532_2b_w) {
	/* logerror("riot6532_2b_w: 0x%02x\n", data); */
	if ( data&0xf0 ) {
		int col = ((data&0xf0)>>4)-1;
		if ( col%2 ) {
			GTS80locals.lampMatrix[col/2] = (GTS80locals.lampMatrix[col/2]&0x0f)|((data&0x0f)<<4);
			if (col==11)
				GTS80locals.lampMatrix[6] = (GTS80locals.lampMatrix[5]>>4)^0x0f;
		}
		else {
                        //int iOld = GTS80locals.lampMatrix[1]&0x02;
			GTS80locals.lampMatrix[col/2] = (GTS80locals.lampMatrix[col/2]&0xf0)|(data&0x0f);
		}
	}
	GTS80locals.swColOp = (data>>4)&0x03;
}

static struct riot6532_interface GTS80_riot6532_intf[] = {
{/* 6532RIOT 0 (0x200) Chip U4 (SWITCH MATRIX)*/
 /* PA0 - PA7 Switch Return  (ROW) */
 /* PB0 - PB7 Switch Strobes (COL) */
 /* in  : A/B, */ riot6532_0a_r, riot6532_0b_r,
 /* out : A/B, */ riot6532_0a_w, riot6532_0b_w,
 /* irq :      */ GTS80_irq
},

{/* 6532RIOT 1 (0x280) Chip U5 (DISPLAY & SWITCH ENABLE)*/
 /* PA0-3:  DIGIT STROBE */
 /* PA4:    Write to PL.1&2 */
 /* PA5:    Write to PL.3&4 */
 /* PA6:    Write to Ball/Credit */
 /* PA7(I): SLAM SWITCH */
 /* PB0-3:  DIGIT DATA */
 /* PB4:    H LINE (1&2) */
 /* PB5:    H LINE (3&4) */
 /* PB6:    H LINE (5&6) */
 /* PB7:    SWITCH ENABLE */
 /* in  : A/B, */ riot6532_1a_r, riot6532_1b_r,
 /* out : A/B, */ riot6532_1a_w, riot6532_1b_w,
 /* irq :      */ GTS80_irq
},

{/* 6532RIOT 2 (0x300) Chip U6*/
 /* PA0-6: FEED Z28(LS139) (SOL1-8) & SOUND 1-4 */
 /* PA7:   SOL.9 */
 /* PB0-3: LD1-4 */
 /* PB4-7: FEED Z33:LS154 (LAMP LATCHES) + PART OF SWITCH ENABLE */
 /* in  : A/B, */ riot6532_2a_r, riot6532_2b_r,
 /* out : A/B, */ riot6532_2a_w, riot6532_2b_w,
 /* irq :      */ GTS80_irq
}};


static WRITE_HANDLER(ram_256w) {
	UINT8 *pMem =  GTS80locals.pRAM + (offset%0x100);
	pMem[0x1800] = pMem[0x1900] = pMem[0x1a00] = pMem[0x1b00] = \
	pMem[0x1c00] = pMem[0x1d00] = pMem[0x1e00] = pMem[0x1f00] = \
		data;
}

static WRITE_HANDLER(riot6532_0_ram_w) {
	UINT8 *pMem = GTS80locals.pRAM + (offset%0x80);
	pMem[0x0000] = pMem[0x4000] = pMem[0x8000] = pMem[0xc000] = data;
}

static WRITE_HANDLER(riot6532_1_ram_w) {
	UINT8 *pMem = GTS80locals.pRAM + (offset%0x80);
	pMem[0x0080] = pMem[0x4080] = pMem[0x8080] = pMem[0xc080] = data;
}

static WRITE_HANDLER(riot6532_2_ram_w) {
	UINT8 *pMem = GTS80locals.pRAM + (offset%0x80);
	pMem[0x0100] = pMem[0x4100] = pMem[0x8100] = pMem[0xc100] = data;
}


/*---------------------------
/  Memory map for main CPU
/----------------------------*/
static MEMORY_READ_START(GTS80_readmem)
{0x0000,0x007f,	MRA_RAM},		/*U4 - 6532 RAM*/
{0x0080,0x00ff,	MRA_RAM},		/*U5 - 6532 RAM*/
{0x0100,0x017f,	MRA_RAM},		/*U6 - 6532 RAM*/
{0x0200,0x027f, riot6532_0_r},	/*U4 - I/O*/
{0x0280,0x02ff, riot6532_1_r},	/*U5 - I/O*/
{0x0300,0x037f, riot6532_2_r},	/*U6 - I/O*/
{0x1000,0x17ff, MRA_ROM},		/*Game Prom(s)*/
{0x1800,0x1fff, MRA_RAM},		/*RAM - 8x the same 256 Bytes*/
{0x2000,0x2fff, MRA_ROM},		/*U2 ROM*/
{0x3000,0x3fff, MRA_ROM},		/*U3 ROM*/

/* A14 & A15 aren't used) */
{0x4000,0x407f,	MRA_RAM},		/*U4 - 6532 RAM*/
{0x4080,0x40ff,	MRA_RAM},		/*U5 - 6532 RAM*/
{0x4100,0x417f,	MRA_RAM},		/*U6 - 6532 RAM*/
{0x4200,0x427f, riot6532_0_r},	/*U4 - I/O*/
{0x4280,0x42ff, riot6532_1_r},	/*U5 - I/O*/
{0x4300,0x437f, riot6532_2_r},	/*U6 - I/O*/
{0x5000,0x57ff, MRA_ROM},		/*Game Prom(s)*/
{0x5800,0x5fff, MRA_RAM},		/*RAM - 8x the same 256 Bytes*/
{0x6000,0x6fff, MRA_ROM},		/*U2 ROM*/
{0x7000,0x7fff, MRA_ROM},		/*U3 ROM*/

{0x8000,0x807f,	MRA_RAM},		/*U4 - 6532 RAM*/
{0x8080,0x80ff,	MRA_RAM},		/*U5 - 6532 RAM*/
{0x8100,0x817f,	MRA_RAM},		/*U6 - 6532 RAM*/
{0x8200,0x827f, riot6532_0_r},	/*U4 - I/O*/
{0x8280,0x82ff, riot6532_1_r},	/*U5 - I/O*/
{0x8300,0x837f, riot6532_2_r},	/*U6 - I/O*/
{0x9000,0x97ff, MRA_ROM},		/*Game Prom(s)*/
{0x9800,0x9fff, MRA_RAM},		/*RAM - 8x the same 256 Bytes*/
{0xa000,0xafff, MRA_ROM},		/*U2 ROM*/
{0xb000,0xbfff, MRA_ROM},		/*U3 ROM*/

{0xc000,0xc07f,	MRA_RAM},		/*U4 - 6532 RAM*/
{0xc080,0xc0ff,	MRA_RAM},		/*U5 - 6532 RAM*/
{0xc100,0xc17f,	MRA_RAM},		/*U6 - 6532 RAM*/
{0xc200,0xc27f, riot6532_0_r},	/*U4 - I/O*/
{0xc280,0xc2ff, riot6532_1_r},	/*U5 - I/O*/
{0xc300,0xc37f, riot6532_2_r},	/*U6 - I/O*/
{0xd000,0xd7ff, MRA_ROM},		/*Game Prom(s)*/
{0xd800,0xdfff, MRA_RAM},		/*RAM - 8x the same 256 Bytes*/
{0xe000,0xefff, MRA_ROM},		/*U2 ROM*/
{0xf000,0xffff, MRA_ROM},		/*U3 ROM*/
MEMORY_END

static MEMORY_WRITE_START(GTS80_writemem)
{0x0000,0x007f,	riot6532_0_ram_w},	/*U4 - 6532 RAM*/
{0x0080,0x00ff,	riot6532_1_ram_w},	/*U5 - 6532 RAM*/
{0x0100,0x017f,	riot6532_2_ram_w},	/*U6 - 6532 RAM*/
{0x0200,0x027f, riot6532_0_w},		/*U4 - I/O*/
{0x0280,0x02ff, riot6532_1_w},		/*U5 - I/O*/
{0x0300,0x037f, riot6532_2_w},		/*U6 - I/O*/
{0x1000,0x17ff, MWA_ROM},			/*Game Prom(s)*/
{0x1800,0x1fff, ram_256w},			/*RAM - 8x the same 256 Bytes*/
{0x2000,0x2fff, MWA_ROM},			/*U2 ROM*/
{0x3000,0x3fff, MWA_ROM},			/*U3 ROM*/

/* A14 & A15 aren't used) */
{0x4000,0x407f,	riot6532_0_ram_w},	/*U4 - 6532 RAM*/
{0x4080,0x40ff,	riot6532_1_ram_w},	/*U5 - 6532 RAM*/
{0x4100,0x417f,	riot6532_2_ram_w},	/*U6 - 6532 RAM*/
{0x4200,0x427f, riot6532_0_w},		/*U4 - I/O*/
{0x4280,0x42ff, riot6532_1_w},		/*U5 - I/O*/
{0x4300,0x437f, riot6532_2_w},		/*U6 - I/O*/
{0x5000,0x57ff, MWA_ROM},			/*Game Prom(s)*/
{0x5800,0x5fff, ram_256w},			/*RAM - 8x the same 256 Bytes*/
{0x6000,0x6fff, MWA_ROM},			/*U2 ROM*/
{0x7000,0x7fff, MWA_ROM},			/*U3 ROM*/

{0x8000,0x807f,	riot6532_0_ram_w},	/*U4 - 6532 RAM*/
{0x8080,0x80ff,	riot6532_1_ram_w},	/*U5 - 6532 RAM*/
{0x8100,0x817f,	riot6532_2_ram_w},	/*U6 - 6532 RAM*/
{0x8200,0x827f, riot6532_0_w},		/*U4 - I/O*/
{0x8280,0x82ff, riot6532_1_w},		/*U5 - I/O*/
{0x8300,0x837f, riot6532_2_w},		/*U6 - I/O*/
{0x9000,0x97ff, MWA_ROM},			/*Game Prom(s)*/
{0x9800,0x9fff, ram_256w},			/*RAM - 8x the same 256 Bytes*/
{0xa000,0xafff, MWA_ROM},			/*U2 ROM*/
{0xb000,0xbfff, MWA_ROM},			/*U3 ROM*/

{0xc000,0xc07f,	riot6532_0_ram_w},	/*U4 - 6532 RAM*/
{0xc080,0xc0ff,	riot6532_1_ram_w},	/*U5 - 6532 RAM*/
{0xc100,0xc17f,	riot6532_2_ram_w},	/*U6 - 6532 RAM*/
{0xc200,0xc27f, riot6532_0_w},		/*U4 - I/O*/
{0xc280,0xc2ff, riot6532_1_w},		/*U5 - I/O*/
{0xc300,0xc37f, riot6532_2_w},		/*U6 - I/O*/
{0xd000,0xd7ff, MWA_ROM},			/*Game Prom(s)*/
{0xd800,0xdfff, ram_256w},			/*RAM - 8x the same 256 Bytes*/
{0xe000,0xefff, MWA_ROM},			/*U2 ROM*/
{0xf000,0xffff, MWA_ROM},			/*U3 ROM*/
MEMORY_END

static MEMORY_READ_START(video_readmem)
{0x00000,0x07fff, MRA_RAM},
{0x08000,0x0ffff, MRA_ROM},
{0xf8000,0xfffff, MRA_ROM},
MEMORY_END

static MEMORY_WRITE_START(video_writemem)
{0x00000,0x07fff, MWA_RAM},
{0x08000,0x0ffff, MWA_ROM},
{0xf8000,0xfffff, MWA_ROM},
MEMORY_END

#if 0
/* GTS80b - Gen 1 Sound Hardware*/
struct MachineDriver machine_driver_GTS80BS1 = {
  {
    {
      CPU_M6502, 850000, /* 0.85 Mhz */
      GTS80_readmem, GTS80_writemem, NULL, NULL,
	  GTS80_vblank, 1,
	  NULL, 0
	}
	GTS80BS1_SOUNDCPU2
    GTS80BS1_SOUNDCPU1},
  GTS80_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50,
  GTS80_init,CORE_EXITFUNC(GTS80_exit)
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_TYPE_RASTER,
  0,
  NULL, NULL, gen_refresh,
  0,0,0,0,{GTS80BS1_SOUND},
  GTS80_nvram
};

/* GTS80b - Gen 2 Sound Hardware*/
struct MachineDriver machine_driver_GTS80BS2 = {
  {
    {
      CPU_M6502, 850000, /* 0.85 Mhz */
      GTS80_readmem, GTS80_writemem, NULL, NULL,
	  GTS80_vblank, 1,
	  NULL, 0
	}
	GTS80BS2_SOUNDCPU2
    GTS80BS2_SOUNDCPU1},
  GTS80_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50,
  GTS80_init,CORE_EXITFUNC(GTS80_exit)
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_TYPE_RASTER,
  0,
  NULL, NULL, gen_refresh,
  0,0,0,0,{GTS80BS2_SOUND},
  GTS80_nvram
};

/* GTS80b - Gen 3 Sound Hardware*/
struct MachineDriver machine_driver_GTS80BS3 = {
  {
    {
      CPU_M6502, 850000, /* 0.85 Mhz */
      GTS80_readmem, GTS80_writemem, NULL, NULL,
	  GTS80_vblank, 1,
	  NULL, 0
	}
	GTS80BS3_SOUNDCPU2
    GTS80BS3_SOUNDCPU1},
  GTS80_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50,
  GTS80_init,CORE_EXITFUNC(GTS80_exit)
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_TYPE_RASTER,
  0,
  NULL, NULL, gen_refresh,
  0,0,0,0,{GTS80BS3_SOUND},
  GTS80_nvram
};
#endif

static MACHINE_INIT(gts80) {
  int ii;

  memset(&GTS80locals, 0, sizeof GTS80locals);

  /* init ROM */
  for(ii = 1; ii<4; ii++) {
	memcpy(memory_region(GTS80_MEMREG_CPU)+0x1000+0x4000*ii, memory_region(GTS80_MEMREG_CPU)+0x1000, 0x0800);
	memcpy(memory_region(GTS80_MEMREG_CPU)+0x2000+0x4000*ii, memory_region(GTS80_MEMREG_CPU)+0x2000, 0x2000);
	if ( core_gameData->gen & GEN_GTS80B4K ) {
		memcpy(memory_region(GTS80_MEMREG_CPU)+0x9000, memory_region(GTS80_MEMREG_CPU)+0x1800, 0x800);
		memcpy(memory_region(GTS80_MEMREG_CPU)+0xd000, memory_region(GTS80_MEMREG_CPU)+0x1800, 0x800);
	}
  }

  /* init RAM pointer */
  GTS80locals.pRAM = memory_region(GTS80_MEMREG_CPU);

  /* init RIOTS */
  for (ii = 0; ii < sizeof(GTS80_riot6532_intf)/sizeof(GTS80_riot6532_intf[0]); ii++)
    riot6532_config(ii, &GTS80_riot6532_intf[ii]);

  sndbrd_0_init(core_gameData->hw.soundBoard, 1, memory_region(GTS80_MEMREG_SCPU1), NULL, NULL);

  riot6532_reset();

  GTS80locals.initDone = TRUE;
}

static MACHINE_STOP(gts80)
{
  sndbrd_0_exit();

  riot6532_unconfig();
}

/*-----------------------------------------------
/ Load/Save static ram
/ Save RAM & CMOS Information
/-------------------------------------------------*/

static NVRAM_HANDLER(gts80) {
	int i;
	core_nvram(file, read_or_write, memory_region(GTS80_MEMREG_CPU)+0x1800, 0x100, 0x00);
	if (!read_or_write)
		for(i=1;i<8;i++)
			memcpy(memory_region(GTS80_MEMREG_CPU)+0x1800+(i*0x100), memory_region(GTS80_MEMREG_CPU)+0x1800, 0x100);
}

VIDEO_UPDATE(gts80vid)
{
	int x, y;

	if (get_vh_global_attribute_changed())
		for (y = Machine->visible_area.min_y; y <= Machine->visible_area.max_y; y+=2)
		{
			for (x = Machine->visible_area.min_x; x <= Machine->visible_area.max_x; x++)
			{
				plot_pixel (tmpbitmap, x, y+1, Machine->pens[videoram[0x80*y+x] & 0x0f]);
				plot_pixel (tmpbitmap, x, y, Machine->pens[(videoram[0x80*y+x] >> 4)& 0x0f]);
			}
		}

	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->visible_area,TRANSPARENCY_NONE,0);
}

MACHINE_DRIVER_START(gts80)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(gts80,NULL,gts80)
  MDRV_CPU_ADD(M6502, 850000)
  MDRV_CPU_MEMORY(GTS80_readmem, GTS80_writemem)
  MDRV_CPU_VBLANK_INT(GTS80_vblank, 1)
  MDRV_NVRAM_HANDLER(gts80)
  MDRV_DIPS(42) /* 42 DIPs (32 for the controller board, 8 for the SS- and 2 for the S-Board*/
  MDRV_SWITCH_UPDATE(GTS80)
  MDRV_DIAGNOSTIC_LEDH(1)
  MDRV_SWITCH_CONV(GTS80_sw2m,GTS80_m2sw)
  MDRV_LAMP_CONV(GTS80_sw2m,GTS80_m2sw)
  MDRV_SOUND_CMD(GTS80_sndCmd_w)
  MDRV_SOUND_CMDHEADING("GTS80")
MACHINE_DRIVER_END

MACHINE_DRIVER_START(gts80s)
  MDRV_IMPORT_FROM(gts80)
  MDRV_IMPORT_FROM(gts80s_s)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(gts80ss)
  MDRV_IMPORT_FROM(gts80)
  MDRV_IMPORT_FROM(gts80s_ss)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(gts80vid)
  MDRV_IMPORT_FROM(gts80ss)
  MDRV_CPU_ADD_TAG("vcpu", I86, 5000000)
  MDRV_CPU_MEMORY(video_readmem, video_writemem)
/* video hardware */
  //MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
  //MDRV_SCREEN_SIZE(256, 256)
  //MDRV_VISIBLE_AREA(0, 255, 0, 255)
  //MDRV_PALETTE_LENGTH(16)
  //MDRV_VIDEO_START(generic)
  //MDRV_VIDEO_UPDATE(gts80vid)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(gts80b)
  MDRV_IMPORT_FROM(gts80)
  MDRV_IMPORT_FROM(gts80s_s)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(gts80bs1)
  MDRV_IMPORT_FROM(gts80)
  MDRV_IMPORT_FROM(gts80s_b1)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(gts80bs2)
  MDRV_IMPORT_FROM(gts80)
  MDRV_IMPORT_FROM(gts80s_b2)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(gts80bs3)
  MDRV_IMPORT_FROM(gts80)
  MDRV_IMPORT_FROM(gts80s_b3)
MACHINE_DRIVER_END
#if 0
static core_tData GTS80Data = {
  42, /* 42 DIPs (32 for the controller board, 8 for the SS- and 2 for the S-Board*/
  GTS80_updSw,
  1,
  GTS80_sndCmd_w, "GTS80",
  GTS80_sw2m, GTS80_m2lamp, GTS80_m2sw, GTS80_lamp2m
};
#endif
