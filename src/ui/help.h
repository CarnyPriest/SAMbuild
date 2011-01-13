/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2003 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/

#ifndef HELP_H
#define HELP_H
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

#include <htmlhelp.h>

typedef struct
{
	int		nMenuItem;
	BOOL	bIsHtmlHelp;
	LPCSTR	lpFile;
} MAMEHELPINFO;

extern const MAMEHELPINFO g_helpInfo[];

#if !defined(MAME32HELP)
#define MAME32HELP "mame32.chm"
#endif

#if !defined(MAME32CONTEXTHELP)
#define MAME32CONTEXTHELP "mame32.chm::/cntx_help.txt"
#endif

extern int HelpInit(void);
extern void HelpExit(void);
extern HWND HelpFunction(HWND hwndCaller, LPCSTR pszFile, UINT uCommand, DWORD_PTR dwData);

#endif
