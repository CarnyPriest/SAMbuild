/*Capcom Sound Hardware

  SOUND BOARD:
	CPU: 87c52 @ 12 Mhz
	I/O: 87c52 has a uart for communication back to main cpu
	SND: 2 x TMS320AV120 MPG DECODER ( Only 1 on Breakshot )
	VOL: x9241 Digital Volume Pot

  Hacks & Issues that need to be looked into:
  #1) /BOF line timing not properly emulated (fails start up test)
  #2) 9241 Digital volume pot not finished (but passes tests!)
  #3) Currently rom is hacked to bypass error messages for the above failed tests..
  #4) Sound board often get's overloaded and resets or just drops out/misses sound commands
  #5) Only KP game fails to report a proper sound board reset (others do, but if you watch received commands, not all commands are always sent)
  #6) Sometimes received data doesn't always get sent (see sound reset of KP for good example)
  ------------------------------------
*/
#include "driver.h"
#include "core.h"
#include "cpu/i8051/i8051.h"
#include "capcom.h"
#include "capcoms.h"
#include "sndbrd.h"

#define VERBOSE
//#define DEBUGGING

#ifdef VERBOSE
//#define LOG(x)	logerror x
#define LOG(x)	printf x
#else
#define LOG(x)
#endif

#ifdef DEBUGGING
#define PRINTF(x) printf x
#else
#define PRINTF(x) 
#endif

//Remove this flag to test the real game sending data (otherwise, we simply feed data from the roms till we hit the end)
//#define TEST_FROM_ROM

//Comment out to remove Test bypass..
#define TEST_BYPASS

//Comment out to allow Volume Control
#define DISABLE_VOLUME

/*Declarations*/
WRITE_HANDLER(capcoms_sndCmd_w);
static void capcoms_init(struct sndbrdData *brdData);
static void capcoms_reset(void);
static void cap_bof(int chipnum,int state);
static void cap_sreq(int chipnum,int state);
#ifdef TEST_FROM_ROM
  void cap_FillBuff(int dummy);
#endif

/*Interfaces*/
static struct TMS320AV120interface capcoms_TMS320AV120Int1 = {
  1,		//# of chips
  {100},	//Volume levels
  cap_bof,	//BOF Line Callback
  cap_sreq	//SREQ Line Callback
};
static struct TMS320AV120interface capcoms_TMS320AV120Int2 = {
  2,		//# of chips
  {50,50},	//Volume levels
  cap_bof,	//BOF Line Callback
  cap_sreq	//SREQ Line Callback
};

/* Sound board */
const struct sndbrdIntf capcomsIntf = {
	"TMS320AV120", capcoms_init, NULL, NULL, capcoms_sndCmd_w, NULL, NULL, NULL, NULL, SNDBRD_NODATASYNC
   //"TMS320AV120", capcoms_init, NULL, NULL, alvg_sndCmd_w, alvgs_data_w, NULL, alvgs_ctrl_w, alvgs_ctrl_r, SNDBRD_NODATASYNC
};

/*-- local data (cannot be reset) --*/
static struct {
  struct sndbrdData brdData;
} locals_cap;

/* -- local data (to be reset -- */
static struct {
  UINT8 ram[0x8000];	//External 32K Ram
  int rombase_offset;	//Offset into current rom
  int rombase;			//Points to current rom
  int bof_line[2];		//Track status of bof line for each tms chip
  int sreq_line[2];		//Track status of sreq line for each tms chip
  int cbof_line[2];		//Clear BOF Line (8752 controlled)
  int cts;				//Clear to send (If 1, cpu will not send data)
  int rts;				//Request to send (If 1, sender will not send data)
  int scl;				//SCL line
  int sda;				//SDA line
  UINT8 from_8752;		//Data from the 8752
  UINT8 to_8752;		//Data to the 8752
#ifdef TEST_FROM_ROM
  void *buffTimer;
  int curr[2];			//Current position we've read from the rom
#endif
} locals;

//Digital Volume Pot (x9241)
static struct {
  int scl;			//Clock Signal
  int sda;			//Data Signal
  int lscl;			//Previous SCL state
  int lsda;			//Previous SDA state
  int bits[8];		//Store 8 received bits
  UINT8 ram[16];	//16 byte internal ram
  int start;		//Set when a start command is received
  int stop;			//Set when a stop command is received
  int nextbit;		//Which bit to store next
  int nextram;		//Which position in ram to store next
  int waitforack;	//Waiting to get ack cycle
  int waitackend;	//Waiting to get ack end cycle
  UINT8 wcr[4];		//The 4 wiper control registers
} x9241;

void set_cts_line_to_8752(int data)
{
	locals.cts = data;
	//PRINTF(("setting cts = %x\n",data));
}

int get_rts_line_from_8752(void)
{
	return locals.rts;
}

void send_data_to_8752(int data)
{
	locals.to_8752 = data;
	PRINTF(("Data TO 8752 Write = %x\n",data));

	//Force the RX Line Callback
	cpu_set_irq_line(locals_cap.brdData.cpuNo, I8051_RX_LINE, ASSERT_LINE);
}

extern void send_data_to_68306(int data);

//Data From 8752 (Transmitted from 8752 Serial UART TX line)
void data_from_8752(int data)
{
	locals.from_8752 = data;
	send_data_to_68306(data);
}

//Data To 8752 (Transmitted To 8752 Serial UART RX line)
int data_to_8752(void)
{
	int data = locals.to_8752;
	PRINTF(("reading 8752 data %x\n",data));
	return data;
}

//Track state of BOF 
void cap_bof(int chipnum,int state)
{
	static int last[2] = {0,0};
	
	//Store the state..
	locals.bof_line[chipnum]=state;
	
	//If line is lo - trigger the interrupt, otherwise clear it

	//Ignore every 5th transition of low->hi (NO IDEA WHY THIS WORKS WELL FOR MUSIC, BUT NOT QUITE RIGHT FOR SOUNDS
	if(!state)
		last[chipnum]++;
	if(last[chipnum] == 5) {
		last[chipnum] = 0;
		return;
	}
	if(chipnum)
		cpu_set_irq_line(locals_cap.brdData.cpuNo, I8051_INT1_LINE, state?CLEAR_LINE:ASSERT_LINE);
	else
		cpu_set_irq_line(locals_cap.brdData.cpuNo, I8051_INT0_LINE, state?CLEAR_LINE:ASSERT_LINE);
//	LOG(("MPG#%d: BOF Set to %d\n",chipnum,state));
	//PRINTF(("MPG#%d: BOF Set to %d\n",chipnum,state));
}

//Track state of SREQ
void cap_sreq(int chipnum,int state)
{
	locals.sreq_line[chipnum]=state;
//	LOG(("MPG#%d: SREQ Set to %d\n",chipnum,state));
//	PRINTF(("MPG#%d: SREQ Set to %d\n",chipnum,state));
}


/***********************************************************************************
X9241 - Digital Volume Pot
--------------------------
Data is clocked in via the SCL CLOCK PULSES, and serial sent via the SDA line..

Start Command: 1->0 of SDA WHILE SCL = 1 (all data is ignored until start command)
Stop Command:  0->1 of SDA WHILE SCL = 1
Bit Data:      Data is sent while SCL = 0... SDA = 0 for no bit set, 1 for bit set.

Todo:	#2) Implement commands & registers
		#3) Adjust volume accordingly

***********************************************************************************/
void data_to_x9241(int scl, int sda)
{
	int i,data,changed=0;

	//store previous signals
	x9241.lscl = x9241.scl;
	x9241.lsda = x9241.sda;

	//store current signals
	x9241.scl = scl;
	x9241.sda = sda;

	//update to external structure for reading
	locals.scl = scl;
	locals.sda = sda;

	//Check for a change in state..
	if(scl == x9241.lscl && sda == x9241.lsda) {
		//If we've started, and scl = 0, then sda in fact has changed, even if values did not!
		if(x9241.start && !x9241.scl)
			x9241.lsda = !x9241.lsda;	//Force a change!
		else
			return;
	}

	//Which bit changed? (0 = SCLK Changed, 1 = SDA Changed)
	if(sda != x9241.lsda)
		changed = 1;

	//If waiting for ack
	if(x9241.waitforack) {
		//Did SCLK change?
		if(!changed) {
			//Begin ACK? (0->1 transition)?
			if(!x9241.lscl && scl) { 
				//Yes - Pull SDA low!
				locals.sda = 0;
				x9241.waitackend = 1;
				return;
			}
			//End ACK? (1->0 transition)?
			if(x9241.waitackend) {
				if(x9241.lscl && !scl) {
					x9241.waitforack = 0;
					x9241.waitackend = 0;
					return;
				}
			}
		}
		//ABORT
		return;
	}

	//Found a start command already?
	if(x9241.start) {
		//Check for new bit (Transition of SCL 0->1)
		if(!x9241.lscl && scl && !changed) {
			//Ok - store next bit
			x9241.bits[x9241.nextbit++] = sda;
			//LOG(("x9241: Received bit #%d, value = %d\n",x9241.nextbit-1,sda));
			//Do we have a full byte?
			if(x9241.nextbit==8) {
				data = 0;
				x9241.nextbit = 0;		//Reset flag
				x9241.waitforack = 1;	//Set Flag
				//Convert to a byte & Clear bits array
				for(i=0; i<8; i++) {
					data |= x9241.bits[i]<<i;
					x9241.bits[i] = 0;
				}
				//Serial data must be loaded MSB first, so we must reverse it..
				data = core_revbyte(data);
				//LOG(("x9241: received byte: %x\n",data));
				//Store in ram (but check for overflow)
				if( (x9241.nextram+1) > 16) {
					LOG(("x9241: Overflow of RAM data - skipping processed byte!\n"));
					return;
				}
				else {
					x9241.ram[x9241.nextram++] = data;
				//	LOG(("x9241: storing byte to ram[%d]\n",x9241.nextram-1));
				}
			}
		}
	}
	//Sending a start or stop command? (Check only when SDA changes, and SCL = 1)
	if(changed && x9241.scl) {
		//is this a start command? (SCL = 1, LSDA = 1, SDA = 0)
		if(x9241.lsda == 1 && sda == 0) {
			x9241.start = 1;
			x9241.stop = 0;
			x9241.nextbit = 0;
			x9241.nextram = 0;
			for(i=0;i<16;i++)	x9241.ram[i] = 0;
			//LOG(("x9241: Start command received!\n"));
			return;
		}
		//is this a stop command? (SCL = 1, LSDA = 0, SDA = 1)
		if(x9241.lsda == 0 && sda == 1) {
			x9241.start = 0;
			x9241.stop = 1;
			x9241.nextbit = 0;
			//LOG(("x9241: Stop command received!\n"));
			//Process the commands (how many bytes were in this command sequence?)
			if(x9241.nextram != 3)
				LOG(("9241 Volume emulation only supports 3 byte command sequences!\n"));
			else {
				//Command sequence must begin with 0x50
				if(x9241.ram[0] != 0x50)
					LOG(("9241 Volume - Illegal first command byte!\n"));
				else {
					//2nd Byte is the Register (bits 2-3), 3rd Byte the Value
					x9241.wcr[(x9241.ram[1] & 0x0c)>>2] = x9241.ram[2];
#ifndef DISABLE_VOLUME
					LOG(("wcr[%x]=%x\n",(x9241.ram[1] & 0x0c)>>2,x9241.ram[2]));
					//Now adjust volume - wipers 0 & 1 control mpg1 & mpg2 ( since 16 is max value of wcr, 16*6 = 96 Volume setting)
					mixer_set_volume(0,x9241.wcr[0]*6);
					mixer_set_volume(1,x9241.wcr[1]*6);
					//Not sure what wipers 2 & 3 do..
#endif
				}
			}
			x9241.nextram = 0;
			return;
		}
	}
}

static READ_HANDLER(port_r)
{
	int data = 0;
	switch(offset) {
		case 0:
		case 2:
			break;
		/*PORT 1:
			P1.0    (O) = LED
			P1.1    (X) = NC
			P1.2    (I) = /CTS = CLEAR TO SEND   (Will NOT send data if line = 1)
			P1.3    (O) = /RTS = REQEUST TO SEND (If receiving too much data, set's line = 1)
			P1.4    (I) = SCL = U10 - Pin 14 - EPOT CLOCK			(Not a mistake, port used for both I/O)
			P1.5    (I) = SDA = U10 - Pin  9 - EPOT SERIAL DATA		(Not a mistake, port used for both I/O)
			P1.6    (O) = /CBOF1 = CLEAR BOF1 IRQ
			P1.7    (O) = /CBOF2 = CLEAR BOF2 IRQ */
		case 1:
			data |= (locals.cts)<<2;
			data |= (locals.scl)<<4;
			data |= (locals.sda)<<5;
			//LOG(("%4x:port read @ %x data = %x\n",activecpu_get_pc(),offset,data));
			return data;
		/*PORT 3:
			P3.0/RXD(I) = Serial Receive -  RXD
			P3.1/TXD(O) = Serial Transmit - TXD
			INT0    (I) = /BOF1  = BEG OF FRAME FOR MPG1
			INT1    (I) = /BOF2  = BEG OF FRAME FOR MPG2
			T0/P3.4 (I) = /SREQ1 = SOUND REQUEST MPG1
			T1/P3.5 (I) = /SREQ2 = SOUND REQUEST MPG2
			P3.6    (O) = /WR
			P3.7    (O) = /RD*/
		case 3:
			//Todo: return RXD line
			data |= (locals.bof_line[0])<<2;
			data |= (locals.bof_line[1])<<3;
			data |= (locals.sreq_line[0])<<4;
			data |= (locals.sreq_line[1])<<5;
			//LOG(("%4x:port read @ %x data = %x\n",activecpu_get_pc(),offset,data));
			return data;
	}
	LOG(("%4x:port read @ %x data = %x\n",activecpu_get_pc(),offset,data));
	return data;
}

static WRITE_HANDLER(port_w)
{
	switch(offset) {
		//Used for external addressing...
		case 0:
		case 2:
			break;
		/*PORT 1:
			P1.0    (O) = LED
			P1.1    (X) = NC
			P1.2    (I) = /CTS = CLEAR TO SEND   (Will NOT send data if line = 1)
			P1.3    (O) = /RTS = REQEUST TO SEND (If receiving too much data, set's line = 1)
			P1.4    (O) = SCL = U10 - Pin 14 - EPOT CLOCK
			P1.5    (O) = SDA = U10 - Pin  9 - EPOT SERIAL DATA
			P1.6    (O) = /CBOF1 = CLEAR BOF1 IRQ
			P1.7    (O) = /CBOF2 = CLEAR BOF2 IRQ */
		case 1:
			//LED
			cap_UpdateSoundLEDS(data&0x01);

			//Update x9241 data lines (SCL & SDA)
			data_to_x9241((data&0x10)>>4,(data&0x20)>>5);

			//Set RTS Line
			locals.rts = (data & 0x08)>>3;
#if 0
			if(locals.rts)
				PRINTF(("rts = %x\n",locals.rts));
#endif

			//CBOF1 & CBOF2 (If cpu writes a 0 here, we set BOF HI, but if we write a 1, BOF line is unchanged!)
			if((data&0x40)==0)	cap_bof(0,1);
			if((data&0x80)==0)	cap_bof(1,1);

			//LOG(("writing to port %x data = %x\n",offset,data));
			break;
		/*PORT 3:
			P3.0/RXD(I) = Serial Receive -  RXD
			P3.1/TXD(O) = Serial Transmit - TXD
			INT0    (I) = /BOF1  = BEG OF FRAME FOR MPG1
			INT1    (I) = /BOF2  = BEG OF FRAME FOR MPG2
			T0/P3.4 (I) = /SREQ1 = SOUND REQUEST MPG1
			T1/P3.5 (I) = /SREQ2 = SOUND REQUEST MPG2
			P3.6    (O) = /WR
			P3.7    (O) = /RD*/
		case 3:
			//Todo: Implement Serial Transmit 
			LOG(("writing to port %x data = %x\n",offset,data));
			break;
		default:
			LOG(("writing to port %x data = %x\n",offset,data));
	}
}

//Return a byte from the bankswitched roms
READ_HANDLER(rom_rd)
{
	int data;
	UINT8* base = (UINT8*)memory_region(REGION_SOUND1);	//Get pointer to 1st ROM..
	offset&=0xffff;	//strip off top bit
	//Is a valid rom set for reading?
	if(locals.rombase < 0) {
		return 0;
	}
	else {
		base+=(locals.rombase+locals.rombase_offset+offset);//Do bankswitching
		data = (int)*base;
		//LOG(("%4x: rom_r[%4x] = %2x (rombase=%8x,base_offset=%8x)\n",activecpu_get_pc(),offset,data,locals.rombase,locals.rombase_offset));
		return data;
	}
}

READ_HANDLER(ram_r)
{
	//LOG(("ram_r %x data = %x\n",offset,locals.ram[offset]));

	//Shift mirrored ram from it's current address range of 0x18000-0x1ffff to actual address range of 0-0x7fff
	if(offset > 0xffff)
		return locals.ram[offset-0x18000];
	//Return normal address range for offsets < 0xffff
	return locals.ram[offset];
}

WRITE_HANDLER(ram_w)
{
	offset&=0xffff;	//strip off top bit
	locals.ram[offset] = data;

	//LOG(("ram_w %x data = %x\n",offset,data));
	
	/* 8752 CAN EXECUTE CODE FROM RAM - SO WE MUST COPY TO CPU REGION STARTING @ 0x8000 */
	*((UINT8 *)(memory_region(CAPCOMS_CPUREGION) + offset +0x8000)) = data;
}

//Set the /MPEG1 or /MPEG2 lines (active low) - clocks data into each of the tms320av120 chips
WRITE_HANDLER(mpeg_clock)
{
#ifndef TEST_FROM_ROM
	TMS320AV120_data_w(offset,data);
#endif
}

/*
U35 (BANK SWITCH)
D0 = A15 (+0x8000*1)
D1 = A16 (+0x8000*2)
D2 = A17 (+0x8000*3)
D3 = A18 (+0x8000*4)
D4 = A19 (+0x8000*5)*/
WRITE_HANDLER(bankswitch)
{
	data &= 0x1f;	//Keep only bits 0-4
	locals.rombase_offset = 0x8000*data;
}

//Calculates which rom to read from next based on inverted 4 bits of the data written
void calc_rombase(int data)
{
	int activerom = (~data&0x0f);
	int chipnum = 0;
	if(activerom) {
		//got to be an easier way than this hack?
		if(activerom==8)	chipnum = 3;
		else				chipnum = activerom>>1;
		locals.rombase = 0x100000*(chipnum);
	}
	else
		locals.rombase = -1;
}

/*
U36 (CONTROL)
D0 = /ROM0 Access (+0x100000*0) - Line is active lo
D1 = /ROM1 Access (+0x100000*1) - Line is active lo
D2 = /ROM2 Access (+0x100000*2) - Line is active lo
D3 = /ROM3 Access (+0x100000*3) - Line is active lo
D4 =  MPG1 Reset  (1 = Reset)
D5 =  MPG2 Reset  (1 = Reset)
D6 = /MPG1 Mute   (0 = MUTE)
D7 = /MPG2 Mute   (0 = MUTE)*/
WRITE_HANDLER(control_data)
{
	calc_rombase(data&0x0f);					//Determine which ROM is active and set rombase
#ifndef TEST_FROM_ROM
	TMS320AV120_set_reset(0,(data&0x10)>>4);	//Reset TMS320AV120 Chip #1 (1 = Reset)
	TMS320AV120_set_reset(1,(data&0x20)>>5);	//Reset TMS320AV120 Chip #2 (1 = Reset)
	TMS320AV120_set_mute(0,((~data&0x40)>>6));	//Mute TMS320AV120 Chip #1 (Active low)
	TMS320AV120_set_mute(1,((~data&0x80)>>7));	//Mute TMS320AV120 Chip #2 (Active low)
#endif
}

/*Control Lines
a00 = U35 (BANK SWITCH)		(0x0001)
a01 = U36 (CONTROL)			(0x0002)
a02 = U33 (/MPEG1 CLOCK)	(0x0004)
a03 = U33 (/MPEG2 CLOCK)	(0x0008)   */
WRITE_HANDLER(control_w)
{
	offset&=0xffff;	//strip off top bit

	switch(offset) {
		case 0:
			LOG(("invalid control line - %x!\n",offset));
			break;
		//U35 - BANK SWITCH
		case 1:
			bankswitch(0,data);
			break;
		//U36 - CONTROL
		case 2:
			control_data(0,data);
			break;
		case 3:
			LOG(("invalid control line - %x!\n",offset));
			break;
		//U33 - /MPEG1 CLOCK
		case 4:
			mpeg_clock(0,data);
			break;
		case 5:
		case 6:
		case 7:
			LOG(("invalid control line - %x!\n",offset));
			break;
		//U33 - /MPEG2 CLOCK
		case 8:
			mpeg_clock(1,data);
			break;
		default:
			LOG(("control_w %x data = %x\n",offset,data));
	}
}

//This should never be called, but here just in case there's a bug in the 8051 cpu core or a wrong assumption by me!
READ_HANDLER(unk_r)
{
	LOG(("unk_r read @ %x\n",offset));
	return 0;
}



//The MC51 cpu's can all access up to 64K ROM & 64K RAM in the SAME ADDRESS SPACE
//It uses separate commands to distinguish which area it's reading/writing!
//So to handle this, the cpu core automatically adjusts all external memory access to the follwing setup..
//00000 -  FFFF is used for MOVC(/PSEN=0) commands
//10000 - 1FFFF is used for MOVX(/RD=0 or /WR=0) commands
static MEMORY_READ_START(capcoms_readmem)
{ 0x000000, 0x001fff, MRA_ROM },	//Internal ROM 
{ 0x002000, 0x007fff, unk_r },		//This should never be accessed
{ 0x008000, 0x00ffff, ram_r },		//MOVC can access external ram here!
{ 0x010000, 0x017fff, rom_rd},		//MOVX can access roms here!
{ 0x018000, 0x01ffff, ram_r },		//MOVX can access external ram (mirrored) here!
MEMORY_END
static MEMORY_WRITE_START(capcoms_writemem)
{ 0x000000, 0x001fff, MWA_ROM },	//Internal ROM
{ 0x002000, 0x00ffff, MWA_NOP },	//This cannot be accessed by the 8051 core.. (there's no MOVC command for writing!)
{ 0x010000, 0x017fff, control_w },	//MOVX can write to Control chips here!
{ 0x018000, 0x01ffff, ram_w },		//MOVX can write to external RAM here!
MEMORY_END

static PORT_READ_START( capcoms_readport )
	{ 0x00,0xff, port_r },
PORT_END
static PORT_WRITE_START( capcoms_writeport )
	{ 0x00,0xff, port_w },
PORT_END


//Driver template with 8752 cpu
MACHINE_DRIVER_START(capcoms)
  MDRV_CPU_ADD(I8752, 12000000)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(capcoms_readmem, capcoms_writemem)
  MDRV_CPU_PORTS(capcoms_readport, capcoms_writeport)
  //MDRV_INTERLEAVE(1500)
  MDRV_INTERLEAVE(50)
MACHINE_DRIVER_END

//Driver with 1 x TMS320av120
MACHINE_DRIVER_START(capcom1s)
  MDRV_IMPORT_FROM(capcoms)
  MDRV_SOUND_ADD_TAG("tms320av120", TMS320AV120, capcoms_TMS320AV120Int1)
MACHINE_DRIVER_END

//Driver with 2 x TMS320av120
MACHINE_DRIVER_START(capcom2s)
  MDRV_IMPORT_FROM(capcoms)
  MDRV_SOUND_ADD_TAG("tms320av120", TMS320AV120, capcoms_TMS320AV120Int2)
MACHINE_DRIVER_END

//Manual reset of cpu, triggered from external source
void capcoms_manual_reset(void)
{
  capcoms_reset();
  TMS320AV120_sh_reset();
  cpu_set_reset_line(locals_cap.brdData.cpuNo, PULSE_LINE);
}

//Initialize anything that should be cleared when cpu is reset
static void capcoms_reset()
{
  memset(&locals, 0, sizeof(locals));
  memset(&x9241, 0, sizeof(x9241));

  //Setup 8752 serial line call backs
  i8752_set_serial_tx_callback(data_from_8752);
  i8752_set_serial_rx_callback(data_to_8752);

  //patch over tests that are failing
  #ifdef TEST_BYPASS
  //U22 Test (MPG 1)
  *((UINT8 *)(memory_region(CAPCOMS_CPUREGION) + 0x7bb))   = 0x70;		//convert jz to jnz
  //U23 Test (MPG 2)
  *((UINT8 *)(memory_region(CAPCOMS_CPUREGION) + 0x7ca))   = 0x70;		//convert jz to jnz
  #endif

//Set up timer to force feed data to the TMS chips from the ROMS
#ifdef TEST_FROM_ROM
  locals.curr[0] = 0;
  locals.curr[1] = 0;

  /* stupid timer/machine init handling in MAME */
  if (locals.buffTimer) timer_remove(locals.buffTimer);

  /*-- Create timer to fill our buffer --*/
  locals.buffTimer = timer_alloc(cap_FillBuff);

  /*-- start the timer --*/
  timer_adjust(locals.buffTimer, 0, 0, TIME_IN_HZ(30));		// Send 30 Frames per second ( 32Khz sample rate ~ 27.8 Frames per second)
#endif
}

//Initialize anything that cannot be reset here..
static void capcoms_init(struct sndbrdData *brdData) {
  memset(&locals_cap, 0, sizeof(locals_cap));
  locals_cap.brdData = *brdData;
  //call reset routine for clearing all data related to a reset
  capcoms_reset();
}

WRITE_HANDLER(capcoms_sndCmd_w)
{
	send_data_to_8752(data);
}

//Code to feed data from the ROMS directly to the TMS chips for testing

#ifdef TEST_FROM_ROM

//Get One Byte - Reads a single byte from the rom stream, returns 0 if unable to read the byte
static int get_one_byte(int num, UINT8* byte, UINT32 pos)
{
	UINT8* base = (UINT8*)memory_region(REGION_SOUND1);
	//If reached end of memory region, abort
	if(pos>0x300000) return 0;
	*byte = base[pos];
	return 1;
}

//Find next MPG Header in rom stream, return 0 if not found - current position will be at start of frame if found
static int Find_MPG_Header(int num)
{	
	UINT8 onebyte;
	int quit = 0;
	while(!quit) {
		//Find start of possible syncword (begins with 0xff)
		do {
			//Grab 1 byte
			if(!get_one_byte(num,&onebyte,locals.curr[num])) 
				quit=1;
			else
				locals.curr[num]++;
		}while(onebyte!=0xff && !quit);

		//Found 0xff?
		if(onebyte==0xff) {
			//Next byte must be 0xfd (note: already positioned on it from above while() code)
			if(get_one_byte(num, &onebyte,locals.curr[num]) && onebyte==0xfd) {
				//Next byte must be 0x18 or 0x19
				if(get_one_byte(num,&onebyte,locals.curr[num]+1) && (onebyte==0x18 || onebyte==0x19)) {
					//Next byte must be 0xc0
					if(get_one_byte(num,&onebyte,locals.curr[num]+2) && onebyte==0xc0) {
						locals.curr[num]+=3;
						return 1;
					}
				}
			}
		}
	}
	return 0;
}

//Find next header/frame, and send enough data to fill 1 frame
void tms_FillBuff(int num) {
	int i;
	UINT8 onebyte;
	//Abort if SREQ line is HI
	if(locals.sreq_line[num])
		return;
	//Find Next Valid Header - Abort if not found
	if(!Find_MPG_Header(num))
		return;
	//Backup and get header
	locals.curr[num]-=4;
	//Read full header+frame and send to tms chip
	for(i=0; i<144; i++) {
		if(!get_one_byte(num,&onebyte,locals.curr[num]++))
			break;
		TMS320AV120_data_w(num,onebyte);
	}
}

void cap_FillBuff(int dummy) {
	tms_FillBuff(0);
	tms_FillBuff(1);
}

#endif
