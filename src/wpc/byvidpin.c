/* Bally Video/Pinball Combination Hardware*/

/* MPU Board: MPU-133 (Equivalent of MPU-35 except for 1 diode change)
   VIDIOT Board: Handles Video/Joystick Switchs/Sound Board

   For simplicity sake, I just copied all code from by35 
   -but maybe someday we should merge common routines for easier maintenance

  Hardware:
	Baby Pac - Videot (1 x TMS9928 Video Chip)
	Granny & Gators - Videot Deluxe (2 x TMS9928 Video Chip - Master/Slave configuration)

  6809 vectors:
  RES: FFFE-F
  NMI: FFFC-D
  IRQ: FFF8-9
  FIRQ: FFF6-7
*/
#include <stdarg.h>
#include <time.h>
#include "driver.h"
#include "cpu/m6800/m6800.h"
#include "cpu/m6809/m6809.h"
#include "machine/6821pia.h"
#include "vidhrdw/tms9928a.h"
#include "core.h"
#include "byvidpin.h"
#include "snd_cmd.h"

static int cmdnum = 0;

#if 0
	#define mlogerror logerror
#else
	#define mlogerror printf
#endif

#define BYVP_VCPUNO 1		/* Video CPU # */
#define BYVP_SCPUNO 2		/* Sound CPU # */

#define BYVP_VBLANKFREQ    60 /* VBLANK frequency */
#define BYVP_IRQFREQ      150 /* IRQ (via PIA) frequency*/
#define BYVP_ZCFREQ        85 /* Zero cross frequency */

static struct {
  int p0_a, p1_a, p1_b, p0_ca2, p1_ca2, p0_cb2, p1_cb2;
  int bcd[6];
  int lampadr1, lampadr2;
  UINT32 solenoids;
  core_tSeg segments,pseg;
  int diagnosticLed;
  int diagnosticLedV;
  int vblankCount;
  int initDone;
  int enable_output;		//Enable Output to Main CPU Flag
  int enable_input;			//Enable Input From Main CPU Flag
  int status_enable;		//Enable Reading of Status Flag
  int vidiot_status;		//Status of Vidiot
  int vidiot_u1_latch;		//U1 Latch -> Data going to Main CPU from Vidiot
  int vidiot_u2_latch;		//U2 Latch -> Data coming from Main CPU
  int u7_portb;				//U7 Port B data
  int u7_portcb2;			//U7 Port CB2
  int snddata;				//Sound Latch
  int lasttin;				//Track Last TIN IRQ Edge
  void *zctimer;
} locals;

static void byVP_exit(void);
static void byVP_nvram(void *file, int write);

static void piaIrq(int state) { cpu_set_irq_line(0, M6800_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE); }

static void byVP_lampStrobe(int board, int lampadr) {
  if (lampadr != 0x0f) {
    int lampdata = (locals.p0_a>>4)^0x0f;
    UINT8 *matrix = &coreGlobals.tmpLampMatrix[(lampadr>>3)+8*board];
    int bit = 1<<(lampadr & 0x07);

    //DBGLOG(("adr=%x data=%x\n",lampadr,lampdata));
    while (lampdata) {
      if (lampdata & 0x01) *matrix |= bit;
      lampdata >>= 1; matrix += 2;
    }
  }
}

/* PIA0:A-W  Control what is read from PIA0:B
(out) PA0-1: Cabinet Switches Strobe (shared below)
(out) PA0-3: Lamp Address (Shared with Switch Strobe)
(out) PA0-4: Switch Strobe(Columns)
(out) PA4-7: BCD Lamp Data
(out) PA5:   S01-S08 Dip Enable
(out) PA6:   S09-S16 Dip Enable
(out) PA7:   S17-S24 Dip Enable
(out) PA0-7: Vidiot Input Data (When Latch Input Flag is set?) */
static WRITE_HANDLER(pia0a_w) {
  locals.p0_a = data;	//Controls Strobing
  byVP_lampStrobe(0,locals.lampadr1);
  //Latch data to U2
  locals.vidiot_u2_latch = data;
  mlogerror("%x: Writing %x to vidiot u2\n",cpu_getpreviouspc(),data);
}

/* PIA1:A-W: Communications to Vidiot
(out) PA0:	 N/A
(out) PA1:   Video Enable Output
(out) PA2:   Video Latch Input Data
(out) PA3:   Video Status Enable
(out) PA4-7: N/A*/
static WRITE_HANDLER(pia1a_w) {
locals.enable_output = (data>>1)&1;
locals.enable_input  = (data>>2)&1;
locals.status_enable = (data>>3)&1;
mlogerror("Setting: e_out = %x, e_in = %x, s_enable = %x\n",
		 locals.enable_output,locals.enable_input,locals.status_enable);
//Update Vidiot PIA
pia_set_input_ca1(2, locals.enable_output);
pia_set_input_ca2(2, locals.enable_input);
}

/* PIA0:B-R  Get Data depending on PIA0:A
(in)  PB0-1: Vidiot Status Bits 0,1 INVERTED - (When Status Data Enabled Flag is Set)
(in)  PB0-7: Vidiot Output Data (When Enable Output Flag is Set(Active Low) )
(in)  PB0-7: Switch Returns/Rows and Cabinet Switch Returns/Rows
(in)  PB0-7: Dip Returns */
static READ_HANDLER(pia0b_r) {
  if (locals.status_enable) {
	    mlogerror("%x: MPU: reading vidiot status %x\n",cpu_getpreviouspc(),locals.vidiot_status&0x03);
		return locals.vidiot_status;
  }
  if (~locals.enable_output) {
	    mlogerror("%x: MPU: reading vidiot data %x\n",cpu_getpreviouspc(),locals.vidiot_u1_latch);
		return locals.vidiot_u1_latch;
  }
  if (locals.p0_a & 0x20) return core_getDip(0); // DIP#1-8
  if (locals.p0_a & 0x40) return core_getDip(1); // DIP#2-16
  if (locals.p0_a & 0x80) return core_getDip(2); // DIP#17-24
  if (locals.p0_cb2)      return core_getDip(3); // DIP#25-32
  return core_getSwCol((locals.p0_a & 0x1f)); //| ((locals.p1_b & 0x80)>>2)); ??
}

/* PIA0:CB2-W Lamp Strobe #1, DIPBank3 STROBE */
static WRITE_HANDLER(pia0cb2_w) {
  if (locals.p0_cb2 & ~data) locals.lampadr1 = locals.p0_a & 0x0f;
  locals.p0_cb2 = data;
}
/* PIA1:CA2-W Diagnostic LED */
static WRITE_HANDLER(pia1ca2_w) {
  locals.diagnosticLed = data;
  locals.p1_ca2 = data;
}

/* PIA1:B-W Solenoid */
static WRITE_HANDLER(pia1b_w) {
  locals.p1_b = data;
  coreGlobals.pulsedSolState = 0;
  if (!locals.p1_cb2)
    locals.solenoids |= coreGlobals.pulsedSolState = (1<<(data & 0x0f)) & 0x7fff;
  data ^= 0xf0;
  coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xfff0ffff) | ((data & 0xf0)<<12);
  locals.solenoids |= (data & 0xf0)<<12;
}

/* PIA1:CB2-W Solenoid Bank Select */
static WRITE_HANDLER(pia1cb2_w) {
  locals.p1_cb2 = data;
}

static int byVP_vblank(void) {
  /*-------------------------------
  /  copy local data to interface
  /--------------------------------*/
  locals.vblankCount += 1;

  /*-- lamps --*/
  if ((locals.vblankCount % BYVP_LAMPSMOOTH) == 0) {
    memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
    memset(coreGlobals.tmpLampMatrix, 0, sizeof(coreGlobals.tmpLampMatrix));
  }

  /*-- solenoids --*/
  if ((locals.vblankCount % BYVP_SOLSMOOTH) == 0) {
    coreGlobals.solenoids = locals.solenoids;
    locals.solenoids = coreGlobals.pulsedSolState;
  }
  /*-- display --*/
  if ((locals.vblankCount % BYVP_DISPLAYSMOOTH) == 0) {
    /*update leds*/
    coreGlobals.diagnosticLed = (locals.diagnosticLedV<<1)|
								(locals.diagnosticLed);
	//Why is this needed?
    locals.diagnosticLed = 0;
	locals.diagnosticLedV = 0;
  }
  core_updateSw(core_getSol(19));
  return 0;
}

static void byVP_updSw(int *inports) {
  if (inports) {
    coreGlobals.swMatrix[0] = (inports[BYVP_COMINPORT]>>11) & 0x0f;
    coreGlobals.swMatrix[1] = (coreGlobals.swMatrix[1] & (~0x24)) |
                              ((inports[BYVP_COMINPORT]<<2) & 0x24);
    coreGlobals.swMatrix[2] = (coreGlobals.swMatrix[2] & (~0xc6)) |
                              ((inports[BYVP_COMINPORT]>>3) & 0xc6);
  }
  /*-- Diagnostic buttons on CPU board --*/
  if (core_getSw(BYVP_SWCPUDIAG))  cpu_set_nmi_line(0, PULSE_LINE);
  if (core_getSw(BYVP_SWSOUNDDIAG)) cpu_set_nmi_line(BYVP_SCPUNO, PULSE_LINE);
  if (core_getSw(BYVP_SWVIDEODIAG)) cpu_set_nmi_line(BYVP_VCPUNO, PULSE_LINE);
  /*-- coin door switches --*/
  pia_set_input_ca1(0, !core_getSw(BYVP_SWSELFTEST));
}

/* PIA2:B Read */
// Video Switch Returns (Bits 5-7 not connected)
static READ_HANDLER(pia2b_r) { 
	logerror("VID: Reading Switch Returns r\n");
	return 0; 
}

/* PIA2:CA1 Read */
// Read Enable Data Output to Main CPU
static READ_HANDLER(pia2ca1_r) { 
	mlogerror("%x:VID: Reading output enable=%x\n",cpu_getpreviouspc(),locals.enable_output);
	return locals.enable_output;
}

/* PIA2:CA2 Read */
// Read Main CPU Latch Data Input
static READ_HANDLER(pia2ca2_r) { 
	mlogerror("%x:VID: Reading input enable\n=%x",cpu_getpreviouspc(),locals.enable_input);
	return locals.enable_input;   //Not Inverted 
}

/* PIA2:A Write */
//PA0-3: Status Data Inverted! (Only Bits 0 & 1 Used however)
//PA4-7: Video Switch Strobes (Only Bit 7 is used however)
static WRITE_HANDLER(pia2a_w) {
	locals.vidiot_status = ~(data & 0x03);
	mlogerror("%x:VID: Setting status to %x\n",cpu_getpreviouspc(),data&0x03);
	mlogerror("Setting Video Switch Strobe to %x\n",(data&0xf0)>>4);
}

/* PIA2:B Write*/
//PB0-3: Output to 6803 CPU
//PB4-7: N/A
static WRITE_HANDLER(pia2b_w) {
	locals.snddata = data<<1;	//Data from PB0-3 is connected to P21-24 but port 2 starts at P20!
}

/* PIA2:CB2 Write */
// Diagnostic LED & Data/Sound Strobe to 6803
static WRITE_HANDLER(pia2cb2_w) { 
	locals.diagnosticLedV = data;

	/*set 6803 P20 line - Thus triggering a sound command*/
	if(data & ~locals.lasttin) {	//Rising Edge
		cmdnum = 0;
		logerror("VID: Send Sound Command %x\n",locals.snddata);
		snd_cmd_log(locals.snddata);
		cpu_set_irq_line(BYVP_SCPUNO, M6800_TIN_LINE, ASSERT_LINE);
	}
	else
		cpu_set_irq_line(BYVP_SCPUNO, M6800_TIN_LINE, CLEAR_LINE);
	locals.lasttin = data;
}

//VIDEO PIA IRQ - TRIGGER VIDEO CPU (6809) FIRQ
static void pia2Irq(int state) {
  mlogerror("VID: PIA IRQ - MPU CAUSED FIRQ\n");
  cpu_set_irq_line(BYVP_VCPUNO, M6809_FIRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}

/*PIA 0*/
/*PIA U10:
(in)  PB0-1: Vidiot Status Bits 0,1 INVERTED - (When Status Data Enabled Flag is Set)
(in)  PB0-7: Vidiot Output Data (When Enable Output Flag is Set)
(in)  PB0-7: Switch Returns/Rows and Cabinet Switch Returns/Rows
(in)  PB0-7: Dip Returns
(in)  CA1:   Self Test Switch
(in)  CB1:   Zero Cross Detection
(in)  CA2:   N/A
(out) PA0-1: Cabinet Switches Strobe (shared below)
(out) PA0-3: Lamp Address (Shared with Switch Strobe)
(out) PA0-4: Switch Strobe(Columns)
(out) PA4-7: BCD Lamp Data
(out) PA5:   S01-S08 Dip Enable
(out) PA6:   S09-S16 Dip Enable
(out) PA7:   S17-S24 Dip Enable
(out) PA0-7: Vidiot Input Data (When Latch Input Flag is set?)
(out) CA2:   NA
(out) CB2:   S25-S32 Dip Enable (shared with Lamp Strobe?)
(out) CB2:   Lamp Strobe
	  IRQ:	 Wired to Main 6800 CPU IRQ.*/

/*PIA 1*/
/*PIA U11:
(in)  PA0-7	 N/A
(in)  PB0-7  N/A
(in)  CA1:   Display Interrupt Generator
(in)  CB1:	 J5 - Pin 32 (Marked as Test Connector)
(in)  CA2:	 N/A
(in)  CB2:	 N/A
(out) PA0:	 N/A
(out) PA1:   Video Enable Output
(out) PA2:   Video Latch Input Data
(out) PA3:   Video Status Enable
(out) PA4-7: N/A
(out) PB0-3: Momentary Solenoid
(out) PB4-7: Continuous Solenoid
(out) CA2:	 Diag LED
(out) CB2:   Solenoid Bank Select
	  IRQ:	 Wired to Main 6800 CPU IRQ.*/

/*---VIDIOT BOARD---*/

/*PIA 2*/
/*PIA U7:
(in)  PA0-7	 N/A
(in)  PB0-7  Video Switch Returns (Bits 5-7 not connected)
(in)  PB0-3  ?? Input from 6803 CPU?
(in)  CA1:   Enable Data Output to Main CPU
(in)  CB1:	 N/A
(in)  CA2:	 Main CPU Latch Data Input
(in)  CB2:	 J2-1 = N/A?
(out) PA0-3: Status Data (Only Bits 0 & 1 Used however) - Inverted before reaching MPU
(out) PA4-7: Video Switch Strobes (Only Bit 7 is used however)
(out) PB0-3: Output to 6803 CPU
(out) PB4-7: N/A
(out) CA2:	 N/A
(out) CB2:   6803 Data Strobe & LED
	  IRQ:	 Wired to Video 6809 CPU FIRQ*/
static struct pia6821_interface piaIntf[] = {{
/* I:  A/B,CA1/B1,CA2/B2 */  0, pia0b_r, 0,0, 0,0,
/* O:  A/B,CA2/B2        */  pia0a_w,0, 0 ,pia0cb2_w,
/* IRQ: A/B              */  piaIrq,piaIrq
},{
/* I:  A/B,CA1/B1,CA2/B2 */  0,0, 0,0, 0,0,
/* O:  A/B,CA2/B2        */  pia1a_w,pia1b_w,pia1ca2_w,pia1cb2_w,
/* IRQ: A/B              */  piaIrq,piaIrq
},{
/* I:  A/B,CA1/B1,CA2/B2 */  0,pia2b_r, 0,0, 0,0,
/* O:  A/B,CA2/B2        */  pia2a_w,pia2b_w,0,pia2cb2_w,
/* IRQ: A/B              */  pia2Irq,pia2Irq
}};

//Read Display Interrupt Generator
static int byVP_irq(void) {
  static int last = 0;
  pia_set_input_ca1(1, last = !last);
  return 0;
}

static WRITE_HANDLER(byVP_soundCmd) {
	snd_cmd_log(data);
	if(data) {
		printf("Sending..%x\n",data);
		locals.snddata = data<<1;	//Data from PB0-3 is connected to P21-24 but port 2 starts at P20!
		cmdnum = 0;
		snd_cmd_log(locals.snddata);
		/*set 6803 P20 line - Thus triggering a sound command*/
		cpu_set_irq_line(BYVP_SCPUNO, M6800_TIN_LINE, PULSE_LINE);
	}
}

static core_tData byVPData = {
  32, /* 32 Dips */
  byVP_updSw, 2 | DIAGLED_VERTICAL, byVP_soundCmd, "byVP",
  core_swSeq2m, core_swSeq2m, core_m2swSeq, core_m2swSeq
};

static void byVP_zeroCross(int data) {
  /*- toggle zero/detection circuit-*/
  pia_set_input_cb1(0, 0); pia_set_input_cb1(0, 1);
}
static void byVP_init(void) {
  if (locals.initDone) CORE_DOEXIT(byVP_exit);

  if (core_init(&byVPData)) return;
  memset(&locals, 0, sizeof(locals));

  /* init PIAs */
  pia_config(0, PIA_STANDARD_ORDERING, &piaIntf[0]);
  pia_config(1, PIA_STANDARD_ORDERING, &piaIntf[1]);
  pia_config(2, PIA_STANDARD_ORDERING, &piaIntf[2]);
  pia_reset();
  locals.vblankCount = 1;
  locals.zctimer = timer_pulse(TIME_IN_HZ(BYVP_ZCFREQ),0,byVP_zeroCross);
  locals.initDone = TRUE;
}

static void byVP_exit(void) {
#ifdef PINMAME_EXIT
  if (locals.zctimer) { timer_remove(locals.zctimer); locals.zctimer = NULL; }
#endif
  core_exit();
}
static UINT8 *byVP_CMOS;
static WRITE_HANDLER(byVP_CMOS_w) { byVP_CMOS[offset] = data; }

//Should never be called!
static READ_HANDLER(sound_port1_r) { 
	//logerror("sound port 1 read\n"); 
	return 0; 
}

//P20(Bit 0) = U7(CB2)
//P21-24 = Video U7(PB0-PB3)

/* NOTE: Sound commands are sent as 2 nibbles, first low byte, then high
         However, it reads the first low nibble 2x, reads the high nibble*/
static READ_HANDLER(sound_port2_r) { 
	cmdnum++;
	if( cmdnum==1 || cmdnum==2 ) {
		printf("cmd = %x: return = %x\n",cmdnum,locals.snddata &0x0f);
		return locals.snddata&0x0f;
	}
	else
		{
			printf("cmd = %x: return = %x\n",cmdnum,(locals.snddata &0xf0)>>4);
			cmdnum = 0;
			return (locals.snddata & 0xf0)>>4;
		}
}

static WRITE_HANDLER(sound_port1_w) { DAC_0_data_w(offset,data); }

/*Is this ever used?
--------------------
*/
//P20(Bit 0) = LED & U7-CB2
//P21-24 = Video U7(PB0-PB3)
static WRITE_HANDLER(sound_port2_w) {
	locals.diagnosticLedV = (data>>0)&1;
	pia_set_input_cb2(2,locals.diagnosticLedV);
	pia_set_input_b(2,data&0x1e);	//Keep only bits 1-4
	logerror("sound port 2 write = %x\n",data);
}

/***************************************************************************

  The interrupts come from the vdp. The vdp (tms9928a) interrupt can go up
  and down; the Coleco only uses nmi interrupts (which is just a pulse). They
  are edge-triggered: as soon as the vdp interrupt line goes up, an interrupt
  is generated. Nothing happens when the line stays up or goes down.

  To emulate this correctly, we set a callback in the tms9928a (they
  can occur mid-frame). At every frame we call the TMS9928A_interrupt
  because the vdp needs to know when the end-of-frame occurs, but we don't
  return an interrupt.

***************************************************************************/

static int by_interrupt(void)
{
    TMS9928A_interrupt();
    return ignore_interrupt ();
}

static void by_vdp_interrupt (int state)
{
	static int last_state = 0;

	logerror("vdp_int\n");

    /* only if it goes up */
	if (state && !last_state) {
		mlogerror("VDP Caused Vidiot CPU IRQ\n");
		cpu_set_irq_line(BYVP_VCPUNO, M6809_IRQ_LINE, PULSE_LINE);
	}
	last_state = state;
}

//Video CPU Vertical Blank
static int byVP_vvblank(void) {
	if(keyboard_pressed_memory_repeat(KEYCODE_A,2))
	{
	static int ct=0;
	locals.vidiot_u2_latch = ct++;
	logerror("sending %x\n",ct);
	pia_set_input_ca2(2, 0);
	pia_set_input_ca2(2, 1);
	}
	return by_interrupt();
}

static READ_HANDLER(vdp_r) {
	logerror("vdp_r\n");
	if(offset == 0)
		return TMS9928A_vram_r(offset);
	else
		return TMS9928A_register_r(offset);
}

static WRITE_HANDLER(vdp_w) {
	logerror("%x:vdp_w=%x\n",offset,data);
	if(offset==0)
		TMS9928A_vram_w(offset,data);
	else
		TMS9928A_register_w(offset,data);
}

static int by_vh_start(void)
{
	if (TMS9928A_start(TMS99x8A, 0x4000)) return 1;
	TMS9928A_int_callback(by_vdp_interrupt);
	return 0;
}
static void by_vh_stop(void)
{
  TMS9928A_stop();
}

#if 1
static void by_drawStatus (struct mame_bitmap *bmp, int full_refresh);
static void by_vh_refresh (struct mame_bitmap *bmp, int full_refresh)
	{
		TMS9928A_refresh(bmp,full_refresh);
		by_drawStatus(bmp, full_refresh);
	}

	#define GRAPHICSETUP \
	  32*8, 32*8, { 0*8, 32*8-1, 0*8, 32*8-1 }, \
	  0, /* gfxdecodeinfo */\
	  TMS9928A_PALETTE_SIZE,\
	  TMS9928A_COLORTABLE_SIZE,\
	  tms9928A_init_palette,\
	  VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,\

#else
	#define by_vh_refresh gen_refresh
	#define GRAPHICSETUP \
		256, CORE_SCREENY, { 0, 256-1, 0, CORE_SCREENY-1 }, \
		0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,\
		VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY, \

#endif

static READ_HANDLER(misc_r)
{
	int data = locals.vidiot_u2_latch;
	logerror("MISC_R: offset=%x, data=%x\n",offset,data);
	return data;
}
static WRITE_HANDLER(misc_w)
{
	locals.vidiot_u1_latch = data;
	logerror("MISC_W: offset=%x, data=%x\n",offset,data);
}

static int vidram[0xff];	

static READ_HANDLER(vid_r) {
	logerror("READ: %x %x\n",offset,vidram[offset]);
	return vidram[offset];
}
static WRITE_HANDLER(vid_w) {
	logerror("WRITE: %x = %x\n",offset,data);
	vidram[offset] = data;
}


static struct DACinterface by_dacInt =
  { 1, { 50 }};

/*-----------------------------------
/  Memory map for MAIN CPU board
/------------------------------------*/
static MEMORY_READ_START(byVP_readmem)
  { 0x0000, 0x0080, MRA_RAM }, /* U7 128 Byte Ram*/
  { 0x0088, 0x008b, pia_0_r }, /* U10 PIA: Switchs + Display + Lamps*/
  { 0x0090, 0x0093, pia_1_r }, /* U11 PIA: Solenoids/Sounds + Display Strobe */
  { 0x0200, 0x02ff, MRA_RAM }, /* CMOS Battery Backed*/
//{0x0300,0x03ff, vid_r},		// What does this do?
  { 0x1000, 0x1fff, MRA_ROM },
  { 0x5000, 0x5fff, MRA_ROM },
  { 0xf000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(byVP_writemem)
  { 0x0000, 0x0080, MWA_RAM }, /* U7 128 Byte Ram*/
  { 0x0088, 0x008b, pia_0_w }, /* U10 PIA: Switchs + Display + Lamps*/
  { 0x0090, 0x0093, pia_1_w }, /* U11 PIA: Solenoids/Sounds + Display Strobe */
  { 0x0200, 0x02ff, byVP_CMOS_w, &byVP_CMOS }, /* CMOS Battery Backed*/
//{0x0300,0x03ff, vid_w},		// What does this do?
  { 0x1000, 0x1fff, MWA_ROM },
  { 0x5000, 0x5fff, MWA_ROM },
  { 0xf000, 0xffff, MWA_ROM },
MEMORY_END

/*----------------------------------------------------
/  Memory map for VIDEO CPU (Located on Vidiot Board)
/----------------------------------------------------*/
static MEMORY_READ_START(byVP_video_readmem)
	{ 0x0000, 0x1fff, misc_r },  
	{ 0x2000, 0x2003, pia_2_r }, /* U7 PIA */
	{ 0x4000, 0x4001, vdp_r },   /* U16 VDP*/
	{ 0x6000, 0x6400, MRA_RAM }, /* U13&U14 1024x4 Byte Ram*/
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(byVP_video_writemem)
	{ 0x0000, 0x1fff, misc_w },  
	{ 0x2000, 0x2003, pia_2_w }, /* U7 PIA */
	{ 0x4000, 0x4001, vdp_w },   /* U16 VDP*/
	{ 0x6000, 0x6400, MWA_RAM }, /* U13&U14 1024x4 Byte Ram*/
	{ 0x8000, 0xffff, MWA_ROM },
MEMORY_END

/*----------------------------------------------------------
/  Memory map for VIDEO CPU (Located on Vidiot Board) - G&G
/---------------------------------------------------------*/
static MEMORY_READ_START(byVP2_video_readmem)
//	{ 0x0000, 0x1fff, misc_r },  
	{ 0x0002, 0x0003, vdp_r },   /* U16 VDP*/
	{ 0x0008, 0x000b, pia_2_r }, /* U7 PIA */
	{ 0x2400, 0x2800, MRA_RAM }, /* U13&U14 1024x4 Byte Ram*/
	{ 0x4000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(byVP2_video_writemem)
//	{ 0x0000, 0x1fff, misc_w },  
	{ 0x0002, 0x0003, vdp_w },   /* U16 VDP*/
	{ 0x0008, 0x000b, pia_2_w }, /* U7 PIA */
	{ 0x2400, 0x2800, MWA_RAM }, /* U13&U14 1024x4 Byte Ram*/
	{ 0x4000, 0xffff, MWA_ROM },
MEMORY_END

/*-----------------------------------------------------
/  Memory map for SOUND CPU (Located on Vidiot Board)
/-----------------------------------------------------*/
/*
  6803 vectors:
  RES: FFFE-F 
  SWI: FFFA-B Software Interrupt (Not Used)
  NMI: FFFC-D (Used)
  IRQ: FFF8-9 (Not Used)
  ICF: FFF6-7 (Input Capture)~IRQ2 (FA6F)
  OCF: FFF4-5 (Output Compare)~IRQ2 (Not Used)
  TOF: FFF2-3 (Timer Overflow)~IRQ2 (Not Used)
  SCI: FFF0-1 (Input Capture)~IRQ2  (Not Used)
*/
/*
NMI: = Sound Test Switch
IRQ: = NA
Port 1:
(out)P10 = DAC 8
(out)P11 = DAC 7
(out)P12 = DAC 6
(out)P13 = DAC 5
(out)P14 = DAC 4
(out)P15 = DAC 3
(out)P16 = DAC 2
(out)P17 = DAC 1
Port 2:
(in)P20 = U7(CB2)->Data Strobe Irq
(in)P21 = U7(PB0)->DATA Nibble
(in)P22 = U7(PB1)->DATA Nibble
(in)P23 = U7(PB2)->DATA Nibble
(in)P24 = U7(PB3)->DATA Nibble
(out)P20 = LED & U7(CB2)
*/
static MEMORY_READ_START(byVP_sound_readmem)
	{ 0x0000, 0x001f, m6803_internal_registers_r },
	{ 0x0080, 0x00ff, MRA_RAM },	/*Internal 128K RAM*/
	{ 0x0000, 0xdfff, MRA_NOP },
	{ 0xe000, 0xffff, MRA_ROM },	/* U29 ROM */
MEMORY_END

static MEMORY_WRITE_START(byVP_sound_writemem)
	{ 0x0000, 0x001f, m6803_internal_registers_w },
	{ 0x0080, 0x00ff, MWA_RAM },	/*Internal 128K RAM*/
	{ 0x0000, 0xdfff, MWA_NOP },
	{ 0xe000, 0xffff, MWA_ROM },	/* U29 ROM */
MEMORY_END

static PORT_READ_START( byVP_sound_readport )
	{ M6803_PORT1, M6803_PORT1, sound_port1_r },
	{ M6803_PORT2, M6803_PORT2, sound_port2_r },
PORT_END

static PORT_WRITE_START( byVP_sound_writeport )
	{ M6803_PORT1, M6803_PORT1, sound_port1_w },
	{ M6803_PORT2, M6803_PORT2, sound_port2_w },
PORT_END

/*--------------------*/
/* Machine Definition */
/*--------------------*/
/*BABYPAC HARDWARE*/
struct MachineDriver machine_driver_byVP = {
  {{  CPU_M6800, 3580000/4, /* 3.58/4 = 900Hz */
      byVP_readmem, byVP_writemem, NULL, NULL,
      byVP_vblank, 1, byVP_irq, BYVP_IRQFREQ
   },
  {  CPU_M6809, 3580000/4, /* 3.58/4 = 900Hz */
      byVP_video_readmem, byVP_video_writemem, NULL, NULL,
      byVP_vvblank, 1
  },
  {  CPU_M6803 | CPU_AUDIO_CPU, 3580000/4, /* 3.58/4 = 900Hz */
      byVP_sound_readmem, byVP_sound_writemem, byVP_sound_readport, byVP_sound_writeport,
      NULL, 1, NULL, 1
  }
  },
  BYVP_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50, byVP_init, CORE_EXITFUNC(byVP_exit)
  GRAPHICSETUP
  0,
  by_vh_start,by_vh_stop, by_vh_refresh,
  0,0,0,0, {{SOUND_DAC,&by_dacInt}},
  byVP_nvram
};

/*GRANNY & THE GATORS HARDWARE*/
struct MachineDriver machine_driver_byVP2 = {
  {{  CPU_M6800, 3580000/4, /* 3.58/4 = 900Hz */
      byVP_readmem, byVP_writemem, NULL, NULL,
      byVP_vblank, 1, byVP_irq, BYVP_IRQFREQ
   },
  {  CPU_M6809, 4000000/2, /* 2MHz */
      byVP2_video_readmem, byVP2_video_writemem, NULL, NULL,
      byVP_vvblank, 1
  },
  {  CPU_M6803 | CPU_AUDIO_CPU, 3580000/4, /* 3.58/4 = 900Hz */
      byVP_sound_readmem, byVP_sound_writemem, byVP_sound_readport, byVP_sound_writeport,
      NULL, 1, NULL, 1
  }
  },
  BYVP_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50, byVP_init, CORE_EXITFUNC(byVP_exit)
  GRAPHICSETUP
  0,
  by_vh_start,by_vh_stop, by_vh_refresh,
  0,0,0,0, {{SOUND_DAC,&by_dacInt}},
  byVP_nvram
};


/*-----------------------------------------------
/ Load/Save static ram
/-------------------------------------------------*/
static void byVP_nvram(void *file, int write) {
  core_nvram(file, write, byVP_CMOS, 0x100,0xff);
}

#if 1
/*--------------------------------------------
/ Draw status display
/ Lamps, Switches, Solenoids, Diagnostic LEDs
/---------------------------------------------*/
void by_drawStatus(struct mame_bitmap *bitmap, int fullRefresh) {
  BMTYPE **lines = (BMTYPE **)bitmap->line;
  int firstRow = 0;
  int ii, jj, bits;
  BMTYPE dotColor[2];


  /*-- anything to do ? --*/
  //if ((coreGlobals_dmd.dmdOnly) ||
  //    (coreGlobals.soundEn && (!manual_sound_commands(bitmap, &fullRefresh))))
  //  return;

  dotColor[0] = CORE_COLOR(6); dotColor[1] = CORE_COLOR(10);
  /*-----------------
  /  Draw the lamps
  /------------------*/
  /*-- Normal square lamp matrix layout --*/
  for (ii = 0; ii < CORE_CUSTLAMPCOL + core_gameData->hw.lampCol; ii++) {
      BMTYPE **line = &lines[firstRow];
      bits = coreGlobals.lampMatrix[ii];

      for (jj = 0; jj < 8; jj++) {
        line[0][ii*2] = dotColor[bits & 0x01];
        line += 2; bits >>= 1;
      }
  }
  osd_mark_dirty(0,firstRow,16,firstRow+ii*2);

  firstRow += 20;
  /*-------------------
  /  Draw the switches
  /--------------------*/
  for (ii = 0; ii < CORE_CUSTSWCOL+core_gameData->hw.swCol; ii++) {
    BMTYPE **line = &lines[firstRow];
    bits = coreGlobals.swMatrix[ii];

    for (jj = 0; jj < 8; jj++) {
      line[0][ii*2] = dotColor[bits & 0x01];
      line += 2; bits >>= 1;
    }
  }
  osd_mark_dirty(0,firstRow,16,firstRow+ii*2);
  firstRow += 20;

  /*------------------------------
  /  Draw Solenoids and Flashers
  /-------------------------------*/
  firstRow += 5;
  {
    BMTYPE **line = &lines[firstRow];
    UINT64 allSol = core_getAllSol();
    for (ii = 0; ii < CORE_FIRSTCUSTSOL+core_gameData->hw.custSol; ii++) {
      line[(ii/8)*2][(ii%8)*2] = dotColor[allSol & 0x01];
      allSol >>= 1;
    }
    osd_mark_dirty(0, firstRow, 16,
        firstRow+((CORE_FIRSTCUSTSOL+core_gameData->hw.custSol+7)/8)*2);
  }

  /*------------------------------*/
  /*-- draw diagnostic LEDs     --*/
  /*------------------------------*/
  firstRow += 20;
  {
    BMTYPE **line = &lines[firstRow];
    bits = coreGlobals.diagnosticLed;

    // Draw LEDS Vertically
    if (coreData.diagLEDs & DIAGLED_VERTICAL) {
       for (ii = 0; ii < (coreData.diagLEDs & ~DIAGLED_VERTICAL); ii++) {
	   	  line[0][5] = dotColor[bits & 0x01];
		  line += 2; bits >>= 1;
	   }
    }
    else { // Draw LEDS Horizontally
		for (ii = 0; ii < coreData.diagLEDs; ii++) {
			line[0][5+ii*2] = dotColor[bits & 0x01];
			bits >>= 1;
			}
	}
  }
  osd_mark_dirty(5, firstRow, 21, firstRow+20);

  firstRow += 25;
  if (core_gameData->gen & GEN_ALLWPC) {
    for (ii = 0; ii < CORE_MAXGI; ii++)
      lines[firstRow][ii*2] = dotColor[coreGlobals.gi[ii]>0];
    osd_mark_dirty(0, firstRow, 2*CORE_MAXGI+1, firstRow+2);
  }
  if (coreGlobals.simAvail) sim_draw(fullRefresh, firstRow);
  /*-- draw game specific mechanics --*/
  if (core_gameData->hw.drawMech) core_gameData->hw.drawMech((void *)&bitmap->line[firstRow]);
}
#endif
