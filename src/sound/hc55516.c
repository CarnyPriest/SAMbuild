#include "driver.h"
#include <math.h>

#define SAMPLE_RATE (4*48000) // 4x oversampling of standard output rate
#ifndef M_E
 #define M_E 2.7182818284590452353602874713527
#endif

#define SHIFTMASK 0x07 // = hc55516 and mc3417
//#define SHIFTMASK 0x0F // = mc3418

#define	INTEGRATOR_LEAK_TC		0.001
#define	FILTER_DECAY_TC			0.004
#define	FILTER_CHARGE_TC		0.004
#define	FILTER_MIN				0.0416
#define	FILTER_MAX				1.0954
#ifdef PINMAME
#define	SAMPLE_GAIN				6500.0
#else
#define	SAMPLE_GAIN				10000.0
#endif

struct hc55516_data
{
	INT8 	channel;
	UINT8	last_clock;
	UINT8	databit;
	UINT8	shiftreg;

	INT16	curr_value;
	INT16	next_value;

	UINT32	update_count;

	double 	filter;
	double	integrator;
	double  gain;
};


static struct hc55516_data hc55516[MAX_HC55516];
static double charge, decay, leak;


static void hc55516_update(int num, INT16 *buffer, int length);



int hc55516_sh_start(const struct MachineSound *msound)
{
	const struct hc55516_interface *intf = msound->sound_interface;
	int i;

	/* compute the fixed charge, decay, and leak time constants */
	charge = pow(1.0/M_E, 1.0 / (FILTER_CHARGE_TC * 16000.0));
	decay = pow(1.0/M_E, 1.0 / (FILTER_DECAY_TC * 16000.0));
	leak = pow(1.0/M_E, 1.0 / (INTEGRATOR_LEAK_TC * 16000.0));

	/* loop over HC55516 chips */
	for (i = 0; i < intf->num; i++)
	{
		struct hc55516_data *chip = &hc55516[i];
		char name[40];

		/* reset the channel */
		memset(chip, 0, sizeof(*chip));

		/* create the stream */
		sprintf(name, "HC55516 #%d", i);
		chip->channel = stream_init(name, intf->volume[i] & 0xff, SAMPLE_RATE, i, hc55516_update);
		chip->gain = SAMPLE_GAIN;
		/* bail on fail */
		if (chip->channel == -1)
			return 1;
	}

	/* success */
	return 0;
}


void hc55516_update(int num, INT16 *buffer, int length)
{
	struct hc55516_data *chip = &hc55516[num];
	INT32 data, slope;
	int i;

	/* zero-length? bail */
	if (length == 0)
		return;

	/* track how many samples we've updated without a clock, e.g. if its too many, then chip got no data = silence */
	chip->update_count += length;
	if (chip->update_count > SAMPLE_RATE / 32)
	{
		chip->update_count = SAMPLE_RATE; // prevent overflow
		chip->next_value = 0;
	}

	/* compute the interpolation slope */
	// as the clock drives the update (99% of the time), we can interpolate only within the current update phase
	// for the remaining cases where the output drives the update, length is rather small (1 or very low 2 digit range): then the last sample will simply be repeated
	data = chip->curr_value;
	slope = (((INT32)chip->next_value - data)<<16) / length; // PINMAME: increase/fix precision!
	data <<= 16;
	chip->curr_value = chip->next_value;

	/* reset the sample count */
	for (i = 0; i < length; i++, data += slope)
		*buffer++ = data>>16;
}


void hc55516_clock_w(int num, int state)
{
	struct hc55516_data *chip = &hc55516[num];
	int clock = state & 1, diffclock;

	/* update the clock */
	diffclock = clock ^ chip->last_clock;
	chip->last_clock = clock;

	/* speech clock changing (active on rising edge) */
	if (diffclock && clock)
	{
		double integrator = chip->integrator, temp;

		/* clear the update count */
		chip->update_count = 0;

		chip->shiftreg = ((chip->shiftreg << 1) | chip->databit) & SHIFTMASK;

		/* move the estimator up or down a step based on the bit */
		if (chip->databit)
			integrator += chip->filter;
		else
			integrator -= chip->filter;

		/* simulate leakage */
		integrator *= leak;

		/* if we got all 0's or all 1's in the last n bits, bump the step up */
		if (chip->shiftreg == 0 || chip->shiftreg == SHIFTMASK)
		{
			chip->filter = FILTER_MAX - (FILTER_MAX - chip->filter) * charge;
			if (chip->filter > FILTER_MAX)
				chip->filter = FILTER_MAX;
		}
		/* simulate decay */
		else
		{
			chip->filter *= decay;
			if (chip->filter < FILTER_MIN)
				chip->filter = FILTER_MIN;
		}

		/* compute the sample as a 32-bit word */
		temp = integrator * chip->gain;
		chip->integrator = integrator;

#ifdef PINMAME
#if 1
		/* compress the sample range to fit better in a 16-bit word */
		// Pharaoh: up to 109000, 'normal' max around 45000-50000, so find a balance between compression and clipping
		if (temp < 0.)
			temp = temp / (temp * -(1.0 / 32768.0) + 1.0) + temp*0.15;
		else
			temp = temp / (temp *  (1.0 / 32768.0) + 1.0) + temp*0.15;

		if(temp <= -32768.)
			chip->next_value = -32768;
		else if(temp >= 32767.)
			chip->next_value = 32767;
		else
			chip->next_value = (INT16)temp;
#else
		/* Cut off extreme peaks produced by bad speech data (eg. Pharaoh) */
		if (temp < -80000.) temp = -80000.;
		else if (temp > 80000.) temp = 80000.;
		/* Just wrap to prevent clipping */
		if (temp < -32768.) chip->next_value = (INT16)(-65536. - temp);
		else if (temp > 32767.) chip->next_value = (INT16)(65535. - temp);
		else chip->next_value = (INT16)temp;
#endif
#else
		/* compress the sample range to fit better in a 16-bit word */
		if (temp < 0)
			chip->next_value = (int)(temp / (-temp * (1.0 / 32768.0) + 1.0));
		else
			chip->next_value = (int)(temp / (temp * (1.0 / 32768.0) + 1.0));
#endif
		/* update the output buffer before changing the registers */
		stream_update(chip->channel, 0);
	}
}

#ifdef PINMAME
void hc55516_set_gain(int num, double gain)
{
	hc55516[num].gain = gain;
}
#endif

void hc55516_digit_w(int num, int data)
{
	hc55516[num].databit = data & 1;
}


void hc55516_clock_clear_w(int num, int data)
{
	hc55516_clock_w(num, 0);
}


void hc55516_clock_set_w(int num, int data)
{
	hc55516_clock_w(num, 1);
}


void hc55516_digit_clock_clear_w(int num, int data)
{
	hc55516[num].databit = data & 1;
	hc55516_clock_w(num, 0);
}


WRITE_HANDLER( hc55516_0_digit_w )	{ hc55516_digit_w(0,data); }
WRITE_HANDLER( hc55516_0_clock_w )	{ hc55516_clock_w(0,data); }
WRITE_HANDLER( hc55516_0_clock_clear_w )	{ hc55516_clock_clear_w(0,data); }
WRITE_HANDLER( hc55516_0_clock_set_w )		{ hc55516_clock_set_w(0,data); }
WRITE_HANDLER( hc55516_0_digit_clock_clear_w )	{ hc55516_digit_clock_clear_w(0,data); }

WRITE_HANDLER( hc55516_1_digit_w ) { hc55516_digit_w(1,data); }
WRITE_HANDLER( hc55516_1_clock_w ) { hc55516_clock_w(1,data); }
WRITE_HANDLER( hc55516_1_clock_clear_w ) { hc55516_clock_clear_w(1,data); }
WRITE_HANDLER( hc55516_1_clock_set_w )  { hc55516_clock_set_w(1,data); }
WRITE_HANDLER( hc55516_1_digit_clock_clear_w ) { hc55516_digit_clock_clear_w(1,data); }
