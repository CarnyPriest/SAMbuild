#include "driver.h"
#include "memory.h"
#include "cpu/m6502/m6502.h"
#include "machine/6532riot.h"
#include "core.h"

#include "gts80s.h"

static int GTS80S_CPUNo;

#define RIOT_TIMERIRQ			0x80
#define RIOT_PA7IRQ				0x40

/* switches on the board (to PB4/7) */
#define ATTRACT_MODE_TUNE		0x10
#define SOUND_TONES				0x80

#define ATTRACT_ENABLED	1
#define SOUND_ENABLED	1


#define BUFFER_SIZE 4096

typedef struct tagGTS80SL {
  UINT8 timer_start;
  UINT16 timer_divider;
  UINT8 timer_irq_enabled;
  
  UINT8 irq_state;
  UINT8 irq;

  double cycles_to_sec;
  double sec_to_cycles;
  
  void *t;
  double time;

  UINT8 edge_detect_ctrl;

  int soundLatch;
  
  int   stream;
  UINT16 buffer[BUFFER_SIZE+1];
  double clock[BUFFER_SIZE+1];
  
  int   buf_pos;
} GTS80SL;

#define V_CYCLES_TO_TIME(c) ((double)(c) * p->cycles_to_sec)
#define V_TIME_TO_CYCLES(t) ((int)((t) * p->sec_to_cycles))

#define IFR_DELAY 3

static GTS80SL GTS80S_locals;

void GTS80S_sound_latch(int data)
{
	// logerror("sound latch: 0x%02x\n", GTS80S_locals.soundLatch);
	GTS80S_locals.soundLatch = (data&0x0f);
}

static void update_6530_interrupts(void)
{
	GTS80SL *p = &GTS80S_locals;
	int new_state;

	new_state = 0;
	if ( (p->irq_state & RIOT_TIMERIRQ) && p->timer_irq_enabled ) 
		new_state = 1;

	if ( (p->irq_state & RIOT_PA7IRQ) && (p->edge_detect_ctrl & 0x02)) 
		new_state = 1;

	if (new_state != p->irq)
	{
		p->irq = new_state;
	}
}

static void riot_6530_timeout(int which)
{
	GTS80SL *p = &GTS80S_locals;

	if ( p->irq_state & RIOT_TIMERIRQ ) {
		p->t = 0;
	}
	else {
		timer_reset(p->t, V_CYCLES_TO_TIME(255));
		p->time = timer_get_time();

		p->irq_state |= RIOT_TIMERIRQ;
		update_6530_interrupts();
	}
//	logerror("timer timeout\n");
}

static READ_HANDLER(riot6530_r)
{
	GTS80SL *p = &GTS80S_locals;
	int val = 0;

	offset &= 0x0f;
	if ( !(offset&0x04) ) {
		// I/O
		switch ( offset&0x03 ) {
		case 0x00:
			// logerror("read PA\n");
			break;

		case 0x01:
			// logerror("read DDRA\n");
			break;

		case 0x02:
			val = GTS80S_locals.soundLatch | ATTRACT_ENABLED*ATTRACT_MODE_TUNE | SOUND_ENABLED*SOUND_TONES;
			break;

		case 0x03:
			// logerror("read DDRB\n");
			break;
		}
	}
	else {
		if ( !(offset&0x01) ) {
			if ( p->t ) {
				if ( p->irq_state & RIOT_TIMERIRQ ) {
					val = 255 - V_TIME_TO_CYCLES(timer_get_time() - p->time);
					timer_remove(p->t);
					p->t = 0;
				}
				else
					val = p->timer_start - V_TIME_TO_CYCLES(timer_get_time() - p->time) / p->timer_divider;
			}
			else {
				val = p->timer_start - V_TIME_TO_CYCLES(timer_get_time() - p->time) / p->timer_divider;
				if ( val<0 )
					val = 0x01;
			}

			p->irq_state &= ~RIOT_TIMERIRQ;
			p->timer_irq_enabled = offset&0x08;
			update_6530_interrupts();

			// logerror("read timer: %i\n", val);
		}
		else
			logerror("read IRQ\n");
	}

	return val;
}

static WRITE_HANDLER(riot6530_w)
{
	GTS80SL *p = &GTS80S_locals;

	offset &= 0x0f;
	if ( !(offset&0x04) ) {
		// I/O
		switch ( offset&0x03 ) {
		case 0x00:
			// logerror("write PA (0x%02x)\n", data);
			if ( GTS80S_locals.buf_pos>=BUFFER_SIZE )
				return;

			GTS80S_locals.clock[GTS80S_locals.buf_pos] = timer_get_time();
			GTS80S_locals.buffer[GTS80S_locals.buf_pos++] = (0x80-data)*0x100;
			break;

		case 0x01:
			// logerror("write DDRA (0x%02x)\n", data);
			break;

		case 0x02:
			// logerror("write PB (0x%02x)\n", data);
			break;

		case 0x03:
			// logerror("write DDRB(0x%02x)\n", data);
			break;
		}
	}
	else {
		// logerror("write timer: %i\n", data);

		p->timer_irq_enabled = offset&0x08;
		p->timer_start = data;

		switch ( offset & 0x03 ) {
		case 0:
			p->timer_divider = 1;
			break;

		case 1:
			p->timer_divider = 8;
			break;

		case 2:
			p->timer_divider = 64;
			break;

		case 3:
			p->timer_divider = 1024;
			break;
		}
		p->irq_state &= ~RIOT_TIMERIRQ;
		update_6530_interrupts();

//		if ( p->timer_irq_enabled )
		{
			if ( p->t )
				timer_reset(p->t, V_CYCLES_TO_TIME(p->timer_divider * p->timer_start + IFR_DELAY));
			else
				p->t = timer_set(V_CYCLES_TO_TIME(p->timer_divider * p->timer_start + IFR_DELAY), 0, riot_6530_timeout);
		}
		p->time = timer_get_time();
	}
}

static UINT8 ram6530[0x0200]; 

static READ_HANDLER(ram_6530r) {
	return ram6530[offset];
}

static WRITE_HANDLER(ram_6530w) {
	ram6530[offset] = data;
}


/*--------------
/  Memory map
/---------------*/
MEMORY_READ_START(GTS80S_readmem)
{ 0x0000, 0x01ff, MRA_RAM},
{ 0x0200, 0x02ff, riot6530_r},
{ 0x0400, 0x07ff, MRA_ROM},
{ 0x0800, 0x0bff, MRA_ROM},
{ 0x0c00, 0x0fff, MRA_ROM},
{ 0xfc00, 0xffff, MRA_ROM},
MEMORY_END

MEMORY_WRITE_START(GTS80S_writemem)
{ 0x0000, 0x01ff, MWA_RAM},
{ 0x0200, 0x02ff, riot6530_w},
{ 0x0400, 0x07ff, MWA_ROM},
{ 0x0800, 0x0bff, MWA_ROM},
{ 0x0c00, 0x0fff, MWA_ROM},
{ 0xfc00, 0xffff, MWA_ROM},
MEMORY_END

static void GTS80S_Update(int num, INT16 *buffer, int length)
{
	double dActClock, dInterval, dCurrentClock;
	int i;
		
	dCurrentClock = GTS80S_locals.clock[0];

	dActClock = timer_get_time();
	dInterval = (dActClock-GTS80S_locals.clock[0]) / length;

	if ( GTS80S_locals.buf_pos>1 )
		GTS80S_locals.buf_pos = GTS80S_locals.buf_pos;

	i = 0;
	GTS80S_locals.clock[GTS80S_locals.buf_pos] = 9e99;
	while ( length ) {
		*buffer++ = GTS80S_locals.buffer[i];
		length--;
		dCurrentClock += dInterval;

		while ( (GTS80S_locals.clock[i+1]<=dCurrentClock) )
			i++;
	}

	GTS80S_locals.clock[0] = dActClock;
	GTS80S_locals.buffer[0] = GTS80S_locals.buffer[GTS80S_locals.buf_pos-1];
	GTS80S_locals.buf_pos = 1;
}

/*--------------
/  init
/---------------*/
void GTS80S_init(int num) {
	GTS80S_CPUNo = num;

	memset(&GTS80S_locals, 0x00, sizeof GTS80S_locals);

	GTS80S_locals.timer_start = 0;
	GTS80S_locals.timer_divider = 1;
	GTS80S_locals.timer_irq_enabled = 0;
	
	GTS80S_locals.time = timer_get_time();
	GTS80S_locals.t = NULL;

	GTS80S_locals.sec_to_cycles = Machine->drv->cpu[GTS80S_CPUNo].cpu_clock;
	GTS80S_locals.cycles_to_sec = 1.0 / GTS80S_locals.sec_to_cycles;

	GTS80S_locals.clock[0]  = 0;
	GTS80S_locals.buffer[0] = 0;
	GTS80S_locals.buf_pos   = 1;

	GTS80S_locals.stream = stream_init("SND DAC", 100, 11025, 0, GTS80S_Update);
}
