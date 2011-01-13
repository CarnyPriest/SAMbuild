/**********************************************************************************************
 *
 *   Data East BSMT2000 driver
 *   by Aaron Giles
 *
 **********************************************************************************************/


#ifndef BSMT2000_H
#define BSMT2000_H
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

#define MAX_BSMT2000 1

struct BSMT2000interface
{
        int num;                                /* total number of chips */
        int baseclock[MAX_BSMT2000];            /* input clock */
        int voices[MAX_BSMT2000];               /* number of voices (11 or 12) */
        int region[MAX_BSMT2000];               /* memory region where the sample ROM lives */
        int mixing_level[MAX_BSMT2000];         /* master volume */
#ifdef PINMAME
        int use_de_rom_banking;                 /* Special flag to use Data East rom bank handling */
        int shift_data;                         /* Shift integer to apply to samples for changing volume - this is most likely done external to the bsmt chip in the real hardware */
        int reverse_stereo;                     /* Special flag to determine if left and right channels should be reversed */
#endif
};

int BSMT2000_sh_start(const struct MachineSound *msound);
void BSMT2000_sh_stop(void);
void BSMT2000_sh_reset(void);

WRITE16_HANDLER( BSMT2000_data_0_w );

#endif
