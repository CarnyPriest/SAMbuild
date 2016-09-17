#ifndef VOTRAX_H
#define VOTRAX_H
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

#define MAX_VOTRAXSC01 1

typedef void (*VOTRAXSC01_BUSYCALLBACK)(int);

struct VOTRAXSC01interface
{
        int num;												/* total number of chips */
        int mixing_level[MAX_VOTRAXSC01];						/* master volume */
		int baseFrequency[MAX_VOTRAXSC01];						/* base frequency */
		VOTRAXSC01_BUSYCALLBACK BusyCallback[MAX_VOTRAXSC01];	/* callback function when busy signal changes */
};

int VOTRAXSC01_sh_start(const struct MachineSound *msound);
void VOTRAXSC01_sh_stop(void);

WRITE_HANDLER(votraxsc01_w);
READ_HANDLER(votraxsc01_status_r);

void votraxsc01_set_base_frequency(int baseFrequency);
void votraxsc01_set_volume(int volume);

#endif
