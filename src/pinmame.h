#ifndef INC_PINMAME
#define INC_PINMAME

#define PINMAME_EXT   1   // PinMAME extensions added to MAME source
#define PINMAME_EXIT  1   // Use Machine->exitfunc (normally only in MESS)
#define WPCDCSSPEEDUP 1   // DCS Speedup added to MAME ADSP emulation
#define DBG_BPR       1   // BPR command added to debugger

#define TINY_COMPILE
#define NEOFREE

#if MAMEVER > 3716
#define BMTYPE UINT16
#define M65C02_INT_IRQ M65C02_IRQ_LINE
#define M65C02_INT_NMI INTERRUPT_NMI
#define VIDEO_MODIFIES_PALETTE 0
#else  /* MAMEVER */
#define BMTYPE UINT8
#define mame_bitmap osd_bitmap
#endif /* MAMEVER */

#ifdef _MSC_VER // These must be in the makefile for WIN32 & DOS
// CPUs
#define HAS_M6809    1
#define HAS_M6808    1
#define HAS_M6800    1
#define HAS_M6803    1
#define HAS_M6802    1
#define HAS_ADSP2105 1
#define HAS_Z80      1
#define HAS_M6502    1
#define HAS_M65C02   1
#define HAS_M68000   1
#define HAS_S2650    1

// Sound
#define HAS_DAC        1
#if MAMEVER > 3716
#define HAS_YM2151_ALT 1
#define HAS_YM2610     1 // To avoid compile errors in fm.c
#else /* MAMEVER */
#define HAS_YM2151     1
#endif /* MAMEVER */
#define HAS_HC55516    1
#define HAS_SAMPLES    1
#define HAS_TMS5220    1
#define HAS_AY8910     1
#define HAS_MSM5205    1
#define HAS_CUSTOM     1
#define HAS_BSMT2000   1
#define HAS_OKIM6295   1
#define HAS_ADPCM      1
#endif /* _MSC_VER */

#ifdef _MSC_VER // Disable some VC++ warnings
// The IDE project manager can't seem to handle "INLINE=static __inline" as
// part of the project settings without screwing up, so the project declares
// "INLINE=__inline", and we overwrite it here with the proper expansion

#ifdef INLINE
#undef INLINE
#endif

#define INLINE static __inline

#pragma warning(disable:4018)		// "signed/unsigned mismatch"
#pragma warning(disable:4022)		// "pointer mismatch for actual parameter"
#pragma warning(disable:4090)		// "different 'const' qualifiers"
#pragma warning(disable:4142)		// "benign redefinition of type"
#pragma warning(disable:4146)		// "unary minus operator applied to unsigned type"
#pragma warning(disable:4244)		// "possible loss of data"
#pragma warning(disable:4305)		// "truncation from 'type' to 'type'
#pragma warning(disable:4761)		// "integral size mismatch in argument"
#pragma warning(disable:4068)
#pragma warning(disable:4005)
#define M_PI 3.14159265358
#define strcasecmp stricmp
#define snprintf _snprintf
#ifndef DD_OK
#define DD_OK DS_OK
#endif /* DD_OK */
#endif /* _MSC_VER */

#endif /* INC_PINMAME */
