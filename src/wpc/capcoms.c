/*Capcom Sound Hardware
  ------------------------------------
*/
#include "driver.h"
#include "core.h"
#include "cpu/i8051/i8051.h"
#include "capcom.h"
#include "capcoms.h"
#include "sndbrd.h"

#define VERBOSE

#ifdef VERBOSE
#define LOG(x)	logerror x
//#define LOG(x)	printf x
#else
#define LOG(x)
#endif

/*Declarations*/
static void capcoms_init(struct sndbrdData *brdData);
static void cap_bof(int chipnum,int state);
static void cap_sreq(int chipnum,int state);

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
	"TMS320AV120", capcoms_init, NULL, NULL, NULL, NULL, NULL, NULL, NULL, SNDBRD_NODATASYNC
   //"TMS320AV120", capcoms_init, NULL, NULL, alvg_sndCmd_w, alvgs_data_w, NULL, alvgs_ctrl_w, alvgs_ctrl_r, SNDBRD_NODATASYNC
};

/*-- local data --*/
static struct {
  struct sndbrdData brdData;
  void *buffTimer;
  UINT8 ram[0x8000+1];	//External 32K Ram
  int rombase_offset;	//Offset into current rom
  int rombase;			//Points to current rom
  int bof_line[2];		//Track status of bof line for each tms chip
  int sreq_line[2];		//Track status of sreq line for each tms chip
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
} x9241;

void cap_bof(int chipnum,int state)
{
	locals.bof_line[chipnum]=state;
	LOG(("MPG#%d: BOF Set to %d\n",chipnum,state));
}
void cap_sreq(int chipnum,int state)
{
	locals.sreq_line[chipnum]=state;
	LOG(("MPG#%d: SREQ Set to %d\n",chipnum,state));
}


/***********************************************************************************
X9241 - Digital Volume Pot
--------------------------
Data is clocked in via the SCL CLOCK PULSES, and serial sent via the SDA line..

Start Command: 1->0 of SDA WHILE SCL = 1 (all data is ignored until start command)
Stop Command:  0->1 of SDA WHILE SCL = 1
Bit Data:      Data is sent while SCL = 0... SDA = 0 for no bit set, 1 for bit set.
***********************************************************************************/
void data_to_x9241(int scl, int sda)
{
	int i,data;

	//store previous signals
	x9241.lscl = x9241.scl;
	x9241.lsda = x9241.sda;

	//store current signals
	x9241.scl = scl;
	x9241.sda = sda;

	//Sending a command?
	if(scl) {
		//is this a start command? (SCL = 1, SDA = 0, LSDA = 1)
		if(sda == 0 && x9241.lsda == 1) {
			x9241.start = 1;
			x9241.stop = 0;
			x9241.nextbit = 0;
			x9241.nextram = 0;
			for(i=0;i<16;i++)	x9241.ram[i] = 0;
			LOG(("x9241: Start command received!\n"));
			return;
		}
		//is this a stop command? (SCL = 1, SDA = 1, LSDA = 0)
		if(sda == 1 && x9241.lsda == 0) {
			x9241.start = 0;
			x9241.stop = 1;
			x9241.nextbit = 0;
			x9241.nextram = 0;
			LOG(("x9241: Stop command received!\n"));
			return;
		}
	}
	//Sending data?
	else {
		//attempting to send data w/o a start command?
		if(!x9241.start) {
			LOG(("ignoring data to x9241 w/o start command!\n"));
			return;
		}
		//attempting to send data after a stop command?
		if(x9241.stop) {
			LOG(("ignoring data to x9241 because stop command was sent already!\n"));
			return;
		}
		//Ok - store next bit
		x9241.bits[x9241.nextbit++] = sda;
		LOG(("x9241: Received bit #%d, value = %d\n",x9241.nextbit-1,sda));
		//Do we have a full byte?
		if(x9241.nextbit==8) {
			data = 0;
			x9241.nextbit = 0;		//Reset flag
			//Convert to a byte & Clear bits array
			for(i=0; i<8; i++) {
				data |= x9241.bits[i]<<i;
				x9241.bits[i] = 0;
			}
			LOG(("x9241: received byte: %x\n",data));
			//Store in ram (but check for overflow)
			if( (x9241.nextram+1) > 16) {
				LOG(("x9241: Overflow of RAM data - skipping processed byte!\n"));
				return;
			}
			else {
				x9241.ram[x9241.nextram++] = data;
				LOG(("x9241: storing byte to ram[%d]\n",x9241.nextram-1));
			}
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
			P1.0    (O) = LED (Inverted?)
			P1.1    (X) = NC
			P1.2    (I) = /CTS = CLEAR TO SEND   - From Main CPU(Active low)
			P1.3    (I) = /RTS = REQEUST TO SEND - From Main CPU(Active low)
			P1.4    (O) = SCL = U10 - Pin 14 - EPOT CLOCK
			P1.5    (O) = SDA = U10 - Pin  9 - EPOT SERIAL DATA
			P1.6    (O) = /CBOF1 = CLEAR BOF1 IRQ
			P1.7    (O) = /CBOF2 = CLEAR BOF2 IRQ */
		case 1:
			LOG(("%4x:port read @ %x data = %x\n",activecpu_get_pc(),offset,data));
			//Todo: return cts/rts lines..
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
			LOG(("%4x:port read @ %x data = %x\n",activecpu_get_pc(),offset,data));
			//Todo: return RXD line
			data |= (locals.bof_line[0])<<2;
			data |= (locals.bof_line[1])<<3;
			data |= (locals.sreq_line[0])<<4;
			data |= (locals.sreq_line[1])<<5;
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
			P1.0    (O) = LED (Inverted?)
			P1.1    (X) = NC
			P1.2    (I) = /CTS = CLEAR TO SEND   - From Main CPU(Active low)
			P1.3    (I) = /RTS = REQEUST TO SEND - From Main CPU(Active low)
			P1.4    (O) = SCL = U10 - Pin 14 - EPOT CLOCK
			P1.5    (O) = SDA = U10 - Pin  9 - EPOT SERIAL DATA
			P1.6    (O) = /CBOF1 = CLEAR BOF1 IRQ
			P1.7    (O) = /CBOF2 = CLEAR BOF2 IRQ */
		case 1:
			//LED
			cap_UpdateSoundLEDS(~data&0x01);
			//CBOF1
			if((data&0x40)==0)	{
				cpu_set_irq_line(locals.brdData.cpuNo, I8051_INT0_LINE, CLEAR_LINE);
				LOG(("Clearing /BOF1 - INT 0 \n"));
			}
			//CBOF2
			if((data&0x80)==0){
				cpu_set_irq_line(locals.brdData.cpuNo, I8051_INT1_LINE, CLEAR_LINE);
				LOG(("Clearing /BOF2 - INT 1 \n"));
			}

			//Update x9241 data lines
			data_to_x9241((data&0x10)>>4,(data&0x20)>>5);

			LOG(("writing to port %x data = %x\n",offset,data));
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
			LOG(("writing to port %x data = %x\n",offset,data));
			break;
		default:
			LOG(("writing to port %x data = %x\n",offset,data));
	}
}

READ_HANDLER(unk_r)
{
	LOG(("unk_r read @ %x\n",offset));
	return 0;
}

//Return a byte from the bankswitched roms
READ_HANDLER(rom_rd)
{
	int data;
	UINT8* base = (UINT8*)memory_region(REGION_SOUND1);	//Get pointer to 1st ROM..
	offset&=0xffff;	//strip off top bit
	//Is a valid rom set for reading?
	if(locals.rombase < 0) {
		//LOG(("error from rom_rd - no ROM selected!\n"));
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
	TMS320AV120_data_w(offset,data);
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
//	LOG(("BANK SWITCH DATA=%x\n",data));
	data &= 0x1f;	//Keep only bits 0-4
	locals.rombase_offset = 0x8000*data;
}


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
//	LOG(("ROM ACCESS - DATA=%x, ROMBASE = %x\n",(~data&0x0f),locals.rombase));
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
	calc_rombase(data);							//Determine which ROM is active and set rombase
	TMS320AV120_set_reset(0,(data&0x10)>>4);	//Reset TMS320AV120 Chip #1 (1 = Reset)
	TMS320AV120_set_reset(1,(data&0x20)>>5);	//Reset TMS320AV120 Chip #2 (1 = Reset)
	TMS320AV120_set_mute(0,((~data&0x40)>>6));	//Mute TMS320AV120 Chip #1 (Active low)
	TMS320AV120_set_mute(1,((~data&0x80)>>7));	//Mute TMS320AV120 Chip #2 (Active low)
//	LOG(("CONTROL_DATA - DATA=%x\n",data));
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

extern void tms_FillBuff(int);

void cap_FillBuff(int dummy) {
	tms_FillBuff(0);
	tms_FillBuff(1);
}

static void capcoms_init(struct sndbrdData *brdData) {
  memset(&locals, 0, sizeof(locals));
  locals.brdData = *brdData;

  memset(&x9241, 0, sizeof(x9241));

  /* stupid timer/machine init handling in MAME */
  if (locals.buffTimer) timer_remove(locals.buffTimer);

  /*-- Create timer to fill our buffer --*/
  locals.buffTimer = timer_alloc(cap_FillBuff);

  /*-- start the timer --*/
  timer_adjust(locals.buffTimer, 0, 0, TIME_IN_HZ(10));		//Frequency is somewhat arbitrary but must be fast enough to work
}
