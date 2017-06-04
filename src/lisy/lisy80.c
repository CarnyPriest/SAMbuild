/*
 LISY80.c
 February 2017
 bontango
*/

#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <wiringPi.h>
#include "xmame.h"
#include "driver.h"
#include "wpc/core.h"
#include "wpc/sndbrd.h"
#include "lisy80.h"
#include "fileio.h"
#include "hw_lib.h"
#include "displays.h"
#include "coils.h"
#include "switches.h"
#include "utils.h"
#include "eeprom.h"
#include "sound.h"
#include "lisy.h"
#include "lisyversion.h"


//global vars for timing & speed
struct timeval lisy80_start_t;
long no_of_tickles = 0;
long no_of_throttles = 0;
int g_lisy80_throttle_val = 1000;

//global var for internal game_name structure, 
//set by  lisy80_get_gamename in unix/main
t_stru_lisy80_games_csv lisy80_game;

//global var for sound options
t_stru_lisy80_sounds_csv lisy80_sound_stru[32];
int lisy80_volume = 80; //SDL range from 0..128

//global counter for nvram write
static int nvram_delayed_write = 0;

//global var for remember state of 'SOUND16'
static int sound16 = 0;

/* remember:
sndbrd.h:#define SNDBRD_GTS80S  SNDBRD_TYPE(18,0) -> small soundboard
sndbrd.h:#define SNDBRD_GTS80SP SNDBRD_TYPE(18,1)  -> small soundboard with piggyback
sndbrd.h:#define SNDBRD_GTS80SS SNDBRD_TYPE(19,0) >big soundboard
sndbrd.h:#define SNDBRD_GTS80SS_VOTRAX SNDBRD_TYPE(19,1) -> big soundboard with speech
sndbrd.h:#define SNDBRD_GTS80B  SNDBRD_TYPE(20,0) -> 80B soundboard
*/

//local switch Matrix, we need 9 elements
//as pinmame internal starts with 1
//there is one value per return
unsigned char swMatrix[9] = { 0,0,0,0,0,0,0,0,0 };

//structs for coil handler
 /* 6532RIOT 2 (0x300) Chip U6*/
 /* PA0-6: FEED Z28(LS139) (SOL1-8) & SOUND 1-4 */
 /* PA7:   SOL.9 */
union u_riot_porta {
    unsigned char byte;
    struct {
    unsigned AB1:2,  AB2:2, SOUND_EN:1, I1G:1, I2G:1, SOL9:1;
    //unsigned b0:1, b1:1, b2:1, b3:1, b4:1, b5:1, b6:1, b7:1;
        } bitv1;
    struct {
    unsigned SOUND:4, SOUND_EN:1, I1G:1, I2G:1, SOL9:1;
    //unsigned b0:1, b1:1, b2:1, b3:1, b4:1, b5:1, b6:1, b7:1;
        } bitv2;
    struct {
    unsigned S1:1, S2:1,  S4:1,  S8:1, SOUND_EN:1, I1G:1, I2G:1, SOL9:1;
    //unsigned b0:1, b1:1, b2:1, b3:1, b4:1, b5:1, b6:1, b7:1;
        } bitv3;
    }lisy80_riot2_porta,lisy80_sound;

 /* PB0-3: LD1-4 */
 /* PB4-7: FEED Z33:LS154 (LAMP LATCHES) + PART OF SWITCH ENABLE */
union u_riot_portb {
    unsigned char byte;
    struct {
    unsigned LD1TO4:4, FEED:4;
    //unsigned b0:1, b1:1, b2:1, b3:1, b4:1, b5:1, b6:1, b7:1;
        } bitv1;
    struct {
    unsigned LD1:1, LD2:1, LD3:1, LD4:1, FEED:4;
    //unsigned b0:1, b1:1, b2:1, b3:1, b4:1, b5:1, b6:1, b7:1;
        } bitv2;
    }lisy80_riot2_portb;


union u_Z28 {
    unsigned char byte;
    struct {
    unsigned SOL1:1, SOL2:1, SOL3:1, SOL4:1, SOL5:1, SOL6:1, SOL7:1, SOL8:1;
    //unsigned b0:1, b1:1, b2:1, b3:1, b4:1, b5:1, b6:1, b7:1;
        } bitv;
    struct {
    unsigned DECODER1:4, DECODER2:4;
        } bitv2;
    }lisy80_Z28,solenoid_state;

int lisy80_SOL9;		//we spend an int for SOL9
unsigned char lisy80_lamp[48];	//all the lamps (48)
int lisy80_flip_flop[12];
int old_sounds;			//remember sound settings


//init the Hardware
void lisy80_hw_init()
{

//store start time in global var
gettimeofday(&lisy80_start_t,(struct timezone *)0);

//init th wiringPI library first
lisy80_hwlib_wiringPI_init();

//any options?
//at this stage only look for basic debug option here
//ls80opt.byte = lisy80_get_dip1();
if ( lisy80_dip1_debug_option() )
{
 fprintf(stderr,"LISY80 basic DEBUG activ\n");
 ls80dbg.bitv.basic = 1;
 lisy80_debug("LISY80 DEBUG timer set"); //first message sets print timer to zero
}
else ls80dbg.bitv.basic = 0;

//do init the hardware
//this also sets the debug options by reading jumpers via switch pic
lisy80_hwlib_init();

//now look for the other dips and for extended debug options
lisy80_get_dips();

}

//init SW portion of lisy80
void lisy80_init( void )
{
 int i;
 char s_lisy_software_version[16];

 //do the init on vars
 solenoid_state.byte = 0;
 lisy80_Z28.byte = 0;
 lisy80_SOL9 = 0;
 old_sounds = 0;
 for (i=0; i<=47; i++) lisy80_lamp[i]=0;
 for (i=0; i<=11; i++) lisy80_flip_flop[i]=0;

 //set signal handler
 lisy80_set_sighandler();

 //show up on calling terminal
 sprintf(s_lisy_software_version,"%02d.%03d ",LISY_SOFTWARE_MAIN,LISY_SOFTWARE_SUB);
 fprintf(stderr,"This is LISY (Lisy80) by bontango, Version %s\n",s_lisy_software_version);

 //show the 'boot' message
 display_show_boot_message(s_lisy_software_version,lisy80_game.gtb_no,lisy80_game.gamename);


 // try say something about LISY80 if sound is requested
 if ( ls80opt.bitv.JustBoom_sound )
 {
  //set volume according to poti
  lisy80_adjust_volume();
  sprintf(debugbuf,"/bin/echo \"Welcome to LISY 80 Version %s\" | /usr/bin/festival --tts",s_lisy_software_version);
  system(debugbuf);
 }

 //show green ligth for now, lisy80 is running
 lisy80_set_red_led(0);
 lisy80_set_yellow_led(0);
 lisy80_set_green_led(1);

 //init the sound if requested
 if ( ls80opt.bitv.JustBoom_sound )
 {
  //first try to read sound opts, as we NEED them
  if ( lisy80_file_get_soundopts() < 0 )
   {
     fprintf(stderr,"no sound opts file; sound init failed, sound emulation disabled\n");
     ls80opt.bitv.JustBoom_sound = 0;
   }
  else
   {
     fprintf(stderr,"info: sound opt file read OK\n");
   
     if ( ls80dbg.bitv.sound) {
     int i;
     for(i=1; i<=31; i++)
       fprintf(stderr,"Sound[%d]: %d %d %d \n",i,lisy80_sound_stru[i].can_be_interrupted,
			lisy80_sound_stru[i].loop,
			lisy80_sound_stru[i].st_a_catchup);
    }
   }
 }

 if ( ls80opt.bitv.JustBoom_sound )
 {
  //now open soundcard, and init soundstream
  if ( lisy80_sound_stream_init() < 0 )
   {
     fprintf(stderr,"sound init failed, sound emulation disabled\n");
     ls80opt.bitv.JustBoom_sound = 0;
   }
 else
   fprintf(stderr,"info: sound init done\n");
 }
  


}  //lisy80_init

//shutdown lisy80
void lisy80_shutdown(void)
{  
 struct timeval now;
 long seconds_passed;
 int tickles_per_second;

 //see what time is now
 gettimeofday(&now,(struct timezone *)0);
 //calculate how many seconds passed 
 seconds_passed = now.tv_sec - lisy80_start_t.tv_sec;
 //and calculate how many tickles per second we had
 tickles_per_second = no_of_tickles / seconds_passed;

 fprintf(stderr,"LISY80 graceful shutdown initiated\n");
 fprintf(stderr,"run %ld seconds in total\n",seconds_passed);
 fprintf(stderr,"we had appr. %d tickles per second\n",tickles_per_second);
 fprintf(stderr,"and %ld throttles\n",no_of_throttles);
 //show the 'shutdown' message
 display_show_shut_message();
 //put all coils and lamps to zero
 lisy80_coil_init();
 lisy80_hwlib_shutdown();
}


static int replay_pushed = 0;
//take care of special functions
//check value and give back new value if special function hits
int lisy80_special_function(int myswitch, int action)
{
 //we check for option freeplay here
 if (myswitch == LISY80_REPLAY_SWITCH)
  {
    //each time the replay switch is released
    // do a delayed nvram write
    //and set volume according to position of poti
    if ( action == 1 ) 
     { 
       nvram_delayed_write = NVRAM_DELAY;
       if ( ls80dbg.bitv.basic) lisy80_debug("NVRAM delayed write initiated by REPLAY Switch");

       //set volume in case position of poti has chnaged
       //AND we do emulate sound with pi soundcard
       if ( ls80opt.bitv.JustBoom_sound )
         {
          lisy80_adjust_volume();
          if ( ls80dbg.bitv.basic) lisy80_debug("Volume setting initiated by REPLAY Switch");
         }
     }

   //if freeplay option is not set give back old ret value
   if (ls80opt.bitv.freeplay == 0) return (myswitch);
   if (action==0) //closed
    {
	//switch closed, let do start counting in lisy80TickleWatchdog
	replay_pushed = 1;
        if ( ls80dbg.bitv.switches ) lisy80_debug("Freeplay BETA: play button pressed");
    }
   else 
    { 
	//stop counting
	replay_pushed = 0;
        if ( ls80dbg.bitv.switches )lisy80_debug("Freeplay BETA: play button released");
    }

   return(509);  //give back something >80
  }
 //return old value
 return(myswitch);
}

//switch simulation via internal FIFO
void lisy80_simulate_switch( int myswitch, int action )
{
 int i;
  //we need to do this severall times to fool Gottlieb debounce routine
  for(i=0; i<=25; i++)
   {
 	LISY80_BufferIn ( (unsigned char) myswitch);
	LISY80_BufferIn ( (unsigned char) action);
   }

  if ( ls80dbg.bitv.switches )
  {
        sprintf(debugbuf,"we simulate switch:%d action:%d",myswitch,action);
        lisy80_debug(debugbuf);
  }
}

//watchdog, this one is called every 15 milliseconds or so
//depending on spee of PI
//we do some usefull stuff in here
void lisy80TickleWatchdog( void )
{
 static int test_button_count = 0;
 static int replay_pushed_count = 0;
 static int replay_was_pushed = 0;
 static int testbut_interval = 0;

 //count the tickles
 no_of_tickles++;

 //check if a delayed nvram write is needed
 switch (nvram_delayed_write)
  {
    case 0: break;
    case 1: lisy80_nvram_handler(1, NULL);
	    nvram_delayed_write--;
	    break;
   default: nvram_delayed_write--;
	    break;
  }


 //we do things here appr. five times  a second
 if ( testbut_interval++  > 10)
 {

 //test button is No:7 
 //if it is pressed for around 2-3 seconds, we assume
 //that the user wants to shutdown
 if ( swMatrix[8] & 0x01 )
 {
  if (test_button_count++ > 13 )
   lisy_time_to_quit_flag = 1;
 }
 else test_button_count = 0;

//reset one interval
  testbut_interval = 0;
 }

 //check if we need to count for freeplay
 //replay button was pushed
 if (replay_pushed)
 {
   replay_was_pushed = 1;
   replay_pushed_count++;
   //check if is still pushed
   if(replay_pushed_count>20)
    {
        if ( ls80dbg.bitv.basic) lisy80_debug("we simulate coins now");
	lisy80_simulate_switch( LISY80_LEFTCOIN_SWITCH, 0);
	lisy80_simulate_switch( LISY80_LEFTCOIN_SWITCH, 1);
	replay_was_pushed = 0;
	replay_pushed_count = 0;
    }
 }

 //replay button was pushed
 //but is now released
 if ((replay_was_pushed) && (replay_pushed==0))
 {
        if ( ls80dbg.bitv.basic) lisy80_debug("replay normal function set");
	lisy80_simulate_switch( LISY80_REPLAY_SWITCH, 0);
	lisy80_simulate_switch( LISY80_REPLAY_SWITCH, 1);
	replay_was_pushed = 0;
	replay_pushed_count = 0;
 }
}//watchdog

//we simulate switches here via buffer
int lisy80_simulated_switch_reader( unsigned char *action )
{

 unsigned char value;

 if ( LISY80_BufferOut( &value) == LISY80_BUFFER_SUCCESS)
  {
   //next byte in buffer have to be action
   LISY80_BufferOut ( action);
   return value;
  }
 else
  return 510;
}

/*
 throttle routine as with sound disabled
 lisy80 runs faster than the org game :-0
*/

void lisy80_throttle(int riot0b)
{
static int first = 1;
unsigned int now;
int sleeptime;
static unsigned int last;

//we do some speed limitations here based
//on the 'normal' frequency a Gottlieb scan the switches
// which is appr. each ms (can be 2 ms with PI zero & sound )
// Note: if riot0b is 1, test showed that we have same spare time than

//if val is zero, we have to run at full speed
if ( g_lisy80_throttle_val == 0) return; 

if (first)
 {
  first = 0;
  //store start time first, which is number of microseconds since wiringPiSetup (wiringPI lib)
  last = micros();
 }

 // if we are faster than 1000 usec (1 msec)
 // which is the default for g_lisy80_throttle_val
 //we need to slow down a bit

 //see how many micros passed
 now = micros();
 //beware of 'wrap' which happens each 71 minutes
 if ( now < last) now = last; //we had a wrap

 //calculate if we are above minimum sleep time 
 sleeptime = g_lisy80_throttle_val - ( now - last);
 if ( sleeptime > 0)
          delayMicroseconds( sleeptime );

 //store current time for next round with speed limitc
 last = micros();
}

/*
  switch handler
  use internal matrix to update lisy80_matrix
  and give back matrix according to riot_0b (strobe)
*/
int lisy80_switch_handler( int riot0b )
{

int ret,bits,ii;
int simulated_flag;
unsigned char strobe,returnval,action;
unsigned char mystrobe,myreturnval;


//read values from pic
//check if there is an update first
ret = lisy80_switch_reader( &action );

//do we need a 'special' routine to handle that switch?
// switch 47 (Replay) could mean to add credits in case of Freeplay
// others to follow
if (ret==47)
{
 ret = lisy80_special_function( ret, action );
}

//if ret >80 lets check if we have switches to simulate in the queue
simulated_flag=0;
if (ret >= 80)
 {
  if ( ( ret = lisy80_simulated_switch_reader( &action )) < 80)
  	simulated_flag=1;
 }

//77 switches ; ret < 80 indicates a change
// values >=80 are for debugging only
 if (ret < 80)
 {
  strobe = ret / 10;
  returnval = ret % 10;

  //set the bit in the Matrix var according to action
  // action 0 means set the bit
  // any other means delete the bit
  if (action == 0) //set bit
    swMatrix[returnval+1] |= ( 1 << strobe );
  else  //delete bit
    swMatrix[returnval+1] &= ~( 1 << strobe );

 }

 
//now set bits in the same way pinmame does it
 bits=0;
 for (ii = 8; ii > 0; ii--) {
      bits <<= 1;
      if (swMatrix[ii] & riot0b) bits |= 1;
    }


return bits;


}

//display handler
//BCD Version
void lisy80_display_handler_bcd( int riot1a, int riot1b )
{

//init with odd value as we send only change cmd via I2C
static int player1[7] = { 80, 80, 80, 80, 80, 80, 80 };
static int player2[7] = { 80, 80, 80, 80, 80, 80, 80 };
static int player3[7] = { 80, 80, 80, 80, 80, 80, 80 };
static int player4[7] = { 80, 80, 80, 80, 80, 80, 80 };
static int player5[6] = { 80, 80, 80, 80, 80, 80 }; //bonus if existing
static int player6[6] = { 80, 80, 80, 80, 80, 80 }; //timer if existing
static int status[4] = { 80, 80, 80, 80 };
static int old_digit = 80;

static int old_riot1a = 0;
static int old_riot1b = 0;
static int seg_A = 0; //Player 1/2
static int seg_B = 0; //Player 3/4
static int seg_C = 0; // status (and Bonus)

int digit; 		//the digit ( 00.. 15 )
int segment_data;	//data for segment ( segA, segB, segC )
int h_line_segA,h_line_segB,h_line_segC;
int i;
char str[80];

//extract number of display, segment_data and h_lines
digit = riot1a & 0x0f;
segment_data = riot1b & 0x0f;
h_line_segA = riot1b & 0x10;
h_line_segB = riot1b & 0x20;
h_line_segC = riot1b & 0x40;


// Load buffers on rising edge 0x10,0x20,0x40
// do we have a push, or is it a 'clean display' -> all three segments pushed with segment data==15
if ((riot1a & ~old_riot1a & 0x10) && (riot1a & ~old_riot1a & 0x20) &&  (riot1a & ~old_riot1a & 0x40) && ( segment_data == 15 ))
  {
	return;  //do nothing ; this is blanking
  }

/*
sprintf(str, "FLAGS: ");
if ( h_line_segA == 0) strcat(str,"hline_A \n");
if ( h_line_segB == 0) strcat(str,"hline_B \n");
if ( h_line_segC == 0) strcat(str,"hline_C \n");
if  (riot1a & ~old_riot1a & 0x10) strcat(str,"push_A ");
if  (riot1a & ~old_riot1a & 0x20) strcat(str,"push_B ");
if  (riot1a & ~old_riot1a & 0x40) strcat(str,"push_C ");
fprintf(stderr,"digit:%d data%d %s\n",digit,segment_data,str);
fprintf(stderr,"----\n");
*/

//we collecting data here
if (old_digit == digit)
{
  if  (riot1a & ~old_riot1a & 0x10) seg_A=segment_data; //push_A ?
  if  (riot1a & ~old_riot1a & 0x20) seg_B=segment_data; //push_B ?
  if  (riot1a & ~old_riot1a & 0x40) seg_C=segment_data; //push_C ?
  //we assume that middle segment actuive means always '1'
  if ( h_line_segA == 0) seg_A=1;
  if ( h_line_segB == 0) seg_B=1;
  if ( h_line_segC == 0) seg_C=1;
}
else //new sequence, we write now the data of the old_digit
{
	//digit 0..5 serves Player1 ; 3 and 5(Bonus)
	if (old_digit <= 5)
        {
	 if ( player1[old_digit] != seg_A) { player1[old_digit] = seg_A; display_show_int(1, old_digit, seg_A); }
	 if ( player3[old_digit] != seg_B) { player3[old_digit] = seg_B; display_show_int(3, old_digit, seg_B); }
	 if ( player5[old_digit] != seg_C) { player5[old_digit] = seg_C; display_show_int(5, old_digit, seg_C); }
	}
	//digit 6..10 serves Player2 ; 4 and 6(Timer)
        if (( old_digit > 5) && ( old_digit <= 11))
	{
	 if ( player2[old_digit-6] != seg_A) { player2[old_digit-6] = seg_A; display_show_int(2, old_digit-6, seg_A); }
	 if ( player4[old_digit-6] != seg_B) { player4[old_digit-6] = seg_B; display_show_int(4, old_digit-6, seg_B); }
	 if ( player6[old_digit-6] != seg_C) { player6[old_digit-6] = seg_C; display_show_int(6, old_digit-6, seg_C); }
	}
 	if ( old_digit > 11)
	{
	  if ( old_digit == 12) //7.Digit SYS80A Player 2 and 4
	  { 
	 	if ( player2[6] != seg_A) { player2[6] = seg_A; display_show_int(2, 6, seg_A); }
	 	if ( player4[6] != seg_B) { player4[6] = seg_B; display_show_int(4, 6, seg_B); }
	  }
	  if ( old_digit == 15) //7.Digit SYS80A Player 1 and 3
	  {
	 	if ( player1[6] != seg_A) { player1[6] = seg_A; display_show_int(1, 6, seg_A); }
	 	if ( player3[6] != seg_B) { player3[6] = seg_B; display_show_int(3, 6, seg_B); }
	  }
	 if ( status[old_digit-12] != seg_C) { status[old_digit-12] = seg_C;  display_show_int(0, old_digit-12, seg_C); }
	}
} //new sequence

//remember
old_digit = digit;
old_riot1a = riot1a;
old_riot1b = riot1b;

} //display_handler_bcd

//display handler
//Alphanumeric Version
// 0 means porta; 1 means portb
void lisy80_display_handler_alpha( int a_or_b, int riot_port )
{

//some unions to ease bit handling
static union u_riot_porta {
    unsigned char byte;
    struct {
    unsigned FREE1:4,  CK1:1, CK2:1, FREE2:2;
    //unsigned b0:1, b1:1, b2:1, b3:1, b4:1, b5:1, b6:1, b7:1;
        } bitv;
    }old_porta,porta;

static union u_riot_portb {
    unsigned char byte;
    struct {
    unsigned data:4,  LD1:1, LD2:1, RESET:1, FREE:1;
    //unsigned b0:1, b1:1, b2:1, b3:1, b4:1, b5:1, b6:1, b7:1;
        } bitv;
    }old_portb,portb;

static union u_data_for_pic {
    unsigned char byte;
    struct {
    unsigned D0toD3:4, D4toD7:4;
    //unsigned b0:1, b1:1, b2:1, b3:1, b4:1, b5:1, b6:1, b7:1;
        } bitv;
    }data_for_pic;

//other vars
int LD1_was_set = 0;
int LD2_was_set = 0;


//static internal vars
//we emulating 74175 and collecting data
// before using I2C for display pic

//is it PORT A?
// with Port A we just collecting data for pic_byte
if ( a_or_b == 0)
 {
   porta.byte = riot_port;

   //falling edge of CK1? If yes set first 4 bits of databyte
   if (( porta.bitv.CK1 == 0 ) & ( old_porta.bitv.CK1 == 1))
    {
     data_for_pic.bitv.D0toD3 = portb.bitv.data;
    }
   //falling edge of CK2? If yes set second 4 bits of databyte
   if (( porta.bitv.CK2 == 0 ) & ( old_porta.bitv.CK2 == 1))
    {
     data_for_pic.bitv.D4toD7 = portb.bitv.data;
    }

   //remember
   old_porta.byte = porta.byte;
  }


// is it PORT B?
// remember values for routine with port A
// check if falling Edge of LD1 and/or LD2 which means setting data
// check for RESET
if ( a_or_b == 1)
 {
   portb.byte = riot_port;

   //RESET?
   if ( portb.bitv.RESET == 1 )
    {
      display_reset();
      if ( ls80dbg.bitv.basic) lisy80_debug("RESET = 1");
    }

   //falling edge of LD1? If yes send the collected byte to PIC to set row 1
   if (( portb.bitv.LD1 == 0 ) & ( old_portb.bitv.LD1 == 1)) LD1_was_set = 1;

   //falling edge of LD2? If yes send the collected byte to PIC to set row 2
   if (( portb.bitv.LD2 == 0 ) & ( old_portb.bitv.LD2 == 1)) LD2_was_set = 1;


   //now send to PIC if needed
   if ( LD1_was_set | LD2_was_set )display_sendtorow( data_for_pic.byte, LD1_was_set, LD2_was_set, (int) ls80dbg.bitv.displays );

   LD1_was_set = LD2_was_set = 0;
   //remember
   old_portb.byte = portb.byte;
 }


} //display handler Alphanumeric Version





//coil_handler
//RIOT A controls the 9 Solenoids plus Sound


void lisy80_coil_handler_a( int data)
{

//read what the RIOT wants to do
lisy80_riot2_porta.byte = data;
// SOL9 has an inverter behind
lisy80_riot2_porta.bitv1.SOL9 = ~lisy80_riot2_porta.bitv1.SOL9;

//emulate a 74LS139
//note that with Enable == 1 ( I1G & I2G ) all outputs are high
//which means LOW for LISY80 because of inverter 
//numbering due to inverter on SYS80 Board
if ( lisy80_riot2_porta.bitv1.I1G == 0) //decoder one enabled
	{
	 lisy80_Z28.bitv2.DECODER1 = 0;
	 switch ( lisy80_riot2_porta.bitv1.AB1)
	  {
		case 3: lisy80_Z28.bitv.SOL1 = 1;  break;
		case 2: lisy80_Z28.bitv.SOL2 = 1;  break;
		case 1: lisy80_Z28.bitv.SOL3 = 1;  break;
		case 0: lisy80_Z28.bitv.SOL4 = 1;  break;
	  }
	}
else lisy80_Z28.bitv2.DECODER1 = 0;

if ( lisy80_riot2_porta.bitv1.I2G == 0) //decoder two enabled
	{
	 lisy80_Z28.bitv2.DECODER2 = 0;
	 switch ( lisy80_riot2_porta.bitv1.AB2)
	  {
		case 3: lisy80_Z28.bitv.SOL5 = 1;  break;
		case 2: lisy80_Z28.bitv.SOL6 = 1;  break;
		case 1: lisy80_Z28.bitv.SOL7 = 1;  break;
		case 0: lisy80_Z28.bitv.SOL8 = 1;  break;
	  }
	}
else lisy80_Z28.bitv2.DECODER2 = 0;

	//check if solenoid 1..4 have changed
	if (solenoid_state.bitv.SOL1 != lisy80_Z28.bitv.SOL1 ) {
	   solenoid_state.bitv.SOL1 = lisy80_Z28.bitv.SOL1;
 if ( ls80dbg.bitv.coils )
  {
	   sprintf(debugbuf,"LISY80_COIL_HANDLER_A:change detected SOL1=%d",solenoid_state.bitv.SOL1);
           lisy80_debug(debugbuf);
  }
	   lisy80_coil_set(Q_SOL1,solenoid_state.bitv.SOL1);
		 }
	if (solenoid_state.bitv.SOL2 != lisy80_Z28.bitv.SOL2 ) {
	   solenoid_state.bitv.SOL2 = lisy80_Z28.bitv.SOL2;
 if ( ls80dbg.bitv.coils )
  {
	   sprintf(debugbuf,"LISY80_COIL_HANDLER_A:change detected SOL2=%d",solenoid_state.bitv.SOL2);
           lisy80_debug(debugbuf);
  }
	   lisy80_coil_set(Q_SOL2,solenoid_state.bitv.SOL2);
		 }
	if (solenoid_state.bitv.SOL3 != lisy80_Z28.bitv.SOL3 ) {
	   solenoid_state.bitv.SOL3 = lisy80_Z28.bitv.SOL3;
 if ( ls80dbg.bitv.coils )
  {
	   sprintf(debugbuf,"LISY80_COIL_HANDLER_A:change detected SOL3=%d",solenoid_state.bitv.SOL3);
           lisy80_debug(debugbuf);
  }
	   lisy80_coil_set(Q_SOL3,solenoid_state.bitv.SOL3);
		 }
	if (solenoid_state.bitv.SOL4 != lisy80_Z28.bitv.SOL4 ) {
	   solenoid_state.bitv.SOL4 = lisy80_Z28.bitv.SOL4;
 if ( ls80dbg.bitv.coils )
  {
	   sprintf(debugbuf,"LISY80_COIL_HANDLER_A:change detected SOL4=%d",solenoid_state.bitv.SOL4);
           lisy80_debug(debugbuf);
  }
	   lisy80_coil_set(Q_SOL4,solenoid_state.bitv.SOL4);
		 }
	//check if solenoid 5..8 have changed
	if (solenoid_state.bitv.SOL5 != lisy80_Z28.bitv.SOL5 ) {
	   solenoid_state.bitv.SOL5 = lisy80_Z28.bitv.SOL5;
 if ( ls80dbg.bitv.coils )
  {
	   sprintf(debugbuf,"LISY80_COIL_HANDLER_A:change detected SOL5=%d",solenoid_state.bitv.SOL5);
           lisy80_debug(debugbuf);
  }
	   lisy80_coil_set(Q_SOL5,solenoid_state.bitv.SOL5);
		 }
	if (solenoid_state.bitv.SOL6 != lisy80_Z28.bitv.SOL6 ) {
	   solenoid_state.bitv.SOL6 = lisy80_Z28.bitv.SOL6;
 if ( ls80dbg.bitv.coils )
  {
	   sprintf(debugbuf,"LISY80_COIL_HANDLER_A:change detected SOL6=%d",solenoid_state.bitv.SOL6);
           lisy80_debug(debugbuf);
  }
	   lisy80_coil_set(Q_SOL6,solenoid_state.bitv.SOL6);
		 }
	if (solenoid_state.bitv.SOL7 != lisy80_Z28.bitv.SOL7 ) {
	   solenoid_state.bitv.SOL7 = lisy80_Z28.bitv.SOL7;
 if ( ls80dbg.bitv.coils )
  {
	   sprintf(debugbuf,"LISY80_COIL_HANDLER_A:change detected SOL7=%d",solenoid_state.bitv.SOL7);
           lisy80_debug(debugbuf);
  }
	   lisy80_coil_set(Q_SOL7,solenoid_state.bitv.SOL7);
		 }
	if (solenoid_state.bitv.SOL8 != lisy80_Z28.bitv.SOL8 ) {
	   solenoid_state.bitv.SOL8 = lisy80_Z28.bitv.SOL8;
 if ( ls80dbg.bitv.coils )
  {
	   sprintf(debugbuf,"LISY80_COIL_HANDLER_A:change detected SOL8=%d",solenoid_state.bitv.SOL8);
           lisy80_debug(debugbuf);
  }
	   lisy80_coil_set(Q_SOL8,solenoid_state.bitv.SOL8);
		 }

//solenoid 9 has a separate line
if (lisy80_SOL9 != lisy80_riot2_porta.bitv1.SOL9 ) {
	   lisy80_SOL9 = lisy80_riot2_porta.bitv1.SOL9;
 if ( ls80dbg.bitv.coils )
  {
	   sprintf(debugbuf,"LISY80_COIL_HANDLER_A:change detected SOL9=%d",lisy80_SOL9);
           lisy80_debug(debugbuf);
  }
	   lisy80_coil_set(Q_SOL9,lisy80_SOL9);
		 }

//we check for sounds here
//Note: Output is negated because of LS04 at PA0..PA4
if ( lisy80_riot2_porta.bitv2.SOUND_EN == 0) // Z31 (7408 'AND') sound control enabled
	 {
	  if (old_sounds != lisy80_riot2_porta.bitv2.SOUND ) {
		//set value for LISY80 which is negated because of Z27
		lisy80_sound.bitv3.S1 = ~lisy80_riot2_porta.bitv3.S1;
		lisy80_sound.bitv3.S2 = ~lisy80_riot2_porta.bitv3.S2;
		lisy80_sound.bitv3.S4 = ~lisy80_riot2_porta.bitv3.S4;
		lisy80_sound.bitv3.S8 = ~lisy80_riot2_porta.bitv3.S8;
	  	lisy80_sound_set(lisy80_sound.bitv2.SOUND);  //set to new value

		//JustBoom Sound? we may want to play wav files here
		//need to add separate sound line here! for sound >16
       		if ( ls80opt.bitv.JustBoom_sound ) lisy80_play_wav(lisy80_sound.bitv2.SOUND + sound16);

	        //remember old value
		old_sounds = lisy80_riot2_porta.bitv2.SOUND;
	   }
	}
else //not enabled, so value is '1111' and output is zero
	 {
	  //only if sound changed
	  if (old_sounds != 0 ) {
	      old_sounds = 0;
	      lisy80_sound_set(0);  // zero
	   }
	}

} //lisy8_coil_handler

//RIOT B controls all the lamps
// Q1= Game Over Relay
// Q2 = Tilt Relay
// Q3 = Coin Lockout coil
// Q4 = Lamp L3, Q5 = Lamp L4, ...

void lisy80_coil_handler_b( int data)
{
  unsigned char new_lamp[4];
  int offset,i,flip_flop_no;

  //read what the RIOT wants to do
  lisy80_riot2_portb.byte = data;

  //the FEED (Flip Flop 74154) need to be in range between 1 and 12
  //as 0 means that the pin wants to activate dip switch reading
  flip_flop_no = lisy80_riot2_portb.bitv1.FEED;
  if (( flip_flop_no < 1 ) ||( flip_flop_no > 12 ))  return;


  //by selecting we went from 0 to 1 ( LOW to HIGH clock transition ) 
  //OK, assume we have valid data now, compute which lamps need to be changed (4 max)
  new_lamp[0] = lisy80_riot2_portb.bitv2.LD1;
  new_lamp[1] = lisy80_riot2_portb.bitv2.LD2;
  new_lamp[2] = lisy80_riot2_portb.bitv2.LD3;
  new_lamp[3] = lisy80_riot2_portb.bitv2.LD4;

  //which of the 12 * 74175 we have adressed? compute the offset
  offset = (flip_flop_no - 1) *4;

  for (i=0; i<=3; i++) {
      if ( lisy80_lamp[ i + offset] != new_lamp[i] ) {
      if ( ls80dbg.bitv.lamps)
       {
	   if ( new_lamp[i] ) sprintf(debugbuf,"LISY80_COIL_HANDLER: Transistor_Q:%d ON",i+1+offset);
	     else sprintf(debugbuf,"LISY80_COIL_HANDLER: Transistor_Q:%d OFF",i+1+offset);
	lisy80_debug(debugbuf);
       }
  	   //do a delayd nvram write each time the game over relay ( lamp[0]) is going from zero to one (Game Over)
	   //this is done in the tickle routine
  	   if ( ((i + offset) == 0) & (lisy80_lamp[0] == 1) ) 
             {
              nvram_delayed_write = NVRAM_DELAY;
              if ( ls80dbg.bitv.basic ) lisy80_debug("NVRAM delayed write initiaed by GAME OVER Relay");
             }
	   //remember
	   lisy80_lamp[i+offset] = new_lamp[i];
	   //and set the lamp/coil
	    if ( new_lamp[i] ) lisy80_coil_set(i+1+offset, 1); else  lisy80_coil_set(i+1+offset, 0);

           //special handling for sound16, which is lamp9/Q10 in case of bigger soundboard
	   //we have to remember that for sound settings
	    if ( (core_gameData->hw.soundBoard == SNDBRD_GTS80S) || (core_gameData->hw.soundBoard == SNDBRD_GTS80SP) )
              {
	       sound16 = 0; //small soundboards have no line sound16
	      }
	    else
              {
	       if ( ( i+1+offset) == 10 ) {  if ( new_lamp[i] ) sound16=16; else sound16=0; }
              }

  //aditional debug output
  //if(ls80dbg.bitv.lamps)
//	fprintf(stderr,"LISY80_COIL_HANDLER_B:LD:%d FEED:%d\n",lisy80_riot2_portb.bitv1.LD1TO4,lisy80_riot2_portb.bitv1.FEED);
       }
  }

} //coil_handler_b

//read the csv file on /boot partition and the DIP switch setting
//give back gamename accordently and line number
// -1 in case we had an error
//this is called early from unix/main.c
int lisy80_get_gamename(char *gamename)
{

 int ret;

 //use function from fileio to get more details about the gamne
 ret =  lisy80_file_get_gamename( &lisy80_game);


  //give back the name and the number of the game
  strcpy(gamename,lisy80_game.gamename);

  //store throttle value from gamelist to global var
  g_lisy80_throttle_val = lisy80_game.throttle;
  //give this info on screen
  fprintf(stderr,"LISY80: Throttle value is %d\n",g_lisy80_throttle_val);

  //other infos are stored in global var

  return ret;
}


//this give back lisy_time_to_quit to cpuexec
//set by signalhandler
int lisy_time_to_quit(void)
{
  return lisy_time_to_quit_flag;
}

//read dipswitchsettings for specific game/mpu
//and give back settings or -1 in case of error
//switch_nr is 0..3
int lisy80_get_mpudips( int switch_nr )
{

 int ret;
 char dip_file_name[80];


 //use function from fileio.c
 ret = lisy80_file_get_mpudips( switch_nr, ls80dbg.bitv.basic, dip_file_name );
 //debug?
 if ( ls80dbg.bitv.basic )
  {
   sprintf(debugbuf,"LISY80: DIP switch settings according to %s",dip_file_name);
   lisy80_debug(debugbuf);
  }

 //return setting, this will be -1 if 'first_time' failed
 return ret;

}


//handling of nvram via eeprom
//read_or_write = 0 means read
int lisy80_nvram_handler(int read_or_write, UINT8 *GTS80_pRAM_GTB)
{
 static unsigned char first_time = 1;
 static eeprom_block_t nvram_block;
 static eeprom_block_t lisy80_block;
 static UINT8 *GTS80_pRAM;
 int i,ret;

 //check content at first time
 if (first_time)
 {
  //we need a read for the init, so ignore write requests for now
  if ( read_or_write ) return 0;

  //remember address for subsequent actions
  GTS80_pRAM = GTS80_pRAM_GTB;

  first_time = 0;
  //do we have a valid signature in block 1? -> 1 =yes
  //if not init eeprom note:gane nr is 80 here
  if ( !lisy80_eeprom_checksignature()) 
     { 
       ret = lisy80_eeprom_init();
 if ( ls80dbg.bitv.basic )
  {
        sprintf(debugbuf,"write nvram INIT done:%d",ret);
        lisy80_debug(debugbuf);
  }

     }
  //read lisy80 block 
  lisy80_eeprom_256byte_read( lisy80_block.byte, 1);

  //if the stored game number is not the current one
  //initialize nvram block with zeros and write to eeprom
  if(lisy80_block.content.gamenr != lisy80_game.gamenr)
   {
    for(i=0;i<=255; i++) nvram_block.byte[i] = '\0';
    ret = lisy80_eeprom_256byte_write( nvram_block.byte, 0);
 if ( ls80dbg.bitv.basic )
  {
        sprintf(debugbuf,"stored game nr not current one, we init with zero:%d",ret);
        lisy80_debug(debugbuf);
   }

    //prepare new gane number to write
    lisy80_block.content.gamenr = lisy80_game.gamenr;
   }

   //now update statistics
   lisy80_block.content.starts++;
   if(ls80dbg.bitv.basic) lisy80_block.content.debugs++;
   lisy80_block.content.counts[lisy80_game.gamenr]++;
   lisy80_block.content.Software_Main = LISY_SOFTWARE_MAIN;
   lisy80_block.content.Software_Sub = LISY_SOFTWARE_SUB;
   ret = lisy80_eeprom_256byte_write( lisy80_block.byte, 1);
   if ( ls80dbg.bitv.basic )
   {
        sprintf(debugbuf,"nvram statistics updated for game:%d",lisy80_block.content.gamenr);
        lisy80_debug(debugbuf);
   }
   
 } //first time only

 if(read_or_write) //1 = write
 {
  //check if NVRAM has changed ?
  // if (memcmp( nvram_block.byte,  GTS80_pRAM, 0x100) != 0)
  //memcpy(nvram_block.byte,(char*)GTS80_pRAM, 0x100);
  
  ret = lisy80_eeprom_256byte_write( (char*)GTS80_pRAM, 0);
 if ( ls80dbg.bitv.basic )
  {
        sprintf(debugbuf,"LISY80 write nvram done:%d",ret);
        lisy80_debug(debugbuf);
  }
 }
 else //we want to read
 {
  ret = lisy80_eeprom_256byte_read( nvram_block.byte, 0);
  //cop to mem if success
  if (ret == 0)  memcpy((char*)GTS80_pRAM, nvram_block.byte, 256);
 if ( ls80dbg.bitv.basic )
  {
        sprintf(debugbuf,"read nvram done:%d",ret);
        lisy80_debug(debugbuf);
  }
 }

 return (ret);
}

//
//sound handling
//


//get postion of poti and give it back
//RTH: number of times to read to adjust
int lisy80_get_position(void)
{
  
  int i,pos;
  long poti_val;

  //first time fake read
  poti_val = lisy80_get_poti_val();
  poti_val = 0;

  //read 100 times
  for(i=1; i<=10; i++) poti_val = poti_val + lisy80_get_poti_val();
 
  //divide result
  pos = poti_val / 10;


  return( pos );
}



//set new volume in case postion of poti have changed
int lisy80_adjust_volume(void)
{
  static int first = 1;
  static int old_position;
  int position,diff,sdl_volume,amix_volume;

  //no poti for hardware 3.11
  if ( lisy_hardware_revision == 311) return(0);


  //read position
  position = lisy80_get_position();

  //we assume pos is in range 600 ... 7500
  //and translate that to the SDL range of 0..128
  //we use steps of 54
  sdl_volume =  ( position / 54 ) - 10;
  if ( sdl_volume > 128 ) sdl_volume = 128; //limit

  if ( first)  //first time called, set volume
  {
    first = 0;
    old_position = position;
    if ( ls80dbg.bitv.sound)
    {
     sprintf(debugbuf,"Volume first setting: position of poti is:%d",position);
     lisy80_debug(debugbuf);
    }
    lisy80_volume = sdl_volume; //set global var for SDL
 
    // first setting, we do it with amixer for now; range here is 0..100
     amix_volume = (sdl_volume*100) / 128;
     sprintf(debugbuf,"/usr/bin/amixer sset Digital %d percent",amix_volume);
    // first setting, we announce here the volume setting
     sprintf(debugbuf,"/bin/echo \"Volume set to %d\" | /usr/bin/festival --tts",amix_volume);
     system(debugbuf);
     if ( ls80dbg.bitv.sound)
     {
      sprintf(debugbuf,"first Volume setting initiated via amixer: %d percent",amix_volume);
      lisy80_debug(debugbuf);
     }
  }
  else
  {

    //is there a significantge change? otherwise it is just because linux is not soo accurate
    if( (diff = abs(old_position - position )) > 300)
    {
     lisy80_volume = sdl_volume; //set global var for SDL
     if ( ls80dbg.bitv.sound)
     {
      sprintf(debugbuf,"new Volume setting initiated: %d",sdl_volume);
      lisy80_debug(debugbuf);
     }
    }
    else
    {
     if ( ls80dbg.bitv.sound)
     {
      sprintf(debugbuf,"new Volume setting initiated, but no significant change:%d",diff);
      lisy80_debug(debugbuf);
     }
    }

    old_position = position;

  }

  return(1);

}
