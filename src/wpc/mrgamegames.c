#include "driver.h"
#include "sim.h"
#include "mrgame.h"
#include "sndbrd.h"

#define GEN_MRGAME GEN_ALVG


/* 10' Color Video Monitor */
core_tLCDLayout mrgame_disp[] = {
  {0,0,248,256,CORE_VIDEO,(void *)mrgame_update},{0}
};

#define INITGAME(name, disptype, flippers, balls, sb, db) \
	MRGAME_INPUT_PORTS_START(name, balls) MRGAME_INPUT_PORTS_END \
	static core_tGameData name##GameData = {GEN_MRGAME,disptype,{flippers,0,2,0,sb,db}}; \
	static void init_##name(void) { \
		core_gameData = &name##GameData; \
	}

/* GAMES APPEAR IN PRODUCTION ORDER (MORE OR LESS) */

/*-------------------------------------------------------------------
/ Dakar (1988?)
/-------------------------------------------------------------------*/
INITGAME(dakar, mrgame_disp, FLIP_SWNO(65,64), 4/*?*/, SNDBRD_MRGAME, 0)
MRGAME_ROMSTART(dakar,	"cpu_ic13.rom", CRC(83183929) SHA1(977ac10a1e78c759eb0550794f2639fe0e2d1507),
						"cpu_ic14.rom", CRC(2010d28d) SHA1(d262dabd9298566df43df298cf71c974bee1434a))
MRGAME_VIDEOROM1(		"vid_ic14.rom", CRC(88a9ca81) SHA1(9660d416b2b8f1937cda7bca51bd287641c7730c),
						"vid_ic55.rom", CRC(3c68b448) SHA1(f416f00d2de0c71c021fec0e9702ba79b761d5e7),
						"vid_ic56.rom", CRC(0aac43e9) SHA1(28edfeddb2d54e40425488bad37e3819e4488b0b),
						"vid_ic66.rom", CRC(c8269b27) SHA1(daa83bfdb1e255b846bbade7f200abeaa9399c06))
MRGAME_SOUNDROM15(		"snd_ic06.rom", CRC(29e9417e) SHA1(24f465993da7c93d385ec453497f2af4d8abb6f4),
						"snd_ic07.rom", CRC(71ab15fe) SHA1(245842bb41410ea481539700f79c7ef94f8f8924),
						"snd_ic22.rom", CRC(e6c1098e) SHA1(06bf8917a27d5e46e4aab93e1f212918418e3a82),
						"snd_ic35.rom", CRC(7b2394d1) SHA1(f588f5105d75b54dd65bb6448a2d7774fb8477ec),
						"snd_ic36.rom", CRC(4039ea65) SHA1(390fce94d1e48b395157d8d9afaa485114c58d52))
MRGAME_ROMEND
CORE_GAMEDEFNV(dakar,"Dakar",1988,"Mr. Game (Italy)",mMRGAME1,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Motor Show (1988?)
/-------------------------------------------------------------------*/
INITGAME(motrshow, mrgame_disp, FLIP_SWNO(65,64), 4/*?*/, SNDBRD_MRGAME, 0)
MRGAME_ROMSTART(motrshow,	"cpu_ic13.rom", CRC(e862ca71) SHA1(b02e5f39f9427d58b70b7999a5ff6075beff05ae),
							"cpu_ic14.rom", CRC(c898ae25) SHA1(f0e1369284a1e0f394f1d40281fd46252016602e))
MRGAME_VIDEOROM1(		"vid_ic14.rom", CRC(1d4568e2) SHA1(bfc2bb59708ce3a09f9a1b3460ed8d5269840c97),
						"vid_ic55.rom", CRC(c27a4ded) SHA1(9c2c9b17f1e71afb74bdfbdcbabb99ef935d32db),
						"vid_ic56.rom", CRC(1664ec8d) SHA1(e7b15acdac7dfc51b668e908ca95f02a2b569737),
						"vid_ic66.rom", CRC(5b585252) SHA1(b88e56ebdce2c3a4b170aff4b05018e7c21a79b8))
MRGAME_SOUNDROM14(		"snd_ic06.rom", CRC(fba5a8f1) SHA1(ddf989abebe05c569c9ecdd498bd8ea409df88ac),
						"snd_ic22.rom", CRC(e6c1098e) SHA1(06bf8917a27d5e46e4aab93e1f212918418e3a82),
						"snd_ic35.rom", CRC(9dec153d) SHA1(8a0140257316aa19c0401456839e11b6896609b1),
						"snd_ic36.rom", CRC(4f42be6e) SHA1(684e988f413cd21c785ad5d60ef5eaddddaf72ab))
MRGAME_ROMEND
CORE_GAMEDEFNV(motrshow,"Motor Show",1988,"Mr. Game (Italy)",mMRGAME1,GAME_IMPERFECT_SOUND)


/*-------------------------------------------------------------------
/ Mac Attack (1989?)
/-------------------------------------------------------------------*/
INITGAME(macattck, mrgame_disp, FLIP_SWNO(65,64), 4/*?*/, SNDBRD_MRGAME, 0)
MRGAME_ROMSTART(macattck,	"cpu_ic13.rom", NO_DUMP,
							"cpu_ic14.rom", NO_DUMP)
MRGAME_VIDEOROM2(		"vid_ic14.rom", NO_DUMP,
						"vid_ic15.rom", NO_DUMP,
						"vid_ic16.rom", NO_DUMP,
						"vid_ic17.rom", NO_DUMP,
						"vid_ic18.rom", NO_DUMP,
						"vid_ic61.rom", NO_DUMP,
						"vid_ic91.rom", NO_DUMP)
MRGAME_SOUNDROM14(		"snd_ic06.rom", NO_DUMP,
						"snd_ic07.rom", NO_DUMP,
						"snd_ic22.rom", NO_DUMP,
						"snd_ic35.rom", NO_DUMP)

MRGAME_ROMEND
CORE_GAMEDEFNV(macattck,"Mac Attack",1989,"Mr. Game (Italy)",mMRGAME1,GAME_IMPERFECT_SOUND)


/*-------------------------------------------------------------------
/ World Cup 90 (1988?)
/-------------------------------------------------------------------*/
INITGAME(wcup90, mrgame_disp, FLIP_SWNO(65,64), 4/*?*/, SNDBRD_MRGAME, 0)
MRGAME_ROMSTART(wcup90,	"cpu_ic13.rom", NO_DUMP,
						"cpu_ic14.rom", NO_DUMP)
MRGAME_VIDEOROM2(		"vid_ic91.rom", CRC(3287ad20) SHA1(d5a453efc7292670073f157dca04897be857b8ed),
						"vid_ic14.rom", CRC(a101d562) SHA1(ad9ad3968f13169572ec60e22e84acf43382b51e),
						"vid_ic15.rom", CRC(40791e7a) SHA1(788760b8527df48d1825be88099491b6e94f0a19),
						"vid_ic16.rom", CRC(a7214157) SHA1(a4660180e8491a37028fec8533cf13daf839a7c4),
						"vid_ic17.rom", CRC(caf4fb04) SHA1(81784a4dc7c671090cf39cafa7d34a6b34523168),
						"vid_ic18.rom", CRC(83ad2a10) SHA1(37664e5872e6322ee6bb61ec9385876626598152),
						"vid_ic61.rom", CRC(538c72ae) SHA1(f704492568257fcc4a4f1189207c6fb6526eb81c))
MRGAME_SOUNDROM25(		"snd_ic06.rom", CRC(19a66331) SHA1(fbd71bc378b5a04247fd1754529c66b086eb33d8),
						"snd_ic21.rom", CRC(e6c1098e) SHA1(06bf8917a27d5e46e4aab93e1f212918418e3a82),
						"snd_ic44.rom", CRC(00946570) SHA1(83e7dd89844679571ab2a803295c8ca8941a4ac7),
						"snd_ic45.rom", CRC(265aa979) SHA1(9ca10c41526a2d227c21f246273ca14bec7f1bc7),
						"snd_ic46.rom", CRC(7edb321e) SHA1(b242e94c24e996d2de803d339aa9bf6e93586a4c))
MRGAME_ROMEND
CORE_GAMEDEFNV(wcup90,"World Cup 90",1990,"Mr. Game (Italy)",mMRGAME2,GAME_IMPERFECT_SOUND)
