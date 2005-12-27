#ifndef DRIVER_RECURSIVE
#  define DRIVER_RECURSIVE
#  include "driver.h"

const struct GameDriver driver_0 = {
 __FILE__, 0, "", 0, 0, 0, 0, 0, 0, 0, 0, NOT_A_DRIVER
};
#  define DRIVER(name, ver) extern struct GameDriver driver_##name##_##ver;
#  define DRIVERNV(name) extern struct GameDriver driver_##name;
#  include "driver.c"
#  undef DRIVER
#  undef DRIVERNV
#  define DRIVER(name, ver) &driver_##name##_##ver,
#  define DRIVERNV(name) &driver_##name,
const struct GameDriver *drivers[] = {
#  include "driver.c"
0 /* end of array */
};
#if MAMEVER >= 6300
const struct GameDriver *test_drivers[] = { 0 };
#endif
#else /* DRIVER_RECURSIVE */

//Include New Stern Game Support if not running VPM or if Test New Stern Game flag found
#ifdef VPINMAME
	#ifdef TEST_NEW_STERN
		#define INCLUDE_NEW_STERN 1
	#endif
#else
	#define INCLUDE_NEW_STERN 1
#endif

DRIVERNV(st_game)       //Unknown game running on old Stern hardware

// --------------------
// ALVIN G. AND CO.
// --------------------
DRIVERNV(agsoccer)		//A.G. Soccer (1992)
//DRIVERNV(agfootbl)	//A.G. Football (1992)
DRIVERNV(wrldtour)		//Al's Garage Band Goes On A World Tour (1992)
//DRIVERNV(usafootb)	//U.S.A. Football (1992)
DRIVERNV(dinoeggs)		//Dinosaur Eggs (1993)
//DRIVERNV(dualpool)	//Dual Pool (1993) - 1 Unit
//DRIVERNV(maxbadaz)	//Max Badazz (1993) - 1 Unit
DRIVERNV(mystcast)		//Mystery Castle (1993)
DRIVERNV(pstlpkr)		//Pistol Poker (1993)
DRIVERNV(punchy)        //Punchy the Clown (1993)
						//A-MAZE-ING Baseball (1994) - any units?
//DRIVERNV(slamnjam)	//Slam N Jam (1994) - 2 Units

#ifdef MAME_DEBUG
  DRIVERNV(test8031)		//Test 8031 cpu core
#endif

// ---------------
// ASTRO GAMES
// ---------------
DRIVERNV(blkshpsq)      //Black Sheep Squadron (1979) - using old Stern hardware

// ---------------
// ATARI GAMES
// ---------------
                        //Triangle (Prototype, 1976?)
DRIVERNV(atarians)      //The Atarians (November 1976)
DRIVERNV(atarianb)      //The Atarians (2002 bootleg)
DRIVERNV(time2000)      //Time 2000 (June 1977)
DRIVERNV(aavenger)      //Airborne Avenger (September 1977)
DRIVERNV(midearth)      //Middle Earth (February 1978)
DRIVERNV(spcrider)      //Space Riders (September 1978)
DRIVERNV(superman)      //Superman (March 1979)
DRIVERNV(hercules)      //Hercules (May 1979)
DRIVERNV(roadrunr)      //Road Runner (Prototype, 1979)
                        //Monza (Prototype, 1980)
                        //Neutron Star (Prototype, 1981)
                        //4x4 (Prototype, 1983)

// ---------------
// BALLY GAMES
// ---------------
#ifdef MAME_DEBUG
DRIVERNV(bingo)         //10/80 Continental Bingo
#endif

//MPU-17
DRIVERNV(bowarrow)      //BY17-720: 08/76 Bow & Arrow (Prototype)
DRIVERNV(freedom )      //BY17-720: 08/76 Freedom
DRIVERNV(nightrdr)      //BY17-721: 01/76 Night Rider (EM release date)
DRIVERNV(evelknie)      //BY17-722: 09/76 Evel Knievel
DRIVERNV(eightbll)      //BY17-723: 01/77 Eight Ball
DRIVERNV(pwerplay)      //BY17-724: 02/77 Power Play
DRIVERNV(stk_sprs)      //BY17-740: 08/77 Strikes and Spares
DRIVERNV(matahari)      //BY17-725: 09/77 Mata Hari
DRIVERNV(matatest)      //BY17:     09/77 Mata Hari (new game rules)
DRIVERNV(blackjck)      //BY17-728: 05/76 Black Jack  (EM release date)
//MPU-35
DRIVERNV(lostwrld)      //BY35-729: 02/77 Lost World
DRIVERNV(sst     )      //BY35-741: 10/76 Supersonic (EM "Star Ship" release date)
DRIVERNV(sstb     )      //BY35 	  : 05/05 Supersonic (7-digit conversion)
DRIVERNV(smman   )      //BY35-742: 08/77 The Six Million Dollar Man
DRIVERNV(playboy )      //BY35-743: 09/76 Playboy (release date is wrong!)
DRIVERNV(playboyb)      //BY35 	  : 05/05 Playboy (7-digit conversion)
                        //??        ??/78 Big Foot
DRIVERNV(voltan  )      //BY35-744: 01/78 Voltan Escapes Cosmic Doom
DRIVERNV(voltanb  )     //BY35 	  : 05/05 Voltan Escapes Cosmic Doom (7-digit conversion)
DRIVERNV(startrek)      //BY35-745: 01/78 Star Trek
DRIVERNV(startreb)      //BY35 	  : 05/05 Star Trek
                        //??        02/78 Skateball (Prototype)
DRIVERNV(kiss    )      //BY35-746: 04/78 Kiss
DRIVERNV(kissb    )     //BY35 	  : 05/05 Kiss (7-digit conversion)
DRIVERNV(ngndshkr)      //BY35-776: 05/78 Nitro Ground Shaker
DRIVERNV(ngndshkb)      //BY35:     11/02 Nitro Ground Shaker (7-digit conversion)
DRIVERNV(slbmania)      //BY35-786: 06/78 Silverball Mania
DRIVERNV(slbmanib)      //BY35:     11/02 Silverball Mania (7-digit conversion)
DRIVERNV(hglbtrtr)      //BY35-750: 08/78 Harlem Globetrotters On Tour
DRIVERNV(hglbtrtb)      //BY35:     11/02 Harlem Globetrotters On Tour (7-digit conversion)
DRIVERNV(dollyptn)      //BY35-777: 10/78 Dolly Parton
DRIVERNV(dollyptb)      //BY35:     11/02 Dolly Parton (7-digit conversion)
DRIVERNV(paragon )      //BY35-748: 12/78 Paragon
DRIVERNV(paragonb )     //BY35 	  : 05/05 Paragon (7-digit conversion)
DRIVERNV(futurspa)      //BY35-781: 03/79 Future Spa
DRIVERNV(futurspb)      //BY35:     11/02 Future Spa (7-digit conversion)
DRIVERNV(spaceinv)      //BY35-792: 05/79 Space Invaders
DRIVERNV(spaceinb)      //BY35:     11/02 Space Invaders (7-digit conversion)
DRIVERNV(rollston)      //BY35-796: 06/79 Rolling Stones
DRIVERNV(rollstob)      //BY35:     11/02 Rolling Stones (7-digit conversion)
DRIVERNV(mystic  )      //BY35-798: 08/79 Mystic
DRIVERNV(mysticb )      //BY35:     11/02 Mystic (7-digit conversion)
DRIVERNV(xenon   )      //BY35-811: 11/79 Xenon
DRIVERNV(xenonf  )      //BY35-811: 11/79 Xenon (French)
DRIVERNV(hotdoggn)      //BY35-809: 12/79 Hotdoggin'
DRIVERNV(hotdoggb)      //BY35:     11/02 Hotdoggin' (7-digit conversion)
DRIVERNV(viking  )      //BY35-802: 12/79 Viking
DRIVERNV(vikingb )      //BY35:     11/02 Viking (7-digit conversion)
DRIVERNV(skatebll)      //BY35-823: 04/80 Skateball
DRIVERNV(skateblb)      //BY35      09/05 Skateball (rev. 3)
DRIVERNV(frontier)      //BY35-819: 05/80 Frontier
DRIVERNV(flashgdn)      //BY35-834: 05/80 Flash Gordon
DRIVERNV(flashgdv)      //BY35-834: 05/80 Flash Gordon (Vocalizer sound)
DRIVERNV(flashgdf)      //BY35-834: 05/80 Flash Gordon (French)
DRIVERNV(eballdlx)      //BY35-838: 09/80 Eight Ball Deluxe
DRIVERNV(eballdlb)      //BY35      02/05 Eight Ball Deluxe custom rom rev.24
DRIVERNV(fball_ii)      //BY35-839: 09/80 Fireball II
DRIVERNV(embryon )      //BY35-841: 09/80 Embryon
DRIVERNV(embryonb)      //BY35:     10/02 Embryon (7-digit conversion rev.1)
DRIVERNV(embryonc)      //BY35:     05/04 Embryon (7-digit conversion rev.8)
DRIVERNV(embryond)      //BY35:     10/04 Embryon (7-digit conversion rev.9)
DRIVERNV(fathom  )      //BY35-842: 12/80 Fathom
DRIVERNV(fathomb )      //BY35-842: 08/04 Fathom (modified rules)
DRIVERNV(medusa  )      //BY35-845: 02/81 Medusa
#ifdef MAME_DEBUG
DRIVERNV(medusaf )      //BY35-845: 02/81 Medusa (6802 board)
#endif
DRIVERNV(centaur )      //BY35-848: 02/81 Centaur
DRIVERNV(elektra )      //BY35-857: 03/81 Elektra
DRIVERNV(vector  )      //BY35-858: 03/81 Vector
DRIVERNV(vectorb )      //BY35-858: 04/04 Vector (modified rules)
DRIVERNV(spectrum)      //BY35-868: 04/81 Spectrum
DRIVERNV(spectru4)      //BY35-868: 04/81 Spectrum (rel 4)
DRIVERNV(speakesy)      //BY35-877: 08/82 Speakeasy
DRIVERNV(speakes4)      //BY35-877: 08/82 Speakeasy 4 (4 player)
DRIVERNV(rapidfir)      //BY35-869: 06/81 Rapid Fire
DRIVERNV(m_mpac  )      //BY35-872: 05/82 Mr. & Mrs. Pac-Man
/*same as eballdlx*/    //BY35      10/82 Eight Ball Deluxe Limited Edition
DRIVERNV(babypac )      //BY35-891  10/82 Baby Pac-Man
DRIVERNV(bmx     )      //BY35-888: 11/82 BMX
DRIVERNV(granslam)      //BY35-1311:01/83 Grand Slam
DRIVERNV(gransla4)      //BY35-1311:01/83 Grand Slam (4 player)
/*same as cenatur*/     //BY35      06/83 Centaur II
DRIVERNV(goldball)      //BY35-1371:10/83 Gold Ball
DRIVERNV(goldbalb)      //BY35:     03/04 Gold Ball (7-digit conversion)
DRIVERNV(goldbalc)      //BY35:     03/05 Gold Ball (6/7-digit alternate set rev.10)
DRIVERNV(goldbaln)      //BY35-1371:10/83 Gold Ball (alternate)
DRIVERNV(xsandos )      //BY35-1391:12/83 X's & O's
                        //??        ??/84 Mysterian
DRIVERNV(granny )       //BY35-1369:01/84 Granny and the Gators
DRIVERNV(kosteel )      //BY35-1390:05/84 Kings of Steel
DRIVERNV(blakpyra)      //BY35-0A44 04/84 Black Pyramid
DRIVERNV(spyhuntr)      //BY35-0A17:07/84 Spy Hunter
                        //??        ??/85 Hot Shotz
DRIVERNV(fbclass )      //BY35-0A40 10/84 Fireball Classic
DRIVERNV(cybrnaut)      //BY35-0B42 02/85 Cybernaut
//MPU-6803
DRIVERNV(eballchp)      //6803-0B38: 09/85 Eight Ball Champ
DRIVERNV(eballch2)      //                 Eight Ball Champ (cheap squeak)
DRIVERNV(beatclck)      //6803-0C70: 11/85 Beat the Clock
DRIVERNV(ladyluck)      //6803-0E34: 02/86 Lady Luck
DRIVERNV(motrdome)      //6803-0E14: 05/86 MotorDome
                        //6803-????: 06/86 Karate Fight (Prototype for Black Belt?)
DRIVERNV(blackblt)      //6803-0E52: 07/86 Black Belt
DRIVERNV(specforc)      //6803-0E47: 08/86 Special Force
DRIVERNV(strngsci)      //6803-0E35: 10/86 Strange Science
DRIVERNV(cityslck)      //6803-0E79: 03/87 City Slicker
DRIVERNV(hardbody)      //6803-0E94: 02/87 Hardbody
DRIVERNV(hardbdyg)      //6803-0E94: 03/87 Hardbody (German)
DRIVERNV(prtyanim)      //6803-0H01: 05/87 Party Animal
DRIVERNV(hvymetal)      //6803-0H03: 08/87 Heavy Metal Meltdown
DRIVERNV(dungdrag)      //6803-0H06: 10/87 Dungeons & Dragons
DRIVERNV(esclwrld)      //6803-0H05: 12/87 Escape from the Lost World
DRIVERNV(esclwrlg)      //6803-0H05: 12/87 Escape from the Lost World (German)
DRIVERNV(black100)      //6803-0H07: 03/88 Blackwater 100
                        //??         06/88 Ramp Warrior (Became Truck Stop after Merger)
//Williams Merger begins here.. but these are still under the Bally name
DRIVERNV(trucksp2)      //6803-2001: 11/88 Truck Stop (P-2)
DRIVERNV(trucksp3)      //6803-2001: 12/88 Truck Stop (P-3)
DRIVERNV(atlantis)      //6803-2006: 03/89 Atlantis
                        //??         05/89 Ice Castle

// ------------------
// (NUOVA) BELL GAMES
// ------------------
DRIVERNV(newwave)       //BY35:     04/85 New Wave, Bally Black Pyramid Clone
DRIVERNV(saturn2)       //BY35:     08/85 Saturn 2, Bally Spy Hunter Clone
DRIVERNV(spacehaw)      //BY35:     04/86 Space Hawks, Bally Cybernaut Clone

// ----------------
// CAPCOM GAMES
// ----------------
DRIVERNV(ghv101)        // ??/95    Goofy Hoops (Romstar license)
DRIVERNV(pmv112)        // 10/95    Pinball Magic
DRIVERNV(pmv112r)       // 10/95    Pinball Magic (Redemption)
DRIVERNV(abv106)        // 03/96    Airborne
DRIVERNV(abv106r)       // 03/96    Airborne (Redemption)
DRIVERNV(bsv100r)       // 05/96    Breakshot (Redemption, 1.0)
DRIVERNV(bsv102r)       // 05/96    Breakshot (Redemption, 1.2)
DRIVERNV(bsv103)        // 05/96    Breakshot (1.3)
DRIVERNV(bsb105)        // 05/96    Breakshot (Beta, 1.5)
DRIVERNV(ffv101)        // 10/96    Flipper Football (1.01)
DRIVERNV(ffv104)        // 10/96    Flipper Football (1.04)
DRIVERNV(bbb108)        // 11/96    Big Bang Bar (Beta, 1.8)
DRIVERNV(bbb109)        // 11/96    Big Bang Bar (Beta, 1.9)
DRIVERNV(kpv106)        // 12/96    Kingpin

// -------------------
// DATA EAST GAMES
// -------------------
//4 x 2 x 7 (mixed) + credits A/N Display
DRIVER(lwar,a83)        //Data East MPU: 05/87 Laser War
//4 x 2 x 7 (mixed) A/N Display
DRIVER(ssvc,a26)        //Data East MPU: 03/88 Secret Service
DRIVER(torp,e21)        //Data East MPU: 08/88 Torpedo Alley
DRIVER(tmac,a18)        //Data East MPU: 12/88 Time Machine (1.8)
DRIVER(tmac,a24)        //Data East MPU: 12/88 Time Machine (2.4)
DRIVER(play,a24)        //Data East MPU: 05/89 Playboy 35th Anniversary
//2 x 16 A/N Display
DRIVER(mnfb,c27)        //Data East MPU: 09/89 ABC Monday Night Football
DRIVER(robo,a34)        //Data East MPU: 11/89 Robo Cop
DRIVER(poto,a32)        //Data East MPU: 01/90 Phantom of the Opera
DRIVER(bttf,a20)        //Data East MPU: 06/90 Back to the Future (2.0)
DRIVER(bttf,a21)        //Data East MPU: ??/90 Back to the Future (2.1)
DRIVER(bttf,a27)        //Data East MPU: 12/90 Back to the Future (2.7)
DRIVER(bttf,g27)        //Data East MPU: ??/9? Back to the Future (2.7 Germany)
DRIVER(simp,a20)        //Data East MPU: 09/90 The Simpsons (2.0)
DRIVER(simp,a27)        //Data East MPU: 09/90 The Simpsons (2.7)
//DMD 128 x 16
DRIVER(ckpt,a17)        //Data East MPU: 02/91 Checkpoint
DRIVER(tmnt,103)        //Data East MPU: 05/91 Teenage Mutant Ninja Turtles (1.03)
DRIVER(tmnt,104)        //Data East MPU: 05/91 Teenage Mutant Ninja Turtles (1.04)
//BSMT2000 Sound chip
DRIVER(btmn,101)        //Data East MPU: 07/91 Batman (1.01)
DRIVER(btmn,103)        //Data East MPU: ??/91 Batman (1.03)
DRIVER(btmn,g13)        //Data East MPU: ??/91 Batman (1.03 Germany)
DRIVER(btmn,106)        //Data East MPU: ??/91 Batman (1.06)
DRIVER(trek,11a)        //Data East MPU: 11/91 Star Trek 25th Anniversary (1.10 Alpha Display)
DRIVER(trek,110)        //Data East MPU: 11/91 Star Trek 25th Anniversary (1.10)
DRIVER(trek,120)        //Data East MPU: 01/92 Star Trek 25th Anniversary (1.20)
DRIVER(trek,200)        //Data East MPU: 04/92 Star Trek 25th Anniversary (2.00)
DRIVER(trek,201)        //Data East MPU: 04/92 Star Trek 25th Anniversary (2.01)
DRIVER(hook,401)        //Data East MPU: 01/92 Hook (4.01)
DRIVER(hook,404)        //Data East MPU: 01/92 Hook (4.04)
DRIVER(hook,408)        //Data East MPU: 01/92 Hook (4.08)
//DMD 128 x 32
DRIVER(lw3,200)         //Data East MPU: 06/92 Lethal Weapon (2.00)
DRIVER(lw3,205)         //Data East MPU: 07/92 Lethal Weapon (2.05)
DRIVER(lw3,207)         //Data East MPU: 08/92 Lethal Weapon (2.07 Canada)
DRIVER(lw3,208)         //Data East MPU: 11/92 Lethal Weapon (2.08)
DRIVER(stwr,103)        //Data East MPU: 10/92 Star Wars (1.03)
DRIVER(stwr,a14)        //Data East MPU: 10/92 Star Wars (Display Rev.1.04)
DRIVER(stwr,g11)        //Data East MPU: 10/92 Star Wars (1.01 Germany)
DRIVER(stwr,102)        //Data East MPU: 11/92 Star Wars (1.02)
DRIVER(stwr,e12)        //Data East MPU: 11/92 Star Wars (1.02 England)
DRIVER(rab,103)         //Data East MPU: 02/93 Rocky & Bullwinkle (1.03 Spain)
DRIVER(rab,130)         //Data East MPU: 04/93 Rocky & Bullwinkle (1.30)
DRIVER(rab,320)         //Data East MPU: 08/93 Rocky & Bullwinkle (3.20)
DRIVER(jupk,501)        //Data East MPU: 09/93 Jurassic Park (5.01)
DRIVER(jupk,g51)        //Data East MPU: 09/93 Jurassic Park (5.01 Germany)
DRIVER(jupk,513)        //Data East MPU: 09/93 Jurassic Park (5.13)
DRIVER(lah,l104)        //Data East MPU: 08/93 Last Action Hero (1.04 Spain)
DRIVER(lah,l108)        //Data East MPU: 08/93 Last Action Hero (1.08 Spain)
DRIVER(lah,110)         //Data East MPU: 08/93 Last Action Hero (1.10)
DRIVER(lah,112)         //Data East MPU: 08/93 Last Action Hero (1.12)
DRIVER(tftc,104)        //Data East MPU: 11/93 Tales From the Crypt (1.04 Spain)
DRIVER(tftc,200)        //Data East MPU: 11/93 Tales From the Crypt (2.00)
DRIVER(tftc,300)		//Data East MPU: 11/93 Tales From the Crypt (3.00)
DRIVER(tftc,303)        //Data East MPU: 11/93 Tales From the Crypt (3.03)
DRIVER(tomy,h30)        //Data East MPU: 02/94 Tommy (3.00 Holland)
DRIVER(tomy,400)        //Data East MPU: 02/94 Tommy (4.00)
DRIVER(wwfr,103)        //Data East MPU: 05/94 WWF Royal Rumble (1.03)
DRIVER(wwfr,106)        //Data East MPU: 08/94 WWF Royal Rumble (1.06)
DRIVER(gnr,300)         //Data East MPU: 07/94 Guns N Roses
//MISC
DRIVERNV(detest)        //Data East MPU: ??/?? DE Test Chip

// ---------------
// GAMATRON GAMES
// ---------------
DRIVERNV(gamatron)      //Pinstar Gamatron (December 85)

// -------------------
// GAME PLAN GAMES
// -------------------
/*Games below are Cocktail #110 Model*/
DRIVERNV(foxylady)      //Foxy Lady (May 1978)
DRIVERNV(blvelvet)      //Black Velvet (May 1978)
DRIVERNV(camlight)      //Camel Lights (May 1978)
DRIVERNV(real)          //Real (to Real) (May 1978)
DRIVERNV(rio)           //Rio (?? / 1978)
DRIVERNV(chucklck)      //Chuck-A-Luck (October 1978)
/*Games below are Cocktail #120 Model*/
DRIVERNV(startrip)      //Star Trip (April 1979)
DRIVERNV(famlyfun)      //Family Fun! (April 1979)
/*Games below are regular standup pinball games*/
DRIVERNV(sshooter)      //Sharpshooter (May 1979)
DRIVERNV(vegasgp)       //Vegas (August 1979)
DRIVERNV(coneyis)       //Coney Island! (December 1979)
DRIVERNV(lizard)        //Lizard (July 1980)
DRIVERNV(gwarfare)      //Global Warfare (June 1981)
DRIVERNV(mbossy)        //Mike Bossy - The Scoring Machine (January 1982)
DRIVERNV(suprnova)      //Super Nova (May 1982)
DRIVERNV(sshootr2)      //Sharp Shooter II (November 1983)
DRIVERNV(attila)        //Attila the Hun (April 1984)
DRIVERNV(agent777)      //Agents 777 (November 1984)
DRIVERNV(cpthook)       //Captain Hook (April 1985)
DRIVERNV(ladyshot)      //Lady Sharpshooter (May 1985)
DRIVERNV(andromed)      //Andromeda (August 1985)
DRIVERNV(andromea)      //Andromeda (alternate set)
DRIVERNV(cyclopes)      //Cyclopes (November 1985)
                        //Loch Ness Monster (November 1985)

// ------------------
// GOTTLIEB GAMES
// ------------------
//System 1
DRIVERNV(cleoptra)      //S1-409    11/77 Cleopatra
DRIVERNV(sinbad)        //S1-412    05/78 Sinbad
DRIVERNV(sinbadn)       //S1-412NO1 05/78 Sinbad (Norway)
DRIVERNV(jokrpokr)      //S1-417    08/78 Joker Poker
DRIVERNV(dragon)        //S1-419    10/78 Dragon
DRIVERNV(closeenc)      //S1-424    10/78 Close Encounters of the Third Kind
DRIVERNV(charlies)      //S1-425    11/78 Charlie's Angels
DRIVERNV(solaride)      //S1-421    02/79 Solar Ride
DRIVERNV(countdwn)      //S1-422    05/79 Count-Down
DRIVERNV(pinpool)       //S1-427    08/79 Pinball Pool
DRIVERNV(totem)         //S1-429    10/79 Totem
DRIVERNV(hulk)          //S1-433    10/79 Incredible Hulk
DRIVERNV(genie)         //S1-435    11/79 Genie
DRIVERNV(buckrgrs)      //S1-437    01/80 Buck Rogers
DRIVERNV(torch)         //S1-438    02/80 Torch
DRIVERNV(roldisco)      //S1-440    02/80 Roller Disco
DRIVERNV(astannie)      //S1-442    12/80 Asteroid Annie and the Aliens
DRIVERNV(sys1test)      //S1-T      ??    System1 test prom
//System 80
DRIVERNV(spidermn)      //S80-653:  05/80 The Amazing Spider-man
DRIVERNV(circus)        //S80-654:  06/80 Circus
DRIVERNV(panthera)      //S80-652:  06/80 Panthera
DRIVERNV(cntforce)      //S80-656:  08/80 Counterforce
DRIVERNV(starrace)      //S80-657:  10/80 Star Race
DRIVERNV(jamesb)        //S80-658:  10/80 James Bond (Timed Play)
DRIVERNV(jamesb2)       //                James Bond (3/5 Ball)
DRIVERNV(timeline)      //S80-659:  10/80 Time Line
DRIVERNV(forceii)       //S80-661:  02/81 Force II
DRIVERNV(pnkpnthr)      //S80-664:  03/81 Pink Panther
DRIVERNV(mars)          //S80-666:  03/81 Mars God of War
DRIVERNV(vlcno_ax)      //S80-667:  09/81 Volcano (Sound & Speech)
DRIVERNV(vlcno_1b)      //                Volcano (Sound Only)
DRIVERNV(blckhole)      //S80-668:  10/81 Black Hole (Sound & Speech, Rev 4)
DRIVERNV(blkhole2)      //                Black Hole (Sound & Speech, Rev 2)
DRIVERNV(blkholea)      //                Black Hole (Sound Only)
DRIVERNV(eclipse)       //S80-671:  ??/82 Eclipse
DRIVERNV(hh)            //S80-669:  06/82 Haunted House (Rev 2)
DRIVERNV(hh_1)          //                Haunted House (Rev 1)
DRIVERNV(s80tst)        //S80: Text Fixture
//System 80a
DRIVERNV(dvlsdre)       //S80a-670: 08/82 Devils Dare (Sound & Speech)
DRIVERNV(dvlsdre2)      //                Devils Dare (Sound Only)
DRIVERNV(caveman)       //S80a-PV810:09/82 Caveman
DRIVERNV(cavemana)      //S80a-PV810:09/82 Caveman (set 2)
DRIVERNV(rocky)         //S80a-672: 09/82 Rocky
DRIVERNV(spirit)        //S80a-673: 11/82 Spirit
DRIVERNV(striker)       //S80a-675: 11/82 Striker
DRIVERNV(punk)          //S80a-674: 12/82 Punk!
DRIVERNV(krull)         //S80a-676: 02/83 Krull
DRIVERNV(goinnuts)      //S80a-682: 02/83 Goin' Nuts
DRIVERNV(qbquest)       //S80a-677: 03/83 Q*bert's Quest
DRIVERNV(sorbit)        //S80a-680: 05/83 Super Orbit
DRIVERNV(rflshdlx)      //S80a-681: 06/83 Royal Flush Deluxe
DRIVERNV(amazonh)       //S80a-684: 09/83 Amazon Hunt
DRIVERNV(rackemup)      //S80a-685: 11/83 Rack 'Em Up
DRIVERNV(raimfire)      //S80a-686: 11/83 Ready Aim Fire
DRIVERNV(jack2opn)      //S80a-687: 05/84 Jacks to Open
DRIVERNV(alienstr)      //S80a-689: 06/84 Alien Star
DRIVERNV(thegames)      //S80a-691: 08/84 The Games
DRIVERNV(eldorado)      //S80a-692: 09/84 El Dorado City of Gold
DRIVERNV(touchdn)       //S80a-688: 10/84 Touchdown
DRIVERNV(icefever)      //S80a-695: 02/85 Ice Fever
//System 80b
DRIVERNV(triplay)       //S80b-696: 05/85 Chicago Cubs Triple Play
DRIVERNV(bountyh)       //S80b-694: 07/85 Bounty Hunter
DRIVERNV(tagteam)       //S80b-698: 09/85 Tag Team Wrestling
DRIVERNV(tagteam2)      //S80b-698: 09/85 Tag Team Wrestling (rev.2)
DRIVERNV(rock)          //S80b-697: 10/85 Rock
DRIVERNV(raven)         //S80b-702: 03/86 Raven
                        //S80b-704: 04/86 Rock Encore
DRIVERNV(hlywoodh)      //S80b-703: 06/86 Hollywood Heat
DRIVERNV(genesis)       //S80b-705: 09/86 Genesis
DRIVERNV(goldwing)      //S80b-707: 10/86 Gold Wings
DRIVERNV(mntecrlo)      //S80b-708: 02/87 Monte Carlo
DRIVERNV(sprbreak)      //S80b-706: 04/87 Spring Break
                        //S80b-???: 05/87 Amazon Hunt II
DRIVERNV(arena)         //S80b-709: 06/87 Arena
DRIVERNV(victory)       //S80b-710: 10/87 Victory
DRIVERNV(diamond)       //S80b-711: 02/88 Diamond Lady
DRIVERNV(txsector)      //S80b-712: 03/88 TX Sector
DRIVERNV(robowars)      //S80b-714: 04/88 Robo-War
DRIVERNV(badgirls)      //S80b-717: 11/88 Bad Girls
DRIVERNV(excalibr)      //S80b-715: 11/88 Excalibur
DRIVERNV(bighouse)      //S80b-713: 04/89 Big House
DRIVERNV(hotshots)      //S80b-718: 04/89 Hot Shots
DRIVERNV(bonebstr)      //S80b-719: 08/89 Bone Busters Inc
//System 3 Alphanumeric
DRIVERNV(ccruise)       //C-102:    ??/89 Caribbean Cruise (for International Concepts)
DRIVERNV(nmoves)        //C-103?    ??/89 Night Moves (for International Concepts)
DRIVERNV(lca)           //S3-720:   11/89 Lights, Camera, Action
DRIVERNV(lca2)          //S3-720:   11/89 Lights, Camera, Action (rev.2)
DRIVERNV(silvslug)      //S3-722:   02/90 Silver Slugger
DRIVERNV(vegas)         //S3-723:   07/90 Vegas
DRIVERNV(deadweap)      //S3-724:   09/90 Deadly Weapon
DRIVERNV(tfight)        //S3-726:   10/90 Title Fight
DRIVERNV(bellring)      //S3-???:   12/90 Bell Ringer
DRIVERNV(nudgeit)       //S3-???:   12/90 Nudge It
                        //??-???:   ??/91 Amazon Hunt III
DRIVERNV(carhop)        //S3-725:   01/91 Car Hop
DRIVERNV(hoops)         //S3-727:   02/91 Hoops
DRIVERNV(cactjack)      //S3-729:   04/91 Cactus Jack's
DRIVERNV(clas1812)      //S3-730:   08/91 Class of 1812
DRIVERNV(surfnsaf)      //S3-731:   11/91 Surf'n Safari
DRIVERNV(opthund)       //S3-732:   02/92 Operation: Thunder
//System 3 128x32 DMD
DRIVERNV(smb)           //S3-733:   04/92 Super Mario Bros.
DRIVERNV(smb1)          //S3-733:   04/92 Super Mario Bros. (rev.1)
DRIVERNV(smb2)          //S3-733:   04/92 Super Mario Bros. (rev.2)
DRIVERNV(smb3)          //S3-733:   04/92 Super Mario Bros. (rev.3)
DRIVERNV(smbmush)       //S3-N105:  06/92 Super Mario Bros. Mushroom World
DRIVERNV(cueball)       //S3-734:   10/92 Cue Ball Wizard
DRIVERNV(sfight2)       //S3-735:   03/93 Street Fighter II
DRIVERNV(sfight2a)      //                Street Fighter II (rev.1)
DRIVERNV(sfight2b)      //                Street Fighter II (rev.2)
DRIVERNV(teedoff)       //S3-736:   05/93 Tee'd Off
DRIVERNV(teedoff1)      //S3-736:   05/93 Tee'd Off (rev.1)
DRIVERNV(teedoff3)      //S3-736:   05/93 Tee'd Off (rev.3)
DRIVERNV(wipeout)       //S3-738:   10/93 Wipe Out
DRIVERNV(gladiatr)      //S3-737:   11/93 Gladiators
DRIVERNV(wcsoccer)      //S3-741:   02/94 World Challenge Soccer
DRIVERNV(rescu911)      //S3-740:   05/94 Rescue 911
DRIVERNV(freddy)        //S3-744:   10/94 Freddy: A Nightmare on Elm Street
DRIVERNV(shaqattq)      //S3-743:   02/95 Shaq Attaq (rev.5)
DRIVERNV(shaqatt2)      //S3-743:   02/95 Shaq Attaq (rev.2)
DRIVERNV(stargate)      //S3-742:   03/95 Stargate
DRIVERNV(stargat1)      //                Stargate (rev.1)
DRIVERNV(stargat2)      //                Stargate (rev.2)
DRIVERNV(stargat3)      //                Stargate (rev.3)
DRIVERNV(stargat4)      //                Stargate (rev.4)
DRIVERNV(bighurt)       //S3-743:   06/95 Big Hurt
DRIVERNV(snspares)      //S3-N111:  10/95 Strikes 'N Spares
DRIVERNV(waterwld)      //S3-746:   10/95 Waterworld (rev.3)
DRIVERNV(waterwl2)      //S3-746:   10/95 Waterworld (rev.2)
DRIVERNV(andretti)      //S3-747:   12/95 Mario Andretti
DRIVERNV(barbwire)      //S3-748:   04/96 Barb Wire
DRIVERNV(brooks)        //S3-749:   08/96 Brooks & Dunn (never produced)

// ----------------
// HANKIN GAMES
// ----------------
DRIVERNV(fjholden)      //FJ Holden
DRIVERNV(orbit1)        //Orbit 1
DRIVERNV(howzat)        //Howzat
DRIVERNV(shark)         //Shark
DRIVERNV(empsback)      //Star Wars - The Empire Strike Back

// ----------------
// INDER GAMES
// ----------------
DRIVERNV(brvteam )      //Brave Team (1985)
DRIVERNV(canasta )      //Canasta '86' (1986)
DRIVERNV(pinclown)      //Pinball Clown (1988)
DRIVERNV(corsario)		//Corsario (1989)
DRIVERNV(ind250cc)		//250 CC (1992)


// ----------------
// JUEGOS POPULARES
// ----------------
DRIVERNV(america )      //America 1492
DRIVERNV(lortium )      //Lortium
DRIVERNV(petaco2 )      //Petaco 2

// ----------------
// LTD (Brazil)
// ----------------
DRIVERNV(bhol_ltd)      //Black Hole (clone of Gottlieb Black Hole)
DRIVERNV(pecman  )      //Pec Man
DRIVERNV(zephy   )      //Zephy (Xenon clone)
DRIVERNV(cowboy  )      //Cowboy
DRIVERNV(atla_ltd)      //Atlantis
DRIVERNV(columbia)      //Columbia

// ----------------
// MIDWAY GAMES
// ----------------
DRIVERNV(flicker )      //Flicker (Prototype, September 1974)
DRIVERNV(rotation)      //Rotation VIII (September 1978)

// ----------------
// MONROE BOWLING CO.
// ----------------
DRIVERNV(monrobwl)      //Stars & Strikes (1980??)

// ----------------
// MR. GAME
// ----------------
DRIVERNV(dakar)			//Dakar (1988?)
DRIVERNV(motrshow)		//Motor Show (1988?)
DRIVERNV(motrshwa)		//Motor Show (alternate set)
DRIVERNV(macattck)		//Mac Attack (1989?)
DRIVERNV(wcup90)		//World Cup 90 (1990)

// ----------------
// NUOVA BELL GAMES
// ----------------
DRIVERNV(f1gp)			//F1 Grand Prix (1987)

// ----------------
// PEYPER
// ----------------
                        // Tally Hoo
                        // Fantastic World (1985)
                        // Odin (1985)
                        // Nemesis (1986)
                        // Wolfman (1987)
DRIVERNV(odisea)		//Odisea Paris-Dakar (1987)
                        // Sir Lancelot (1994)

// ----------------
// PLAYMATIC
// ----------------
						//Last Lap (1978)
						//Chance (1978)
						//Space Gambler (1978)
						//Attack (1979)
						//Big Town (1979)
DRIVERNV(antar)			//Antar (1979)
						//Party (1979)
						//Black Fever (1980)
DRIVERNV(evlfight)		//Evil Fight (1980)
						//Zira (1980)
						//Cerberus (1981)
						//Spain 82 (1982)
DRIVERNV(kz26)			//KZ-26 (1984)
						//The Raid (1984)
						//UFO-X (1984)
						//Nautilus (1984)
DRIVERNV(megaaton)		//Meg-Aaton (1985)
						//Trailer (1985)
DRIVERNV(madrace)		//Mad Race (1985)
						//Flash Dragon (1986)
						//Star Fire (1986)
						//Pinball Champ (1986)
						//Rock 2500 (1986)
						//Phantom Ship (1987)
						//Skill Flight (1987)


// --------------
// SEGA GAMES
// --------------
//Data East Hardare, DMD 192x64
DRIVERNV(mav_100)       //DE/Sega MPU: 09/94 Maverick 1.00
DRIVERNV(mav_401)       //DE/Sega MPU: 09/94 Maverick 4.01 Display
DRIVERNV(mav_402)       //DE/Sega MPU: 09/94 Maverick 4.02 Display
DRIVERNV(frankst)       //DE/Sega MPU: 12/94 Frankenstein
DRIVERNV(frankstg)      //DE/Sega MPU: 01/95 Frankenstein (Germany)
DRIVERNV(baywatch)      //DE/Sega MPU: 03/95 Baywatch
DRIVERNV(batmanf)       //DE/Sega MPU: 07/95 Batman Forever (4.0)
DRIVERNV(batmanf3)      //DE/Sega MPU: 07/95 Batman Forever (3.0)
DRIVERNV(bmf_uk)        //DE/Sega MPU: 07/95 Batman Forever (UK)
DRIVERNV(bmf_at)        //DE/Sega MPU: 07/95 Batman Forever (Austria)
DRIVERNV(bmf_be)        //DE/Sega MPU: 07/95 Batman Forever (Belgium)
DRIVERNV(bmf_ch)        //DE/Sega MPU: 07/95 Batman Forever (Switzerland)
DRIVERNV(bmf_cn)        //DE/Sega MPU: 07/95 Batman Forever (Canada)
DRIVERNV(bmf_de)        //DE/Sega MPU: 07/95 Batman Forever (Germany)
DRIVERNV(bmf_fr)        //DE/Sega MPU: 07/95 Batman Forever (France)
DRIVERNV(bmf_nl)        //DE/Sega MPU: 07/95 Batman Forever (Holland)
DRIVERNV(bmf_it)        //DE/Sega MPU: 07/95 Batman Forever (Italy)
DRIVERNV(bmf_sp)        //DE/Sega MPU: 07/95 Batman Forever (Spain)
DRIVERNV(bmf_no)        //DE/Sega MPU: 07/95 Batman Forever (Norway)
DRIVERNV(bmf_sv)        //DE/Sega MPU: 07/95 Batman Forever (Sweden)
DRIVERNV(bmf_jp)        //DE/Sega MPU: 07/95 Batman Forever (Japan)
DRIVERNV(bmf_time)      //DE/Sega MPU: 07/95 Batman Forever (Timed Version)
//Whitestar Hardware DMD 128x32
DRIVERNV(apollo13)      //Whitestar: 11/95 Apollo 13
DRIVERNV(gldneye)       //Whitestar: 02/96 Golden Eye
DRIVER  (twst,300)      //Whitestar: 05/96 Twister (3.00)
DRIVER  (twst,404)      //Whitestar: 05/96 Twister (4.04)
DRIVER  (twst,405)      //Whitestar: 05/96 Twister (4.05)
DRIVERNV(id4)           //Whitestar: 07/96 ID4: Independance Day
DRIVERNV(spacejam)      //Whitestar: 08/96 Space Jam
DRIVERNV(spacejmf)      //Whitestar: 08/96 Space Jam (France)
DRIVERNV(spacejmg)      //Whitestar: 08/96 Space Jam (Germany)
DRIVERNV(spacejmi)      //Whitestar: 08/96 Space Jam (Italy)
DRIVERNV(swtril43)      //Whitestar: 02/97 Star Wars Trilogy (4.03)
DRIVERNV(swtril41)      //Whitestar: 02/97 Star Wars Trilogy (4.01)
DRIVERNV(jplstw22)      //Whitestar: 06/97 The Lost World: Jurassic Park (2.02)
DRIVERNV(jplstw20)      //Whitestar: 06/97 The Lost World: Jurassic Park (2.00)
DRIVERNV(xfiles)        //Whitestar: 08/97 X-Files
DRIVERNV(startrp)       //Whitestar: 11/97 Starship Troopers
DRIVERNV(viprsega)      //Whitestar: 02/98 Viper Night Drivin'
DRIVERNV(lostspc)       //Whitestar: 06/98 Lost in Space
DRIVERNV(goldcue)       //Whitestar: 06/98 Golden Cue
DRIVERNV(godzilla)      //Whitestar: 09/98 Godzilla
DRIVER  (sprk,090)      //Whitestar: 01/99 South Park (0.90)
DRIVER  (sprk,103)      //Whitestar: 01/99 South Park (1.03)
DRIVER  (harl,a10)      //Whitestar: 09/99 Harley Davidson (1.03, Display 1.00)
DRIVER  (harl,a13)      //Whitestar: 10/99 Harley Davidson (1.03, Display 1.04)
DRIVER  (harl,f13)      //Whitestar: 10/99 Harley Davidson (1.03 France)
DRIVER  (harl,g13)      //Whitestar: 10/99 Harley Davidson (1.03 Germany)
DRIVER  (harl,i13)      //Whitestar: 10/99 Harley Davidson (1.03 Italy)
DRIVER  (harl,l13)      //Whitestar: 10/99 Harley Davidson (1.03 Spain)

// ----------------
// SLEIC
// ----------------
						// (1993)
DRIVERNV(sleicpin)		// Pin-Ball (1993)
						// (1993)

// ----------------
// SONIC
// ----------------
						// Night Fever (1979)
						// Storm (1979)
						// Odin De Luxe (1985)
						// Gamatron (1986)
						// Solar Wars (1986)
DRIVERNV(sonstwar)		// Star Wars (1987)
DRIVERNV(poleposn)		// Pole Position (1987)
						// Hang-On (1988)

// ---------------
// SPINBALL GAMES
// ---------------
DRIVERNV(bushido)		//1993 - Bushido     ( Last game by Inder - before becomming Spinball - but same hardware)
DRIVERNV(bushidoa)		//1993 - Bushido (alternate set)
DRIVERNV(mach2)			//1995 - Mach 2
DRIVERNV(jolypark)      //1996 - Jolly Park
DRIVERNV(vrnwrld)		//1996 - Verne's World


// ---------------
// STERN GAMES
// ---------------
// MPU-100 - Chime Sound
DRIVERNV(stingray)      //MPU-100: 03/77 Stingray
DRIVERNV(pinball)       //MPU-100: 07/77 Pinball
DRIVERNV(stars)         //MPU-100: 03/78 Stars
DRIVERNV(memlane)       //MPU-100: 06/78 Memory Lane
// MPU-100 - Sound Board: SB-100
DRIVERNV(lectrono)      //MPU-100: 08/78 Lectronamo
DRIVERNV(wildfyre)      //MPU-100: 10/78 Wildfyre
DRIVERNV(nugent)        //MPU-100: 11/78 Nugent
DRIVERNV(dracula)       //MPU-100: 01/79 Dracula
DRIVERNV(trident)       //MPU-100: 03/79 Trident
DRIVERNV(hothand)       //MPU-100: 06/79 Hot Hand
DRIVERNV(magic)         //MPU-100: 08/79 Magic
DRIVERNV(princess)      //MPU-100: 08/79 Cosmic Princess
// MPU-200 - Sound Board: SB-300
DRIVERNV(meteor)        //MPU-200: 09/79 Meteor
DRIVERNV(meteorb)       //MPU-200: 11/03 Meteor (7-Digit conversion)
DRIVERNV(galaxy)        //MPU-200: 01/80 Galaxy
DRIVERNV(galaxyb)       //MPU-200: ??/04 Galaxy (7-Digit conversion)
DRIVERNV(ali)           //MPU-200: 03/80 Ali
DRIVERNV(biggame)       //MPU-200: 03/80 Big Game
DRIVERNV(seawitch)      //MPU-200: 05/80 Seawitch
DRIVERNV(cheetah)       //MPU-200: 06/80 Cheetah
DRIVERNV(quicksil)      //MPU-200: 06/80 Quicksilver
DRIVERNV(nineball)      //MPU-200: 12/80 Nineball
DRIVERNV(freefall)      //MPU-200: 01/81 Free Fall
DRIVERNV(splitsec)      //MPU-200: 08/81 Split Second
DRIVERNV(catacomb)      //MPU-200: 10/81 Catacomb
DRIVERNV(ironmaid)      //MPU-200: 10/81 Iron Maiden
DRIVERNV(viper)         //MPU-200: 12/81 Viper
DRIVERNV(dragfist)      //MPU-200: 01/82 Dragonfist
// MPU-200 - Sound Board: SB-300, VS-100
DRIVERNV(flight2k)      //MPU-200: 08/80 Flight 2000
DRIVERNV(stargzr)       //MPU-200: 08/80 Stargazer
DRIVERNV(lightnin)      //MPU-200: 03/81 Lightning
DRIVERNV(orbitor1)      //MPU-200: 04/82 Orbitor One
DRIVERNV(cue)           //MPU-200: ??/82 Cue            (Proto - Never released)
DRIVERNV(lazrlord)      //MPU-200: 10/84 Lazer Lord     (Proto - Never released)
DRIVERNV(blbeauty)      //MPU-200: ??/84 Black Beauty (Bowler)
// Whitestar System
DRIVERNV(strikext)      //Whitestar: 03/00 Striker Extreme
DRIVERNV(strxt_uk)      //Whitestar: 03/00 Striker Extreme (UK)
DRIVERNV(strxt_gr)      //Whitestar: 03/00 Striker Extreme (Germany)
DRIVERNV(strxt_sp)      //Whitestar: 03/00 Striker Extreme (Spain)
DRIVERNV(strxt_fr)      //Whitestar: 03/00 Striker Extreme (France)
DRIVERNV(strxt_it)      //Whitestar: 03/00 Striker Extreme (Italy)
DRIVERNV(shrkysht)      //Whitestar: 09/00 Sharkey's Shootout (2.11)
DRIVERNV(shrky_gr)      //Whitestar: 09/00 Sharkey's Shootout (German display)
DRIVERNV(shrky_fr)      //Whitestar: 09/00 Sharkey's Shootout (French display)
DRIVERNV(shrky_it)      //Whitestar: 09/00 Sharkey's Shootout (Italian display)
DRIVERNV(hirolcas)      //Whitestar: 01/01 High Roller Casino (3.0)
DRIVERNV(hirolcat)      //Whitestar: 01/01 High Roller Casino (2.90) TEST BUILD 1820
DRIVERNV(hirol_gr)      //Whitestar: 01/01 High Roller Casino (Germany, 2.10)
DRIVERNV(hirol_fr)      //Whitestar: 01/01 High Roller Casino (French display)
DRIVERNV(hirol_it)      //Whitestar: 01/01 High Roller Casino (Italian display)
DRIVERNV(austin)        //Whitestar: 05/01 Austin Powers
DRIVERNV(austing)       //Whitestar: 05/01 Austin Powers (German display)
DRIVERNV(austinf)       //Whitestar: 05/01 Austin Powers (French display)
DRIVERNV(austini)       //Whitestar: 05/01 Austin Powers (Italian display)
DRIVERNV(monopoly)      //Whitestar: 09/01 Monopoly
DRIVERNV(monopolg)      //Whitestar: 09/01 Monopoly (German display)
DRIVERNV(monopoll)      //Whitestar: 09/01 Monopoly (Spanish display)
DRIVERNV(monopolf)      //Whitestar: 09/01 Monopoly (French display)
DRIVERNV(monopoli)      //Whitestar: 09/01 Monopoly (Italian display)
DRIVERNV(nfl)			//Whitestar: 11/01 NFL
DRIVERNV(playboys)      //Whitestar: 02/02 Playboy
DRIVERNV(playboyg)      //Whitestar: 02/02 Playboy (German display)
DRIVERNV(playboyl)      //Whitestar: 02/02 Playboy (Spanish display)
DRIVERNV(playboyf)      //Whitestar: 02/02 Playboy (French display)
DRIVERNV(playboyi)      //Whitestar: 02/02 Playboy (Italian display)
DRIVERNV(rctycn)        //Whitestar: 09/02 Roller Coaster Tycoon
DRIVERNV(rctycng)       //Whitestar: 09/02 Roller Coaster Tycoon (German display)
DRIVERNV(rctycnl)       //Whitestar: 09/02 Roller Coaster Tycoon (Spanish display)
DRIVERNV(rctycnf)       //Whitestar: 09/02 Roller Coaster Tycoon (French display)
DRIVERNV(rctycni)       //Whitestar: 09/02 Roller Coaster Tycoon (Italian display)

#ifdef INCLUDE_NEW_STERN
DRIVER  (harl,a18)      //Whitestar: 07/03 Harley Davidson (1.08)
DRIVER  (harl,f18)      //Whitestar: 07/03 Harley Davidson (1.08 France)
DRIVER  (harl,g18)      //Whitestar: 07/03 Harley Davidson (1.08 Germany)
DRIVER  (harl,i18)      //Whitestar: 07/03 Harley Davidson (1.08 Italy)
DRIVER  (harl,l18)      //Whitestar: 07/03 Harley Davidson (1.08 Spain)
DRIVER  (harl,a30)      //Whitestar: 10/04 Harley Davidson (3.00)
DRIVER  (harl,f30)      //Whitestar: 10/04 Harley Davidson (3.00 France)
DRIVER  (harl,g30)      //Whitestar: 10/04 Harley Davidson (3.00 Germany)
DRIVER  (harl,i30)      //Whitestar: 10/04 Harley Davidson (3.00 Italy)
DRIVER  (harl,l30)      //Whitestar: 10/04 Harley Davidson (3.00 Spain)
DRIVERNV(simpprty)      //Whitestar: 01/03 The Simpsons Pinball Party
DRIVERNV(simpprtg)      //Whitestar: 01/03 The Simpsons Pinball Party (German display)
DRIVERNV(simpprtl)      //Whitestar: 01/03 The Simpsons Pinball Party (Spanish display)
DRIVERNV(simpprtf)      //Whitestar: 01/03 The Simpsons Pinball Party (French display)
DRIVERNV(simpprti)      //Whitestar: 01/03 The Simpsons Pinball Party (Italian display)
DRIVERNV(term3)         //Whitestar: 06/03 Terminator 3: Rise of the Machines
DRIVERNV(term3g)        //Whitestar: 06/03 Terminator 3: Rise of the Machines (Germany)
DRIVERNV(term3l)        //Whitestar: 06/03 Terminator 3: Rise of the Machines (Spain)
DRIVERNV(term3f)        //Whitestar: 06/03 Terminator 3: Rise of the Machines (France)
DRIVERNV(term3i)        //Whitestar: 06/03 Terminator 3: Rise of the Machines (Italy)

//The following are all Stern Whitestar games using 8MB Roms running on the new ARM7 sound board for testing the ARM7
#ifdef TEST_NEW_SOUND
DRIVERNV(lotrsnd)		//Test ARM7 cpu/sound core
DRIVERNV(seflashb)		//Sound OS Flash Upgrade
//
DRIVERNV(strknew)
DRIVERNV(shrknew)
DRIVERNV(hironew)
DRIVERNV(austnew)
DRIVERNV(mononew)
DRIVERNV(playnew)
DRIVERNV(rctnew)
DRIVERNV(simpnew)
DRIVERNV(t3new)
#endif

// New CPU/Sound Board with ARM7 CPU + Xilinx FPGA controlling sound
DRIVERNV(lotr)          //Whitestar: 11/03 Lord Of The Rings
DRIVERNV(lotr_gr)       //Whitestar: 11/03 Lord Of The Rings (Germany)
DRIVERNV(lotr_sp)       //Whitestar: 11/03 Lord Of The Rings (Spain)
DRIVERNV(lotr_fr)       //Whitestar: 11/03 Lord Of The Rings (France)
DRIVERNV(lotr_it)       //Whitestar: 11/03 Lord Of The Rings (Italy)
DRIVERNV(ripleys)       //Whitestar: 03/04 Ripley's Believe It or Not!
DRIVERNV(ripleysg)      //Whitestar: 03/04 Ripley's Believe It or Not! (Germany)
DRIVERNV(ripleysl)      //Whitestar: 03/04 Ripley's Believe It or Not! (Spain)
DRIVERNV(ripleysf)      //Whitestar: 03/04 Ripley's Believe It or Not! (France)
DRIVERNV(ripleysi)      //Whitestar: 03/04 Ripley's Believe It or Not! (Italy)
DRIVERNV(elvis)         //Whitestar: 10/04 Elvis
DRIVERNV(elvisg)        //Whitestar: 10/04 Elvis (Germany)
DRIVERNV(elvisl)        //Whitestar: 10/04 Elvis (Spain)
DRIVERNV(elvisf)        //Whitestar: 10/04 Elvis (France)
DRIVERNV(elvisi)        //Whitestar: 10/04 Elvis (Italy)
DRIVERNV(sopranos)      //Whitestar: 02/05 The Sopranos
DRIVERNV(sopranog)      //Whitestar: 02/05 The Sopranos (Germany)
DRIVERNV(sopranol)      //Whitestar: 02/05 The Sopranos (Spain)
DRIVERNV(sopranof)      //Whitestar: 02/05 The Sopranos (France)
DRIVERNV(sopranoi)      //Whitestar: 02/05 The Sopranos (Italy)
DRIVERNV(nascar)		//Whitestar: 05/05 Nascar
DRIVERNV(nascarl)		//Whitestar: 05/05 Nascar (Spain)
DRIVERNV(gprix)			//Whitestar: 05/05 Grand Prix
DRIVERNV(gprixg)		//Whitestar: 05/05 Grand Prix (Germany)
DRIVERNV(gprixl)		//Whitestar: 05/05 Grand Prix (Spain)
DRIVERNV(gprixf)		//Whitestar: 05/05 Grand Prix (France)
DRIVERNV(gprixi)		//Whitestar: 05/05 Grand Prix (Italy)
#endif /* INCLUDE_NEW_STERN */

// ---------------
// TAITO GAMES
// ---------------
                        //??/?? Apache!
                        //??/?? Football
                        //??/79 Hot Ball (B Eight Ball, 01/77)
DRIVERNV(shock   )      //??/79 Shock (W Flash, 01/79)
                        //??/?? Sultan (G Sinbad, 05/78)
DRIVERNV(obaoba  )      //??/80 Oba Oba (B Playboy, 10/77)
DRIVERNV(obaoba1 )      //??/80 Oba Oba alternate set
DRIVERNV(drakor  )      //??/80 Drakor (W Gorgar, 11/79)
DRIVERNV(meteort )      //??/80 Meteor (S Meteor, 09/79)
DRIVERNV(fireact )      //??/81 Fire Action (W Firepower, 02/80)
DRIVERNV(cavnegro)      //??/81 Cavaleiro Negro (W Black Knight, 11/80)
DRIVERNV(cavnegr1)      //??/81 Cavaleiro Negro alternate set 1
DRIVERNV(cavnegr2)      //??/81 Cavaleiro Negro alternate set 2
DRIVERNV(sureshot)      //??/81 Sure Shot (B Eight Ball Deluxe, 09/80)
DRIVERNV(vegast  )      //??/81 Vegas (B Mata Hari, 09/77)
DRIVERNV(ladylukt)      //??/81 Lady Luck (B Mata Hari, 09/77)
DRIVERNV(cosmic  )      //??/81 Cosmic (S Galaxy, 01/80)
DRIVERNV(gemini  )      //??/82 Gemini 2000 (B Centaur, 02/81)
DRIVERNV(gemini1 )      //??/82 Gemini 2000 alternate set
DRIVERNV(vortex  )      //??/82 Vortex (W Blackout, 06/80)
DRIVERNV(titan   )      //??/82 Titan (W Barracora, 09/81)
DRIVERNV(titan1  )      //??/82 Titan alternate set
DRIVERNV(zarza   )      //??/82 Zarza (B Xenon, 11/79)
DRIVERNV(zarza1  )      //??/82 Zarza alternate set
DRIVERNV(sharkt  )      //??/82 Shark
DRIVERNV(hawkman )		//??/82 Hawkman (B Fathom, 12/80)
DRIVERNV(hawkman1)		//??/82 Hawkman alternate set
DRIVERNV(stest   )      //??/82 Speed Test
DRIVERNV(lunelle )      //??/82 Lunelle (W Alien Poker, 10/80)
DRIVERNV(rally   )      //??/82 Rally (B Skateball, 04/80)
DRIVERNV(snake   )      //??/82 Snake Machine
DRIVERNV(gork    )      //??/82 Gork
DRIVERNV(voleybal)      //??/?? Voley Ball
                        //??/8? Ogar
DRIVERNV(mrblack )		//??/84 Mr. Black (W Defender, 12/82)
DRIVERNV(mrblack1)		//??/84 Mr. Black alternate set
DRIVERNV(fireactd)      //??/8? Fire Action De Luxe (W Firepower II, 08/83)
DRIVERNV(sshuttle)      //??/85 Space Shuttle (W Space Shuttle, 12/84)
DRIVERNV(sshuttl1)      //??/85 Space Shuttle alternate set
DRIVERNV(polar   )      //??/8? Polar Explorer
DRIVERNV(taitest )      //??/8? Test Fixture

// ----------------
// TECHNOPLAY GAMES
// ----------------

DRIVERNV(scram_tp)      //03/87 Scramble
DRIVERNV(xforce)        //??/87 X Force

// ----------------
// UNITED GAMES
// ----------------
DRIVERNV(bbbowlin)      //Big Ball Bowling - using Bally hardware

// -----------------------------------
// WILLIAMS & WILLIAMS/BALLY GAMES
// -----------------------------------
DRIVER(topaz,l1)        //???       Topaz (Shuffle)
DRIVER(taurs,l1)        //???       Taurus (Shuffle)
DRIVER(omni,l1)         //???       Omni (Shuffle)
DRIVER(bstrk,l1)        //???       Big Strike (Bowler)
DRIVER(tstrk,l1)        //???       Triple Strike (Bowler)

                        //??-466:   06/76 W Aztec
                        //??-470:   10/77 W Wild Card
//System 3
DRIVER(httip,l1)        //S3-477:   11/77 W Hot Tip
DRIVER(lucky,l1)        //S3-480:   03/78 W Lucky Seven
DRIVER(cntct,l1)        //S3-482:   05/78 W Contact
DRIVER(wldcp,l1)        //S3-481:   05/78 W World Cup
DRIVER(disco,l1)        //S3-483:   08/78 W Disco Fever
//System 4
DRIVER(pkrno,l1)        //S4-488:   10/78 W Pokerino
DRIVER(phnix,l1)        //S4-485:   11/78 W Phoenix
DRIVER(flash,l1)        //S4-486:   01/79 W Flash
DRIVER(flash,t1)        //S4-486:   01/79 W Flash /10 Scoring Ted Estes
DRIVER(stlwr,l2)        //S4-490:   03/79 W Stellar Wars
                        //S?-491:   06/79 W Rock'N Roll
//System 6
DRIVER(trizn,l1)        //S6-487:   07/79 W TriZone
DRIVER(trizn,t1)        //S6-487:   07/79 W TriZone /10 Scoring Ted Estes
DRIVER(tmwrp,l2)        //S6-489:   09/79 W Time Warp
DRIVER(tmwrp,t2)        //S6-489:   09/79 W Time Warp /10 Scoring Ted Estes
DRIVER(grgar,l1)        //S6-496:   11/79 W Gorgar
DRIVER(grgar,t1)        //S6-496:   11/79 W Gorgar /10 Scoring Ted Estes
DRIVER(lzbal,l2)        //S6-493:   12/79 W Laser Ball
DRIVER(lzbal,t2)        //S6-493:   12/79 W Laser Ball /10 Scoring Ted Estes
DRIVER(frpwr,l2)        //S6-497:   02/80 W Firepower (L-2)
DRIVER(frpwr,l6)        //S6-497:   02/80 W Firepower (L-6)
DRIVER(frpwr,t6)        //S6-497:   02/80 W Firepower (L-6) /10 Scoring Ted Estes
DRIVER(frpwr,b6)        //S6:       12/03 W Firepower (Sys.6 7-digit conversion)
DRIVER(blkou,l1)        //S6-495:   06/80 W Blackout
DRIVER(blkou,t1)        //S6-495:   06/80 W Blackout /10 Scoring Ted Estes
DRIVER(scrpn,l1)        //S6-494:   07/80 W Scorpion
DRIVER(scrpn,t1)        //S6-494:   07/80 W Scorpion /10 Scoring Ted Estes
DRIVER(algar,l1)        //S6-499:   09/80 W Algar
DRIVER(alpok,l2)        //S6-501:   10/80 W Alien Poker L-2
DRIVER(alpok,l6)        //S6-501:   10/80 W Alien Poker L-6
DRIVER(alpok,f6)        //S6-501:   10/80 W Alien Poker L-6, French speech
DRIVER(alpok,b6)        //S6-501:   08/04 W Alien Poker Multiball mod
//System 7
DRIVER(frpwr,b7)        //S7:       12/03 W Firepower (Sys.7 7-digit conversion)
DRIVER(bk,l4)           //S7-500:   11/80 W Black Knight
DRIVER(jngld,l2)        //S7-503:   02/81 W Jungle Lord
DRIVER(pharo,l2)        //S7-504:   05/81 W Pharoah
                        //S7-506:   06/81 W Black Knight Limited Edition
DRIVER(solar,l2)        //S7-507:   07/81 W Solar Fire
DRIVER(barra,l1)        //S7-510:   09/81 W Barracora
DRIVER(hypbl,l4)        //S7-509:   12/81 W HyperBall
DRIVER(hypbl,l5)        //S7-509:   04/98 W HyperBall (bootleg w/ high score save)
DRIVER(thund,p1)        //S7-508:   05/82 W Thunderball
DRIVER(csmic,l1)        //S7-502:   06/82 W Cosmic Gunfight
DRIVER(vrkon,l1)        //S7-512:   09/82 W Varkon
DRIVER(wrlok,l3)        //S7-516:   10/82 W Warlok
DRIVER(dfndr,l4)        //S7-517:   12/82 W Defender
DRIVER(ratrc,l1)        //S7-5??:   01/83 W Rat Race (never produced)
DRIVER(tmfnt,l5)        //S7-515:   03/83 W Time Fantasy
DRIVER(jst,l2)          //S7-519:   04/83 W Joust
DRIVER(fpwr2,l2)        //S7-521:   08/83 W Firepower II
DRIVER(lsrcu,l2)        //S7-520:   02/84 W Laser Cue
DRIVER(strlt,l1)        //S7-530:   06/84 W Star Light (Came out after System 9 produced)
//System 9
DRIVER(pfevr,p3)        //S9-526:   05/84 W Pennant Fever (pitch & bat)
DRIVER(pfevr,l2)        //S9-526:   05/84 W Pennant Fever (pitch & bat)
                        //S?-538:   10/84 W Gridiron
DRIVER(sshtl,l7)        //S9-535:   12/84 W Space Shuttle
DRIVER(szone,l2)        //S9-916:   ??/84 W Strike Zone (L-2) (Shuffle)
DRIVER(szone,l5)        //S9-916:   ??/84 W Strike Zone (L-5) (Shuffle)
DRIVER(sorcr,l1)        //S9-532:   03/85 W Sorcerer
DRIVER(sorcr,l2)        //S9-532:   03/85 W Sorcerer (L-2)
DRIVER(comet,l4)        //S9-540:   06/85 W Comet (L-4)
DRIVER(comet,l5)        //S9-540:   06/85 W Comet (L-5)
//System 11
DRIVER(hs,l3)           //S11-541:  01/86 W High Speed (L-3)
DRIVER(hs,l4)           //S11-541:  01/86 W High Speed (L-4)
DRIVER(alcat,l7)        //S11:      02/86 W Alley Cats (Bowler)
DRIVER(grand,l4)        //S11-523:  04/86 W Grand Lizard
DRIVER(rdkng,l1)        //S11-542:  07/86 W Road Kings (L-1)
DRIVER(rdkng,l2)        //S11-542:  07/86 W Road Kings (L-2)
DRIVER(rdkng,l3)        //S11-542:  07/86 W Road Kings (L-3)
DRIVER(rdkng,l4)        //S11-542:  07/86 W Road Kings (L-4)
DRIVER(pb,l5)           //S11-549:  10/86 W Pin-bot (L-5)
DRIVER(pb,l2)           //S11-549:  10/86 W Pin-bot (L-2)
DRIVER(pb,l3)           //S11-549:  10/86 W Pin-bot (L-3)
                        //S11:      10/86 W Strike Force
DRIVER(milln,l3)        //S11-555:  01/87 W Millionaire
DRIVER(f14,l1)          //S11-554:  03/87 W F-14 Tomcat
DRIVER(fire,l3)         //S11-556:  08/87 W Fire!
DRIVER(bguns,la)        //S11-557:  10/87 W Big Guns (L-A)
DRIVER(bguns,l7)        //S11-557:  10/87 W Big Guns (L-7)
DRIVER(bguns,l8)        //S11-557:  10/87 W Big Guns (L-8)
DRIVER(spstn,l5)        //S11-552:  12/87 W Space Station
DRIVER(gmine,l2)        //S11-920:  ??/88 W Gold Mine (Bowler)
DRIVER(tdawg,l1)        //S11-921:  ??/88 W Top Dawg (Bowler)
DRIVER(cycln,l4)        //S11-564:  02/88 W Cyclone (L-4)
DRIVER(cycln,l5)        //S11-564:  02/88 W Cyclone (L-5)
DRIVER(bnzai,l3)        //S11-566:  05/88 W Banzai Run (L-3)
DRIVER(bnzai,l1)        //S11-566:  05/88 W Banzai Run (L-1)
DRIVER(bnzai,p1)        //S11-566:  05/88 W Banzai Run (P-1)
DRIVER(swrds,l2)        //S11-559:  06/88 W Swords of Fury
DRIVER(taxi,lg1)        //S11-553:  08/88 W Taxi (Marilyn L-1) Germany
DRIVER(taxi,l4)         //S11-553:  08/88 W Taxi (Lola)
DRIVER(taxi,l3)                           // Marilyn
DRIVER(jokrz,l6)        //S11-567:  12/88 W Jokerz! (L-6)
DRIVER(jokrz,l3)        //S11-567:  12/88 W Jokerz! (L-3)
DRIVER(esha,la3)        //S11-568:  02/89 W Earthshaker
DRIVER(esha,pr4)                          // Family version
DRIVER(esha,lg1)                          // LG-1 version (German)
DRIVER(esha,la1)                          // LA-1 version
DRIVER(esha,pa1)                          // Prototype version
DRIVER(bk2k,l4)         //S11-563:  04/89 W Black Knight 2000 (L-4)
DRIVER(bk2k,lg1)        //S11-563:  04/89 W Black Knight 2000 (LG-1)
DRIVER(bk2k,lg3)        //S11-563:  04/89 W Black Knight 2000 (LG-3)
DRIVER(bk2k,pu1)        //S11-563:  04/89 W Black Knight 2000 (PU-1)
//First Game produced entirely by Williams after Merger to use Bally Name
DRIVER(tsptr,l3)        //S11-2630: 04/89 B Transporter the Rescue
                        //S11:      05/89 W Pool
DRIVER(polic,l2)        //S11-573:  08/89 W Police Force (LA-2)
DRIVER(polic,l3)        //S11-573:  08/89 W Police Force (LA-3)
DRIVER(polic,l4)        //S11-573:  08/89 W Police Force (LA-4)
DRIVER(eatpm,l4)        //S11-782:  10/89 B Elvira and the Party Monsters (L-4)
DRIVER(eatpm,l1)        //S11-782:  10/89 B Elvira and the Party Monsters (L-1)
DRIVER(eatpm,l2)        //S11-782:  10/89 B Elvira and the Party Monsters (L-2)
DRIVER(eatpm,4u)        //S11-782:  10/89 B Elvira and the Party Monsters (L-4 European)
DRIVER(eatpm,4g)        //S11-782:  10/89 B Elvira and the Party Monsters (L-4 German)
DRIVER(eatpm,p7)        //S11-782:  10/89 B Elvira and the Party Monsters (PA-7)
DRIVER(bcats,l5)        //S11-575:  11/89 W Bad Cats (L-5)
DRIVER(bcats,l2)        //S11-575:  11/89 W Bad Cats (LA-2)
DRIVER(rvrbt,l3)        //S11-1966: 11/89 W Riverboat Gambler
DRIVER(mousn,l1)        //S11-1635: 12/89 B Mousin' Around! (L-1)
DRIVER(mousn,lu)        //S11-1635: 12/89 B Mousin' Around! (Europe)
DRIVER(mousn,l4)        //S11-1635: 12/89 B Mousin' Around! (L-4)
DRIVER(mousn,lx)        //S11-1635: 12/89 B Mousin' Around! (L-4)
                        //???:      ??/90 B Mazatron
                        //???:      ??/90 B Player's Choice
                        //???:      ??/90 B Ghost Gallery
DRIVER(whirl,l3)        //S11-574:  01/90 W Whirlwind (L-3)
DRIVER(whirl,l2)        //S11-574:  01/90 W Whirlwind (L-2)
DRIVER(gs,l3)           //S11-985:  04/90 B Game Show (L-3)
DRIVER(gs,l4)           //S11-985:  04/90 B Game Show (L-4)
DRIVER(rollr,l2)        //S11-576:  06/90 W Rollergames (L-2)
DRIVER(rollr,l3)        //S11-576:  06/90 W Rollergames (L-3) Europe
DRIVER(rollr,g3)        //S11-576:  06/90 W Rollergames (L-3) Germany
DRIVER(rollr,ex)        //S11-576:  01/91 W Rollergames (EXPERIMENTAL)
DRIVER(rollr,e1)        //S11-576:  01/91 W Rollergames (PU-1) Europe
DRIVER(rollr,p2)        //S11-576:  01/91 W Rollergames (PA-2, PA-1 Sound)
DRIVER(pool,l7)         //S11-1848: 06/90 B Pool Sharks (Shark?)
DRIVER(diner,l4)        //S11-571:  09/90 W Diner (L-4)
DRIVER(diner,l3)        //S11-571:  09/90 W Diner (L-3)
DRIVER(diner,l1)        //S11-571:  09/90 W Diner (L-1)
DRIVER(radcl,l1)        //S11-1904: 09/90 B Radical!
DRIVER(radcl,g1)        //S11-1904: 09/90 B Radical! (G-1)
DRIVER(dd,l2)           //S11-737:  11/90 B Dr. Dude (L-2)
DRIVER(dd,p6)           //S11-737:  11/90 B Dr. Dude (P-6)
//WPC
DRIVER(dd,p7)           //WPC:      11/90 B Dr. Dude (P-7 WPC)
DRIVER(dd,p06)          //WPC:      11/90 B Dr. Dude (P-6 WPC)
DRIVER(fh,l9)           //WPC-503:  12/90 W Funhouse (L=9)
DRIVER(fh,l3)           //WPC-503:  12/90 W Funhouse (L-3)
DRIVER(fh,l4)           //WPC-503:  12/90 W Funhouse (L-4)
DRIVER(fh,l5)           //WPC-503:  12/90 W Funhouse (L-5)
DRIVER(fh,l9b)                            // bootleg with correct German translation
DRIVER(bbnny,l2)        //S11-396:  01/91 B Bugs Bunny's Birthday Ball (L-2)
DRIVER(bbnny,lu)        //S11-396:  01/91 B Bugs Bunny's Birthday Ball (LU-2) European
DRIVER(bop,l2)          //WPC-502:  04/91 W The Machine: Bride of Pinbot (L-2)
DRIVER(bop,l4)          //WPC-502:  04/91 W The Machine: Bride of Pinbot (L-4)
DRIVER(bop,l5)          //WPC-502:  05/91 W The Machine: Bride of Pinbot (L-5)
DRIVER(bop,l6)          //WPC-502:  05/91 W The Machine: Bride of Pinbot (L-6)
DRIVER(bop,l7)          //WPC-502:  12/92 W The Machine: Bride of Pinbot
DRIVER(hd,l1)           //WPC:      02/91 B Harley Davidson (L-1)
DRIVER(hd,l3)           //WPC:      02/91 B Harley Davidson (L-3)
DRIVER(sf,l1)           //WPC-601:  03/91 W SlugFest
DRIVER(t2,l2)           //WPC-513:  10/91 W Terminator 2: Judgement Day (L-2)
DRIVER(t2,l3)           //WPC-513:  10/91 W Terminator 2: Judgement Day (L-3)
DRIVER(t2,l4)           //WPC-513:  10/91 W Terminator 2: Judgement Day (L-4)
DRIVER(t2,l6)           //WPC-513:  10/91 W Terminator 2: Judgement Day (L-6)
DRIVER(t2,l8)           //WPC-513:  12/91 W Terminator 2: Judgement Day (L-8)
DRIVER(t2,p2f)                            // Profanity Speech version
DRIVER(hurr,l2)         //WPC-512:  08/91 W Hurricane
DRIVER(pz,f4)           //WPC:      08/91 B Party Zone (F-4)
DRIVER(pz,l1)           //WPC:      08/91 B Party Zone (L-1)
DRIVER(pz,l2)           //WPC:      08/91 B Party Zone (L-2)
DRIVER(gi,l3)           //WPC-203:  07/91 B Gilligan's Island (L-3)
DRIVER(gi,l4)           //WPC-203:  07/91 B Gilligan's Island (L-4)
DRIVER(gi,l6)           //WPC-203:  08/91 B Gilligan's Island (L-6)
DRIVER(gi,l9)           //WPC-203:  12/92 B Gilligan's Island (L-9)
DRIVER(strik,l4)        //WPC:      05/92 W Strike Master
DRIVER(taf,p2)          //WPC:      01/92 B The Addams Family (P-2)
DRIVER(taf,l1)          //WPC:      01/92 B The Addams Family (L-1)
DRIVER(taf,l2)          //WPC:      03/92 B The Addams Family (L-2)
DRIVER(taf,l3)          //WPC:      05/92 B The Addams Family (L-3)
DRIVER(taf,l4)          //WPC:      05/92 B The Addams Family (L-4)
DRIVER(taf,l7)          //WPC:      10/92 B The Addams Family (L-7) (Prototype L-5)
DRIVER(taf,l5)          //WPC:      12/92 B The Addams Family (L-5)
DRIVER(taf,l6)          //WPC:      03/93 B The Addams Family (L-6)
DRIVER(taf,h4)          //WPC:      05/94 B The Addams Family (H-4)
DRIVER(gw,pc)           //WPC-504:  03/92 W The Getaway: High Speed II (P-C)
DRIVER(gw,l1)           //WPC-504:  04/92 W The Getaway: High Speed II (L-1)
DRIVER(gw,l2)           //WPC-504:  06/92 W The Getaway: High Speed II (L-2)
DRIVER(gw,l5)           //WPC-504:  12/92 W The Getaway: High Speed II (L-5)
DRIVER(br,p17)          //WPC:      05/92 B Black Rose (P-17)
DRIVER(br,l1)           //WPC:      08/92 B Black Rose (L-1)
DRIVER(br,l3)           //WPC:      01/93 B Black Rose (L-3)
DRIVER(br,l4)           //WPC:      11/93 B Black Rose (L-4)
DRIVER(ft,p4)           //WPC-505:  07/92 W Fish Tales (P-4)
DRIVER(ft,l3)           //WPC-505:  09/92 W Fish Tales (L-3)
DRIVER(ft,l4)           //WPC-505:  09/92 W Fish Tales (L-4)
DRIVER(ft,l5)           //WPC-505:  12/92 W Fish Tales (L-5)
DRIVER(dw,l1)           //WPC-206   10/92 B Doctor Who (L-1)
DRIVER(dw,l2)           //WPC-206:  11/92 B Doctor Who (L-2)
DRIVER(cftbl,l3)        //WPC:      01/93 B Creature from the Black Lagoon (L-3, Sound P-1)
DRIVER(cftbl,l4)        //WPC:      02/93 B Creature from the Black Lagoon (L-4)
DRIVER(hshot,p8)        //WPC-617:  11/92 M Hot Shot Basketball (P-8)
DRIVER(ww,p1)           //WPC-518:  12/92 W White Water (L-2, P-1 sound)
DRIVER(ww,p2)           //WPC-518:  12/92 W White Water (L-2, P-1 sound)
DRIVER(ww,p8)           //WPC-518:  12/92 W White Water (P-8, P-1 sound)
DRIVER(ww,l2)           //WPC-518:  12/92 W White Water (L-2)
DRIVER(ww,l3)           //WPC-518:  01/93 W White Water (L-3)
DRIVER(ww,l4)           //WPC-518:  02/93 W White Water (L-4)
DRIVER(ww,l5)           //WPC-518:  05/93 W White Water (L-5)
DRIVER(drac,p11)        //WPC-501:  02/93 W Bram Stoker's Dracula (P-11)
DRIVER(drac,l1)         //WPC-501:  02/93 W Bram Stoker's Dracula
DRIVER(tz,pa1)          //WPC-520:  03/93 B Twilight Zone (PA-1)
DRIVER(tz,p4)           //WPC-520:  04/93 B Twilight Zone (P-4)
DRIVER(tz,l1)           //WPC-520:  04/93 B Twilight Zone (L-1)
DRIVER(tz,l2)           //WPC-520:  05/93 B Twilight Zone (L-2)
DRIVER(tz,l3)           //WPC-520:  05/93 B Twilight Zone (L-3)
DRIVER(tz,ifpa)         //WPC-520:  05/93 B Twilight Zone (IFPA)
DRIVER(tz,l4)           //WPC-520:  06/93 B Twilight Zone (L-4)
DRIVER(tz,h7)           //WPC-520:  10/94 B Twilight Zone (H-7)
DRIVER(tz,h8)           //WPC-520:  11/94 B Twilight Zone (H-8)
DRIVER(tz,92)           //WPC-520:  01/95 B Twilight Zone (9.2)
DRIVER(tz,94h)			//WPC-520:  10/98 B Twilight Zone (9.4H - Home version)
DRIVER(ij,l3)           //WPC-517:  07/93 W Indiana Jones (L-3)
DRIVER(ij,l4)           //WPC-517:  08/93 W Indiana Jones (L-4)
DRIVER(ij,l5)           //WPC-517:  09/93 W Indiana Jones (L-5)
DRIVER(ij,l6)           //WPC-517:  10/93 W Indiana Jones (L-6)
DRIVER(ij,l7)           //WPC-517:  11/93 W Indiana Jones (L-7)
DRIVER(ij,lg7)          //WPC-517:  11/93 W Indiana Jones (LG-7)
DRIVER(jd,l1)           //WPC-220:  10/93 B Judge Dredd (L-1)
DRIVER(jd,l6)           //WPC-220:  10/93 B Judge Dredd (L-6)
DRIVER(jd,l7)           //WPC-220:  12/93 B Judge Dredd (L-7)
DRIVER(sttng,p5)        //WPC-523:  11/93 W Star Trek: The Next Generation (P-5)
DRIVER(sttng,l7)        //WPC-523:  02/94 W Star Trek: The Next Generation (LX-7)
DRIVER(sttng,s7)        //WPC-523:  02/94 W Star Trek: The Next Generation (LX-7) SP1
DRIVER(dm,lx4)          //WPC-528:  02/94 W Demolition Man (LX-4)
DRIVER(dm,pa2)          //WPC-528:  02/94 W Demolition Man (PA-2)
DRIVER(dm,px5)          //WPC-528:  02/94 W Demolition Man (PX-5)
DRIVER(pop,lx5)         //WPC:      02/94 B Popeye Saves the Earth
DRIVER(wcs,l2)          //WPC-531:  02/94 B World Cup Soccer (Lx-2)
DRIVER(wcs,p2)                            // (Pa-2)
DRIVER(wcs,p3)                            // (Px-3)
                        //WPC-620:  06/94 W Pinball Circus
DRIVER(fs,lx2)          //WPC-529:  07/94 W The Flintstones (LX-2)
DRIVER(fs,sp2)          //WPC-529:  07/94 W The Flintstones (SP-2)
DRIVER(fs,lx4)          //WPC-529:  09/94 W The Flintstones (LX-4)
DRIVER(fs,lx5)          //WPC-529:  11/94 W The Flintstones (LX-5)
DRIVER(corv,21)         //WPC-536:  08/94 B Corvette (2.1)
DRIVER(corv,lx1)        //WPC-536:  08/94 B Corvette (LX1)
DRIVER(corv,px4)        //WPC-536:  08/94 B Corvette (PX4)
DRIVER(rs,lx2)          //WPC-524:  10/94 W Red & Ted's Road Show (Lx_2)
DRIVER(rs,lx5)          //WPC-524:  10/94 W Red & Ted's Road Show (Lx_5)
DRIVER(rs,la5)          //WPC-524:  10/94 W Red & Ted's Road Show (La_5)
DRIVER(rs,l6)           //WPC-524:  10/94 W Red & Ted's Road Show (L_6)
DRIVER(rs,la4)          //WPC-524:  10/94 W Red & Ted's Road Show (La_4)
DRIVER(rs,lx4)							  // (Lx-4)
DRIVER(rs,lx3)          //WPC-524:  10/94 W Red & Ted's Road Show (Lx_3)
DRIVER(tafg,lx3)        //WPC:      10/94 B The Addams Family Special Collectors Edition
DRIVER(tafg,la2)        //WPC:      10/94 B The Addams Family Special Collectors Edition (LA-2)
DRIVER(tafg,la3)        //WPC:      10/94 B The Addams Family Special Collectors Edition (LA-3)
DRIVER(tafg,h3)							  // Home version
DRIVER(ts,pa1)          //WPC-532:  11/94 B The Shadow (PA-1)
DRIVER(ts,la2)          //WPC-532:  12/94 B The Shadow (LA-2)
DRIVER(ts,la4)          //WPC-532:  02/95 B The Shadow (LA-4)
DRIVER(ts,lx4)          //WPC-532:  02/95 B The Shadow (LX-4)
DRIVER(ts,lx5)          //WPC-532:  05/95 B The Shadow (LX-5)
DRIVER(ts,lh6)          //WPC-532:  05/95 B The Shadow (LH-6, Home version)
DRIVER(dh,lx2)          //WPC-530:  03/95 W Dirty Harry
DRIVER(tom,13)          //WPC-539:  03/95 B Theatre of Magic
DRIVER(tom,12)							  // 1.2
DRIVER(tom,06)							  // 0.6
DRIVER(nf,23x)          //WPC-525:  05/95 W No Fear: Dangerous Sports
DRIVER(i500,11r)        //WPC-526:  06/95 B Indianapolis 500
DRIVER(i500,11b)        //WPC-526:  06/95 B Indianapolis 500 (Belgium)
DRIVER(jm,12r)          //WPC-542:  08/95 W Johnny Mnemonic
DRIVER(wd,03r)          //WPC-544: ?09/95 B Who dunnit (0.3 R)
DRIVER(wd,10r)          //WPC-544: ?11/95 B Who dunnit (1.0 R)
DRIVER(wd,10g)          //WPC-544: ?11/95 B Who dunnit (1.0 German)
DRIVER(wd,10f)          //WPC-544: ?11/95 B Who dunnit (1.0 French)
DRIVER(wd,11)           //WPC-544:  04/95 B Who dunnit (1.1)
DRIVER(wd,12)           //WPC-544:  05/95 B Who dunnit (1.2)
DRIVER(wd,12g)          //WPC-544:  05/95 B Who dunnit (1.2 German)
DRIVER(jb,10r)          //WPC-551:  10/95 W Jack*Bot
DRIVER(jb,10b)                            // Belgium/Canada
DRIVER(congo,21)        //WPC-550:  11/95 W Congo (2.1)
DRIVER(congo,20)        //WPC-550:  11/95 W Congo (2.0)
DRIVER(afm,10)          //WPC-541:  12/95 B Attack from Mars (1.0)
DRIVER(afm,11)          //WPC-541:  12/95 B Attack from Mars
DRIVER(afm,113)							  // Home version
DRIVER(afm,113b)						  // (1.13b Coin Play)
DRIVER(lc,11)           //WPC:      ??/96 B League Champ (Shuffle Alley)
DRIVER(ttt,10)          //WPC-905:  03/96 W Ticket Tac Toe
DRIVER(sc,14)           //WPC-903:  06/96 B Safe Cracker (1.4)
DRIVER(sc,17)           //WPC-903:  11/96 B Safe Cracker (1.7)
DRIVER(sc,17n)          //WPC-903:  11/96 B Safe Cracker (1.7, alternate version)
DRIVER(sc,18)           //WPC-903:  04/98 B Safe Cracker (1.8)
DRIVER(sc,18n)          //WPC-903:  04/98 B Safe Cracker (1.8, alternate version)
DRIVER(sc,18s2)         //WPC-903:  04/98 B Safe Cracker (1.8, alternate sound)
DRIVER(totan,14)        //WPC-547:  05/96 W Tales of the Arabian Nights (1.4)
DRIVER(totan,13)        //WPC-547:  05/96 W Tales of the Arabian Nights (1.3)
DRIVER(totan,12)        //WPC-547:  05/96 W Tales of the Arabian Nights (1.2)
DRIVER(ss,15)           //WPC-548:  09/96 B Scared Stiff (1.5)
DRIVER(ss,14)           //WPC-548:  09/96 B Scared Stiff (1.4)
DRIVER(ss,12)           //WPC-548:  09/96 B Scared Stiff (1.2)
DRIVER(jy,03)           //WPC-552:  10/96 W Junk Yard (0.3)
DRIVER(jy,11)           //WPC-552:  01/97 W Junk Yard (1.1)
DRIVER(jy,12)           //WPC-552:  07/97 W Junk Yard (1.2)
DRIVER(nbaf,11)         //WPC-553:  03/97 B NBA Fastbreak (1.1)
DRIVER(nbaf,11a)        //WPC-553:  03/97 B NBA Fastbreak (1.1 - S2.0)
DRIVER(nbaf,115)        //WPC-553:  05/97 B NBA Fastbreak (1.15)
DRIVER(nbaf,21)         //WPC-553:  05/97 B NBA Fastbreak (2.1)
DRIVER(nbaf,22)         //WPC-553:  05/97 B NBA Fastbreak (2.2)
DRIVER(nbaf,23)         //WPC-553:  06/97 B NBA Fastbreak (2.3)
DRIVER(nbaf,31)         //WPC-553:  09/97 B NBA Fastbreak (3.1)
DRIVER(nbaf,31a)        //WPC-553:  09/97 B NBA Fastbreak (3.1a)
DRIVER(mm,05)           //WPC-559:  06/97 W Medieval Madness (0.5)
DRIVER(mm,10)           //WPC-559:  07/97 W Medieval Madness (1.0)
DRIVER(mm,109)          //WPC-559:  06/99 W Medieval Madness (1.09, Home version)
DRIVER(mm,109b)         //WPC-559:  06/99 W Medieval Madness (1.09B, Home version Coin Play)
DRIVER(mm,109c)         //WPC-559:  06/99 W Medieval Madness (1.09C, Home version w/ profanity speech)
DRIVER(cv,14)           //WPC-562:  10/97 B Cirqus Voltaire (1.4)
DRIVER(cv,10)           //WPC-562:  10/97 B Cirqus Voltaire (1.0)
DRIVER(cv,11)           //WPC-562:  10/97 B Cirqus Voltaire (1.1)
DRIVER(cv,13)           //WPC-562:  10/97 B Cirqus Voltaire (1.3)
DRIVER(cv,20h)                            // Home version
DRIVER(ngg,13)          //WPC-561:  12/97 W No Good Gofers
DRIVER(ngg,p06)                           // Prototype
DRIVER(cp,16)           //WPC-563:  04/98 B The Champion Pub (1.6)
DRIVER(cp,15)           //WPC-563:  04/98 B The Champion Pub (1.5)
DRIVER(mb,10)           //WPC-565:  07/98 W Monster Bash
DRIVER(mb,106)                            // Home version
DRIVER(mb,106b)                           // (1.06b Coin Play)
DRIVER(cc,12)           //WPC-566:  10/98 B Cactus Canyon
DRIVER(cc,13)							  // 1.3 version
//Test Fixtures
DRIVER(tfdmd,l3)        //WPC:              Test fixture DMD
DRIVER(tfs,12)          //WPC-648:          Test fixture Security
DRIVER(tfa,13)          //WPC:              Test fixture Alphanumeric
DRIVER(tf95,12)         //WPC-648:          Test fixture WPC95

// ------------------
// ZACCARIA GAMES
// ------------------
//Other games created earlier by Zaccaria are EM
                        //10/77 Combat
DRIVERNV(wsports)       //01/78 Winter Sports
DRIVERNV(hod)           //07/78 House of Diamonds
DRIVERNV(strike)        //09/78 Strike
                        //10/78 Ski Jump
DRIVERNV(futurwld)      //10/78 Future World
DRIVERNV(strapids)      //04/79 Shooting the Rapids
DRIVERNV(hotwheel)      //09/79 Hot Wheels
                        //09/79 Space City
DRIVERNV(firemntn)      //01/80 Fire Mountain
DRIVERNV(stargod)       //05/80 Star God
DRIVERNV(stargoda)      //05/80 Star God (alternate sound)
DRIVERNV(sshtlzac)      //09/80 Space Shuttle
DRIVERNV(ewf)           //04/81 Earth, Wind & Fire
DRIVERNV(locomotn)      //09/81 Locomotion
DRIVERNV(pinchp82)      //04/82 Pinball Champ '82 (German speech - is this really different from the '83?)
DRIVERNV(socrking)      //09/82 Soccer Kings
DRIVERNV(socrkngg)      //09/82 Soccer Kings (German speech)
DRIVERNV(pinchamp)      //??/83 Pinball Champ
DRIVERNV(tmachzac)      //04/83 Time Machine
DRIVERNV(farfalla)      //09/83 Farfalla
DRIVERNV(farfalli)      //09/83 Farfalla (Italian speech)
DRIVERNV(dvlrider)      //04/84 Devil Riders
DRIVERNV(dvlrideg)      //04/84 Devil Riders (German speech)
DRIVERNV(mcastle)       //09/84 Magic Castle
DRIVERNV(mcastleg)      //09/84 Magic Castle (German speech)
DRIVERNV(myststar)      //??/84 Mystic Star
DRIVERNV(robot)         //01/85 Robot
DRIVERNV(robotg)        //01/85 Robot (German speech)
DRIVERNV(clown)         //07/85 Clown
DRIVERNV(poolcham)      //12/85 Pool Champion
DRIVERNV(poolchap)      //12/85 Pool Champion (alternate sound)
DRIVERNV(bbeltzac)      //??/86 Blackbelt
DRIVERNV(mexico)        //??/86 Mexico '86
DRIVERNV(zankor)        //??/86 Zankor
DRIVERNV(spooky)        //??/87 Spooky
DRIVERNV(strsphnx)      //??/87 Star's Phoenix
                        //??/86 New Star's Phoenix

#endif /* DRIVER_RECURSIVE */
