
#include "driver.h"
#include "core.h"
#include "snd_cmd.h"
#include "sndbrd.h"
#include "stsnd.h"
#include "math.h"
/*----------------------------------------
/ Stern Sound System
/ 3 different boards:
/
/ ST-100:  discrete
/ ST-300:  discrete, (+ VS-1000 speech)
/ ASTRO:   discrete, can switch between SB-100 and SB-300
/-----------------------------------------*/
// support added soundboard st-300 by Oliver Kaegi (14/09/2004)
// support added for vs-100 speech by Oliver Kaegi (10/10/2004)
// speech roms for freefall missing
// not sure if catacombs roms are correct...
// interface definition for soundboard st-300
#define ST300_INTCLOCK    1000000  // clock speed in hz of mpu ST-200 board ! -> 0.000'001 sec

#define ST300_VOL 1 // volume for channels

static const char *f2k_sample_names[] =
{
	"f2k00.wav",
	"f2k01.wav",
	"f2k02.wav",
	"f2k03.wav",
	"f2k04.wav",
	"f2k05.wav",
	"f2k06.wav",
	"f2k07.wav",
	"f2k08.wav",
	"f2k09.wav",
	"f2k10.wav",
	"f2k11.wav",
	"f2k12.wav",
	"f2k13.wav",
	"f2k14.wav",
	"f2k15.wav",
	"f2k16.wav",
	"f2k17.wav",
	"f2k18.wav",
	"f2k19.wav",
	"f2k20.wav",
	0   /* end of array */
};

struct Samplesinterface f2k_samples_interface =
{
	1,
	100,	/* volume */
	f2k_sample_names,
	"TSI S14001A"
};


static const char *spl_sample_names[] =
{
	"spl00.wav",
	"spl01.wav",
	"spl02.wav",
	"spl03.wav",
	"spl04.wav",
	"spl05.wav",
	"spl06.wav",
	"spl07.wav",
	"spl08.wav",
	"spl09.wav",
	"spl10.wav",
	"spl11.wav",
	"spl12.wav",
	"spl13.wav",
	"spl14.wav",
	"spl15.wav",
	"spl16.wav",
	"spl17.wav",
	"spl18.wav",
	"spl19.wav",
	"spl20.wav",
	"spl21.wav",
	"spl22.wav",
	"spl23.wav",
	"spl24.wav",
	"spl25.wav",
	"spl26.wav",
	"spl27.wav",
	"spl28.wav",
	"spl29.wav",
	"spl30.wav",
	"spl31.wav",
	"spl32.wav",
	"spl33.wav",
	"spl34.wav",
	"spl35.wav",
	"spl36.wav",
	"spl37.wav",
	"spl38.wav",
	"spl39.wav",
	"spl40.wav",
	0   /* end of array */
};

struct Samplesinterface spl_samples_interface =
{
	1,
	100,	/* volume */
	spl_sample_names,
	"TSI S14001A"
};


static const char *lig_sample_names[] =
{
	"lig00.wav",
	"lig01.wav",
	"lig02.wav",
	"lig03.wav",
	"lig04.wav",
	"lig05.wav",
	"lig06.wav",
	"lig07.wav",
	"lig08.wav",
	"lig09.wav",
	"lig10.wav",
	"lig11.wav",
	"lig12.wav",
	"lig13.wav",
	"lig14.wav",
	"lig15.wav",
	"lig16.wav",
	"lig17.wav",
	"lig18.wav",
	"lig19.wav",
	"lig20.wav",
	"lig21.wav",
	"lig22.wav",
	"lig23.wav",
	"lig24.wav",
	"lig25.wav",
	"lig26.wav",
	"lig27.wav",
	"lig28.wav",
	"lig29.wav",
	"lig30.wav",
	"lig31.wav",
	"lig32.wav",
	"lig33.wav",
	"lig34.wav",
	0   /* end of array */
};

struct Samplesinterface lig_samples_interface =
{
	1,
	100,	/* volume */
	lig_sample_names,
	"TSI S14001A"
};

static const char *orb_sample_names[] =
{
	"orb00.wav",
	"orb01.wav",
	"orb02.wav",
	"orb03.wav",
	"orb04.wav",
	"orb05.wav",
	"orb06.wav",
	"orb07.wav",
	"orb08.wav",
	"orb09.wav",
	"orb10.wav",
	"orb11.wav",
	"orb12.wav",
	"orb13.wav",
	"orb14.wav",
	"orb15.wav",
	"orb16.wav",
	"orb17.wav",
	"orb18.wav",
	"orb19.wav",
	"orb20.wav",
	"orb21.wav",
	"orb22.wav",
	"orb23.wav",
	"orb24.wav",
	"orb25.wav",
	"orb26.wav",
	"orb27.wav",
	"orb28.wav",
	"orb29.wav",
	"orb30.wav",
	"orb31.wav",
	"orb32.wav",
	"orb33.wav",
	"orb34.wav",
	"orb35.wav",
	"orb36.wav",
	"orb37.wav",
	"orb38.wav",
	"orb39.wav",
	"orb40.wav",
	"orb41.wav",
	"orb42.wav",
	"orb43.wav",
	"orb44.wav",
	"orb45.wav",
	"orb46.wav",
	"orb47.wav",
	"orb48.wav",
	"orb49.wav",
	"orb50.wav",
	"orb51.wav",
	"orb52.wav",
	"orb53.wav",
	"orb54.wav",
	"orb55.wav",
	"orb56.wav",
	"orb57.wav",
	"orb58.wav",
	"orb59.wav",
	"orb60.wav",
	0   /* end of array */
};

struct Samplesinterface orb_samples_interface =
{
	1,
	100,	/* volume */
	orb_sample_names,
	"TSI S14001A"
};


static const char *cat_sample_names[] =
{
	"cat00.wav",
	"cat01.wav",
	"cat02.wav",
	"cat03.wav",
	"cat04.wav",
	"cat05.wav",
	"cat06.wav",
	"cat07.wav",
	"cat08.wav",
	"cat09.wav",
	"cat10.wav",
	"cat11.wav",
	"cat12.wav",
	"cat13.wav",
	"cat14.wav",
	"cat15.wav",
	"cat16.wav",
	"cat17.wav",
	"cat18.wav",
	"cat19.wav",
	"cat20.wav",
	"cat21.wav",
	"cat22.wav",
	"cat23.wav",
	"cat24.wav",
	"cat25.wav",
	"cat26.wav",
	"cat27.wav",
	"cat28.wav",
	"cat29.wav",
	"cat30.wav",
	"cat31.wav",
	"cat32.wav",
	"cat33.wav",
	"cat34.wav",
	"cat35.wav",
	"cat36.wav",
	"cat37.wav",
	"cat38.wav",
	"cat39.wav",
	0   /* end of array */
};

struct Samplesinterface cat_samples_interface =
{
	1,
	100,	/* volume */
	cat_sample_names,
	"TSI S14001A"
};


static const char *fff_sample_names[] =
{
	0   /* end of array */
};

struct Samplesinterface fff_samples_interface =
{
	1,
	100,	/* volume */
	fff_sample_names,
	"TSI S14001A"
};

struct sndbrdst300 snddatst300;

static struct {
  UINT16 timlat1,timlat2,timlat3,timer1,timer2,timer3,t4562c;
  UINT16 timlats1,timlats2,timlats3;
  int    cr1,cr2,cr3,channel,timp1,timp2,timp3,tfre1,tfre2,tfre3,noise,conx,altx,dir;
  int    volnr,reset,extfreq;	
  int    speechsam;
} st300loc;

static  INT16  sineWaveinp[] = {
   0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF,
  -0x7FFF,-0x7FFF,-0x7FFF,-0x7FFF,-0x7FFF,-0x7FFF,-0x7FFF,-0x7FFF
}; // q2 wave 

static  INT16  sineWaveinpq1[] = {
   0x4000, 0x2000, 0x7fff, 0x1000, 0x0000, 0x0000, 0x0000, 0x0000,
  -0x4000,-0x2000,-0x7fff,-0x1000,-0x0000,-0x0000,-0x0000,-0x0000
}; // q1 wave 


static  INT16  volume300[] = {
       0,  0,  0,  0, 0, 10, 20, 20, 30, 30, 40, 40,  60,  60,  80,  90,100 ,100	
};

static  INT16  sineWaveext[32000]; // wave triggered by external clock


static int setvol(int param)	{
  if ((st300loc.volnr) == 0) return 0;
  if (st300loc.dir) { // count up   
//        logerror("volume up %04d ind %04d \n",volume300[st300loc.volnr],st300loc.volnr );
    	return volume300[st300loc.volnr];
  } else {
//        logerror("volume down %04d ind %04d \n",volume300[17-st300loc.volnr],st300loc.volnr );
    	return volume300[17-st300loc.volnr] ;  	
  }  
}
static void startvol(int param)	{
  st300loc.volnr = 1;
 }

static void nextvol(int param) {
  if ((st300loc.volnr > 0) && (st300loc.volnr <= 16)) st300loc.volnr++;	
  if (st300loc.volnr > 16) {
	if (st300loc.conx == 0) {
		if ((st300loc.dir) == 0) {
			st300loc.volnr = 0;	// quiet if we are going down...
		} else {
		}	
	} else {
		startvol(0);
		if (st300loc.altx) {
			 st300loc.dir =  (st300loc.dir ? 0 : 1) ;
		}
	} 
  }
}


static void playsam3(int param) {
// timer 3 (q3) is volume....
   if ((st300loc.cr3 & 0x80) && (st300loc.timlat3 > 0) && (st300loc.reset == 0))   { // output is enabled...
	startvol(0);
	mixer_set_volume(st300loc.channel,setvol(0)*ST300_VOL);
	mixer_set_volume(st300loc.channel+2,setvol(0)*ST300_VOL);	
//	logerror("playsam3 \n");
    } else {	// q3 is not running...
	startvol(0);
	mixer_set_volume(st300loc.channel,setvol(0)*ST300_VOL);
	mixer_set_volume(st300loc.channel+2,setvol(0)*ST300_VOL);	
//	logerror("playsam3 don't move \n");	
    }	
}	

static void playsam2(int param){
// timer 2 (q2) is easy wave + volume from q3
   if ((st300loc.cr2 & 0x80)  && (st300loc.timlat2 > 0) && (st300loc.reset == 0))   { // output is enabled...
 	mixer_play_sample_16(st300loc.channel,sineWaveinp, sizeof(sineWaveinp), st300loc.tfre2*sizeof(sineWaveinp) / 2 / 1.137, 1);
//	logerror("playsam2 %04d \n",st300loc.tfre2);
	}
}	

static void playsam1(int param){
// timer 1 (q1) is easy wave + volume always 100 
   if ((st300loc.cr1 & 0x80)  && (st300loc.timlat1 > 0) && (st300loc.reset == 0))   { // output is enabled...
 	mixer_play_sample_16(st300loc.channel+1,sineWaveinpq1, sizeof(sineWaveinpq1), st300loc.tfre1*sizeof(sineWaveinpq1) / 2 / 1.137, 1);
//	logerror("playsam1 %04d \n",st300loc.tfre1);
	}
}	


static void playsamext(int param){
// external timer + volume from q3
   int f;

//   f = (17-(st300loc.extfreq))*sizeof(sineWaveext);
// Noise is not 100 % accurate, there is a wav file availiable from the author (solenoid 22 - 29)
// formular for f not correct
   f = 625000 / (17-(st300loc.extfreq));

   if (st300loc.noise)   { // output is enabled...
 	mixer_play_sample_16(st300loc.channel+2,sineWaveext, sizeof(sineWaveext), f , 1);
//	logerror("playsamext frequenz %08d data %04d \n",f,st300loc.extfreq);
        } else {
	mixer_stop_sample(st300loc.channel+2);
//	logerror("playsamext stop \n");
   	}
}	 



static void softreset (int param) {
  st300loc.reset = param;
  if (st300loc.reset) { // reset
	snddatst300.timer1 = st300loc.timlat1;
	snddatst300.timer2 = st300loc.timlat2;	
	snddatst300.timer3 = st300loc.timlat3;		
	st300loc.timp1 = 0;
	st300loc.timp2 = 0;
	st300loc.timp2 = 0;
	mixer_stop_sample(st300loc.channel);
	mixer_stop_sample(st300loc.channel+1);
  } else {
  }
}


	
static void st300_pulse (int param) {
// param = 0x02 -> internal 6840 clock
// param = 0 -> external 4049 clock
// timpX is the output level of the 6840 (only 0 or 1)
// decrase timers and update interface
// missig is external clock for the 6840 timers
  if (((st300loc.cr1 & 0x02) == param) && (st300loc.cr1 & 0x80) && (st300loc.reset ==0)) {
  	if (snddatst300.timer1 > 0) {
        	snddatst300.timer1--;
  	}
  	if ((snddatst300.timer1 == 0) && (st300loc.timlat1 != 0)) {
    		snddatst300.timer1 = st300loc.timlat1;
    		if (st300loc.timlat1 != st300loc.timlats1) {
    			playsam1(0);
    		}
    		st300loc.timlats1 = st300loc.timlat1;
    		st300loc.timp1 =  (st300loc.timp1 ? 0 : 1);
    	}
  }
  if (((st300loc.cr2 & 0x02) == param) && (st300loc.cr2 & 0x80) && (st300loc.reset ==0)) {
  	if (snddatst300.timer2 > 0) {
        	snddatst300.timer2--;
  	}
  	if ((snddatst300.timer2 == 0) && (st300loc.timlat2 != 0)) {
    		snddatst300.timer2 = st300loc.timlat2;
    		if (st300loc.timlat2 != st300loc.timlats2) {
    			playsam2(0);
    		}
    		st300loc.timlats2 = st300loc.timlat2;
    		st300loc.timp2 =  (st300loc.timp2 ? 0 : 1);

    	}
  }

  if (((st300loc.cr3 & 0x02) == param) && (st300loc.cr3 & 0x80) && (st300loc.reset ==0)) {
  	if (snddatst300.timer3 > 0) {
        	snddatst300.timer3--;
  	}
  	if ((snddatst300.timer3 == 0) && (st300loc.timlat3 != 0)) {
    		snddatst300.timer3 = st300loc.timlat3;
    		st300loc.timp3 =  (st300loc.timp3 ? 0 : 1);
		if (st300loc.timp3) {
			nextvol(0);
			mixer_set_volume(st300loc.channel,setvol(0)*ST300_VOL);
			mixer_set_volume(st300loc.channel+2,setvol(0)*ST300_VOL);			
    		}
    	}
  }
 
}

static void st300_start_common(void) {
  int i;
  int s = 0;
	
  memset(&st300loc, 0, sizeof(st300loc)); 
  for (i = 0;i < 9;i++) {
    snddatst300.ax[i] = 0;
    snddatst300.axb[i] = 0;  	
    snddatst300.c0 = 0;
  }
  for (i = 0;i < 32000;i++) {
    s =  (s ? 0 : 1);
    if (s) {
      sineWaveext[i] = rand();
    } else
      sineWaveext[i] = 0-rand();
  }
  int mixing_levels[3] = {30,30,30};
  st300loc.channel = mixer_allocate_channels(3, mixing_levels);
  mixer_set_name  (st300loc.channel, "MC6840 #0");
  mixer_set_volume(st300loc.channel,0);	   // the sound from timer pulse q2
  mixer_set_name  (st300loc.channel+1, "MC6840 #1");
  mixer_set_volume(st300loc.channel+1,70*ST300_VOL);  // the sound from timer pulse q1
  mixer_set_name  (st300loc.channel+2, "MC6840 #2");
  mixer_set_volume(st300loc.channel+2,0);  // external timer pulse (after 4536)
  timer_pulse(TIME_IN_HZ(ST300_INTCLOCK),0x02,st300_pulse); // start internal clock
}

static int st300_sh_start(const struct MachineSound *msound)  {
  if (!st300loc.channel) st300_start_common();
  st300loc.speechsam = 0; // no samples for speech
  return 0;
}

static void checksam (int param) {
  if (sample_playing(6) == 0) snddatst300.sampleisplaying = 0;
}

static int st300sam_sh_start(const struct MachineSound *msound)  {
  if (!&st300loc.channel) st300_start_common();
  st300loc.speechsam = 1; // samples for speech used
  timer_pulse(TIME_IN_SEC(0.1),0,checksam); // check if sample is finished
  return 0;
}


static void st300_sh_stop(void) {
}

static struct CustomSound_interface st300_custInt = {st300_sh_start, st300_sh_stop};
static struct CustomSound_interface st300sam_custInt = {st300sam_sh_start, st300_sh_stop};


static struct {
  struct sndbrdData brdData;
} sts_locals;


static WRITE_HANDLER(st300_ctrl_w) {
// cycles for 4536
// c0 -> 00 -> 2 clock cycles
// c0 -> 10 -> 4 clock cycles
// c0 -> 20 -> 8 clock cycles
  int wordsam, wordcmd, wordvol, wordspeed, speedcmd;
  if (offset == 0) {	
     	snddatst300.c0=data;
  	st300loc.extfreq  = ((data & 0xf0) >> 4) + 1;
  	st300loc.noise    =  (data & 0x08) >> 3;
  	st300loc.conx = (data & 0x04) >> 2;
  	st300loc.altx = (data & 0x02) >> 1;  
  	if (snddatst300.c0 & 0x01)  { // count up 
  		st300loc.dir  = 1;
  	} else {
  		st300loc.dir  = 0;
  	}
  	logerror("%04x: st300_CTRL_W adress C0 data %02x noise %04x \n", activecpu_get_previouspc(), data,st300loc.noise);
  	playsamext(0); 
  	playsam3(0);
  } else {
	speedcmd = (data & 0x80) >> 7;
	wordcmd = (data & 0x40) >> 6;	
 	logerror("%04x: st300_CTRL_W xxxx data %02x  \n", activecpu_get_previouspc(), data);
	if (speedcmd) {
		wordspeed = data & 0x07;
		wordvol = (data & 0x38) >> 3;	 
  		logerror("%04x: st300_CTRL_W Voicespeed data %02x speed %02x vol %02x  \n", activecpu_get_previouspc(), data,wordspeed,wordvol);
		
	}
	if (wordcmd)  {
		wordsam = data & 0x3f;
  		logerror("%04x: st300_CTRL_W Voice data %02x sam %02x \n", activecpu_get_previouspc(), data,wordsam);
		if (st300loc.speechsam) 
	    {
// all samples were recorded with speed = 2, vol = 5
			sample_start(6,18 + wordsam,0);
			snddatst300.sampleisplaying = 1;
		}
	}

  }
}
	

	
static WRITE_HANDLER(st300_data_w) {
  int w;
  long int w1;
  if (data == 3) {
	st300loc.timlat1 = snddatst300.ax[data] + snddatst300.ax[(data-1)] * 256;
	snddatst300.timer1 = st300loc.timlat1;
	w1 = ST300_INTCLOCK / (2 * (snddatst300.timer1 + 1));
	st300loc.tfre1 = w1;
//	logerror("%04x: st300_data_w timlat1 loaded %04x  \n", activecpu_get_previouspc(), st300loc.timlat1);
  }
  if (data == 5) {
	st300loc.timlat2 = snddatst300.ax[data] + snddatst300.ax[(data-1)] * 256;
	snddatst300.timer2 = st300loc.timlat2;
	st300loc.tfre2 = ST300_INTCLOCK / (2 * (snddatst300.timer2 + 1));
//        logerror("%04x: st300_data_w timlat2 loaded %04x freq %04d  \n", activecpu_get_previouspc(), st300loc.timlat2,st300loc.tfre2);
  }
  if (data == 7) {
	st300loc.timlat3 = snddatst300.ax[data] + snddatst300.ax[(data-1)] * 256;
	snddatst300.timer3 = st300loc.timlat3;
	st300loc.tfre3 = (ST300_INTCLOCK / (2 * (snddatst300.timer3 + 1)));		
//	logerror("%04x: st300_data_w timlat3 loaded %04x freq %04d  \n", activecpu_get_previouspc(), st300loc.timlat3,st300loc.tfre3);
  }
  if (data == 1)  {
	st300loc.cr2= snddatst300.ax[data];
//	logerror("%04x: st300_data_w CR2 %02x ", activecpu_get_previouspc(), st300loc.cr2);
	if ((st300loc.cr2 & 0x80) == 0) {	
  	}  
	if (st300loc.cr2 & 0x80)  {
		logerror ("Output enabl ");
 		playsam2(0);
	} else {
		logerror ("Output OFF   ");
		mixer_stop_sample(st300loc.channel);
  		st300loc.volnr = 0;
	}
	if (st300loc.cr2 & 0x40)  {
		logerror ("Inter  ENABLE ");
	} else {
		logerror ("Inter  off    ");
	}
	w = (st300loc.cr2 & 0x38) >> 3;
 	logerror ("Mode (N 2)   %01x ",w);
	if (st300loc.cr2 & 0x04)  {
		logerror ("count d 8 ");
	} else {
		logerror ("count 16  ");
	}
	if (st300loc.cr2 & 0x02)  {
		logerror ("int clock ");
	} else {
		logerror ("ext clock ");
	}
	logerror ("\n");
        st300loc.timp2 =  0;
        }

	if (data == 0) {
	        if ((st300loc.cr2 & 0x01) == 0x01) {
			st300loc.cr1 = snddatst300.ax[data];
			logerror("%04x: st300_data_w CR1 %02x ", activecpu_get_previouspc(), st300loc.cr1);
// check reset very early !!!
			if (st300loc.cr1 & 0x01)  {
				logerror ("reset ");
				softreset(1);
			}
			else {
				softreset(0);
				logerror ("normal mode ");
			}
			if (st300loc.cr1 & 0x80)  {
				logerror ("Output enabl ");
  				playsam1(0);
			}
			else {
				logerror ("Output OFF   ");
				mixer_stop_sample(st300loc.channel+1);
			}
			if (st300loc.cr1 & 0x40)  {
				logerror ("Inter  ENABLE ");
			}
			else {
				logerror ("Inter  off    ");
			}
			w = (st300loc.cr1 & 0x38) >> 3;
 			logerror ("Mode (N 2)   %01x ",w);
			if (st300loc.cr1 & 0x04)  {
				logerror ("count d 8 ");
			}
			else {
				logerror ("count 16  ");
			}
			if (st300loc.cr1 & 0x02)  {
				logerror ("int clock ");
			}
			else {
				logerror ("ext clock ");
			}
			logerror ("\n");
        	} else {
			st300loc.cr3 = snddatst300.ax[data];
			logerror("%04x: st300_data_w CR3 %02x ", activecpu_get_previouspc(), st300loc.cr3);
			if (st300loc.cr3 & 0x80)  {
				logerror ("Output enabl ");
			} else {
				logerror ("Output OFFM   ");
			}
			if (st300loc.cr3 & 0x40)  {
				logerror ("Inter  ENABLE ");
			}
			else {
				logerror ("Inter  off    ");
			}
			w = (st300loc.cr3 & 0x38) >> 3;
 			logerror ("Mode (N 2)   %01x ",w);
			if (st300loc.cr3 & 0x04)  {
				logerror ("count d 8 ");
			} else {
				logerror ("count 16  ");
			}
			if (st300loc.cr3 & 0x02)  {
				logerror ("int clock ");
			} else {
				logerror ("ext clock ");
			}
			if (st300loc.cr3 & 0x01)  {
				logerror ("clock / 8 ");
			} else {
				logerror ("clock / 1 ");
			}
			logerror ("\n");
       		}
       }
  }




static WRITE_HANDLER(sts_ctrl_w)
{
	logerror("snd_ctrl_w: %i\n", data);
}

static WRITE_HANDLER(sts_data_w)
{
    logerror("snd_data_w: %i\n", data);
}

static void sts_init(struct sndbrdData *brdData)
{
	memset(&sts_locals, 0x00, sizeof(sts_locals));
	sts_locals.brdData = *brdData;
}

/*-------------------
/ exported interfaces
/--------------------*/
const struct sndbrdIntf st100Intf = {
  "ST100", sts_init, NULL, NULL, sts_data_w, sts_data_w, NULL, sts_ctrl_w, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};

const struct sndbrdIntf st300Intf = {
  "ST300", sts_init, NULL, NULL, st300_data_w, st300_data_w,NULL, st300_ctrl_w, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};

const struct sndbrdIntf astroIntf = {
  "ASTRO", sts_init, NULL, NULL, st300_data_w, st300_data_w,NULL, st300_ctrl_w, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};

MACHINE_DRIVER_START(st300)
  MDRV_SOUND_ADD(CUSTOM, st300_custInt)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(st300f2k)
  MDRV_SOUND_ADD(CUSTOM, st300sam_custInt)
  MDRV_SOUND_ADD(SAMPLES, samples_interface)
  MDRV_SOUND_ADD_TAG("flight2k", SAMPLES, f2k_samples_interface)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(st300fal)
  MDRV_SOUND_ADD(CUSTOM, st300sam_custInt)
  MDRV_SOUND_ADD(SAMPLES, samples_interface)
  MDRV_SOUND_ADD_TAG("freefall", SAMPLES, fff_samples_interface)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(st300lit)
  MDRV_SOUND_ADD(CUSTOM, st300sam_custInt)
  MDRV_SOUND_ADD(SAMPLES, samples_interface)
  MDRV_SOUND_ADD_TAG("lightnin", SAMPLES, lig_samples_interface)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(st300sec)
  MDRV_SOUND_ADD(CUSTOM, st300sam_custInt)
  MDRV_SOUND_ADD(SAMPLES, samples_interface)
  MDRV_SOUND_ADD_TAG("splitsec", SAMPLES, spl_samples_interface)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(st300cat)
  MDRV_SOUND_ADD(CUSTOM, st300sam_custInt)
  MDRV_SOUND_ADD(SAMPLES, samples_interface)
  MDRV_SOUND_ADD_TAG("catacomb", SAMPLES, cat_samples_interface)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(st300orb)
  MDRV_SOUND_ADD(CUSTOM, st300sam_custInt)
  MDRV_SOUND_ADD(SAMPLES, samples_interface)
  MDRV_SOUND_ADD_TAG("orbitor1", SAMPLES, orb_samples_interface)
MACHINE_DRIVER_END
