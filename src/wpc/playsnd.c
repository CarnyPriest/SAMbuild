#include "driver.h"
#include "core.h"
#include "play.h"
#include "sndbrd.h"
#include "cpu/cdp1802/cdp1802.h"
#include "cpu/cop400/cop400.h"
#include "sound/tms5220.h"

/*----------------
/  Local variables
/-----------------*/
static struct {
  UINT8  sndCmd;
  int    ayctrl;
  UINT8  aydata;
  int    enSn;
  int    volume;
  int    channel;
  UINT8  freq;
  int    ef[5];
  mame_timer *timer;
} sndlocals;

static void play1s_timer_callback(int n) {
  if (sndlocals.volume) mixer_set_volume(0, (--sndlocals.volume));
  else timer_adjust(sndlocals.timer, TIME_NEVER, 0, 0);
}

static void play1s_init(struct sndbrdData *brdData) {
  memset(&sndlocals, 0, sizeof sndlocals);
  sndlocals.timer = timer_alloc(play1s_timer_callback);
  sndlocals.volume = 100;
}

static WRITE_HANDLER(play1s_data_w) {
  static UINT8 oldData;
  static int timer_on;
  if (data & 0x0f) {
    if (oldData != data) sndlocals.volume = 100;
    oldData = data;
    discrete_sound_w(8, data & 0x01);
    discrete_sound_w(4, data & 0x02);
    discrete_sound_w(2, data & 0x04);
    discrete_sound_w(1, data & 0x08);
    if (~data & 0x10) { // start fading
      timer_adjust(sndlocals.timer, 0.02, 0, 0.02);
      timer_on = 1;
    } else { // no fading used
      timer_adjust(sndlocals.timer, TIME_NEVER, 0, 0);
      timer_on = 0;
      mixer_set_volume(0, sndlocals.volume);
    }
  } else if (!timer_on) { // no fading going on, so stop sound
    discrete_sound_w(8, 0);
    discrete_sound_w(4, 0);
    discrete_sound_w(2, 0);
    discrete_sound_w(1, 0);
  }
}

static const UINT8 squareWave[] = {
  0x7f, 0x7f, 0x7f, 0x7f, // *******
  0x7f, 0x7f, 0x7f, 0x7f, //       *
  0x7f, 0x7f, 0x7f, 0x7f, //       *
  0x7f, 0x7f, 0x7f, 0x7f, //       *
  0x80, 0x80, 0x80, 0x80, //       *
  0x80, 0x80, 0x80, 0x80, //       *
  0x80, 0x80, 0x80, 0x80, //       *
  0x80, 0x80, 0x80, 0x80  //       *******
};

static void play2s_timer_callback(int n) {
  if (sndlocals.volume) {
    mixer_set_volume(sndlocals.channel, (--sndlocals.volume));
    if (!sndlocals.volume) {
      timer_adjust(sndlocals.timer, TIME_NEVER, 0, 0);
      mixer_stop_sample(sndlocals.channel);
    }
  }
}

static void play2s_init(struct sndbrdData *brdData) {
  memset(&sndlocals, 0, sizeof sndlocals);
  sndlocals.timer = timer_alloc(play2s_timer_callback);
}

static WRITE_HANDLER(play2s_data_w) {
  sndlocals.freq = data;
  if (mixer_is_sample_playing(sndlocals.channel)) {
    mixer_set_sample_frequency(sndlocals.channel, 2950000.0 / 4 / (sndlocals.freq + 1));
  }
}

static WRITE_HANDLER(play2s_ctrl_w) {
  if (!sndlocals.enSn && (data & 1)) { // sound on to full volume
    timer_adjust(sndlocals.timer, TIME_NEVER, 0, 0);
    if (!mixer_is_sample_playing(sndlocals.channel)) {
      mixer_play_sample(sndlocals.channel, (signed char *)squareWave, sizeof(squareWave), 2950000.0 / 4 / (sndlocals.freq + 1), 1);
    }
    sndlocals.volume = 100;
    mixer_set_volume(sndlocals.channel, sndlocals.volume);
  } else if (!(data & 1) && sndlocals.enSn) { // start fading
    timer_adjust(sndlocals.timer, TIME_IN_HZ(120), 0, TIME_IN_HZ(120));
  }
  sndlocals.enSn = data & 1;
}

static WRITE_HANDLER(play2s_man_w) {
  play2s_data_w(0, data);
  play2s_ctrl_w(0, 1);
  play2s_ctrl_w(0, 0);
}

static void play3s_init(struct sndbrdData *brdData) {
  play2s_init(brdData);
  tms5220_reset();
  tms5220_set_variant(TMS5220_IS_5200);
  cpu_set_irq_line(PLAYMATIC_SCPU, CDP1802_INPUT_LINE_INT, CLEAR_LINE);
  sndlocals.ef[3] = sndlocals.ef[4] = 1;
}

static WRITE_HANDLER(play3s_ctrl_w) {
  sndlocals.sndCmd = data;
  sndlocals.ef[2] = (sndlocals.sndCmd & 0x70) ? 0 : 1;
  play2s_ctrl_w(0, data);
}

static void play4s_timer_callback(int n) {
  cpu_set_irq_line(PLAYMATIC_SCPU, CDP1802_INPUT_LINE_INT, CLEAR_LINE);
  cpu_set_irq_line(PLAYMATIC_SCPU, CDP1802_INPUT_LINE_INT, ASSERT_LINE);
}

static void play4s_init(struct sndbrdData *brdData) {
  memset(&sndlocals, 0, sizeof sndlocals);
  sndlocals.timer = timer_alloc(play4s_timer_callback);
  timer_adjust(sndlocals.timer, TIME_IN_HZ(3496), 0, TIME_IN_HZ(3496)); // ought to be correct according to 1863 datasheet
}

static WRITE_HANDLER(play4s_data_w) {
  logerror("snd data: %02x\n", data);
  sndlocals.sndCmd = data;
}

static WRITE_HANDLER(play4s_ctrl_w) {
  logerror("snd en: %x\n", data);
  sndlocals.enSn = data & 1;
}

static WRITE_HANDLER(play4s_man_w) {
  play4s_data_w(0, data);
  play4s_ctrl_w(0, 0);
}

/*-------------------
/ exported interfaces
/--------------------*/
const struct sndbrdIntf play1sIntf = {
  "PLAY1", play1s_init, NULL, NULL, play1s_data_w, play1s_data_w, NULL, NULL, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};
const struct sndbrdIntf play2sIntf = {
  "PLAY2", play2s_init, NULL, NULL, play2s_man_w, play2s_data_w, NULL, play2s_ctrl_w, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};
const struct sndbrdIntf playzsIntf = {
  "PLAYZ", play2s_init, NULL, NULL, play3s_ctrl_w, play2s_data_w, NULL, play3s_ctrl_w, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};
const struct sndbrdIntf play3sIntf = {
  "PLAY3", play3s_init, NULL, NULL, play3s_ctrl_w, play2s_data_w, NULL, play3s_ctrl_w, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};
const struct sndbrdIntf play4sIntf = {
  "PLAY4", play4s_init, NULL, NULL, play4s_man_w, play4s_data_w, NULL, play4s_ctrl_w, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};

static UINT8 snd_mode(void) { return CDP1802_MODE_RUN; }

static DISCRETE_SOUND_START(play_tones)
	DISCRETE_INPUT(NODE_01,1,0x000f,0)                         // Input handlers, mostly for enable
	DISCRETE_INPUT(NODE_02,2,0x000f,0)
	DISCRETE_INPUT(NODE_04,4,0x000f,0)
	DISCRETE_INPUT(NODE_08,8,0x000f,0)

	DISCRETE_TRIANGLEWAVE(NODE_10,NODE_01,523,50000,10000,0) // C' note
	DISCRETE_TRIANGLEWAVE(NODE_20,NODE_02,659,50000,10000,0) // E' note
	DISCRETE_TRIANGLEWAVE(NODE_30,NODE_04,784,50000,10000,0) // G' note
	DISCRETE_TRIANGLEWAVE(NODE_40,NODE_08,988,50000,10000,0) // H' note

	DISCRETE_ADDER4(NODE_50,1,NODE_10,NODE_20,NODE_30,NODE_40) // Mix all four sound sources

	DISCRETE_OUTPUT(NODE_50, 75)                               // Take the output from the mixer
DISCRETE_SOUND_END

MACHINE_DRIVER_START(PLAYMATICS1)
  MDRV_SOUND_ADD(DISCRETE, play_tones)
MACHINE_DRIVER_END

static int play2s_start(const struct MachineSound *msound) {
  if (Machine->gamedrv->flags & GAME_NO_SOUND) {
    return -1;
  }
  sndlocals.channel = mixer_allocate_channel(15);
  return 0;
}

static void play2s_stop(void) {
}

static struct CustomSound_interface play2s_custInt = {play2s_start, play2s_stop};

MACHINE_DRIVER_START(PLAYMATICS2)
  MDRV_SOUND_ADD(CUSTOM, play2s_custInt)
MACHINE_DRIVER_END

static void play_5220Irq(int state) {
  cpu_set_irq_line(PLAYMATIC_SCPU, CDP1802_INPUT_LINE_INT, state ? CLEAR_LINE : ASSERT_LINE);
printf(" I%x ", state);
}
static void play_5220Rdy(int state) {
  sndlocals.ef[1] = state;
printf(" R%x ", state);
}
static struct TMS5220interface play3s_5220Int = {
  640000,
  75,
  play_5220Irq,
  play_5220Rdy
};

static READ_HANDLER(in_snd_3) {
  return (sndlocals.sndCmd >> 4) & 0x07;
}

static int q;
static WRITE_HANDLER(out_snd_3) {
  if (!q) tms5220_data_w(0, data);
printf("o");
}

static UINT8 snd_ef3(void) {
  sndlocals.ef[1] = !tms5220_ready_r();
  return sndlocals.ef[1] | (sndlocals.ef[2] << 1) | (sndlocals.ef[3] << 2) | (sndlocals.ef[4] << 3);
}

static void snd_q(int data) {
  q = data;
  if (data) {
    UINT8 val = tms5220_status_r(0);
    sndlocals.ef[3] = !((val & 0x40) >> 6);
    sndlocals.ef[4] = ((val & 0x80) >> 7);
printf("i");
  } else {
    sndlocals.ef[3] = sndlocals.ef[4] = 1;
printf("-");
  }
}

static CDP1802_CONFIG playsound_config3 =
{
	snd_mode,	// MODE
	snd_ef3,		// EF
	NULL,		// SC
	snd_q,		// Q
	NULL,		// DMA read
	NULL		// DMA write
};

static MEMORY_READ_START(playsound_readmem3)
  {0x0000,0x1fff, MRA_ROM},
  {0x2000,0x201f, MRA_RAM},
MEMORY_END

static MEMORY_WRITE_START(playsound_writemem3)
  {0x2000,0x201f, MWA_RAM},
MEMORY_END

static PORT_READ_START(playsound_readport3)
  {0x02, 0x02, in_snd_3},
MEMORY_END

static PORT_WRITE_START(playsound_writeport3)
  {0x01, 0x01, out_snd_3},
MEMORY_END

MACHINE_DRIVER_START(PLAYMATICS3)
  MDRV_CPU_ADD_TAG("scpu", CDP1802, 2950000)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_CONFIG(playsound_config3)
  MDRV_CPU_MEMORY(playsound_readmem3, playsound_writemem3)
  MDRV_CPU_PORTS(playsound_readport3, playsound_writeport3)
  MDRV_SOUND_ADD(CUSTOM, play2s_custInt)
  MDRV_SOUND_ADD(TMS5220, play3s_5220Int)
MACHINE_DRIVER_END

static WRITE_HANDLER(ay8910_0_porta_w)	{
  int volume = 100 - 25 * (data >> 6);
  AY8910_set_volume(0, 0, volume);
  AY8910_set_volume(0, 1, volume);
  AY8910_set_volume(0, 2, volume);
  // TODO bits 1 to 6 control a slight reverb effect!?
}
static WRITE_HANDLER(ay8910_1_porta_w)	{
  int volume = 100 - 25 * (data >> 6);
  AY8910_set_volume(1, 0, volume);
  AY8910_set_volume(1, 1, volume);
  AY8910_set_volume(1, 2, volume);
  // TODO bits 1 to 6 control a slight reverb effect!?
}
struct AY8910interface play4s_8910Int = {
	2,			/* 2 chips */
	3579545.0/2,	/* 1.79 MHz */
	{ MIXER(50,MIXER_PAN_LEFT), MIXER(50,MIXER_PAN_RIGHT) },	/* Volume */
	{ 0, 0 },
	{ 0, 0 },
	{ ay8910_0_porta_w, ay8910_1_porta_w }
};

static WRITE_HANDLER(ay0_w) {
	AY8910Write(0,offset % 2,data);
}

static WRITE_HANDLER(ay1_w) {
	AY8910Write(1,offset % 2,data);
}

static READ_HANDLER(in_snd_4) {
  sndlocals.enSn = 1;
  return sndlocals.sndCmd;
}

static WRITE_HANDLER(clk_snd) {
  logerror("snd clk: %02x\n", data);
  timer_adjust(sndlocals.timer, TIME_IN_HZ((3579545 >> 6) / (data + 1)), 0, TIME_IN_HZ((3579545 >> 6) / (data + 1))); // too fast? but sounds right!
}

static UINT8 snd_ef4(void) {
  return sndlocals.enSn;
}

static void snd_sc(int data) {
  if (data & 2) {
    cpu_set_irq_line(PLAYMATIC_SCPU, CDP1802_INPUT_LINE_INT, CLEAR_LINE);
    timer_enable(sndlocals.timer, 0);
  } else {
    timer_enable(sndlocals.timer, 1);
  }
}

static CDP1802_CONFIG playsound_config4 =
{
	snd_mode,	// MODE
	snd_ef4,		// EF
	snd_sc,		// SC
	NULL,		// Q
	NULL,		// DMA read
	NULL		// DMA write
};

static MEMORY_READ_START(playsound_readmem4)
  {0x0000,0x3fff, MRA_ROM},
  {0x4000,0x5fff, AY8910_read_port_0_r},
  {0x6000,0x7fff, AY8910_read_port_1_r},
  {0x8000,0x80ff, MRA_RAM},
MEMORY_END

static MEMORY_WRITE_START(playsound_writemem4)
  {0x4000,0x5fff, ay0_w},
  {0x6000,0x7fff, ay1_w},
  {0x8000,0x80ff, MWA_RAM},
MEMORY_END

static PORT_READ_START(playsound_readport4)
  {0x02, 0x02, in_snd_4},
MEMORY_END

static PORT_WRITE_START(playsound_writeport4)
  {0x01, 0x01, clk_snd},
MEMORY_END

MACHINE_DRIVER_START(PLAYMATICS4)
  MDRV_CPU_ADD_TAG("scpu", CDP1802, 3579545)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_CONFIG(playsound_config4)
  MDRV_CPU_MEMORY(playsound_readmem4, playsound_writemem4)
  MDRV_CPU_PORTS(playsound_readport4, playsound_writeport4)
  MDRV_SOUND_ADD(AY8910, play4s_8910Int)
  MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
MACHINE_DRIVER_END

static READ_HANDLER(in_snd_z) {
  return (~sndlocals.sndCmd >> 4) & 0x07;
}

static WRITE_HANDLER(ay_data_w) {
  sndlocals.aydata = data;
//printf("d:%x:%02x\n", sndlocals.ayctrl, sndlocals.aydata);
}

static WRITE_HANDLER(ay_ctrl_w) {
  sndlocals.ayctrl = data & 3;
  switch (sndlocals.ayctrl) {
    case 1: AY8910_write_port_0_w(0, sndlocals.aydata); break;
    case 2:
    case 3: AY8910_control_port_0_w(0, sndlocals.aydata); break;
  }
//printf("c:%x:%02x\n", sndlocals.ayctrl, sndlocals.aydata);
}

static READ_HANDLER(ay_data_r) {
  static int readcnt;
  UINT8 data = AY8910Read(0);
//printf("r:%x:%02x\n", sndlocals.ayctrl, data);
  if (++readcnt > 750) { // to avoid CPU getting stuck forever, reset it
    readcnt = 0;
    cop420_reset(0);
  }
  return data;
}

static WRITE_HANDLER(ay8910_z_porta_w)	{
  coreGlobals.lampMatrix[8] = data;
}
static WRITE_HANDLER(ay8910_z_portb_w)	{
  coreGlobals.solenoids2 = data;
}
struct AY8910interface playzs_8910Int = {
	1,			/* 1 chip */
	2000000,	/* 2.01216 MHz quartz on pic! */
	{ 25 },	/* Volume */
	{ 0 },
	{ 0 },
	{ ay8910_z_porta_w },
	{ ay8910_z_portb_w },
};

static WRITE_HANDLER(romsel_w) {
  if (data & 0x08) {
    logerror("Selecting 2nd ROM bank!\n");
  }
}

static MEMORY_READ_START(playsound_readmemz)
  {0x0000,0x03ff, MRA_ROM},
MEMORY_END

static MEMORY_WRITE_START(playsound_writememz)
MEMORY_END

static PORT_READ_START(playsound_readportz)
  {COP400_PORT_L, COP400_PORT_L, ay_data_r},
  {COP400_PORT_IN, COP400_PORT_IN, in_snd_z},
MEMORY_END

static PORT_WRITE_START(playsound_writeportz)
  {COP400_PORT_L, COP400_PORT_L, ay_data_w },
  {COP400_PORT_G, COP400_PORT_G, ay_ctrl_w },
  {COP400_PORT_D, COP400_PORT_D, romsel_w },
MEMORY_END

MACHINE_DRIVER_START(PLAYMATICSZ)
  MDRV_CPU_ADD_TAG("scpu", COP420, 2000000 / COP400_CLOCK_DIVIDER)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(playsound_readmemz, playsound_writememz)
  MDRV_CPU_PORTS(playsound_readportz, playsound_writeportz)
  MDRV_SOUND_ADD(CUSTOM, play2s_custInt)
  MDRV_SOUND_ADD(AY8910, playzs_8910Int)
MACHINE_DRIVER_END
