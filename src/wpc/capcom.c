#include <stdarg.h>
#include "driver.h"
#include "cpu/m68000/m68000.h"
#include "core.h"

#define CC_VBLANKFREQ    60 /* VBLANK frequency */
#define CC_IRQFREQ      150 /* IRQ (via PIA) frequency*/
#define CC_ZCFREQ        85 /* Zero cross frequency */

#define CC_SOLSMOOTH       2 /* Smooth the Solenoids over this numer of VBLANKS */
#define CC_LAMPSMOOTH      2 /* Smooth the lamps over this number of VBLANKS */
#define CC_DISPLAYSMOOTH   4 /* Smooth the display over this number of VBLANKS */

static struct {
  UINT16 u16a[4],u16b[4];
  int vblankCount;
  int u16irqcount;
  int diagnosticLed;
  int visible_page;
  UINT8 lastb;
} locals;

static NVRAM_HANDLER(cc);

static INTERRUPT_GEN(cc_vblank) {
  /*-------------------------------
  /  copy local data to interface
  /--------------------------------*/
  locals.vblankCount += 1;

  /*-- lamps --*/
  if ((locals.vblankCount % CC_LAMPSMOOTH) == 0) {
    memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
    memset(coreGlobals.tmpLampMatrix, 0, sizeof(coreGlobals.tmpLampMatrix));
  }

#if 0
  /*-- solenoids --*/
  if ((locals.vblankCount % CC_SOLSMOOTH) == 0) {
    coreGlobals.solenoids = locals.solenoids;
    locals.solenoids = coreGlobals.pulsedSolState;
  }

#endif

  /*-- display --*/
  if ((locals.vblankCount % CC_DISPLAYSMOOTH) == 0) {
#if 0
    memcpy(coreGlobals.segments, locals.segments, sizeof(coreGlobals.segments));
    memcpy(locals.segments, locals.pseg, sizeof(locals.segments));
    memset(locals.pseg,0,sizeof(locals.pseg));
#endif
    /*update leds*/
    coreGlobals.diagnosticLed = locals.diagnosticLed;
	locals.diagnosticLed = 0;
  }
  core_updateSw(TRUE);
}

static SWITCH_UPDATE(cc) {
  if (inports) {
    CORE_SETKEYSW(inports[CORE_COREINPORT],0xcf,9);
    CORE_SETKEYSW(inports[CORE_COREINPORT]>>8,0xff,0);
  }
}

static void cc_zeroCross(int data) {
 cpu_set_irq_line(0,MC68306_IRQ_2,ASSERT_LINE);
}
static READ16_HANDLER(cc_porta_r) { DBGLOG(("Port A read\n")); return 0; }
static READ16_HANDLER(cc_portb_r) { DBGLOG(("Port B read\n")); return 0; }
static WRITE16_HANDLER(cc_porta_w) {
  DBGLOG(("Port A write %04x\n",data));
  locals.diagnosticLed = ((~data)&0x08>>3);
}
static WRITE16_HANDLER(cc_portb_w) {
  locals.lastb = data;
  DBGLOG(("Port B write %04x\n",data));
  if (data & ~locals.lastb & 0x01) cpu_set_irq_line(0,MC68306_IRQ_2,CLEAR_LINE);
}
static WRITE16_HANDLER(u16_w) {
  offset &= 0x203;
  DBGLOG(("U16w [%03x]=%04x (%04x)\n",offset,data,mem_mask));

  //Visible Page offset
  if(offset==3) {
	locals.visible_page = data >> 12;
	//printf("U16w [%03x]=%04x (%04x)\n",offset,data,mem_mask);
  }

  switch (offset) {
    case 0x000: case 0x001: case 0x002: case 0x003:
      locals.u16a[offset] = (locals.u16a[offset] & mem_mask) | data; break;
    case 0x200: case 0x201: case 0x202: case 0x203:
      locals.u16b[offset&3] = (locals.u16b[offset&3] & mem_mask) | data; break;
  }
}

static void cc_u16irq1(int data) {
  locals.u16irqcount += 1;
  if (locals.u16irqcount == (0x08>>((locals.u16b[0] & 0xc0)>>6))) {
    cpu_set_irq_line(0,MC68306_IRQ_1,PULSE_LINE);
    locals.u16irqcount = 0;
  }
}

static void cc_u16irq4(int data) {
	cpu_set_irq_line(0,MC68306_IRQ_4,PULSE_LINE);
}

static UINT32 fixaddr = 0;

static READ16_HANDLER(u16_r) {
  static int readnum = 0;
  int numcheck;

  //Once we've read the U16 3 times, we can safely return the ROM data we changed back, so U1 ROM test won't fail
  if(offset==0)
	readnum++;
  
  if(core_gameData->gen == 9 || core_gameData->gen == 11)
	numcheck = 2;
  else
    numcheck = 3;
  if(readnum == numcheck)
	  *((UINT16 *)(memory_region(REGION_CPU1) + fixaddr)) = 0x6600;
  //

  offset &= 0x203;
  DBGLOG(("U16r [%03x] (%04x)\n",offset,mem_mask));
  //printf("U16r [%03x] (%04x)\n",offset,mem_mask);
  //force U16 test to succeed
  //return 0x00bc;
  switch (offset) {
    case 0x000: case 0x001: case 0x002: case 0x003:
      return locals.u16a[offset];
    case 0x200: case 0x201: case 0x202: case 0x203:
      return locals.u16b[offset&3];
  }
  return 0;
}

static READ16_HANDLER(io_r) {
  UINT16 sw = 0;
  switch (offset) {
    case 0x200008:
      sw = coreGlobals.swMatrix[5] << 8 | coreGlobals.swMatrix[1];
      break;
    case 0x200009:
      sw = coreGlobals.swMatrix[6] << 8 | coreGlobals.swMatrix[2];
      break;
    case 0x20000a:
      sw = coreGlobals.swMatrix[7] << 8 | coreGlobals.swMatrix[3];
      break;
    case 0x20000b:
      sw = coreGlobals.swMatrix[8] << 8 | coreGlobals.swMatrix[4];
      break;
    case 0x400000:
      sw = coreGlobals.swMatrix[0] << 8 | coreGlobals.swMatrix[9];
  }
  return ~sw;
}

static WRITE16_HANDLER(io_w) {
#ifdef MAME_DEBUG
	if(keyboard_pressed_memory_repeat(KEYCODE_Z,2))
		printf("io_w [%03x]=%04x (%04x)\n",offset,data,mem_mask);
#endif
}

static MACHINE_INIT(cc) {
  memset(&locals, 0, sizeof(locals));
  locals.u16a[0] = 0x00bc;
  locals.vblankCount = 1;
  timer_pulse(TIME_IN_CYCLES(2811,0),0,cc_u16irq1);
  timer_pulse(TIME_IN_HZ(400),0,cc_u16irq4);

  //Force U16 Check to succeed
	switch(core_gameData->gen){
		case 0:
			break;
		case 1:
			fixaddr = 0x00091FD8; //PM
			break;
		case 2:
			break;
		case 3:
			fixaddr = 0x00089486; //AB
			break;
		case 4:
			break;
		case 5:
			fixaddr = 0x0008b198; //BS
			break;
		case 6:
		case 7:
		case 8:
			break;
		case 9:
			fixaddr = 0x00000302;	//FF
			break;
		case 10:
			fixaddr = 0x0004dd1c;	//BBB
			break;
		case 11:
			fixaddr = 0x00000316;	//KP
			break;
		default:
			break;
  }
  *((UINT16 *)(memory_region(REGION_CPU1) + fixaddr)) = 0x6700;
}

/*-----------------------------------
/  Memory map for CPU board
/------------------------------------*/
/*
PB0/IACK2 : XC Ack
PB1/IACK3 : Pulse ?
PB2/IACK5 : SW3 ?
PB3/IACK6 : SW4 ?
PB4/IRQ2  : XC
PB6/IRQ5  : SW6
IRQ1      : Not used
PB5/IRQ3  : Not used
IRQ4      : Not used
PB7/IRQ6  : Not used
IRQ7      : Not used
BG        : ?
BGACK     : Not used
BR        : Not used
BRERR     : Not used
RESET     : Not used
HALT      : Not used
PA0       : ?
PA1
PA2
PA3       : Diagnostic LED1
PA4       : Line S
PA5       : Line V
PA6       : V Set
PA7       : GReset

CS1 DRAM
CS2 I/O
CS3 NVRAM (D0-D7, A0-A14)
CS4 A20
CS5 A21
CS6 A22
CS7 A23
!I/O & A23

CS0
0x000000-0x1fffff : ROM0 (U1)
0x400000-0x5fffff : ROM1 (U2)
0x800000-0x9fffff : ROM2 (U3)
0xc00000-0xdfffff : ROM3 (U4)

CS2
AUX
EXT
SWITCH0
CS
From the very beginning of program execution, we see writes to the following registers, each bit specifies how CS0-CS7 will act:
Note: Invert the mask to see the end address range

CHIP  DATA        MASK     ADDRESS     RANGE             R/W
--------------------------------------------------------------
CS0 = 1000e680 => ff000000,10000000 => 10000000-10ffffff (R)
CS1 = 0001a2df => fff80000,00000000 => 00000000-0007ffff (RW)
CS2 = 4001a280 => ff000000,40000000 => 40000000-40ffffff (RW)
CS3 = 3001a2f0 => fffe0000,30000000 => 30000000-3001ffff (RW)

There are a # of issues with the memory map that need to be discussed:
First - ROM accesses is of course @ address 0, but after the CS0 is redefined, it should be at 0x10000000
DRAM access is moved to 0-7fffff.

However, Martin said he couldn't map stuff that way, so he mapped it as 24 bit addresses and moved stuff around..
Most obvious is that CS2 is @ 0x02000000 instead of 0x40000000, which means all I/O is in that range.
All other addresses are mapped >> 4 bits, ie, 0x30000000 now becomes 0x03000000
*/

/*-----------------------------------------------
/ Load/Save static ram
/-------------------------------------------------*/
static UINT16 *CMOS;

static NVRAM_HANDLER(cc) {
  core_nvram(file, read_or_write, CMOS, 0x10000, 0x00);
}

static int cc_sw2m(int no) {
	return no + 7;
}

static int cc_m2sw(int col, int row) {
	return col*8 + row - 9;
}

static data16_t *ramptr;
static MEMORY_READ16_START(cc_readmem)
  { 0x00000000, 0x00ffffff, MRA16_ROM },
  { 0x01000000, 0x0107ffff, MRA16_RAM },
  { 0x02000000, 0x02bfffff, io_r }, 
//{ 0x02000000, 0x02000001, MRA16_RAM }, /* AUX I/O */
//{ 0x02400000, 0x02400001, MRA16_RAM }, /* EXT I/O */
//{ 0x02800000, 0x02800001, MRA16_RAM }, /* SWITCH0 */
  { 0x02C00000, 0x02C007ff, u16_r },     /* U16 (A10,A2,A1)*/
  { 0x03000000, 0x0300ffff, MRA16_RAM }, /* NVRAM */
MEMORY_END

static MEMORY_WRITE16_START(cc_writemem)
  { 0x00000000, 0x00ffffbf, MWA16_ROM },
  { 0x01000000, 0x0107ffff, MWA16_RAM, &ramptr },
  { 0x02000000, 0x02bfffff, io_w },		
  { 0x02C00000, 0x02C007ff, u16_w },    /* U16 (A10,A2,A1)*/
  { 0x03000000, 0x0300ffff, MWA16_RAM, &CMOS }, /* NVRAM */
MEMORY_END

static PORT_READ16_START(cc_readport)
  { M68306_PORTA, M68306_PORTA+1, cc_porta_r },
  { M68306_PORTB+1, M68306_PORTB+2, cc_portb_r },
PORT_END
static PORT_WRITE16_START(cc_writeport)
  { M68306_PORTA, M68306_PORTA+1, cc_porta_w },
  { M68306_PORTB+1, M68306_PORTB+2, cc_portb_w },
PORT_END

MACHINE_DRIVER_START(cc)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(cc, NULL, NULL)
  MDRV_CPU_ADD(M68306, 16670000/1)
  MDRV_CPU_MEMORY(cc_readmem, cc_writemem)
  MDRV_CPU_PORTS(cc_readport, cc_writeport)
  MDRV_CPU_VBLANK_INT(cc_vblank, 1)
  MDRV_NVRAM_HANDLER(cc)
  MDRV_SWITCH_UPDATE(cc)
  MDRV_SWITCH_CONV(cc_sw2m,cc_m2sw)
  MDRV_DIAGNOSTIC_LEDH(1)
  MDRV_TIMER_ADD(cc_zeroCross, CC_ZCFREQ)
MACHINE_DRIVER_END

PINMAME_VIDEO_UPDATE(cc_dmd) {
  static UINT32 offset;
  tDMDDot dotCol;
  int ii, jj, kk;
  UINT16 *RAM;

  offset = 0x37ff0+(0x800*locals.visible_page);
  RAM = ramptr+offset;
  for (ii = 0; ii <= 32; ii++) {
    UINT8 *line = &dotCol[ii][0];
      for (kk = 0; kk < 16; kk++) {
		UINT16 intens1 = RAM[0];
		for(jj=0;jj<8;jj++) {
			*line++ = (intens1&0xc000)>>14;
			intens1 = intens1<<2;
		}
		RAM+=1;
	  }
    *line++ = 0;
	RAM+=16;
  }
  video_update_core_dmd(bitmap, cliprect, dotCol, layout);
  return 0;
}

