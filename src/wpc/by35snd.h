#ifndef INC_BY35SND
#define INC_BY35SND

/*Bally Sound Hardware Info:

  Sound module -32 & -50
  ---------------------
  Custom Chips

  Sounds Plus -51
  ---------------
  M6802/M6808
  AY8910 Sound Generator
  ?NO DAC?

  Sounds Plus -56, Vocalizer -57
  ------------------------------
  M6802/M6808
  AY8910 Sound Generator
  HC55516 DAC

  Squalk N Talk -61
  -------------
  M6809 CPU
  TMS5220 Speach Generator
  AY8910 Sound Generator
  DAC

  Cheap Squeak  -45
  ------------
  M6803 CPU
  DAC
*/

/* Squawk n Talk */
extern const struct Memory_ReadAddress  snt_readmem[];
extern const struct Memory_WriteAddress snt_writemem[];
extern struct TMS5220interface    snt_tms5220Int;
extern struct DACinterface        snt_dacInt;
extern struct AY8910interface     snt_ay8910Int;

#define BY61_SOUND_CPU { \
  CPU_M6802 | CPU_AUDIO_CPU, 3580000/4,	/* .8 MHz */					\
  snt_readmem, snt_writemem, 0, 0, \
  ignore_interrupt,1 \
}
#define BY61_SOUND { SOUND_TMS5220, &snt_tms5220Int },\
                   { SOUND_DAC,     &snt_dacInt }, \
                   { SOUND_AY8910,  &snt_ay8910Int }, \
                   SAMPLESINTERFACE

#define BY61_SOUNDROMxxx0(n1,chk1) \
  SOUNDREGION(0x10000, BY35_MEMREG_S1CPU) \
    ROM_LOAD(n1, 0xf000, 0x1000, chk1)

#define BY61_SOUNDROM0xx0(n1,chk1,n2,chk2) \
  SOUNDREGION(0x10000, BY35_MEMREG_S1CPU) \
    ROM_LOAD(n1, 0xc000, 0x1000, chk1) \
    ROM_LOAD(n2, 0xf000, 0x1000, chk2)

#define BY61_SOUNDROMxx80(n1,chk1,n2,chk2) \
  SOUNDREGION(0x10000, BY35_MEMREG_S1CPU) \
    ROM_LOAD(n1, 0xe000, 0x0800, chk1) \
    ROM_RELOAD(  0xe800, 0x0800) \
    ROM_LOAD(n2, 0xf000, 0x1000, chk2)

#define BY61_SOUNDROMxx00(n1,chk1,n2,chk2) \
  SOUNDREGION(0x10000, BY35_MEMREG_S1CPU) \
    ROM_LOAD(n1, 0xe000, 0x1000, chk1) \
    ROM_LOAD(n2, 0xf000, 0x1000, chk2)

#define BY61_SOUNDROMx080(n1,chk1,n2,chk2,n3,chk3) \
  SOUNDREGION(0x10000, BY35_MEMREG_S1CPU) \
    ROM_LOAD(n1, 0xd000, 0x1000, chk1) \
    ROM_LOAD(n2, 0xe000, 0x0800, chk2) \
    ROM_RELOAD(  0xe800, 0x0800) \
    ROM_LOAD(n3, 0xf000, 0x1000, chk3)

#define BY61_SOUNDROMx008(n1,chk1,n2,chk2,n3,chk3) \
  SOUNDREGION(0x10000, BY35_MEMREG_S1CPU) \
    ROM_LOAD(n1, 0xd000, 0x1000, chk1) \
    ROM_LOAD(n2, 0xe000, 0x1000, chk2) \
    ROM_LOAD(n3, 0xf000, 0x0800, chk3) \
    ROM_RELOAD(  0xf800, 0x0800)

#define BY61_SOUNDROMx000(n1,chk1,n2,chk2,n3,chk3) \
  SOUNDREGION(0x10000, BY35_MEMREG_S1CPU) \
    ROM_LOAD(n1, 0xd000, 0x1000, chk1) \
    ROM_LOAD(n2, 0xe000, 0x1000, chk2) \
    ROM_LOAD(n3, 0xf000, 0x1000, chk3)

#define BY61_SOUNDROM0000(n1,chk1,n2,chk2,n3,chk3,n4,chk4) \
  SOUNDREGION(0x10000, BY35_MEMREG_S1CPU) \
    ROM_LOAD(n1, 0xc000, 0x1000, chk1) \
    ROM_LOAD(n2, 0xd000, 0x1000, chk2) \
    ROM_LOAD(n3, 0xe000, 0x1000, chk3) \
    ROM_LOAD(n4, 0xf000, 0x1000, chk4)

/* -32, -50 Sound module */
extern struct CustomSound_interface by32_custInt;

#define BY32_SOUND {SOUND_CUSTOM,&by32_custInt}, SAMPLESINTERFACE

#define BY32_SOUNDROM(n1,chk1) \
  SOUNDREGION(0x0020, BY35_MEMREG_SROM) \
    ROM_LOAD( n1, 0x0000, 0x0020, chk1)

#define BY50_SOUNDROM(n1,chk1) \
  SOUNDREGION(0x0020, BY35_MEMREG_SROM) \
    ROM_LOAD( n1, 0x0000, 0x0020, chk1)

/* Sounds Plus -51 */
extern const struct Memory_ReadAddress sp51_readmem[];
#define BY51_SOUND { SOUND_AY8910, &sp_ay8910Int }, SAMPLESINTERFACE

#define BY51_SOUND_CPU { \
  CPU_M6802 | CPU_AUDIO_CPU, 3580000/4,	/* .8 MHz */					\
  sp51_readmem, sp_writemem, 0, 0, \
  ignore_interrupt,1 \
}
/* Sounds Plus -56, Vocalizer -57 */
extern const struct Memory_ReadAddress sp56_readmem[];
extern struct hc55516_interface sp_hc55516Int;

#define BY56_SOUND { SOUND_AY8910, &sp_ay8910Int }, \
                   { SOUND_HC55516, &sp_hc55516Int }, \
                   SAMPLESINTERFACE

#define BY56_SOUND_CPU { \
  CPU_M6802 | CPU_AUDIO_CPU, 3580000/4,	/* .8 MHz */					\
  sp56_readmem, sp_writemem, 0, 0, \
  ignore_interrupt,1 \
}

/* Common -51, -56/-57 */
extern const struct Memory_WriteAddress sp_writemem[];
extern struct AY8910interface  sp_ay8910Int;

#define BY51_SOUNDROM8(n1,chk1) \
  SOUNDREGION(0x10000, BY35_MEMREG_S1CPU) \
    ROM_LOAD(n1, 0xf000, 0x0800, chk1) \
    ROM_RELOAD(  0xf800, 0x0800) \
    ROM_RELOAD(  0x1000, 0x0800) \
    ROM_RELOAD(  0x1800, 0x0800)

#define BY51_SOUNDROM0(n1,chk1) \
  SOUNDREGION(0x10000, BY35_MEMREG_S1CPU) \
    ROM_LOAD(n1, 0xf000, 0x1000, chk1) \
    ROM_RELOAD(  0x1000, 0x1000)

#define BY56_SOUNDROM(n1,chk1) \
  SOUNDREGION(0x10000, BY35_MEMREG_S1CPU) \
    ROM_LOAD(n1, 0xf000, 0x1000, chk1)

#define BY57_SOUNDROM(n1,chk1,n2,chk2,n3,chk3,n4,chk4,n5,chk5,n6,chk6,n7,chk7) \
    ROM_LOAD(n1, 0x8000, 0x1000, chk1) \
    ROM_LOAD(n2, 0x9000, 0x1000, chk2) \
    ROM_LOAD(n3, 0xa000, 0x1000, chk3) \
    ROM_LOAD(n4, 0xb000, 0x1000, chk4) \
    ROM_LOAD(n5, 0xc000, 0x1000, chk5) \
    ROM_LOAD(n6, 0xd000, 0x1000, chk6) \
    ROM_LOAD(n7, 0xe000, 0x1000, chk7)

/* Cheap Squeak -45 */
extern const struct Memory_ReadAddress cs_readmem[];
extern const struct Memory_WriteAddress cs_writemem[];
extern const struct IO_ReadPort cs_readport[];
extern const struct IO_WritePort cs_writeport[];
extern struct DACinterface cs_dacInt;

#define BY45_SOUND { SOUND_DAC, &cs_dacInt }, SAMPLESINTERFACE

#define BY45_SOUND_CPU { \
  CPU_M6803 | CPU_AUDIO_CPU, 3580000/4,	/* .8 MHz */  			  \
  cs_readmem, cs_writemem, cs_readport, cs_writeport, \
  ignore_interrupt,1 \
}

#define BY45_SOUNDROMx2(n1,chk1) \
  SOUNDREGION(0x10000, BY35_MEMREG_S1CPU) \
    ROM_LOAD(n1, 0xc000, 0x2000, chk1) \
    ROM_RELOAD(  0xe000, 0x2000)

/* 2 x 4K ROMS */
#define BY45_SOUNDROM11(n2,chk2,n1,chk1) \
  SOUNDREGION(0x10000, BY35_MEMREG_S1CPU) \
    ROM_LOAD(n1,0x8000, 0x1000, chk1) \
    ROM_RELOAD( 0x9000, 0x1000) \
    ROM_RELOAD( 0xa000, 0x1000) \
    ROM_RELOAD( 0xb000, 0x1000) \
    ROM_LOAD(n2,0xc000, 0x1000, chk2) \
    ROM_RELOAD( 0xd000, 0x1000) \
    ROM_RELOAD( 0xe000, 0x1000) \
    ROM_RELOAD( 0xf000, 0x1000)

/* 2 x 8K ROMS */
#define BY45_SOUNDROM22(n2,chk2,n1,chk1) \
  SOUNDREGION(0x10000, BY35_MEMREG_S1CPU) \
    ROM_LOAD(n1, 0x8000, 0x2000, chk1) \
      ROM_RELOAD(0xa000, 0x2000) \
    ROM_LOAD(n2, 0xc000, 0x2000, chk2) \
      ROM_RELOAD(0xe000, 0x2000)

/* Turbo Cheap Squeak */
extern const struct Memory_ReadAddress  tcs_readmem[];
extern const struct Memory_WriteAddress tcs_writemem[];
extern const struct Memory_ReadAddress  tcs2_readmem[];
extern const struct Memory_WriteAddress tcs2_writemem[];
extern struct DACinterface        tcs_dacInt;

#define BYTCS_SOUND { SOUND_DAC, &tcs_dacInt }, SAMPLESINTERFACE

#define BYTCS_SOUND_CPU { \
  CPU_M6809 | CPU_AUDIO_CPU, 2000000,	/* 2MHz */					\
  tcs_readmem, tcs_writemem, 0, 0, \
  ignore_interrupt,1 \
}
#define BYTCS2_SOUND_CPU { \
  CPU_M6809 | CPU_AUDIO_CPU, 2000000,	/* 2MHz */					\
  tcs2_readmem, tcs2_writemem, 0, 0, \
  ignore_interrupt,1 \
}

/* Sounds Delux */
extern const struct Memory_ReadAddress16  sd_readmem[];
extern const struct Memory_WriteAddress16 sd_writemem[];
extern struct DACinterface          sd_dacInt;

#define BYSD_SOUND_CPU { \
  CPU_M68000 | CPU_AUDIO_CPU, 8000000,	/* 8MHz */					\
  sd_readmem, sd_writemem, 0, 0, \
  ignore_interrupt,1 \
}
#define BYSD_SOUND { SOUND_DAC, &sd_dacInt }, SAMPLESINTERFACE

/* generic handler */
void by35_soundInit(void);
void by35_soundExit(void);
WRITE_HANDLER(by35_soundCmd);
#endif /* INC_BY35SND */


