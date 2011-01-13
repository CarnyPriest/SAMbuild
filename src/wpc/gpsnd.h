#ifndef INC_GPSND
#define INC_GPSND
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

/* Game Plan Sound Hardware Info:

  Sound System
  ------------
  M6802
  2 x PIA 6821
  M6840 Timer
*/

extern MACHINE_DRIVER_EXTERN(gpSSU1);
extern MACHINE_DRIVER_EXTERN(gpSSU2);
extern MACHINE_DRIVER_EXTERN(gpSSU4);
extern MACHINE_DRIVER_EXTERN(gpMSU1);
extern MACHINE_DRIVER_EXTERN(gpMSU3);

#define GP_SOUNDROM8(u9,chk9) \
  SOUNDREGION(0x10000, GP_MEMREG_SCPU) \
    ROM_LOAD(u9, 0x3800, 0x0800, chk9) \
      ROM_RELOAD(0x7800, 0x0800) \
      ROM_RELOAD(0xf800, 0x0800)

#define GP_SOUNDROM88(u9,chk9,u10,chk10) \
  SOUNDREGION(0x10000, GP_MEMREG_SCPU) \
    ROM_LOAD(u9, 0x3800, 0x0800, chk9) \
      ROM_RELOAD(0xf800, 0x0800) \
    ROM_LOAD(u10,0x3000, 0x0800, chk10)

#define GP_SOUNDROM0(u9,chk9) \
  SOUNDREGION(0x10000, GP_MEMREG_SCPU) \
    ROM_LOAD  (u9, 0x3800, 0x0800, chk9) \
      ROM_CONTINUE(0x7800, 0x0800) \
      ROM_RELOAD  (0xf000, 0x1000)

#endif /* INC_GPSND */
