#include "driver.h"
#include "sim.h"
#include "wmssnd.h"
#include "s6.h"                 //*System 4 Is Nearly Identical to 6, so we share functions*/
#include "s4.h"

#define INITGAME(name, disp, flip) \
static core_tGameData name##GameData = { GEN_S4, disp, {flip} }; \
static void init_##name(void) { core_gameData = &name##GameData; }

#define INITGAMEFULL(name, disp,lflip,rflip,ss17,ss18,ss19,ss20,ss21,ss22) \
static core_tGameData name##GameData = { GEN_S4, disp, {FLIP_SWNO(lflip,rflip)}, \
 NULL, {{0}},{0,{ss17,ss18,ss19,ss20,ss21,ss22}}}; \
static void init_##name(void) { core_gameData = &name##GameData; }

S4_INPUT_PORTS_START(s4, 1) S4_INPUT_PORTS_END

/*--------------------------------
/ Phoenix - Sys.4 (Game #485)
/-------------------------------*/
INITGAMEFULL(phnix,s4_disp,0,0,18,17,16,30,46,0)
S6_ROMSTART(phnix,l1,"gamerom.716", 0x3aba6eac,
                     "white1.716", 0x9bbbf14f,
                     "white2.716", 0x4d4010dd)
S67S_SOUNDROMS8(     "sound1.716",0xf4190ca3)
S6_ROMEND
#define input_ports_phnix input_ports_s4
CORE_GAMEDEF(phnix,l1,"Phoenix (L-1)",1978,"Williams",s4_mS4S,0)

/*--------------------------------
/ Flash - Sys.4 (Game #486)
/-------------------------------*/
INITGAMEFULL(flash,s4_disp,0,0,19,18,20,42,41,0)
S6_ROMSTART(flash,l1,"gamerom.716", 0x287f12d6,
                     "green1.716",  0x2145f8ab,
                     "green2.716",  0x1c978a4a)
S67S_SOUNDROMS8(     "sound1.716",0xf4190ca3)
S6_ROMEND
#define input_ports_flash input_ports_s4
CORE_GAMEDEF(flash,l1,"Flash (L-1)",1979,"Williams",s4_mS4S,0)

/*--------------------------------
/ Tri Zone - Sys.4 (Game #487)
/-------------------------------*/
INITGAMEFULL(trizn,s4_disp,0,0,13,14,15,26,24,0)
S6_ROMSTART(trizn,l1,"gamerom.716", 0x757091c5,
                     "green1.716", 0x2145f8ab,
                     "green2.716", 0x1c978a4a)
S67S_SOUNDROMS8(     "sound1.716",0xf4190ca3)
S6_ROMEND
#define input_ports_trizn input_ports_s4
CORE_GAMEDEF(trizn,l1,"Tri Zone (L-1)",1978,"Williams",s4_mS4S,0)

/*--------------------------------
/ Pokerino - Sys.4 (Game #488)
/-------------------------------*/
INITGAME(pkrno,s4_disp,FLIP_SW(FLIP_L))
S6_ROMSTART(pkrno,l1,"gamerom.716", 0x9b4d01a8,
                     "white1.716", 0x9bbbf14f,
                     "white2.716", 0x4d4010dd)
S67S_SOUNDROMS8(     "sound1.716",0xf4190ca3)
S6_ROMEND
#define input_ports_pkrno input_ports_s4
CORE_GAMEDEF(pkrno,l1,"Pokerino (L-1)",1978,"Williams",s4_mS4S,0)

/*--------------------------------
/ Time Warp - Sys.4 (Game #489)
/-------------------------------*/
INITGAMEFULL(tmwrp,s4_disp,0,0,19,20,21,22,23,0)
S6_ROMSTART(tmwrp,l2,"gamerom.716", 0xb168df09,
                     "green1.716", 0x2145f8ab,
                     "green2.716", 0x1c978a4a)
S67S_SOUNDROMS8(     "sound1.716",0xf4190ca3)
S6_ROMEND
#define input_ports_tmwrp input_ports_s4
CORE_GAMEDEF(tmwrp,l2,"Time Warp (L-2)",1979,"Williams",s4_mS4S,0)

/*--------------------------------
/ Stellar Wars - Sys.4 (Game #490)
/-------------------------------*/
INITGAMEFULL(stlwr,s4_disp,0,0,14,13,45,46,40,44)
S6_ROMSTART(stlwr,l2,"gamerom.716", 0x874e7ef7,
                     "yellow1.716", 0xd251738c,
                     "yellow2.716", 0x5049326d)
S67S_SOUNDROMS8(     "sound1.716",0xf4190ca3)
S6_ROMEND
#define input_ports_stlwr input_ports_s4
CORE_GAMEDEF(stlwr,l2,"Stellar Wars (L-2)",1979,"Williams",s4_mS4S,0)



