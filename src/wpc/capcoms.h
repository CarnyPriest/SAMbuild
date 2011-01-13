#ifndef INC_CAPCOMSOUND
#define INC_CAPCOMSOUND
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

#define CAPCOMS_CPUNO 1
#define CAPCOMS_CPUREGION (REGION_CPU1+CAPCOMS_CPUNO)
#define CAPCOMS_ROMREGION (REGION_SOUND1)

extern MACHINE_DRIVER_EXTERN(capcom1s);
extern MACHINE_DRIVER_EXTERN(capcom2s);

/*-- Sound rom macros --*/

//NOTE: SOUND CPU requires 128K of region space, since the ROM is mapped in the lower 64K, and the RAM in the upper 64K
#define CPU_REGION(reg)		 SOUNDREGION(0x20000, reg)

//IMPORTANT: ALL UNPOPULATED ROMS MUST BE FILLED WITH 0xFF TO PASS ROM CHECKS!

/*-- 64K Sound CPU Rom, 1 X 512K, 1 X 128K MPG Roms --*/
#define CAPCOMS_SOUNDROM2(n1,chk1,n2,chk2,n3,chk3) \
  CPU_REGION(CAPCOMS_CPUREGION) \
    ROM_LOAD(n1, 0x0000,  0x2000, chk1) \
  SOUNDREGION(0x400000, CAPCOMS_ROMREGION) \
    ROM_LOAD(n2,  0       ,  0x80000, chk2) \
	ROM_LOAD(n3,  0x100000,  0x20000, chk3) \
	ROM_FILL(     0x200000,  0x100000,0xff) \
	ROM_FILL(     0x300000,  0x100000,0xff)

/*-- 64K Sound CPU Rom, 1 X 512K, 2 X 128K MPG Roms --*/
#define CAPCOMS_SOUNDROM2a(n1,chk1,n2,chk2,n3,chk3,n4,chk4) \
  CPU_REGION(CAPCOMS_CPUREGION) \
    ROM_LOAD(n1, 0x0000,  0x2000, chk1) \
  SOUNDREGION(0x400000, CAPCOMS_ROMREGION) \
    ROM_LOAD(n2,  0       ,  0x80000, chk2) \
	ROM_LOAD(n3,  0x100000,  0x20000, chk3) \
	ROM_LOAD(n4,  0x200000,  0x20000, chk4) \
	ROM_FILL(     0x300000,  0x100000,0xff)

/*-- 64K Sound CPU Rom, 1 X 512K MPG Rom --*/
#define CAPCOMS_SOUNDROM2b(n1,chk1,n2,chk2) \
  CPU_REGION(CAPCOMS_CPUREGION) \
    ROM_LOAD(n1, 0x0000,  0x2000, chk1) \
  SOUNDREGION(0x400000, CAPCOMS_ROMREGION) \
    ROM_LOAD(n2,  0       ,  0x80000, chk2) \
	ROM_FILL(     0x100000,  0x100000,0xff) \
	ROM_FILL(     0x200000,  0x100000,0xff) \
	ROM_FILL(     0x300000,  0x100000,0xff)

/*-- 64K Sound CPU Rom, 2 X 1024K, 1 X 512K MPG Roms --*/
#define CAPCOMS_SOUNDROM3(n1,chk1,n2,chk2,n3,chk3,n4,chk4) \
  CPU_REGION(CAPCOMS_CPUREGION) \
    ROM_LOAD(n1, 0x0000,  0x2000, chk1) \
  SOUNDREGION(0x400000, CAPCOMS_ROMREGION) \
    ROM_LOAD(n2,  0       ,  0x100000, chk2) \
	ROM_LOAD(n3,  0x100000,  0x100000, chk3) \
	ROM_LOAD(n4,  0x200000,  0x80000, chk4) \
	ROM_FILL(     0x300000,  0x100000,0xff)

/*-- 64K Sound CPU Rom, 2 X 1024K, 1 X 512K, 1 X 128K MPG Roms --*/
#define CAPCOMS_SOUNDROM3a(n1,chk1,n2,chk2,n3,chk3,n4,chk4,n5,chk5) \
  CPU_REGION(CAPCOMS_CPUREGION) \
    ROM_LOAD(n1, 0x0000,  0x2000, chk1) \
  SOUNDREGION(0x400000, CAPCOMS_ROMREGION) \
    ROM_LOAD(n2,  0       ,  0x100000, chk2) \
	ROM_LOAD(n3,  0x100000,  0x100000, chk3) \
	ROM_LOAD(n4,  0x200000,  0x80000, chk4) \
	ROM_LOAD(n5,  0x300000,  0x20000, chk5)

/*-- 64K Sound CPU Rom, 1 X 1024K, 3 X 512K MPG Roms --*/
#define CAPCOMS_SOUNDROM4a(n1,chk1,n2,chk2,n3,chk3,n4,chk4,n5,chk5) \
  CPU_REGION(CAPCOMS_CPUREGION) \
     ROM_LOAD(n1, 0x0000,  0x2000, chk1) \
  SOUNDREGION(0x400000, CAPCOMS_ROMREGION) \
    ROM_LOAD(n2,  0       ,  0x100000, chk2) \
	ROM_LOAD(n3,  0x100000,  0x80000, chk3) \
	ROM_LOAD(n4,  0x200000,  0x80000, chk4) \
	ROM_LOAD(n5,  0x300000,  0x80000, chk5)

/*-- 64K Sound CPU Rom, 2 X 1024K, 2 X 512K MPG Roms --*/
#define CAPCOMS_SOUNDROM4b(n1,chk1,n2,chk2,n3,chk3,n4,chk4,n5,chk5) \
  CPU_REGION(CAPCOMS_CPUREGION) \
     ROM_LOAD(n1, 0x0000,  0x2000, chk1) \
  SOUNDREGION(0x400000, CAPCOMS_ROMREGION) \
    ROM_LOAD(n2,  0       ,  0x100000, chk2) \
	ROM_LOAD(n3,  0x100000,  0x100000, chk3) \
	ROM_LOAD(n4,  0x200000,  0x80000, chk4) \
	ROM_LOAD(n5,  0x300000,  0x80000, chk5)

#endif /* INC_CAPCOMSOUND */
