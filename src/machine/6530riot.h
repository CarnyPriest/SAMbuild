/**********************************************************************

	6530 RIOT interface and emulation

	This function emulates all the functionality of up to 8 6530 RIOT
	peripheral interface adapters.

**********************************************************************/

#ifndef RIOT_6530
#define RIOT_6530
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

#define MAX_RIOT_6530 8

#define RIOT6530_PORTA	0x00
#define RIOT6530_DDRA	0x01
#define RIOT6530_PORTB	0x02
#define RIOT6530_DDRB	0x03

#define RIOT6530_TIMER	0x00
#define RIOT6530_IRF	0x01

struct riot6530_interface
{
	mem_read_handler in_a_func;
	mem_read_handler in_b_func;
	mem_write_handler out_a_func;
	mem_write_handler out_b_func;
	void (*irq_func)(int state);
};

#ifdef __cplusplus
extern "C" {
#endif

void riot6530_unconfig(void);
void riot6530_config(int which, const struct riot6530_interface *intf);
void riot6530_set_clock(int which, int clock);
void riot6530_reset(void);
int  riot6530_read(int which, int offset);
void riot6530_write(int which, int offset, int data);
void riot6530_set_input_a(int which, int data);
void riot6530_set_input_b(int which, int data);

/******************* Standard 8-bit CPU interfaces, D0-D7 *******************/

READ_HANDLER( riot6530_0_r );
READ_HANDLER( riot6530_1_r );
READ_HANDLER( riot6530_2_r );
READ_HANDLER( riot6530_3_r );
READ_HANDLER( riot6530_4_r );
READ_HANDLER( riot6530_5_r );
READ_HANDLER( riot6530_6_r );
READ_HANDLER( riot6530_7_r );

WRITE_HANDLER( riot6530_0_w );
WRITE_HANDLER( riot6530_1_w );
WRITE_HANDLER( riot6530_2_w );
WRITE_HANDLER( riot6530_3_w );
WRITE_HANDLER( riot6530_4_w );
WRITE_HANDLER( riot6530_5_w );
WRITE_HANDLER( riot6530_6_w );
WRITE_HANDLER( riot6530_7_w );

/******************* Standard 16-bit CPU interfaces, D0-D7 *******************/

READ16_HANDLER( riot6530_0_lsb_r );
READ16_HANDLER( riot6530_1_lsb_r );
READ16_HANDLER( riot6530_2_lsb_r );
READ16_HANDLER( riot6530_3_lsb_r );
READ16_HANDLER( riot6530_4_lsb_r );
READ16_HANDLER( riot6530_5_lsb_r );
READ16_HANDLER( riot6530_6_lsb_r );
READ16_HANDLER( riot6530_7_lsb_r );

WRITE16_HANDLER( riot6530_0_lsb_w );
WRITE16_HANDLER( riot6530_1_lsb_w );
WRITE16_HANDLER( riot6530_2_lsb_w );
WRITE16_HANDLER( riot6530_3_lsb_w );
WRITE16_HANDLER( riot6530_4_lsb_w );
WRITE16_HANDLER( riot6530_5_lsb_w );
WRITE16_HANDLER( riot6530_6_lsb_w );
WRITE16_HANDLER( riot6530_7_lsb_w );

/******************* Standard 16-bit CPU interfaces, D8-D15 *******************/

READ16_HANDLER( riot6530_0_msb_r );
READ16_HANDLER( riot6530_1_msb_r );
READ16_HANDLER( riot6530_2_msb_r );
READ16_HANDLER( riot6530_3_msb_r );
READ16_HANDLER( riot6530_4_msb_r );
READ16_HANDLER( riot6530_5_msb_r );
READ16_HANDLER( riot6530_6_msb_r );
READ16_HANDLER( riot6530_7_msb_r );

WRITE16_HANDLER( riot6530_0_msb_w );
WRITE16_HANDLER( riot6530_1_msb_w );
WRITE16_HANDLER( riot6530_2_msb_w );
WRITE16_HANDLER( riot6530_3_msb_w );
WRITE16_HANDLER( riot6530_4_msb_w );
WRITE16_HANDLER( riot6530_5_msb_w );
WRITE16_HANDLER( riot6530_6_msb_w );
WRITE16_HANDLER( riot6530_7_msb_w );

/******************* 8-bit A/B port interfaces *******************/

WRITE_HANDLER( riot6530_0_porta_w );
WRITE_HANDLER( riot6530_1_porta_w );
WRITE_HANDLER( riot6530_2_porta_w );
WRITE_HANDLER( riot6530_3_porta_w );
WRITE_HANDLER( riot6530_4_porta_w );
WRITE_HANDLER( riot6530_5_porta_w );
WRITE_HANDLER( riot6530_6_porta_w );
WRITE_HANDLER( riot6530_7_porta_w );

WRITE_HANDLER( riot6530_0_portb_w );
WRITE_HANDLER( riot6530_1_portb_w );
WRITE_HANDLER( riot6530_2_portb_w );
WRITE_HANDLER( riot6530_3_portb_w );
WRITE_HANDLER( riot6530_4_portb_w );
WRITE_HANDLER( riot6530_5_portb_w );
WRITE_HANDLER( riot6530_6_portb_w );
WRITE_HANDLER( riot6530_7_portb_w );

READ_HANDLER( riot6530_0_porta_r );
READ_HANDLER( riot6530_1_porta_r );
READ_HANDLER( riot6530_2_porta_r );
READ_HANDLER( riot6530_3_porta_r );
READ_HANDLER( riot6530_4_porta_r );
READ_HANDLER( riot6530_5_porta_r );
READ_HANDLER( riot6530_6_porta_r );
READ_HANDLER( riot6530_7_porta_r );

READ_HANDLER( riot6530_0_portb_r );
READ_HANDLER( riot6530_1_portb_r );
READ_HANDLER( riot6530_2_portb_r );
READ_HANDLER( riot6530_3_portb_r );
READ_HANDLER( riot6530_4_portb_r );
READ_HANDLER( riot6530_5_portb_r );
READ_HANDLER( riot6530_6_portb_r );
READ_HANDLER( riot6530_7_portb_r );

#ifdef __cplusplus
}
#endif

#endif
