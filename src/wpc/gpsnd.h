#ifndef INC_GPSND
#define INC_GPSND

/* Game Plan Sound Hardware Info:

  Sound System
  ------------
  M6802
  2 x PIA 6821
  M6840 Timer
*/

extern MACHINE_DRIVER_EXTERN(gpMSU1);

#define GP_SOUNDROM8(u9,chk9) \
  SOUNDREGION(0x10000, GP_MEMREG_SCPU) \
    ROM_LOAD(u9, 0x3800, 0x0800, chk9) \
      ROM_RELOAD(0xf800, 0x0800)

#define GP_SOUNDROM88(u9,chk9,u10,chk10) \
  SOUNDREGION(0x10000, GP_MEMREG_SCPU) \
    ROM_LOAD(u9, 0x3800, 0x0800, chk9) \
      ROM_RELOAD(0xf800, 0x0800) \
    ROM_LOAD(u10,0x3000, 0x0800, chk10)

#define GP_SOUNDROM0(u9,chk9) \
  SOUNDREGION(0x10000, GP_MEMREG_SCPU) \
    ROM_LOAD(u9, 0x3000, 0x1000, chk9) \
      ROM_RELOAD(0x7000, 0x1000) \
      ROM_RELOAD(0xf000, 0x1000)

#endif /* INC_GPSND */
