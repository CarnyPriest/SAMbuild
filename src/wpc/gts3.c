/************************************************************************************************
  Gottlieb Pinball - System 3

  Hardware from 1989-1996

  Earlier games used 2 - 20 column 16 segment alphanumeric displays.
  Later games used a 128x32 DMD.

  There maybe an additional hardware change from games prior to deadly weapon, since
  some of the games don't seem to respond to the Diagnostic switch and are marked game not working.


  65c02: Vectors: FFFE&F = IRQ, FFFA&B = NMI, FFFC&D = RESET
  //Cueball: CPU0: RST = FEF0, IRQ=462F, NMI=477A
**************************************************************************************/
#include <stdarg.h>
#include "driver.h"
#include "cpu/m6502/m65ce02.h"
#include "machine/6522via.h"
#include "core.h"
#include "sndbrd.h"
#include "gts3.h"
#include "vidhrdw/crtc6845.h"
#include "gts3dmd.h"
#include "gts80s.h"

UINT8 DMDFrames[GTS3DMD_FRAMES][0x200];
#define GTS3_VBLANKDIV	      6 /* Break VBLANK into pieces*/

#define GTS3_VBLANKFREQ      60 /* VBLANK frequency*/
#define GTS3_IRQFREQ        975 /* IRQ Frequency (Guessed)*/
#define GTS3_DMDNMIFREQ     175 /* DMD NMI Frequency (Guessed)*/
#define GTS3_ALPHANMIFREQ   175 /* Alpha NMI Frequency (Guessed)*/
#define GTS3_TRIGIRQ	   1875 /* HACK To Keep the IRQ Firing */

#define GTS3_CPUNO	0
#define GTS3_DCPUNO 1
#define GTS3_SCPUNO 2

#if 1
#define logerror1 logerror
#else
#define logerror1 printf
#endif

/* FORCE The 16 Segment Layout to match the output order expected by core.c */
static const int alpha_adjust[16] =   {0,1,2,3,4,5,9,10,11,12,13,14,6,8,15,7};

static void GTS3_init(void);
static void GTS3_init2(void);
static void GTS3_exit(void);
static void GTS3_nvram(void *file, int write);
static WRITE_HANDLER(display_control);

/*Alpha Display Generation Specific*/
static void alphanmi(int data);
static WRITE_HANDLER(alpha_u4_pb_w);
static READ_HANDLER(alpha_u4_pb_r);
static WRITE_HANDLER(alpha_u5_pb_w);
static WRITE_HANDLER(alpha_u5_cb1_w);
static WRITE_HANDLER(alpha_u5_cb2_w);
static WRITE_HANDLER(alpha_display);
static WRITE_HANDLER(alpha_aux);
static void alpha_vblank(void);
static void alpha_update(void);
/*DMD Generation Specific*/
static void dmdswitchbank(void);
static void dmdnmi(int data);
static WRITE_HANDLER(dmd_u4_pb_w);
static READ_HANDLER(dmd_u4_pb_r);
static WRITE_HANDLER(dmd_u5_pb_w);
static WRITE_HANDLER(dmd_u5_cb1_w);
static WRITE_HANDLER(dmd_u5_cb2_w);
static WRITE_HANDLER(dmd_display);
static WRITE_HANDLER(dmd_aux);
static void dmd_vblank(void);
static void dmd_update(void);

/*----------------
/  Local varibles
/-----------------*/
struct {
  int    alphagen;
  core_tSeg segments, pseg;
  int    vblankCount;
  int    initDone;
  UINT32 solenoids;
  int    lampRow, lampColumn;
  int    diagnosticLed;
  int    diagnosticLeds1;
  int    diagnosticLeds2;
  int    swCol;
  int    ssEn;
  int    mainIrq;
  int	 swDiag;
  int    swTilt;
  int    swSlam;
  int    acol;
  int    u4pb;
  WRITE_HANDLER((*U4_PB_W));
  READ_HANDLER((*U4_PB_R));
  WRITE_HANDLER((*U5_PB_W));
  WRITE_HANDLER((*U5_CB1_W));
  WRITE_HANDLER((*U5_CB2_W));
  WRITE_HANDLER((*DISPLAY_CONTROL));
  WRITE_HANDLER((*AUX_W));
  void (*UPDATE_DISPLAY)(void);
  void (*VBLANK_PROC)(void);

  void* timer_nmi;
  void* timer_irq;

} GTS3locals;

struct {
  int	 pa0;
  int	 pa1;
  int	 pa2;
  int	 pa3;
  int	 a18;
  int	 q3;
  int	 dmd_latch;
  int	 diagnosticLed;
  int	 status1;
  int	 status2;
  int    dstrb;
  UINT8  dmd_visible_addr;
} GTS3_dmdlocals;


/*Hack to keep IRQ going*/
static void via_irq(int state);
static void trigirq(int data) { 
	static int lastirq=0;
	lastirq = !lastirq;
	//via_irq(lastirq);
}

/* U4 */

//PA0-7 Switch Rows/Returns (Switches are inverted)
static READ_HANDLER( xvia_0_a_r ) { return ~core_getSwCol(GTS3locals.lampColumn); }

//PB0-7 Varies on Alpha or DMD Generation!
static READ_HANDLER( xvia_0_b_r ) { return GTS3locals.U4_PB_R(offset); }

/* ALPHA GENERATION
   ----------------
   PB0-2: Output only
   PB3:  Slam Switch (NOTE: Test Switch on later Generations!)
   PB4:  Tilt Switch
   PB5-7: Output only
*/
static READ_HANDLER(alpha_u4_pb_r)
{
	int data = 0;
	//Gen 1 checks Slam switch here
	if(GTS3locals.alphagen==1)
		data |= (GTS3locals.swSlam <<3);	//Slam Switch (NOT INVERTED!) 
	else
		data |= (GTS3locals.swDiag << 3);	//Diag Switch (NOT INVERTED!)
	data |= (GTS3locals.swTilt << 4);   //Tilt Switch (NOT INVERTED!)
	return data;
}

/* DMD GENERATION
   ----------------
   PB0-2: Output only
   PB3:  Test Switch
   PB4:  Tilt Switch
   PB5-  A1P3-3 - Display Controller - Status1 (Not labeld on Schematic)
   PB6-  A1P3-1 - Display Controller - DMD Display Strobe (DSTB) - Output only, but might be read!
   PB7-  A1P3-2 - Display Controller - Status2 (Not labeld on Schematic)
*/
static READ_HANDLER(dmd_u4_pb_r)
{
	int data = 0;
	data |= (GTS3locals.swDiag << 3);	//Diag Switch (NOT INVERTED!)
	data |= (GTS3locals.swTilt << 4);   //Tilt Switch (NOT INVERTED!)
	data |= (GTS3_dmdlocals.status1 << 5);
	data |= (GTS3_dmdlocals.dstrb << 6);
	data |= (GTS3_dmdlocals.status2 << 7);
	return data;
}

//CA2:  To A1P6-12 & A1P7-6 Auxiliary (INPUT???)
static READ_HANDLER( xvia_0_ca2_r )
{
	// logerror1("READ: NA?: via_0_ca2_r\n");
	return 0;
}
static READ_HANDLER( xvia_0_cb1_r )
{
	// logerror1("READ: NA?: via_0_cb1_r\n");
	return 0;
}
static READ_HANDLER( xvia_0_cb2_r )
{
	// logerror1("READ: NA?: via_0_cb2_r\n");
	return 0;
}

static WRITE_HANDLER( xvia_0_a_w )
{
	// logerror1("WRITE:NA?: via_0_a_w: %x\n",data);
}

//PB0-7 Varies on Alpha or DMD Generation!
static WRITE_HANDLER( xvia_0_b_w ) { GTS3locals.U4_PB_W(offset,data); }

/* ALPHA GENERATION
   ----------------
  PB0:  Lamp Data      (LDATA)
  PB1:  Lamp Strobe    (LSTRB)
  PB2:  Lamp Clear     (LCLR)
  PB5:  Display Data   (DDATA)
  PB6:  Display Strobe (DSTRB)
  PB7:  Display Blank  (DBLNK)
*/
#define LDATA 0x01
#define LSTRB 0x02
#define LCLR  0x04
#define DDATA 0x20
#define DSTRB 0x40
#define DBLNK 0x80

static WRITE_HANDLER(alpha_u4_pb_w) {
	//logerror("lampcolumn=%4x STRB=%d LCLR=%d\n",GTS3locals.lampColumn,data&LSTRB,data&LCLR);
//	if (GTS3locals.u4pb & LCLR)  GTS3locals.lampColumn = 0; // Negative edge
	if (data & ~GTS3locals.u4pb & LSTRB) { // Positive edge
		GTS3locals.lampColumn = ((GTS3locals.lampColumn<<1) | (data & LDATA)) & 0x0fff;
	}
//	if (data & ~GTS3locals.u4pb & DBLNK) GTS3.digSel = 0; // Positive edge
	if (data & ~GTS3locals.u4pb & DSTRB) { // Positive edge
		if (data & DDATA) GTS3locals.acol = 0;
		else if (GTS3locals.acol < 20) GTS3locals.acol += 1;
	}
	GTS3locals.u4pb = data;
	core_setLamp(coreGlobals.tmpLampMatrix, GTS3locals.lampColumn, GTS3locals.lampRow);
}

/* DMD GENERATION
   ----------------
  PB0:  Lamp Data      (LDATA)
  PB1:  Lamp Strobe    (LSTRB)
  PB2:  Lamp Clear     (LCLR)
  PB6:  Display Strobe (DSTRB)
*/
static WRITE_HANDLER(dmd_u4_pb_w) {
//	if (~data & GTS3locals.u4pb & LCLR)  GTS3locals.lampColumn = 0; // Negative edge
	if (data & ~GTS3locals.u4pb & LSTRB) // Positive edge
		GTS3locals.lampColumn = ((GTS3locals.lampColumn<<1) | (data & LDATA)) & 0x0fff;
	GTS3_dmdlocals.dstrb = (data & DSTRB) != 0;
	GTS3locals.u4pb = data;
	core_setLamp(coreGlobals.tmpLampMatrix, GTS3locals.lampColumn, GTS3locals.lampRow);
}

//AUX DATA? See ca2 above!
static WRITE_HANDLER( xvia_0_ca2_w )
{
	// logerror1("WRITE:AUX W??:via_0_ca2_w: %x\n",data);
}

//CB2:  NMI to Main CPU
static WRITE_HANDLER( xvia_0_cb2_w )
{
	//logerror1("NMI: via_0_cb2_w: %x\n",data);
	cpu_cause_interrupt(0,M65C02_INT_NMI);
}

/* U5 */
//Not used?
static READ_HANDLER( xvia_1_a_r )
{
	// logerror1("via_1_a_r\n");
	return 0;
}

//Data to A1P6 Auxilary? Not used on a read?
static READ_HANDLER( xvia_1_b_r )
{
	// logerror1("via_1_b_r\n");
	return 0;
}
//CA1:   Sound Return/Status
static READ_HANDLER( xvia_1_ca1_r )
{
	// logerror1("SOUND RET READ: via_1_ca1_r\n");
	return 0;
}
//Should be NA!
static READ_HANDLER( xvia_1_ca2_r )
{
	// logerror1("via_1_ca2_r\n");
	return 0;
}
//CB1:   CX1 - A1P6 (Auxilary)
static READ_HANDLER( xvia_1_cb1_r )
{
	// logerror1("via_1_cb1_r\n");
	return 0;
}
//CB2:   CX2 - A1P6 (Auxilary)
static READ_HANDLER( xvia_1_cb2_r )
{
	// logerror1("via_1_cb2_r\n");
	return 0;
}

//PA0-7: SD0-7 - A1P4 (Sound data)
static WRITE_HANDLER( xvia_1_a_w )
{
	// logerror1("Sound Command: WRITE:via_1_a_w: %x\n",data);
	sndbrd_0_data_w(0, data);
}

//PB0-7 Varies on Alpha or DMD Generation!
static WRITE_HANDLER( xvia_1_b_w ) { GTS3locals.U4_PB_W(offset,data); }

/* ALPHA GENERATION
   ----------------
   Data to A1P6 LED Display Board
		DX0-DX3 = BCD Data
		DX4-DX6 = Column
		AX4(DX7?) = Latch the Data
*/
static WRITE_HANDLER(alpha_u5_pb_w) {
	// logerror1("ALPHA DATA: %x\n",data);
}

/* DMD GENERATION
   ----------------
   Data to A1P6 Auxilary
*/
static WRITE_HANDLER(dmd_u5_pb_w){ /* logerror1("AUX: WRITE:via_1_b_w: %x\n",data); */ }

//Should be not used!
static WRITE_HANDLER( xvia_1_ca1_w )
{
	logerror1("NOT USED!: via_1_ca1_w %x\n",data);
}
//CPU LED
static WRITE_HANDLER( xvia_1_ca2_w )
{
	GTS3locals.diagnosticLed = data;
}

//CB1 Varies on Alpha or DMD Generation!
static WRITE_HANDLER( xvia_1_cb1_w ) { GTS3locals.U5_CB1_W(offset,data); }
/* ALPHA GENERATION
   ----------------
   CB1:   CX1 - A1P6 Where Does this GO?
*/
static WRITE_HANDLER(alpha_u5_cb1_w) { /* logerror1("WRITE:via_1_cb1_w: %x\n",data); */ }
/* DMD GENERATION
   ----------------
   CB1:   CX1 - A1P6 (Auxilary)
*/
static WRITE_HANDLER(dmd_u5_cb1_w) { /* logerror1("WRITE:via_1_cb1_w: %x\n",data); */ }

//CB2 Varies on Alpha or DMD Generation!
static WRITE_HANDLER( xvia_1_cb2_w ) { GTS3locals.U5_CB2_W(offset,data); }
/* ALPHA GENERATION
   ----------------
   CB2:   CX2 - A1P6 Where Does this GO?
*/
static WRITE_HANDLER(alpha_u5_cb2_w){ /* logerror1("WRITE:via_1_cb2_w: %x\n",data); */ }
/* DMD GENERATION
   ----------------
   CB2:   CX2 - A1P6 (Auxilary)
*/
static WRITE_HANDLER(dmd_u5_cb2_w) { /* logerror1("WRITE:via_1_cb2_w: %x\n",data); */ }

//IRQ:  IRQ to Main CPU
static void via_irq(int state) { 
	// logerror("IN VIA_IRQ - STATE = %x\n",state);
#if 0
	if(state)
		printf("IRQ = 1\n");
	else
		printf("IRQ = 0\n");
#endif
	cpu_set_irq_line(0, M6502_INT_IRQ, PULSE_LINE); 
	//cpu_set_irq_line(0, M6502_INT_IRQ, state?ASSERT_LINE:CLEAR_LINE); 
}

/*

DMD Generation Listed Below (for different hardware see code)

U4:
---
(I)PA0-7 Switch Rows/Returns
(I)PB3:  Test Switch
(I)PB4:  Tilt Switch
(I)PB5-  A1P3-3 - Display Controller - Status1
(I)PB6-  A1P3-1 - Display Controller - Not used
(I)PB7-  A1P3-2 - Display Controller - Status2
(I)CA1: Slam Switch
(I)CB1: N/A
(O)PB0:  Lamp Data    (LDATA)
(O)PB1:  Lamp Strobe  (LSTRB)
(O)PB2:  Lamp Clear   (LCLR)
(O)PB6:  Display Strobe (DSTRB)
(O)PB7:  Display Blanking (DBLNK) - Alpha Generation Only!
(O)CA2: To A1P6-12 & A1P7-6 Auxiliary
(O)CB2: NMI to Main CPU
IRQ:  IRQ to Main CPU

U5:
--
(O)PA0-7: SD0-7 - A1P4 (Sound data)
(?)PB0-7: DX0-7 - A1P6 (Auxilary)
(I)CA1:   Sound Return/Status
(O)CA2:   LED
(I)CB1:   CX1 - A1P6 (Auxilary)
(O)CB2:   CX2 - A1P6 (Auxilary)
IRQ:  IRQ to Main CPU
*/
static struct via6522_interface via_0_interface =
{
	/*inputs : A/B         */ xvia_0_a_r, xvia_0_b_r,
	/*inputs : CA1/B1,CA2/B2 */ 0, xvia_0_cb1_r, xvia_0_ca2_r, xvia_0_cb2_r,
	/*outputs: A/B,CA2/B2   */ xvia_0_a_w, xvia_0_b_w, xvia_0_ca2_w, xvia_0_cb2_w,
	/*irq                  */ via_irq
};
static struct via6522_interface via_1_interface =
{
	/*inputs : A/B         */ xvia_1_a_r, xvia_1_b_r,
	/*inputs : CA1/B1,CA2/B2 */ xvia_1_ca1_r, xvia_1_cb1_r, xvia_1_ca2_r, xvia_1_cb2_r,
	/*outputs: A/B,CA2/B2   */ xvia_1_a_w, xvia_1_b_w, xvia_1_ca2_w, xvia_1_cb2_w,
	/*irq                  */ via_irq
};

static int GTS3_irq(void) {
  return ignore_interrupt();	//NO INT OR NMI GENERATED - External Devices Trigger it!
}

static int GTS3_vblank(void) {
  /*-------------------------------
  /  copy local data to interface
  /--------------------------------*/
  GTS3locals.vblankCount += 1;

  //Call the VBLANK Procedure which is meant to be called every time!!
  GTS3locals.VBLANK_PROC();

  /*-- Process during the REAL VBLANK period only--*/
  if ((GTS3locals.vblankCount % GTS3_VBLANKDIV) == 0) {

	  /*-- lamps --*/
	  if ((GTS3locals.vblankCount % GTS3_LAMPSMOOTH) == 0) {
		memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
		memset(coreGlobals.tmpLampMatrix, 0, sizeof(coreGlobals.tmpLampMatrix));
	  }
	  /*-- solenoids --*/
	  if ((GTS3locals.vblankCount % GTS3_SOLSMOOTH) == 0) {
		coreGlobals.solenoids = GTS3locals.solenoids;
		if (GTS3locals.ssEn) {
		  int ii;
		  coreGlobals.solenoids |= CORE_SOLBIT(CORE_SSFLIPENSOL);
		  /*-- special solenoids updated based on switches --*/
		  for (ii = 0; ii < 6; ii++)
			if (core_gameData->sxx.ssSw[ii] && core_getSw(core_gameData->sxx.ssSw[ii]))
			  coreGlobals.solenoids |= CORE_SOLBIT(CORE_FIRSTSSSOL+ii);
		}
		GTS3locals.solenoids = coreGlobals.pulsedSolState;
	  }
	  /*-- display --*/
	  if ((GTS3locals.vblankCount % GTS3_DISPLAYSMOOTH) == 0) {

		/*Update alpha or dmd display*/
		GTS3locals.UPDATE_DISPLAY();

		/*update leds*/
		coreGlobals.diagnosticLed = (GTS3locals.diagnosticLeds2<<3) |
									(GTS3locals.diagnosticLeds1<<2) |
									(GTS3_dmdlocals.diagnosticLed<<1) |
									GTS3locals.diagnosticLed;
		GTS3locals.diagnosticLed = 0;
		GTS3_dmdlocals.diagnosticLed = 0;
	  }
	  core_updateSw(GTS3locals.solenoids & 0x80000000);
  }
  return 0;
}

static void GTS3_updSw(int *inports) {
via_irq(1);
#if 0
	//HACK to make the IRQ work. Must be a bug in the VIA code, since this shouldn't be necessary!
	static int last=0;
	if(keyboard_pressed_memory_repeat(KEYCODE_A,2)) {
		via_irq(last);
		last = !last;
	}
#endif

  if (inports) {
    coreGlobals.swMatrix[0] = (inports[GTS3_COMINPORT] & 0x7f00)>>8;
    coreGlobals.swMatrix[1] = (coreGlobals.swMatrix[1] & 0x80) | (inports[GTS3_COMINPORT] & 0x7f);
  }
  GTS3locals.swDiag = (core_getSw(GTS3_SWDIAG)>0?1:0);
  GTS3locals.swTilt = (core_getSw(GTS3_SWTILT)>0?1:0);
  GTS3locals.swSlam = (core_getSw(GTS3_SWSLAM)>0?1:0);

  //Force CA1 to read our input!
  /*Alpha Gen 1 returns TEST Switch here - ALL others Slam Switch*/
  if(GTS3locals.alphagen==1)
	via_set_input_ca1(0,GTS3locals.swDiag);
  else
	via_set_input_ca1(0,GTS3locals.swSlam);
}

static WRITE_HANDLER(GTS3_sndCmd_w)
{
	sndbrd_0_data_w(0, data^0xff);
}

static int gts3_sw2m(int no) {
  if (no % 10 > 7) return -1;
  return (no / 10 + 1) * 8 + (no % 10);
}

static int gts3_m2sw(int col, int row) {
  return (col - 1) * 10 + row;
}

static core_tData GTS3Data = {
  0,	/* No DIPs */
  GTS3_updSw,
  4,	/* 4 Diagnostic LEDS (CPU,DMD,Sound 1, Sound 2) */
  GTS3_sndCmd_w, "GTS3",
  gts3_sw2m, gts3_sw2m, gts3_m2sw, gts3_m2sw
};

/*Alpha Numeric First Generation Init*/
static void GTS3_alpha_common_init(void) {
  if (GTS3locals.initDone) CORE_DOEXIT(GTS3_exit);
  if (core_init(&GTS3Data)) return;

  memset(&GTS3_dmdlocals, 0, sizeof(GTS3_dmdlocals));
  memset(&DMDFrames, 0, sizeof(DMDFrames));

  via_config(0, &via_0_interface);
  via_config(1, &via_1_interface);
  via_reset();

  GTS3locals.U4_PB_W  = alpha_u4_pb_w;
  GTS3locals.U4_PB_R  = alpha_u4_pb_r;
  GTS3locals.U5_PB_W  = alpha_u5_pb_w;
  GTS3locals.U5_CB1_W = alpha_u5_cb1_w;
  GTS3locals.U5_CB2_W = alpha_u5_cb2_w;
  GTS3locals.DISPLAY_CONTROL = alpha_display;
  GTS3locals.UPDATE_DISPLAY = alpha_update;
  GTS3locals.AUX_W = alpha_aux;
  GTS3locals.VBLANK_PROC = alpha_vblank;

  //Manually call the CPU NMI at the specified rate
  GTS3locals.timer_nmi = timer_pulse(TIME_IN_HZ(GTS3_ALPHANMIFREQ), 0, alphanmi);

  //Manually call the CPU IRQ at the specified rate
  GTS3locals.timer_irq = timer_pulse(TIME_IN_HZ(GTS3_TRIGIRQ), 0, trigirq);

  /* Init the sound board */
  sndbrd_0_init(core_gameData->hw.soundBoard, GTS3_SCPUNO-1, memory_region(GTS3_MEMREG_SCPU1), NULL, NULL);

  GTS3locals.initDone = TRUE;
}

/*Alpha Numeric First Generation Init*/
static void GTS3_init(void) {
	GTS3_alpha_common_init();
	GTS3locals.alphagen = 1;
}

/*Alpha Numeric Second Generation Init*/
static void GTS3b_init(void) {
	GTS3_alpha_common_init();
	GTS3locals.alphagen = 2;
}

/*DMD Generation Init*/
static void GTS3_init2(void) {
  if (GTS3locals.initDone) CORE_DOEXIT(GTS3_exit);
  if (core_init(&GTS3Data)) return;

  memset(&GTS3_dmdlocals, 0, sizeof(GTS3_dmdlocals));
  memset(&DMDFrames, 0, sizeof(DMDFrames));

  via_config(0, &via_0_interface);
  via_config(1, &via_1_interface);
  via_reset();

  /*DMD*/
  /*copy last 32K of ROM into last 32K of CPU region*/
  /*Setup ROM Swap so opcode will work*/
  if(memory_region(GTS3_MEMREG_DCPU1))
  {
  memcpy(memory_region(GTS3_MEMREG_DCPU1)+0x8000,
  memory_region(GTS3_MEMREG_DROM1) +
  (memory_region_length(GTS3_MEMREG_DROM1) - 0x8000), 0x8000);
  }
  GTS3_dmdlocals.pa0 = GTS3_dmdlocals.pa1 = GTS3_dmdlocals.pa2 = GTS3_dmdlocals.pa3 = 0;
  GTS3_dmdlocals.a18 = 0;
  //dmdswitchbank();

  GTS3locals.U4_PB_W  = dmd_u4_pb_w;
  GTS3locals.U4_PB_R  = dmd_u4_pb_r;
  GTS3locals.U5_PB_W  = dmd_u5_pb_w;
  GTS3locals.U5_CB1_W = dmd_u5_cb1_w;
  GTS3locals.U5_CB2_W = dmd_u5_cb2_w;
  GTS3locals.DISPLAY_CONTROL = dmd_display;
  GTS3locals.UPDATE_DISPLAY = dmd_update;
  GTS3locals.AUX_W = dmd_aux;
  GTS3locals.VBLANK_PROC = dmd_vblank;

  //Manually call the DMD NMI at the specified rate  (Although the code simply returns rti in most cases, we should call the nmi anyway, incase a game uses it)
  // GTS3locals.timer_nmi = timer_pulse(TIME_IN_HZ(GTS3_DMDNMIFREQ), 0, dmdnmi);

  //Manually call the CPU IRQ at the specified rate
  GTS3locals.timer_irq = timer_pulse(TIME_IN_HZ(GTS3_TRIGIRQ), 0, trigirq);

  /* Init the sound board */
  sndbrd_0_init(core_gameData->hw.soundBoard, GTS3_SCPUNO, memory_region(GTS3_MEMREG_SCPU1), NULL, NULL);

  GTS3locals.initDone = TRUE;
}

static void GTS3_exit(void) {
  if ( GTS3locals.timer_nmi ) {
	  timer_remove(GTS3locals.timer_nmi);
	  GTS3locals.timer_nmi = NULL;
  }

  if ( GTS3locals.timer_irq ) {
	  timer_remove(GTS3locals.timer_irq);
	  GTS3locals.timer_irq = NULL;
  }

  sndbrd_0_exit();
  core_exit();
}

/*Solenoids - Need to verify correct solenoid # here!*/
static WRITE_HANDLER(solenoid_w)
{
	//logerror1("SS Write: Offset: %x Data: %x\n",offset,data);
	switch(offset){
		case 0:
			coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xFFFFFF00) | data;
			break;
		case 1:
			coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xFFFF00FF) | (data<<8);
            break;
		case 2:
			coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xFF00FFFF) | (data<<16);
            break;
		case 3:
			coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0x00FFFFFF) | (data<<24);
			break;
		default:
			logerror1("Solenoid_W Logic Error\n");
	}
}

/*DMD*/
static void dmdswitchbank(void)
{
	int	addr =	(GTS3_dmdlocals.pa0 *0x04000)+
				(GTS3_dmdlocals.pa1 *0x08000)+
				(GTS3_dmdlocals.pa2 *0x10000)+
 				(GTS3_dmdlocals.pa3 *0x20000)+
				(GTS3_dmdlocals.a18 *0x40000);
	cpu_setbank(1, memory_region(GTS3_MEMREG_DROM1) + addr);
}

static READ_HANDLER(dmdlatch_r) { return GTS3_dmdlocals.dmd_latch; }


//PB0-7 Varies on Alpha or DMD Generation!
static WRITE_HANDLER(display_control) { GTS3locals.DISPLAY_CONTROL(offset,data); }

/* ALPHA GENERATION
   ----------------
   Alpha Strobe:
		DS0 = enable a,b,c,d,e,f,g,h of Bottom Segment
		DS1 = enable i,j,k,l,m,n,dot,comma of Bottom Segment
		DS2 = enable a,b,c,d,e,f,g,h of Top Segment
		DS3 = enable i,j,k,l,m,n,dot,comma of Top Segment
*/
static WRITE_HANDLER(alpha_display){
	if ((GTS3locals.u4pb & ~DBLNK) && (GTS3locals.acol < 20)) {
		if(offset == 0) GTS3locals.segments[1][GTS3locals.acol].hi |= GTS3locals.pseg[1][GTS3locals.acol].hi = data;
		else
		if(offset == 1) GTS3locals.segments[1][GTS3locals.acol].lo |= GTS3locals.pseg[1][GTS3locals.acol].lo = data;
		else
		if(offset == 2) GTS3locals.segments[0][GTS3locals.acol].hi |= GTS3locals.pseg[0][GTS3locals.acol].hi = data;
		else
		if(offset == 3) GTS3locals.segments[0][GTS3locals.acol].lo |= GTS3locals.pseg[0][GTS3locals.acol].lo = data;
	}
}

/* DMD GENERATION
   ----------------
   DMD Strobe & Reset:
		DS0 = Pulse the DMD CPU IRQ Line
		DS1 = Reset the DMD CPU
		DS2 = Not Used
		DS3 = Not Used
*/
static WRITE_HANDLER(dmd_display){
	//Latch DMD Data from U7
    GTS3_dmdlocals.dmd_latch = data;
	if(offset==0) cpu_cause_interrupt(1,M65C02_INT_IRQ);
	else
		if(offset==1) cpu_set_reset_line(1, PULSE_LINE);
		else
			logerror("DMD Signal: Offset: %x Data: %x\n",offset,data);
}

//FIRE NMI FOR DMD!
static void dmdnmi(int data){ cpu_cause_interrupt(1,M65C02_INT_NMI); }

/*Chip U14 - LS273:
  D0=Q0=PA0=A14 of DMD Eprom
  D1=Q1=PA1=A15 of DMD Eprom
  D2=Q2=PA2=A16 of DMD Eprom (Incorrectly identified as D5,Q5 on the schematic!)
  D3=Q3=PA3=A17 of DMD Eprom (Incorrectly identified as D4,Q4 on the schematic!)
  D4=Q4=Fed to GAL16V8(A18?) (Incorrectly identified as D3,Q3 on the schematic!)
  D5=Q5=PB5(U4)=DMD Status 1 (Incorrectly identified as D2,Q2 on the schematic!)
  D6=Q6=PB7(U4)=DMD Status 2
  D7=DMD LED
*/
static WRITE_HANDLER(dmdoport)
{
GTS3_dmdlocals.pa0=(data>>0)&1;
GTS3_dmdlocals.pa1=(data>>1)&1;
GTS3_dmdlocals.pa2=(data>>2)&1;
GTS3_dmdlocals.pa3=(data>>3)&1;
GTS3_dmdlocals.q3 =(data>>4)&1; GTS3_dmdlocals.a18=GTS3_dmdlocals.q3;
GTS3_dmdlocals.status1=(data>>5)&1;
GTS3_dmdlocals.status2=(data>>6)&1;
GTS3_dmdlocals.diagnosticLed = data>>7;
dmdswitchbank();
}

//This should never be called!
static READ_HANDLER(display_r){ /* logerror("DISPLAY_R\n"); */ return 0;}


//Writes Lamp Returns
static WRITE_HANDLER(lds_w)
{
	//logerror1("LDS Write: Data: %x\n",data);
    GTS3locals.lampRow = data;
	core_setLamp(coreGlobals.tmpLampMatrix, GTS3locals.lampColumn, GTS3locals.lampRow);
}

//PB0-7 Varies on Alpha or DMD Generation!
static WRITE_HANDLER(aux_w) {
	if(offset==0) GTS3locals.AUX_W(offset,data);
	else
	   logerror("aux_w: %x %x\n",offset,data);
}
/* ALPHA GENERATION
   ----------------
   LED Board Digit Strobe
*/
static WRITE_HANDLER(alpha_aux) {
//	GTS3locals.ax4 = data;
	logerror1("LED Strobe: %x\n",data);
}
/* DMD GENERATION
   ----------------
   Auxilary Data (Not Used)
*/
static WRITE_HANDLER(dmd_aux) { logerror1("AUX Write: Offset: %x Data: %x\n",offset,data); }

static WRITE_HANDLER(aux1_w)
{
	logerror1("Aux1 Write: Offset: %x Data: %x\n",offset,data);
}

static void alphanmi(int data) { xvia_0_cb2_w(0,0); }

static void alpha_update(){
    /* FORCE The 16 Segment Layout to match the output order expected by core.c */
	// There's got to be a better way than this junky code!
	UINT16 segbits, tempbits;
	int i,j,k, pos;
	for(i=0;i<2;i++) {
		for(j=0;j<20;j++){
			segbits = ((GTS3locals.segments[i][j].lo<<8) | GTS3locals.segments[i][j].hi);
			tempbits = 0;
			if(segbits > 0) {
				for(k=0;k<16;k++){
					if((segbits>>k)&1){
						pos = alpha_adjust[k];
						tempbits |= (1<<pos);
					}
				}
			}
			GTS3locals.segments[i][j].lo=(tempbits&0xff00)>>8;
			GTS3locals.segments[i][j].hi=tempbits&0x00ff;
		}
	}
	memcpy(coreGlobals.segments, GTS3locals.segments, sizeof(coreGlobals.segments));
    memcpy(GTS3locals.segments, GTS3locals.pseg, sizeof(GTS3locals.segments));
}
static void dmd_update() {}

//Show Sound Diagnostic LEDS
void UpdateSoundLEDS(int num,int data)
{
	if(num==0)
		GTS3locals.diagnosticLeds1 = data;
	else
		GTS3locals.diagnosticLeds2 = data;
}

void alpha_vblank(void) { }

//Update the DMD Frames
void dmd_vblank(void){
  int offset = (crtc6845_start_addr>>2);
  memcpy(DMDFrames[coreGlobals_dmd.nextDMDFrame],memory_region(GTS3_MEMREG_DCPU1)+0x1000+offset,0x200);
  coreGlobals_dmd.nextDMDFrame = (coreGlobals_dmd.nextDMDFrame + 1) % GTS3DMD_FRAMES;
  dmdnmi(0);
}

/*---------------------------
/  Memory map for main CPU
/----------------------------*/
static MEMORY_READ_START(GTS3_readmem)
{0x0000,0x1fff,MRA_RAM},
{0x2000,0x200f,via_0_r},
{0x2010,0x201f,via_1_r},
{0x2020,0x2023,display_r},
{0x4000,0xffff,MRA_ROM},
MEMORY_END


static MEMORY_WRITE_START(GTS3_writemem)
{0x0000,0x1fff,MWA_RAM},
{0x2000,0x200f,via_0_w},
{0x2010,0x201f,via_1_w},
{0x2020,0x2023,display_control},
{0x2030,0x2033,solenoid_w},
{0x2040,0x2040,lds_w},
{0x2041,0x2043,aux_w},
{0x2050,0x2070,aux1_w},
{0x4000,0xffff,MWA_ROM},
MEMORY_END


/*---------------------------
/  Memory map for DMD CPU
/----------------------------*/
static MEMORY_READ_START(GTS3_dmdreadmem)
{0x0000,0x1fff, MRA_RAM},
{0x3000,0x3000, dmdlatch_r}, /*Input Enable*/
{0x4000,0x7fff, MRA_BANK1},
{0x8000,0xffff, MRA_ROM},
MEMORY_END

static MEMORY_WRITE_START(GTS3_dmdwritemem)
{0x0000,0x0fff, MWA_RAM},
{0x1000,0x1fff, MWA_RAM},    /*DMD Display RAM*/
{0x2800,0x2800, crtc6845_address_w},
{0x2801,0x2801, crtc6845_register_w},
{0x3800,0x3800, dmdoport},   /*Output Enable*/
{0x4000,0x7fff, MWA_BANK1},
{0x8000,0xffff, MWA_ROM},
MEMORY_END


/* First Generation Alpha Numeric Games */
struct MachineDriver machine_driver_GTS3_1A = {
  {
    {
      CPU_M65C02, 2000000, /* 2 Mhz */
      GTS3_readmem, GTS3_writemem, NULL, NULL,
      GTS3_vblank, GTS3_VBLANKDIV,
      GTS3_irq, GTS3_IRQFREQ
    }
  },
  GTS3_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50,
  GTS3_init,CORE_EXITFUNC(GTS3_exit)
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_TYPE_RASTER,
  0,
  NULL, NULL, gen_refresh,
  0,0,0,0,{{ 0 }},
  GTS3_nvram
};

/* Second Generation Alpha Numeric Games */
struct MachineDriver machine_driver_GTS3_1B = {
  {
    {
      CPU_M65C02, 2000000, /* 2 Mhz */
      GTS3_readmem, GTS3_writemem, NULL, NULL,
      GTS3_vblank, GTS3_VBLANKDIV,
      GTS3_irq, GTS3_IRQFREQ
    }
  },
  GTS3_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50,
  GTS3b_init,CORE_EXITFUNC(GTS3_exit)
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_TYPE_RASTER,
  0,
  NULL, NULL, gen_refresh,
  0,0,0,0,{{ 0 }},
  GTS3_nvram
};

/* First Generation Alpha Numeric Games WITH SOUND */
struct MachineDriver machine_driver_GTS3_1AS = {
  {
    {
      CPU_M65C02, 2000000, /* 2 Mhz */
      GTS3_readmem, GTS3_writemem, NULL, NULL,
      GTS3_vblank, GTS3_VBLANKDIV,
      GTS3_irq, GTS3_IRQFREQ
    }
	GTS3_SOUNDCPU2
    GTS3_SOUNDCPU1},
  GTS3_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50,
  GTS3_init,CORE_EXITFUNC(GTS3_exit)
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_TYPE_RASTER,
  0,
  NULL, NULL, gen_refresh,
  0,0,0,0,{GTS3_SOUND},
  GTS3_nvram
};

/* Second Generation Alpha Numeric Games WITH SOUND */
struct MachineDriver machine_driver_GTS3_1BS = {
  {
    {
      CPU_M65C02, 2000000, /* 2 Mhz */
      GTS3_readmem, GTS3_writemem, NULL, NULL,
      GTS3_vblank, GTS3_VBLANKDIV,
      GTS3_irq, GTS3_IRQFREQ
    }
	GTS3_SOUNDCPU2
    GTS3_SOUNDCPU1},
  GTS3_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50,
  GTS3b_init,CORE_EXITFUNC(GTS3_exit)
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_TYPE_RASTER,
  0,
  NULL, NULL, gen_refresh,
  0,0,0,0,{GTS3_SOUND},
  GTS3_nvram
};

/* DMD Generation Games */
struct MachineDriver machine_driver_GTS3_2 = {
  {
    {
      CPU_M65C02, 2000000, /* 2 Mhz */
      GTS3_readmem, GTS3_writemem, NULL, NULL,
      GTS3_vblank, GTS3_VBLANKDIV,
      GTS3_irq, GTS3_IRQFREQ
    },
	{
      CPU_M65C02, 3579000/2, /* 1.76? Mhz*/
      GTS3_dmdreadmem, GTS3_dmdwritemem, NULL, NULL,
      ignore_interrupt,0    /*IRQ & NMI are generated by other devices*/
    }
  },
  GTS3_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50,
  GTS3_init2,CORE_EXITFUNC(GTS3_exit)
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_TYPE_RASTER,
  0,
  NULL, NULL, gts3_dmd128x32_refresh,
  0,0,0,0,{{ 0 }},
  GTS3_nvram
};

/* DMD Generation Games WITH SOUND */
struct MachineDriver machine_driver_GTS3_2S = {
  {
    {
      CPU_M65C02, 2000000, /* 2 Mhz */
      GTS3_readmem, GTS3_writemem, NULL, NULL,
      GTS3_vblank, GTS3_VBLANKDIV,
      GTS3_irq, GTS3_IRQFREQ
    },
	{
      CPU_M65C02, 3579000/2, /* 1.76? Mhz*/
      GTS3_dmdreadmem, GTS3_dmdwritemem, NULL, NULL,
      ignore_interrupt,0    /*IRQ & NMI are generated by other devices*/
    }
    GTS3_SOUNDCPU2
    GTS3_SOUNDCPU1},
  GTS3_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50,
  GTS3_init2,CORE_EXITFUNC(GTS3_exit)
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_TYPE_RASTER,
  0,
  NULL, NULL, gts3_dmd128x32_refresh,
  0,0,0,0,{GTS3_2_SOUND},
  GTS3_nvram
};


/*-----------------------------------------------
/ Load/Save static ram
/ Save RAM & CMOS Information
/-------------------------------------------------*/
void GTS3_nvram(void *file, int write) {
  if (write)  /* save nvram */
    osd_fwrite(file, memory_region(GTS3_MEMREG_CPU), 0x2000);
  else if (file) /* load nvram */
    osd_fread(file, memory_region(GTS3_MEMREG_CPU), 0x2000);
  else        /* first time */
    memset(memory_region(GTS3_MEMREG_CPU), 0x00, 0x2000);
}
