/************************************************************************************************
 Midway Pinball games
 --------------------

   Hardware:
   ---------
   Flicker: (Customized version of Bally EM machine by Dave Nutting; no production run.
             The SS patent was sold to Midway in 1977, and a few home pinball models
             were built using the same hardware design)

		CPU:     I4004 @ 750 kHz
		IO:      4004 Ports
		DISPLAY: 2 x 5 Digit, 3 x 2 Digit 7 Segment panels
		SOUND:	 Chimes

   Rotation VIII:
		CPU:     Z80 @ 1.1 MHz ?
			INT: NMI @ 900Hz ?
		IO:      Z80 Ports
		DISPLAY: 4 x 6 Digit, 4 x 2 Digit 7 Segment panels
		SOUND:	 ?
************************************************************************************************/
#include <stdarg.h>
#include "driver.h"
#include "cpu/i4004/i4004.h"
#include "cpu/z80/z80.h"
#include "core.h"
#include "midway.h"

#define MIDWAY_VBLANKFREQ  60 /* VBLANK frequency in HZ*/
#define MIDWAY_NMIFREQ    900 /* NMI frequency in HZ - at this rate, solenoid test seems OK. */

/*----------------
/  Local variables
/-----------------*/
static struct {
  int    vblankCount;
  int    diagnosticLed;
  int    tmpSwCol;
  UINT32 solenoids;
  UINT8  tmpLampData;
  UINT8  swMatrix[CORE_MAXSWCOL];
  UINT8  lampMatrix[CORE_MAXLAMPCOL];
  core_tSeg segments, pseg;
} locals;

static INTERRUPT_GEN(MIDWAY_nmihi) {
	cpu_set_nmi_line(MIDWAY_CPU, PULSE_LINE);
}

/*-------------------------------
/  copy local data to interface
/--------------------------------*/
static INTERRUPT_GEN(MIDWAY_vblank) {
  locals.vblankCount++;
  /*-- lamps --*/
  if ((locals.vblankCount % MIDWAY_LAMPSMOOTH) == 0) {
    memcpy(coreGlobals.lampMatrix, locals.lampMatrix, sizeof(locals.lampMatrix));
  }
  /*-- solenoids --*/
  coreGlobals.solenoids = locals.solenoids;
  if ((locals.vblankCount % MIDWAY_SOLSMOOTH) == 0)
  	locals.solenoids = 0;

  /*-- display --*/
  if ((locals.vblankCount % MIDWAY_DISPLAYSMOOTH) == 0)
  {
    memcpy(coreGlobals.segments, locals.segments, sizeof(coreGlobals.segments));
	memset(locals.segments, 0x00, sizeof locals.segments);
  }

  /*update leds*/
  coreGlobals.diagnosticLed = locals.diagnosticLed;

  core_updateSw(core_getSol(12));
}

static INTERRUPT_GEN(MIDWAYP_vblank) {
  locals.vblankCount++;
  /*-- lamps --*/
  if ((locals.vblankCount % MIDWAY_LAMPSMOOTH) == 0) {
    memcpy(coreGlobals.lampMatrix, locals.lampMatrix, sizeof(locals.lampMatrix));
  }
  /*-- solenoids --*/
  coreGlobals.solenoids = locals.solenoids;
  if ((locals.vblankCount % (MIDWAY_SOLSMOOTH)) == 0)
  	locals.solenoids = 0;

  /*-- display --*/
  if ((locals.vblankCount % MIDWAY_DISPLAYSMOOTH) == 0)
  {
    memcpy(coreGlobals.segments, locals.segments, sizeof(coreGlobals.segments));
	memset(locals.segments, 0x00, sizeof locals.segments);
  }

  /*update leds*/
  coreGlobals.diagnosticLed = locals.diagnosticLed;

  core_updateSw(1); // flippers always enabled
}

static SWITCH_UPDATE(MIDWAY) {
	if (inports) {
		coreGlobals.swMatrix[0] = inports[MIDWAY_COMINPORT] & 0xff;
		coreGlobals.swMatrix[3] = (coreGlobals.swMatrix[3] & 0xbf) | ((inports[MIDWAY_COMINPORT] & 0x0f00) >> 4);
		coreGlobals.swMatrix[4] = (coreGlobals.swMatrix[4] & 0xbf) | ((inports[MIDWAY_COMINPORT] & 0xf000) >> 8);
	}
}

static SWITCH_UPDATE(MIDWAYP) {
	if (inports) {
		coreGlobals.swMatrix[0] = inports[MIDWAY_COMINPORT] & 0x40;
		if (coreGlobals.swMatrix[0] & 0x40)
			i4004_set_TEST(1);
		/* test line has to be held longer than a few milliseconds */
		else if (locals.vblankCount % 5 == 0)
			i4004_set_TEST(0);
	}
}

static int MIDWAY_sw2m(int no) {
	return no + 8;
}

static int MIDWAY_m2sw(int col, int row) {
	return col*8 + row - 8;
}

/* if this read operation returns 0xc3, more rom code exists, starting at address 0x1800 */
static READ_HANDLER(mem1800_r) {
	return 0x00; /* 0xc3 */
}

/* game switches */
static READ_HANDLER(port_2x_r) {
	return coreGlobals.swMatrix[offset ? 0 : locals.tmpSwCol];
}

/* lamps & solenoids */
static WRITE_HANDLER(port_0x_w) {
	switch (offset) {
		case 0:
			locals.tmpLampData = data; // latch lamp data for strobe_w call
			break;
		case 1:
			locals.solenoids |= data;
			break;
		case 2:
			locals.solenoids |= (data << 8);
			break;
	}
}

/* sound maybe? */
static WRITE_HANDLER(port_1x_w) {
	// Deposit the output as solenoids until we know what it's for.
	// So in case the sounds are just solenoid chimes, we're already done.
	if (offset == 1)
		locals.solenoids = (locals.solenoids & 0xff00ffff) | (data << 16);
	else if (data != 0 && !((offset == 0 && data == 0x47) || (offset == 6 && data == 0x0f)))
		logerror("Unexpected output on port %02x = %02x\n", offset, data);
}

/* display data */
static WRITE_HANDLER(disp_w) {
	((int *)locals.pseg)[2*offset] = core_bcd2seg[data >> 4];
	((int *)locals.pseg)[2*offset + 1] = core_bcd2seg[data & 0x0f];
}

/* this handler updates lamps, switch columns & displays - all at the same time!!! */
static WRITE_HANDLER(strobe_w) {
	if (data < 7) { // only 7 columns are used
		int ii;
		for (ii = 0; ii < 6; ii++) {
			((int *)locals.segments)[ii*7 + data] = ((int *)locals.pseg)[ii];
		}
		locals.lampMatrix[data] = locals.tmpLampData;
		locals.tmpSwCol = data + 1;
	} else
		locals.tmpSwCol = 8;
}

/* port read / write for Rotation VIII */
PORT_READ_START( midway_readport )
	{ 0x21, 0x22, port_2x_r },
PORT_END

PORT_WRITE_START( midway_writeport )
	{ 0x00, 0x02, disp_w },
	{ 0x03, 0x05, port_0x_w },
	{ 0x10, 0x17, port_1x_w },
	{ 0x23, 0x23, strobe_w },
PORT_END

static READ_HANDLER(rom_r) {
	if (offset%2)
		return coreGlobals.swMatrix[offset/2 + 1] >> 4;
	else
		return coreGlobals.swMatrix[offset/2 + 1] & 0x0f;
}

static void flicker_disp(int offset, UINT8 data) {
	((int *)locals.segments)[offset] = core_bcd2seg[data];
}

static WRITE_HANDLER(rom1_w) {
	if (data > 0x09)
		data = 0;
	flicker_disp(15 - offset, data);
}

static WRITE_HANDLER(rom2_w) {
	if (offset%2)
		locals.lampMatrix[offset/2] = (locals.lampMatrix[offset/2] & 0x0f) | (data << 4);
	else
		locals.lampMatrix[offset/2] = (locals.lampMatrix[offset/2] & 0xf0) | data;
}

static WRITE_HANDLER(ram_w) {
	if (data != 0 && data != offset) {
		locals.solenoids |= (1 << data);
	}
}

/* port read / write for Flicker */
PORT_READ_START( midway_readport2 )
	{ 0x20, 0x2f, rom_r },
PORT_END

PORT_WRITE_START( midway_writeport2 )
	{ 0x00, 0x0f, rom1_w },
	{ 0x10, 0x1f, rom2_w },
	{ 0x110,0x11f,ram_w },
PORT_END

/*-----------------------------------------------
/ Load/Save static ram
/ Save RAM & CMOS Information
/-------------------------------------------------*/
static UINT8 *MIDWAY_CMOS;
static WRITE_HANDLER(MIDWAY_CMOS_w) {
  MIDWAY_CMOS[offset] = data;
}

static NVRAM_HANDLER(MIDWAY) {
	core_nvram(file, read_or_write, MIDWAY_CMOS, 256, 0x00);
}

/*-----------------------------------------
/  Memory map for Rotation VIII CPU board
/------------------------------------------
0000-17ff  3 x 2K ROM
c000-c0ff  RAM
e000-e0ff  NVRAM
*/
static MEMORY_READ_START(MIDWAY_readmem)
{0x0000,0x17ff,	MRA_ROM},	/* ROM */
{0x1800,0x1800,	mem1800_r}, /* Possible code extension. More roms to come? */
{0xc000,0xc0ff, MRA_RAM},   /* RAM */
{0xe000,0xe0ff,	MRA_RAM},	/* NVRAM */
MEMORY_END

static MEMORY_WRITE_START(MIDWAY_writemem)
{0x0000,0x17ff,	MWA_ROM},	/* ROM */
{0xc000,0xc0ff, MWA_RAM},   /* RAM */
{0xe000,0xe0ff,	MIDWAY_CMOS_w, &MIDWAY_CMOS},	/* NVRAM */
MEMORY_END

/*-----------------------------------------
/  Memory map for Flicker CPU board
/------------------------------------------
0000-03ff  1K ROM
1000-10ff  RAM
2000-20ff  RAM
*/
static MEMORY_READ_START(MIDWAYP_readmem)
{0x0000,0x03ff,	MRA_ROM},	/* ROM */
{0x1000,0x10ff, MRA_RAM},   /* RAM */
{0x2000,0x20ff, MRA_RAM},   /* RAM */
MEMORY_END

static MEMORY_WRITE_START(MIDWAYP_writemem)
{0x0000,0x03ff,	MWA_ROM},	/* ROM */
{0x1000,0x10ff, MWA_RAM},   /* RAM */
{0x2000,0x20ff, MWA_RAM},   /* RAM */
MEMORY_END

static MACHINE_INIT(MIDWAY) {
  memset(&locals, 0, sizeof locals);
}

static MACHINE_RESET(MIDWAYP) {
  UINT8* ram = memory_region(MIDWAY_CPU);
  /* clear RAM area */
  memset(ram+0x1000, 0x00, 0x100);
  memset(ram+0x2000, 0x00, 0x100);
  i4004_set_TEST(1);
}

static MACHINE_STOP(MIDWAY) {
}

MACHINE_DRIVER_START(MIDWAY)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CPU_ADD_TAG("mcpu", Z80, 1100000)
  MDRV_CPU_MEMORY(MIDWAY_readmem, MIDWAY_writemem)
  MDRV_CPU_PORTS(midway_readport,midway_writeport)
  MDRV_CPU_VBLANK_INT(MIDWAY_vblank, 1)
  MDRV_CPU_PERIODIC_INT(MIDWAY_nmihi, MIDWAY_NMIFREQ)
  MDRV_CORE_INIT_RESET_STOP(MIDWAY,NULL,MIDWAY)
  MDRV_NVRAM_HANDLER(MIDWAY)
  MDRV_DIPS(0) // no dips!
  MDRV_SWITCH_UPDATE(MIDWAY)
//  MDRV_DIAGNOSTIC_LEDH(4)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(MIDWAYProto)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CPU_ADD_TAG("mcpu", 4004, 750000)
  MDRV_CPU_MEMORY(MIDWAYP_readmem, MIDWAYP_writemem)
  MDRV_CPU_PORTS(midway_readport2,midway_writeport2)
  MDRV_CPU_VBLANK_INT(MIDWAYP_vblank, 1)
  MDRV_CORE_INIT_RESET_STOP(MIDWAY,MIDWAYP,MIDWAY)
  MDRV_DIPS(0) // no dips!
  MDRV_SWITCH_UPDATE(MIDWAYP)
//  MDRV_DIAGNOSTIC_LEDH(4)
MACHINE_DRIVER_END

#if 0
struct MachineDriver machine_driver_MIDWAY = {
  {
    {
      CPU_Z80, 1100000, /* 1.1 Mhz */
      MIDWAY_readmem, MIDWAY_writemem, NULL, NULL,
	  MIDWAY_vblank, 1,
	  NULL, 0
	},
	{ 0 }, /* MIDWAYS_SOUNDCPU */
  },
  MIDWAY_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50,
  MIDWAY_init,CORE_EXITFUNC(MIDWAY_exit)
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_TYPE_RASTER,
  0,
  NULL, NULL, gen_refresh,
  0,0,0,0,{{0}},
  MIDWAY_nvram
};
#endif
