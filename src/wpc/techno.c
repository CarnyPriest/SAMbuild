/************************************************************************************************
  Technoplay
  ----------
  by Gerrit Volkenborn & Steve Ellenoff
  
  Main CPU Board:

  CPU: Motorola M68000
  Clock: 8 Mhz
  Interrupt: Tied to a Fixed System Timer
  I/O: DMA

  Issues/Todo:
  #0) Display is going too fast @ 8Mhz cpu
  #1) Not 100% sure of the IRQ timing, although I think it should be correct from the schematics.
  #2) Seems the display might go too fast in places, ie, doesn't scroll enough sometimes
  #3) In relation to #2 - not sure if calculation of the display column is always 100% correct
  #4) Need sound roms to work on sound emulation
  #5) I think switches need to be flipped around as they don't line up as the manual shows
  #6) Proper 16 Segment display should be implemented, instead of hacking it to fit the core

  NOTE ON TIMING: The manual claims the "Display Message" will appear every 15 seconds
************************************************************************************************/
#include "driver.h"
#include "cpu/m68000/m68000.h"
#include "core.h"
#include "sim.h"

#define TECHNO_SOLSMOOTH 4
//#define TECHNO_CPUFREQ 8000000		//As written in manual
#define TECHNO_CPUFREQ 4000000			//Seems to work better

//8Mhz Crystal from CPU feeds an LS393 - Q1 (acts as divide by 2) - Feeds 4040 which divides by 128
#define XFORCE_IRQ_FREQ TIME_IN_HZ((TECHNO_CPUFREQ/2)/128)		//Not 100% sure on this one..

#if 0
#define LOG(x) printf x
#else
#define LOG(x) logerror x
#endif

static struct {
  int vblankCount;
  core_tSeg segments;
  UINT32 solenoids;
  UINT16 sols2;
  int irq_count;
  int DispNoWait;
  int DispCol;
  int LampCol;
} locals;

//Convert Bit Column Data to corresponding #, ie, if Bit 3=1, return 3 - Zero Based (Bit1=1 returns 0)
//Probably should be moved to core.c since it could be used in many drivers.
static int BitColToNum(int tmp)
{
	int i, data;
	i = data = 0;
	while(tmp)
	{
		if(tmp&1) data+=i;
		tmp = tmp>>1;
		i++;
	}
	return data;
}

/* Each time an IRQ is fired, the Vector # is incremented (since the IRQ generation is via a 4040 timer)
 
   Bit 7, 3 are always 1, and 5, 6 are always 0!
   Therefore, the sequence is:
   0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f (and repeat)

   The very first time, 0x88 is skipped, since an IRQ is only triggered from a 1->0 transition */
int xforce_irq_callback(int x)
{
	int vector_num = 0;
	if(locals.irq_count < 0x08)
		vector_num = 0x88+locals.irq_count;
	else
		vector_num = (0x98-0x08)+locals.irq_count;

	//LOG(("servicing irq vector - time #%x - returning %x!\n",locals.irq_count,vector_num));

	locals.irq_count = (locals.irq_count+1) % 0x10;
	return vector_num;
}

//Generate a level 1 IRQ - IP0,IP1,IP2 = 0
static xforce_irq(int data)
{
  cpu_set_irq_line(0, MC68000_IRQ_1, PULSE_LINE);
}

static INTERRUPT_GEN(vblank) {
  /*-------------------------------
  /  copy local data to interface
  /--------------------------------*/
  locals.vblankCount += 1;

  memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
  memcpy(coreGlobals.segments, locals.segments, sizeof(coreGlobals.segments));
  
  coreGlobals.solenoids = locals.solenoids;
  if ((locals.vblankCount % TECHNO_SOLSMOOTH) == 0) {
	locals.solenoids = coreGlobals.pulsedSolState;
  }
  core_updateSw(1);
}

static SWITCH_UPDATE(xforce) {
  if (inports) {
	  coreGlobals.swMatrix[1] = (inports[CORE_COREINPORT] & 0x00ff);		//Column 0 Switches
	  coreGlobals.swMatrix[2] = (coreGlobals.swMatrix[2] & 0xfc) | (inports[CORE_COREINPORT] & 0x0300)>>8;     //Column 1 Switches
  }
}

static MACHINE_INIT(xforce) {
  memset(&locals, 0, sizeof(locals));

  //setup 68000 IRQ callback to generate IRQ Vector #
  cpu_set_irq_callback(0,xforce_irq_callback);

  //setup IRQ timer
  timer_pulse(XFORCE_IRQ_FREQ,0,xforce_irq);

  //start count on 1 - because 1st time through 0x89 is the vector done.
  locals.irq_count = 1;
}

//Input Key - Return Switches (uses Lamp Column Strobe)
READ16_HANDLER(input_key_r) { 
	UINT8 switches = coreGlobals.swMatrix[locals.LampCol+1];	//+1 so we begin by reading column 1 of input matrix instead of 0 which is used for special switches in many drivers
	return (UINT16)core_revbyte(switches);	//Reverse bits to align with switch matrix from manual
}

//Return Soudn Status?
READ16_HANDLER(input_sound_r) { 
	LOG(("input_sound_r\n"));
	return 0; 
}

//The value here is read, which is tied to a ls74 flip, not sure what it's purpose.
READ16_HANDLER(rtrg_r) { 
	//LOG(("%08x: rtrg_r\n",activecpu_get_pc())); 
	return 0; 
}

//Lamp Rows (actually columns) 1-8
WRITE16_HANDLER(lamp1_w) { locals.LampCol = BitColToNum(data >> 8); }
//Lamp Cols (actually rows) 1-8
WRITE16_HANDLER(lamp2_w) { 	coreGlobals.tmpLampMatrix[locals.LampCol] = data>>8; }

//Solenoids 1-16
WRITE16_HANDLER(sol1_w) { coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xFFFF0000) | data; }
//Solenoids 17-32
WRITE16_HANDLER(sol2_w) { coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0x0000FFFF) | (data<<16); }


//*****************************
//D0-D7   = Sound Data 0-7
//D8      = Strobe
//D9      = Reset
//D10     = Display Data Clock
//D11-D15 = AUX 1-5
//******************************
WRITE16_HANDLER(sound_w) { 
	//LOG(("sound_w = %04x\n",data)); 

	int dclk = (data & 0x400) >> 10;
	//LOG(("%08x: dclk_w = %04x\n",activecpu_get_pc(),dclk)); 

	//Increment Display Column on if not waiting for transition!
	if(locals.DispNoWait) {
		locals.DispCol = (locals.DispCol + 1) % 16;
	}
	if(dclk) {
		locals.DispNoWait = 1;
		locals.DispCol = 0;
	}
	//LOG(("DispCol = %x\n",locals.DispCol));
}

/*********************************************************************************************
    Convert Techno Segment Arrangement to our standard core 16 segment

	Tecmo Layout ( this was guessed by me since not shown in schematics, but seems correct )


     a1   a2
    ---- ----
   |\   |   /|
 f |h\ j| k/ | b
   |  \ | /  |
    ---- ----
   |q / |\ m |
 e | / p| \  | c
   |/r  | n\ |
    ---- ----
     d2   d1

   Core16 Segment Layout

         1
    ---------
   |\ 9 |10 /|
 6 | \  |  / | 2 
   |7 \ | /11|
    ---- ----
   |15/ |\ 12|
 5 | /  | \  | 3 
   |/ 14|13\ |
    ---------  
        4      8 = comma, 16 = period

Conversion to core.c:
Bit0(a1) & Bit1(a2) = Bit 0 (1)
Bit2(b)             = Bit 1 (2)
Bit3(c)             = Bit 2 (3)
Bit4(d1) & Bit5(d2) = Bit 3 (4)
Bit6(e)             = Bit 4 (5)
Bit7(f)             = Bit 5 (6)
Bit8(h)             = Bit 8 (9)
Bit9(j)             = Bit 9 (10)
Bit10(k)            = Bit10 (11)
Bit11(m)            = Bit11 (12)
Bit12(n)            = Bit12 (13)
Bit13(p)            = Bit13 (14)
Bit14(r)            = Bit14 (15)
Bit15(q)            = Bit6  (7)
 xx                 = Bit7  (8)
 xx                 = Bit15 (16)
*/

//Todo: Someone please clean up this code! :)
static int techno_seg_to_coreseg(UINT16 data)
{
	int coreseg = 0;
	//a1 & a2 = bit 0 of coreseg
	if( (data & 0x01) || (data & 0x02) )	coreseg |= 0x01;
	//b = bit 1 of corseg;
	if(data & 0x04) coreseg |= 0x02;
	//c = bit 2 of coreseg;
	if(data & 0x08) coreseg |= 0x04;
	//d1 & d2 = bit 3 of coreseg
	if( (data & 0x10) || (data & 0x20) )	coreseg |= 0x08;
	//e = bit 4 of coreseg;
	if(data & 0x40) coreseg |= 0x10;
	//f = bit 5 of coreseg;
	if(data & 0x80) coreseg |= 0x20;
	//h = bit 8 of coreseg;
	if(data & 0x100) coreseg |= 0x100;
	//h = bit 8 of coreseg;
	if(data & 0x100) coreseg |= 0x100;
	//j = bit 9 of coreseg;
	if(data & 0x200) coreseg |= 0x200;
	//k = bit10 of coreseg;
	if(data & 0x400) coreseg |= 0x400;
	//m = bit11 of coreseg;
	if(data & 0x800) coreseg |= 0x800;
	//n = bit12 of coreseg;
	if(data & 0x1000) coreseg |= 0x1000;
	//p = bit13 of coreseg;
	if(data & 0x2000) coreseg |= 0x2000;
	//r = bit14 of coreseg;
	if(data & 0x4000) coreseg |= 0x4000;
	//q = bit6 of coreseg;
	if(data & 0x8000) coreseg |= 0x40;
	return coreseg;
}

WRITE16_HANDLER(disp1_w) { 
	//LOG(("%08x: disp1_w = %04x\n",activecpu_get_pc(),data)); 
    locals.segments[locals.DispCol].w = techno_seg_to_coreseg(data);
}
WRITE16_HANDLER(disp2_w) { 
	//LOG(("%08x: disp2_w = %04x\n",activecpu_get_pc(),data)); 
	locals.segments[locals.DispCol+16].w = techno_seg_to_coreseg(data);
}

//Like rtrg - setout is connected to the same ls74 flip flop - not sure of it's purpose.
WRITE16_HANDLER(setout_w) { 
	//LOG(("%08x: setout_w = %04x\n",activecpu_get_pc(),data)); 
}

//NVRAM
static UINT16 *NVRAM;
static NVRAM_HANDLER(techno_nvram) {
  core_nvram(file, read_or_write, NVRAM, 0x2000, 0x00);
}

//Memory Map for Main CPU
static MEMORY_READ16_START(readmem)
  { 0x000000, 0x003fff, MRA16_ROM },
  { 0x004000, 0x005fff, MRA16_RAM },
  { 0x014000, 0x014001, input_key_r },
  { 0x014800, 0x014801, input_sound_r },
  { 0x015000, 0x015001, rtrg_r },
  { 0x006000, 0x00ffff, MRA16_ROM },
MEMORY_END

static MEMORY_WRITE16_START(writemem)
  { 0x000000, 0x003fff, MWA16_ROM },
  { 0x004000, 0x005fff, MWA16_RAM, &NVRAM },
  { 0x014000, 0x014001, lamp1_w },
  { 0x014800, 0x014801, lamp2_w },
  { 0x015000, 0x015001, sol1_w },
  { 0x015800, 0x015801, sol2_w },
  { 0x016000, 0x016001, sound_w },
  { 0x016800, 0x016801, disp1_w },
  { 0x017000, 0x017001, disp2_w },
  { 0x017800, 0x017801, setout_w },
MEMORY_END

static core_tLCDLayout disp[] = {
  {0, 0, 0,16,CORE_SEG16},
  {3, 0,16,16,CORE_SEG16},
  {0}
};
static core_tGameData xforceGameData = {GEN_ZAC2, disp};
static void init_xforce(void) {
  core_gameData = &xforceGameData;
}

MACHINE_DRIVER_START(xforce)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(xforce, NULL, NULL)
  MDRV_CPU_ADD_TAG("mcpu", M68000, TECHNO_CPUFREQ)
  MDRV_CPU_MEMORY(readmem, writemem)
  MDRV_CPU_VBLANK_INT(vblank, 1)
  MDRV_SWITCH_UPDATE(xforce)
  MDRV_NVRAM_HANDLER(techno_nvram)
MACHINE_DRIVER_END

INPUT_PORTS_START(xforce) \
  CORE_PORTS \
  SIM_PORTS(4) \
  PORT_START /* 0 */ \

    /* Switch Column 1 */ \
    COREPORT_BIT(     0x0001, "Test Up",		  KEYCODE_7)\
	COREPORT_BIT(     0x0002, "Test Down",		  KEYCODE_8)\
	COREPORT_BIT(     0x0004, "Ball Tilt",        KEYCODE_INSERT)\
	COREPORT_BIT(     0x0008, "Service",          KEYCODE_9)\
	COREPORT_BITDEF(  0x0010, IPT_COIN1,          KEYCODE_3)\
	COREPORT_BITDEF(  0x0020, IPT_COIN2,          KEYCODE_4)\
	COREPORT_BITDEF(  0x0040, IPT_COIN3,          KEYCODE_5)\
	COREPORT_BIT(     0x0080, "Letter Sel ->",    KEYCODE_PGUP)\
	/* Switch Column 2 */ \
	COREPORT_BIT(     0x0100, "Letter Sel <-",    KEYCODE_PGDN)\
	COREPORT_BITDEF(  0x0200, IPT_START1,         IP_KEY_DEFAULT)\
  PORT_START /* 1 */ \
    COREPORT_DIPNAME( 0x0001, 0x0000, "S1") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0001, "1" ) \
    COREPORT_DIPNAME( 0x0002, 0x0000, "S2") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0002, "1" ) \
    COREPORT_DIPNAME( 0x0004, 0x0000, "S3") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0004, "1" ) \
    COREPORT_DIPNAME( 0x0008, 0x0000, "S4") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0008, "1" ) \
    COREPORT_DIPNAME( 0x0010, 0x0000, "S5") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0010, "1" ) \
    COREPORT_DIPNAME( 0x0020, 0x0000, "S6") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0020, "1" ) \
    COREPORT_DIPNAME( 0x0040, 0x0000, "S7") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0040, "1" ) \
    COREPORT_DIPNAME( 0x0080, 0x0000, "S8") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0080, "1" )
INPUT_PORTS_END

ROM_START(xforce) \
  NORMALREGION(0x1000000, REGION_CPU1) \
    ROM_LOAD16_BYTE("ic15", 0x000001, 0x8000, CRC(fb8d2853)) \
    ROM_LOAD16_BYTE("ic17", 0x000000, 0x8000, CRC(122ef649))
ROM_END

CORE_GAMEDEFNV(xforce, "X Force", 1987, "Technoplay", xforce, GAME_NO_SOUND)
