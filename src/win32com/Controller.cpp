/*

  Changes:
	01/04/29 (TOM): Product version handling changed, removed the m_iProductVersion member variable,
					fixed the version check in .run() and added the version property to the interface
	01/07/26 (SJE): Added MessageBox confirmation if game is flagged not working!
					Added Warning message for game flagged with NO SOUND!
	01/08/12 (SJE): Added Warning message for game flagged with IMPERFECT SOUND
	01/09/26 (SJE): Added Check to see if Invalid CRC games can be run.
	03/09/22 (SJE): Added IMPERFECT GRAPHICS FLAG and reworked code to allow more than 1 message to display together
*/

// Controller.cpp : Implementation of Controller and DLL registration.

#include "stdafx.h"
#include "VPinMAME_h.h"
#include "VPinMAMEAboutDlg.h"
#include "VPinMAMEConfig.h"

#include "Controller.h"
#include "ControllerDisclaimerDlg.h"
#include "ControllerGames.h"
#include "ControllerRegkeys.h"

extern "C" {
#include "driver.h"
#include "core.h"
#include "vpintf.h"
#include "mame.h"
#include "video.h"

extern HWND win_video_window;
extern int g_fPause;
extern HANDLE g_hEnterThrottle;
extern int g_iSyncFactor;
extern struct RunningMachine *Machine;
extern struct mame_display *current_display_ptr;

extern UINT8  g_raw_dmdbuffer[DMD_MAXY*DMD_MAXX];
extern UINT32 g_raw_colordmdbuffer[DMD_MAXY*DMD_MAXX];
extern UINT32 g_raw_dmdx;
extern UINT32 g_raw_dmdy;
extern UINT32 g_needs_DMD_update;
extern int g_cpu_affinity_mask;

extern char g_fShowWinDMD;

// from ticker.c
extern void uSleep(const UINT64 u);
}
#include "alias.h"

extern int fAllowWriteAccess;
extern int synclevel;

void CreateEventWindow(CController* pController);
void DestroyEventWindow(CController* pController);

/////////////////////////////////////////////////////////////////////////////
//

/***********************/
/*COM ERROR INTERFACE  */
/***********************/
STDMETHODIMP CController::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IController,
	};

	for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}


/****************************************/
/*Get product version from the resource *
/****************************************/
void CController::GetProductVersion(int *nVersionNo0, int *nVersionNo1, int *nVersionNo2, int *nVersionNo3)
{
	DWORD   dwVerHandle; 
	DWORD	dwInfoSize;
	HANDLE  hVersionInfo;
	LPVOID  lpEntryInfo;
	UINT    wVarSize;
	
	VS_FIXEDFILEINFO *pFixedFileInfo;

	char szFilename[MAX_PATH];
	GetModuleFileName(_Module.m_hInst, szFilename, sizeof szFilename);

	if ( !(dwInfoSize = GetFileVersionInfoSize((LPSTR) szFilename, &dwVerHandle)) ) {
		return;
	}
	
	hVersionInfo = GlobalAlloc(GPTR, dwInfoSize);
	if ( !hVersionInfo )
		return;

	lpEntryInfo = GlobalLock(hVersionInfo);
	if ( !lpEntryInfo ) {
		GlobalFree(hVersionInfo);
		return;
	}

	if ( !GetFileVersionInfo((LPSTR) szFilename, dwVerHandle, dwInfoSize, lpEntryInfo) ) {
		GlobalUnlock(hVersionInfo);
		GlobalFree(hVersionInfo);
		return;
	}

	if ( !VerQueryValue(lpEntryInfo, "\\", (void**) &pFixedFileInfo, &wVarSize) ) {
		GlobalUnlock(hVersionInfo);
		GlobalFree(hVersionInfo);
		return;
	}

	if ( nVersionNo0 )
		*nVersionNo0 = (int) HIWORD(pFixedFileInfo->dwProductVersionMS);
	
	if ( nVersionNo1 )
		*nVersionNo1 = (int) LOWORD(pFixedFileInfo->dwProductVersionMS);

	if ( nVersionNo2 )
		*nVersionNo2 = (int) HIWORD(pFixedFileInfo->dwProductVersionLS);

	if ( nVersionNo3 )
		*nVersionNo3 = (int) LOWORD(pFixedFileInfo->dwProductVersionLS);

	GlobalUnlock(hVersionInfo);
	GlobalFree(hVersionInfo);
}

/*************************************************
 * IController: Class construction and destruction
 *************************************************/
CController::CController() {
	MMRESULT result;

	result = timeGetDevCaps(&caps, sizeof(caps));
	if (result == TIMERR_NOERROR)
		timeBeginPeriod(caps.wPeriodMin);
	cli_frontend_init();

	lstrcpy(m_szSplashInfoLine, "");

	m_hThreadRun    = INVALID_HANDLE_VALUE;
	m_hEmuIsRunning = CreateEvent(NULL, TRUE, FALSE, NULL);
	m_hEventWnd = 0;

	lstrcpy(m_szROM,"");
	m_nGameNo = -1;

	LoadGlobalSettings();

	// get a pointer to out ControllerSettings object
	HRESULT hr = CComObject<CControllerSettings>::CreateInstance(&m_pControllerSettings);
	if ( SUCCEEDED(hr) ) 
		m_pControllerSettings->AddRef();

	hr = CComObject<CGames>::CreateInstance(&m_pGames);
	if ( SUCCEEDED(hr) )
		m_pGames->AddRef();

	// get a pointer to the settings object for the "default" game
	m_pGames->get_Item(&CComVariant(m_szROM), &m_pGame);
	m_pGame->get_Settings((IGameSettings**) &m_pGameSettings);

	// these value are not stored to the registry
	m_fHandleKeyboard = FALSE;
	m_iHandleMechanics = 0xFF;
	m_fDisplayLocked = FALSE;

	char szRegKey[256];
	lstrcpy(szRegKey, REG_BASEKEY);

	fAllowWriteAccess = ReadRegistry(szRegKey, REG_DWALLOWWRITEACCESS, REG_DWALLOWWRITEACCESSDEF);

	vp_init();

	// Never started before? 
	// Check this by asking if the default registry key is existing
	if ( GameUsedTheFirstTime("") ) {
		ShowAboutDialog(0);
		MessageBox(GetActiveWindow(),"Welcome to Visual PinMAME! \n Please setup the default game options by clicking \"OK\"!","Notice!",MB_OK | MB_ICONINFORMATION);
		m_pControllerSettings->ShowSettingsDlg(0);
		m_pGameSettings->ShowSettingsDlg(0);
	}
}

CController::~CController() {
	Stop();
	CloseHandle(m_hEmuIsRunning);
	timeEndPeriod(caps.wPeriodMin);
	m_pGame->Release();
	m_pGameSettings->Release();
	m_pGames->Release();
	m_pControllerSettings->Release();

	cli_frontend_exit();
}


/*****************************************
 * IController.Run method: start emulation
 *****************************************/
STDMETHODIMP CController::Run(/*[in]*/ long hParentWnd, /*[in,defaultvalue(100)]*/ int nMinVersion)
{
	/*Make sure GameName Specified!*/
	if (!m_szROM)
		return Error(TEXT("Game name not specified!"));

	int nVersionNo0, nVersionNo1;
	GetProductVersion(&nVersionNo0, &nVersionNo1, NULL, NULL);

	if ( (nVersionNo0*100+nVersionNo1)<nMinVersion )
		return Error(TEXT("You need a newer version of Visual PinMame to run this emulation!"));

	if ( m_hThreadRun!=INVALID_HANDLE_VALUE ) {
		if ( WaitForSingleObject(m_hThreadRun, 0)==WAIT_TIMEOUT )
			return Error(TEXT("Emulation already running!"));
		else {
			CloseHandle(m_hThreadRun);
			m_hThreadRun = INVALID_HANDLE_VALUE;
		}
	}

	char szTemp[256];

	if (m_nGameNo<0) {
		sprintf(szTemp, "Machine '%s' not found! Invalid game name, or game name not set!", m_szROM);
		return Error(TEXT(szTemp));
	}

	// set the parent window
	m_hParentWnd = (HWND) hParentWnd;

	// if we don't have gotten a window handle try to find the VP player window
	if ( !hParentWnd || !IsWindow(m_hParentWnd) ) {
		m_hParentWnd = FindWindow("VPPlayer", "Visual Pinball Player");

		if ( !IsWindow(m_hParentWnd) )
			m_hParentWnd = (HWND) 0;
	}

	VARIANT vValue;
	VariantInit(&vValue);
	m_pGameSettings->get_Value(CComBSTR("sound"), &vValue);

	int iCheckVal = IDOK;

	m_pGame->ShowInfoDlg(0x8000|CHECKOPTIONS_SHOWRESULTSIFFAIL|((vValue.boolVal==VARIANT_TRUE)?0x0000:CHECKOPTIONS_IGNORESOUNDROMS), (long) m_hParentWnd, &iCheckVal);
	if ( iCheckVal==IDCANCEL ) {
		sprintf(szTemp, "Game ROMs for '%s' (%s) invalid!", m_szROM, drivers[m_nGameNo]->description);
		return Error(TEXT(szTemp));
	}

	VariantInit(&vValue);
	m_pGameSettings->get_Value(CComBSTR("cabinet_mode"), &vValue);
	BOOL cabinetMode = (vValue.boolVal==VARIANT_TRUE);

	if ( GameWasNeverStarted(m_szROM) ) {
		int fFirstTime = GameUsedTheFirstTime(m_szROM);

		if(!cabinetMode) {
		CComBSTR sDescription;
		m_pGame->get_Description(&sDescription);
		char szDescription[256];
		WideCharToMultiByte(CP_ACP, 0, sDescription, -1, szDescription, sizeof szDescription, NULL, NULL);

		if ( !ShowDisclaimer(m_hParentWnd, szDescription) )
			return Error(TEXT("User is not legally entitled to play this game!"));
		}

		SetGameWasStarted(m_szROM);

		if(!cabinetMode) {
		if ( fFirstTime ) {
			char szTemp[256];
			sprintf(szTemp,"This is the first time you use: %s\nPlease specify options for this game by clicking \"OK\"!",drivers[m_nGameNo]->description);
			MessageBox(GetActiveWindow(),szTemp,"Notice!",MB_OK | MB_ICONINFORMATION);
			m_pGameSettings->ShowSettingsDlg(0);
		}
		}
	}	

#ifndef DEBUG
	if(!cabinetMode) {
	// See if game is flagged as GAME_NOT_WORKING and ask user if they wish to continue!!
	if ( drivers[m_nGameNo]->flags&GAME_NOT_WORKING ) {
		if(MessageBox(GetActiveWindow(),"This game DOES NOT WORK properly!\n Running it may cause VPinMAME to crash or lock up \n\n Are you sure you would like to continue?","Notice!",MB_YESNO | MB_ICONWARNING) == IDNO)
			return 1;
	}
	// See if the game is flagged as GAME_NOCRC so that the CRC *must* be correct
	if ((iCheckVal!=IDOK) && (drivers[m_nGameNo]->flags & GAME_NOCRC)) {
		MessageBox(GetActiveWindow(),"This game can only run with the EXACT romset required!","Notice!",MB_OK | MB_ICONINFORMATION);
		sprintf(szTemp, "CRC Errors! Game '%s' (%s) cannot be run!", m_szROM, drivers[m_nGameNo]->description);
		return Error(TEXT(szTemp));
	}

		//Any game messages to display (messages that allow game to continue)
	if ( drivers[m_nGameNo]->flags )
	{
		sprintf(szTemp,"");

		// See if game is flagged as GAME_NO_SOUND and show user a message!
		if ( drivers[m_nGameNo]->flags&GAME_NO_SOUND )
			sprintf(szTemp,"%sPlease be aware that this game does not have sound emulated yet!\n\n",szTemp);

		// See if game is flagged as GAME_IMPERFECT_SOUND and show user a message!
		if ( drivers[m_nGameNo]->flags&GAME_IMPERFECT_SOUND )
			sprintf(szTemp,"%sPlease be aware that this game's sound emulation is not working 100%% properly!\n\n",szTemp);

		// See if game is flagged as GAME_IMPERFECT_GRAPHICS and show user a message!
		if ( drivers[m_nGameNo]->flags&GAME_IMPERFECT_GRAPHICS )
			sprintf(szTemp,"%sPlease be aware that this game's graphics are not working 100%% properly!\n\n",szTemp);

		if(strlen(szTemp)>0)
			MessageBox(GetActiveWindow(),szTemp,"Notice!",MB_OK | MB_ICONINFORMATION);
	}
	}
#endif

	ResetEvent(m_hEmuIsRunning);

	CreateEventWindow(this);

	DWORD dwThreadID;
	m_hThreadRun = /*_beginthreadex*/CreateThread(NULL,
								0,
								(LPTHREAD_START_ROUTINE) RunController,
								(LPVOID) this,
								0, &dwThreadID);

	if ( !dwThreadID ) {
		DestroyEventWindow(this);
		return Error(TEXT("Unable to start thread!"));
	}

	if (g_cpu_affinity_mask > 0)
		SetThreadAffinityMask(m_hThreadRun, g_cpu_affinity_mask);

	// ok, let's wait for either the machine is set up or the thread terminates for some reason
	HANDLE StartHandles[2] = {m_hEmuIsRunning, m_hThreadRun};

	if ( WaitForMultipleObjects(2, StartHandles, FALSE, INFINITE)==WAIT_OBJECT_0)  {
		// fine, here we go
		return S_OK;
	}

	CloseHandle(m_hThreadRun);
	m_hThreadRun = INVALID_HANDLE_VALUE;

	DestroyEventWindow(this);

	sprintf(szTemp, "Machine '%s' (%s) terminated before intialized, check the rom path or rom file!", m_szROM, drivers[m_nGameNo]->description);
	return Error(TEXT(szTemp));
}

/*********************************************
 * IController.Stop() method: stop emulation 
 *********************************************/
STDMETHODIMP CController::Stop()
{
	if ( m_hThreadRun==INVALID_HANDLE_VALUE )
		return S_OK;

	PostMessage(win_video_window, WM_CLOSE, 0, 0);
	WaitForSingleObject(m_hThreadRun,INFINITE);
	
	CloseHandle(m_hThreadRun);
	m_hThreadRun = INVALID_HANDLE_VALUE; 

	DestroyEventWindow(this);

	return S_OK;
}

/****************************************************************
 * IController.Lamp property (read only): get the state of a lamp
 ****************************************************************/
STDMETHODIMP CController::get_Lamp(int nLamp, VARIANT_BOOL *pVal)
{
  if (!pVal) return S_FALSE;

  if (WaitForSingleObject(m_hEmuIsRunning, 0) == WAIT_TIMEOUT)
    *pVal= false;
  else 
    *pVal = vp_getLamp(nLamp)?VARIANT_TRUE:VARIANT_FALSE;

  return S_OK;
}

/************************************************************************
 * IController.Solenoid property (read only): get the state of a solenoid
 ************************************************************************/
STDMETHODIMP CController::get_Solenoid(int nSolenoid, VARIANT_BOOL *pVal)
{
  if (!pVal) return S_FALSE;

  if (WaitForSingleObject(m_hEmuIsRunning, 0) == WAIT_TIMEOUT)
    *pVal= false;
  else
    *pVal = vp_getSolenoid(nSolenoid)?VARIANT_TRUE:VARIANT_FALSE;
  return S_OK;
}

/************************************************************************
 * IController.Switch property: get/set the state of a switch
 ************************************************************************/
STDMETHODIMP CController::get_Switch(int nSwitchNo, VARIANT_BOOL *pVal) {
  if (pVal)
    if (WaitForSingleObject(m_hEmuIsRunning, 0) == WAIT_TIMEOUT)
      *pVal = false;
    else 
      *pVal = vp_getSwitch(nSwitchNo)?VARIANT_TRUE:VARIANT_FALSE;
  return S_OK;
}

STDMETHODIMP CController::put_Switch(int nSwitchNo, VARIANT_BOOL newVal)
{
  if (WaitForSingleObject(m_hEmuIsRunning, 0) == WAIT_TIMEOUT)
    return S_OK;
  else
    vp_putSwitch(nSwitchNo, newVal);
  return S_OK;
}

/*************************************************
 * IController.Pause property: Pause the emulation
 *************************************************/
STDMETHODIMP CController::get_Pause(VARIANT_BOOL *pVal)
{
	*pVal = (WaitForSingleObject(m_hEmuIsRunning, 0)==WAIT_OBJECT_0)?(g_fPause?VARIANT_TRUE:VARIANT_FALSE):VARIANT_FALSE;
	return S_OK;
}

STDMETHODIMP CController::put_Pause(VARIANT_BOOL newVal)
{
	if (WaitForSingleObject(m_hEmuIsRunning, 0) == WAIT_TIMEOUT)
		return S_OK;

	g_fPause = newVal;

	return S_OK;
}

/***************************************************************************************
 * IController.WPCNumbering property (read-only): returns if WPCNumbering is used or not
 ***************************************************************************************/
STDMETHODIMP CController::get_WPCNumbering(/*[out, retval]*/ VARIANT_BOOL *pVal)
{
	if (!pVal) return S_FALSE;

	if (WaitForSingleObject(m_hEmuIsRunning, 0) == WAIT_TIMEOUT)
		*pVal= 0;
	else
		*pVal = vp_getWPCNumbering()?VARIANT_TRUE:VARIANT_FALSE;
	
	return S_OK;
}

/*********************************************************************
 * IController.Lamps property (read-only): gets the state of all lamps
 *********************************************************************/
STDMETHODIMP CController::get_Lamps(VARIANT *pVal)
{
	if ( !pVal )
		return S_FALSE;
	
	SAFEARRAY *psa = SafeArrayCreateVector(VT_VARIANT, 0, 89);
	
	VARIANT LampState;
	LampState.vt = VT_BOOL;

	long ix;

	if ( WaitForSingleObject(m_hEmuIsRunning, 0)==WAIT_TIMEOUT ) {
		LampState.boolVal = VARIANT_FALSE;
		for (ix=11; ix<89; ix++)
			SafeArrayPutElement(psa, &ix, &LampState);
	}
	else {
		for (ix=0; ix<89; ix++) {
			LampState.boolVal = vp_getLamp(ix)?VARIANT_TRUE:VARIANT_FALSE;
			SafeArrayPutElement(psa, &ix, &LampState);
		};
	}
	pVal->vt = VT_ARRAY|VT_VARIANT;
	pVal->parray = psa;
	return S_OK;
}

/************************************************************************
 * IController.RawDmdWidth property (read-only): get the width of the DMD
 ************************************************************************/
STDMETHODIMP CController::get_RawDmdWidth(int *pVal)
{
	*pVal = g_raw_dmdx;
	return S_OK;
}

/**************************************************************************
 * IController.RawDmdHeight property (read-only): get the height of the DMD
 **************************************************************************/
STDMETHODIMP CController::get_RawDmdHeight(int *pVal)
{
	*pVal = g_raw_dmdy;
	return S_OK;
}

/**************************************************************************
* IController.NVRAM (read-only): Copy whole NVRAM to a self allocated array
***************************************************************************/
STDMETHODIMP CController::get_NVRAM(VARIANT *pVal)
{
	if (Machine && Machine->drv && Machine->drv->nvram_handler && pVal)
	{
		// setup a ram file manually (MAME has no mechanism so far)
		mame_file* nvram_file = (mame_file*)malloc(sizeof(mame_file));
		memset(nvram_file, 0, sizeof(mame_file));
		nvram_file->type = RAM_FILE;
		// call nvram handler to write to file
		(*Machine->drv->nvram_handler)(nvram_file, 1);

		if (nvram_file->offset == 0)
		{
			mame_fclose(nvram_file);
			return S_FALSE;
		}

		SAFEARRAY *psa = SafeArrayCreateVector(VT_VARIANT, 0, (ULONG)nvram_file->offset);

		VARIANT NVState;
		NVState.vt = VT_UI1;

		for (LONG ofs = 0; ofs < nvram_file->offset; ++ofs)
		{
			NVState.cVal = nvram_file->data[ofs];
			SafeArrayPutElement(psa, &ofs, &NVState);
		}

		pVal->vt = VT_ARRAY | VT_VARIANT;
		pVal->parray = psa;

		mame_fclose(nvram_file);

		return S_OK;
	}
	else
		return S_FALSE;
}

static UINT8 oldNVRAM[CORE_MAXNVRAM];
static char* oldNVRAMname = 0;
static vp_tChgNVRAMs chgNVRAMs; // stack overflow when put into get_ChangedNVRAM??

/***************************************************************
* IController.ChangedNVRAM property: returns a list of the
* numbers of NVRAM locations, which state has changed since the last call
* number is in the first, state in the second part, previous state in the third
***************************************************************/
STDMETHODIMP CController::get_ChangedNVRAM(VARIANT *pVal)
{
	if (!pVal) return S_FALSE;

	if (WaitForSingleObject(m_hEmuIsRunning, 0) == WAIT_TIMEOUT)
	{
		pVal->vt = 0; return S_OK;
	}

	if (!(Machine && Machine->drv && Machine->drv->nvram_handler))
		return S_FALSE;

	// setup a ram file manually (MAME has no mechanism so far)
	mame_file* nvram_file = (mame_file*)malloc(sizeof(mame_file));
	memset(nvram_file, 0, sizeof(mame_file));
	nvram_file->type = RAM_FILE;
	// call nvram handler to write to file
	(*Machine->drv->nvram_handler)(nvram_file, 1);

	if (nvram_file->offset == 0)
	{
		mame_fclose(nvram_file);
		return S_FALSE;
	}

	/*-- if enabled: wait for the worker thread to enter "throttle_speed()" --*/
	if ((g_hEnterThrottle != INVALID_HANDLE_VALUE) && g_iSyncFactor)
		WaitForSingleObject(g_hEnterThrottle, (synclevel <= 20) ? synclevel : 50);
	else if (synclevel<0)
		uSleep(-synclevel * 1000);

	/*-- Count changes --*/
	size_t uCount;

	if (oldNVRAMname == 0 || strstr(Machine->gamedrv->name, oldNVRAMname) == 0) // detect initial VPM start or game change
	{
		uCount = min((size_t)nvram_file->offset, CORE_MAXNVRAM);
		for (size_t i = 0; i < uCount; ++i)
		{
			chgNVRAMs[i].nvramNo = i;
			chgNVRAMs[i].oldStat = 0; //!!
			chgNVRAMs[i].currStat = nvram_file->data[i];
		}
		memcpy(oldNVRAM, nvram_file->data, uCount);

		if (oldNVRAMname)
			free(oldNVRAMname);
		oldNVRAMname = (char*)malloc(strlen(Machine->gamedrv->name) + 1);
		strcpy(oldNVRAMname, Machine->gamedrv->name);


		mame_fclose(nvram_file);
		pVal->vt = 0; return S_OK; //!! for now, as too many changes initially!?!
	}
	else
	{
		uCount = 0;
		size_t uCountMax = min((size_t)nvram_file->offset, CORE_MAXNVRAM);
		for (size_t i = 0; i < uCountMax; ++i)
		{
			if (oldNVRAM[i] != nvram_file->data[i])
			{
				chgNVRAMs[uCount].nvramNo = i;
				chgNVRAMs[uCount].oldStat = oldNVRAM[i];
				chgNVRAMs[uCount].currStat = nvram_file->data[i];
				uCount++;

				oldNVRAM[i] = nvram_file->data[i];
			}
		}
	}

	mame_fclose(nvram_file);

	if (uCount == 0)
	{
		pVal->vt = 0; return S_OK;
	}

	/*-- Create array --*/
	SAFEARRAYBOUND Bounds[] = { { (ULONG)uCount, 0 }, { 3, 0 } };
	SAFEARRAY *psa = SafeArrayCreate(VT_VARIANT, 2, Bounds);
	long ix[2];
	VARIANT varValue;

	varValue.vt = VT_I4;

	/*-- add changed locations to array --*/
	for (ix[0] = 0; ix[0] < (long)uCount; ix[0]++) {
		ix[1] = 0;
		varValue.lVal = chgNVRAMs[ix[0]].nvramNo;
		SafeArrayPutElement(psa, ix, &varValue);
		ix[1] = 1; // NVRAM value
		varValue.lVal = chgNVRAMs[ix[0]].currStat;
		SafeArrayPutElement(psa, ix, &varValue);
		ix[1] = 2; // Old NVRAM value
		varValue.lVal = chgNVRAMs[ix[0]].oldStat;
		SafeArrayPutElement(psa, ix, &varValue);
	}

	pVal->vt = VT_ARRAY | VT_VARIANT;
	pVal->parray = psa;

	return S_OK;
}

/************************************************************************************************
 * IController.RawDmdPixels (read-only): Copy whole DMD to a self allocated array (values 0..100)
 ************************************************************************************************/
STDMETHODIMP CController::get_RawDmdPixels(VARIANT *pVal)
{
	if(Machine && g_needs_DMD_update && (int)g_raw_dmdx > 0 && (int)g_raw_dmdy > 0 && pVal)
	{
		SAFEARRAY *psa = SafeArrayCreateVector(VT_VARIANT, 0, g_raw_dmdx*g_raw_dmdy);

		VARIANT DMDState;
		DMDState.vt = VT_UI1;
	
		LONG ofs = 0;
		for(unsigned int y = 0; y < g_raw_dmdy; ++y)
		for(unsigned int x = 0; x < g_raw_dmdx; ++x,++ofs)
		{
			DMDState.cVal = g_raw_dmdbuffer[ofs];
			SafeArrayPutElement(psa, &ofs, &DMDState);
		}

		pVal->vt = VT_ARRAY|VT_VARIANT;
		pVal->parray = psa;

		g_needs_DMD_update = 0;

		return S_OK;
	}
	else
		return S_FALSE;
}

/******************************************************************************************************
* IController.RawDmdColoredPixels (read-only): Copy whole DMD to a self allocated array (RGB(A) values)
*******************************************************************************************************/
STDMETHODIMP CController::get_RawDmdColoredPixels(VARIANT *pVal)
{
	if(Machine && g_needs_DMD_update && (int)g_raw_dmdx > 0 && (int)g_raw_dmdy > 0 && pVal)
	{
		SAFEARRAY *psa = SafeArrayCreateVector(VT_VARIANT, 0, g_raw_dmdx*g_raw_dmdy);

		VARIANT DMDState;
		DMDState.vt = VT_UI4;

		LONG ofs = 0;
		for(unsigned int y = 0; y < g_raw_dmdy; ++y)
		for(unsigned int x = 0; x < g_raw_dmdx; ++x, ++ofs)
		{
			DMDState.uintVal = g_raw_colordmdbuffer[ofs];
			SafeArrayPutElement(psa, &ofs, &DMDState);
		}

		pVal->vt = VT_ARRAY|VT_VARIANT;
		pVal->parray = psa;

		g_needs_DMD_update = 0;
		
		return S_OK;
	}
	else
		return S_FALSE;
}

/************************************************************************
 * IController.DmdWidth property (read-only): get the width of DMD bitmap
 ************************************************************************/
STDMETHODIMP CController::get_DmdWidth(int *pVal)
{
	*pVal = current_display_ptr ? current_display_ptr->game_visible_area.max_x-current_display_ptr->game_visible_area.min_x+1 : 0;
	return S_OK;
}

/**************************************************************************
 * IController.DmdHeight property (read-only): get the height of DMD bitmap
 **************************************************************************/
STDMETHODIMP CController::get_DmdHeight(int *pVal)
{
	*pVal = current_display_ptr ? current_display_ptr->game_visible_area.max_y-current_display_ptr->game_visible_area.min_y+1 : 0;
	return S_OK;
}

/*************************************************************************
 * IController.DmdPixel (read-only): read a given pixel of the DMD (slow!)
 *************************************************************************/
STDMETHODIMP CController::get_DmdPixel(int x, int y, int *pVal)
{
	if(Machine && Machine->scrbitmap)
		*pVal = Machine->scrbitmap->read(Machine->scrbitmap,x,y);
	else
		*pVal = 0;
	return S_OK;
}

/*******************************************************************************
 * updateDmdPixels  (read-only): Copy whole Dmd Bitmap to a user allocated array
 *******************************************************************************/
STDMETHODIMP CController::get_updateDmdPixels(int **buf, int width, int height, int *pVal)
{
	if(!buf)
	{
		*pVal = 0;
		return S_FALSE;
	}

	mame_bitmap * btm = current_display_ptr ? current_display_ptr->game_bitmap : 0;
	if(Machine && current_display_ptr && btm)
	{
		if(width  != (current_display_ptr->game_visible_area.max_x-current_display_ptr->game_visible_area.min_x+1) || 
		   height != (current_display_ptr->game_visible_area.max_y-current_display_ptr->game_visible_area.min_y+1)  )
		{
			*pVal = 0;
			return S_OK;
		}

		float *dst = reinterpret_cast<float*>(buf);
		if (btm->depth == 8)
		{
			for(int j=current_display_ptr->game_visible_area.max_y;j>=current_display_ptr->game_visible_area.min_y;j--)
			{
				UINT8 *src = (UINT8*)btm->line[j] + current_display_ptr->game_visible_area.min_x;
				for(int i=current_display_ptr->game_visible_area.min_x;i<=current_display_ptr->game_visible_area.max_x;i++)
				{
					UINT8 r,g,b;
					palette_get_color((*src++),&r,&g,&b);
					*(dst++) = (float)r*(float)(1.0/255.0);
					*(dst++) = (float)g*(float)(1.0/255.0);
					*(dst++) = (float)b*(float)(1.0/255.0);
					*(dst++) = 1.0f;
				}
			}
		}
		else if(btm->depth == 15 || btm->depth == 16)
		{
			for(int j=current_display_ptr->game_visible_area.max_y;j>=current_display_ptr->game_visible_area.min_y;j--)
			{
				UINT16 *src = (UINT16*)btm->line[j] + current_display_ptr->game_visible_area.min_x;
				for(int i=current_display_ptr->game_visible_area.min_x;i<=current_display_ptr->game_visible_area.max_x;i++)
				{
					UINT8 r,g,b;
					palette_get_color((*src++),&r,&g,&b);
					*(dst++) = (float)r*(float)(1.0/255.0);
					*(dst++) = (float)g*(float)(1.0/255.0);
					*(dst++) = (float)b*(float)(1.0/255.0);
					*(dst++) = 1.0f;
				}
			}
		}
		else
		{
			for(int j=current_display_ptr->game_visible_area.max_y;j>=current_display_ptr->game_visible_area.min_y;j--)
			{
				UINT32 *src = (UINT32*)btm->line[j] + current_display_ptr->game_visible_area.min_x;
				for(int i=current_display_ptr->game_visible_area.min_x;i<=current_display_ptr->game_visible_area.max_x;i++)
				{
					UINT8 r,g,b;
					palette_get_color((*src++),&r,&g,&b);
					*(dst++) = (float)r*(float)(1.0/255.0);
					*(dst++) = (float)g*(float)(1.0/255.0);
					*(dst++) = (float)b*(float)(1.0/255.0);
					*(dst++) = 1.0f;
				}
			}
		}

		//*pVal = Machine->scrbitmap->read(Machine->scrbitmap,x,y);

		*pVal = 1;
	}
	else
		*pVal = 0;
	return S_OK;
}

/*****************************************************************************************
 * ChangedLampsState (read-only): Copy whole Changed Lamps array to a user allocated array
 *****************************************************************************************/
STDMETHODIMP CController::get_ChangedLampsState(int **buf, int *pVal)
{
	if(!buf)
	{
		*pVal = 0;
		return S_FALSE;
	}

  vp_tChgLamps chgLamps;

  if (!pVal) return S_FALSE;

  if (WaitForSingleObject(m_hEmuIsRunning, 0) == WAIT_TIMEOUT)
    { pVal = 0; return S_OK; }

  /*-- if enabled: wait for the worker thread to enter "throttle_speed()" --*/
  if ( (g_hEnterThrottle!=INVALID_HANDLE_VALUE) && g_iSyncFactor ) 
	WaitForSingleObject(g_hEnterThrottle, (synclevel<=20) ? synclevel : 50);
  else if ( synclevel<0 )
	  uSleep(-synclevel*1000);

  /*-- Count changes --*/
  int uCount = vp_getChangedLamps(chgLamps);

  if (uCount == 0)
    { pVal = 0; return S_OK; }

  /*-- add changed lamps to array --*/
  int *dst = reinterpret_cast<int*>(buf);
  for (int i = 0; i < uCount; i++)
  {
    *(dst++) = chgLamps[i].lampNo;
    *(dst++) = chgLamps[i].currStat;
  }

  *pVal = uCount;

  return S_OK;
}

/**************************************************************************
 * LampsState (read-only): Copy whole Lamps array to a user allocated array
 **************************************************************************/
STDMETHODIMP CController::get_LampsState(int **buf, int *pVal)
{
	if(!buf)
	{
		*pVal = 0;
		return S_FALSE;
	}

	if (!pVal) return S_FALSE;

	/*-- list lamps states to array --*/
	int *dst = reinterpret_cast<int*>(buf);

	if ( WaitForSingleObject(m_hEmuIsRunning, 0)==WAIT_TIMEOUT ) {
		for (int ix=0; ix<89; ix++)
			*(dst++) = 0;
	}
	else {
		for (int ix=0; ix<89; ix++)
			*(dst++) = vp_getLamp(ix) ? 1:0;
	}

	*pVal = 89;

	return S_OK;
}

/*************************************************************************************************
 * ChangedSolenoidsState (read-only): Copy whole Changed Solenoids array to a user allocated array
 *************************************************************************************************/
STDMETHODIMP CController::get_ChangedSolenoidsState(int **buf, int *pVal)
{
	if(!buf)
	{
		*pVal = 0;
		return S_FALSE;
	}

	vp_tChgSols chgSol;

	if (!pVal) return S_FALSE;

	if (WaitForSingleObject(m_hEmuIsRunning, 0) == WAIT_TIMEOUT)
	{ pVal = 0; return S_OK; }

	/*-- if enabled: wait for the worker thread to enter "throttle_speed()" --*/
	if ( (g_hEnterThrottle!=INVALID_HANDLE_VALUE) && g_iSyncFactor ) 
		WaitForSingleObject(g_hEnterThrottle, (synclevel<=20) ? synclevel : 50);
	else if ( synclevel<0 )
		uSleep(-synclevel*1000);

	/*-- Count changes --*/
	int uCount = vp_getChangedSolenoids(chgSol);

	if (uCount == 0)
	{ pVal = 0; return S_OK; }

	/*-- add changed lamps to array --*/
	int *dst = reinterpret_cast<int*>(buf);
	for (int i = 0; i < uCount; i++)
	{
		*(dst++) = chgSol[i].solNo;
		*(dst++) = chgSol[i].currStat;
	}

	*pVal = uCount;

	return S_OK;
}


/**********************************************************************************
 * SolenoidsState (read-only): Copy whole Solenoids array to a user allocated array
 **********************************************************************************/
STDMETHODIMP CController::get_SolenoidsState(int **buf, int *pVal)
{
	if(!buf)
	{
		*pVal = 0;
		return S_FALSE;
	}

	if (!pVal) return S_FALSE;

	/*-- list lamps states to array --*/
	int *dst = reinterpret_cast<int*>(buf);

	if ( WaitForSingleObject(m_hEmuIsRunning, 0)==WAIT_TIMEOUT ) {
		for (int ix=0; ix<65; ix++)
			*(dst++) = 0;
	}
	else {
		for (int ix=0; ix<65; ix++)
			*(dst++) = vp_getSolenoid(ix);
	}

	*pVal = 65;

	return S_OK;
}

/*************************************************************************************
 * ChangedGIsState (read-only): Copy whole Changed GIs array to a user allocated array
 *************************************************************************************/
STDMETHODIMP CController::get_ChangedGIsState(int **buf, int *pVal)
{
	if(!buf)
	{
		*pVal = 0;
		return S_FALSE;
	}

	vp_tChgGIs chgGI;

	if (!pVal) return S_FALSE;

	if (WaitForSingleObject(m_hEmuIsRunning, 0) == WAIT_TIMEOUT)
	{ pVal = 0; return S_OK; }

	/*-- if enabled: wait for the worker thread to enter "throttle_speed()" --*/
	if ( (g_hEnterThrottle!=INVALID_HANDLE_VALUE) && g_iSyncFactor ) 
		WaitForSingleObject(g_hEnterThrottle, (synclevel<=20) ? synclevel : 50);
	else if ( synclevel<0 )
		uSleep(-synclevel*1000);

	/*-- Count changes --*/
	int uCount = vp_getChangedGI(chgGI);

	if (uCount == 0)
	{ pVal = 0; return S_OK; }

	/*-- add changed lamps to array --*/
	int *dst = reinterpret_cast<int*>(buf);
	for (int i = 0; i < uCount; i++)
	{
		*(dst++) = chgGI[i].giNo;
		*(dst++) = chgGI[i].currStat;
	}

	*pVal = uCount;

	return S_OK;
}

/****************************************************************************
 * IController.Switches property: sets/gets the state of all switches at once
 ****************************************************************************/
STDMETHODIMP CController::get_Switches(VARIANT *pVal)
{
	if (!pVal)
		return S_FALSE;

	pVal->vt = 0;

	if ( WaitForSingleObject(m_hEmuIsRunning, 0)==WAIT_TIMEOUT )
		return S_OK;

	SAFEARRAY *psa = SafeArrayCreateVector(VT_VARIANT, 0, 129);

	long ix;

	VARIANT SwitchState;
	SwitchState.vt = VT_BOOL;

	for (ix=0; ix<=128; ix++) {
		SwitchState.boolVal = vp_getSwitch(ix)?VARIANT_TRUE:VARIANT_FALSE;
		SafeArrayPutElement(psa, &ix, &SwitchState);
	};
	
	pVal->vt = VT_ARRAY|VT_VARIANT;
	pVal->parray = psa;
	
	return S_OK;
}

STDMETHODIMP CController::put_Switches(VARIANT newVal)
{
	if ( WaitForSingleObject(m_hEmuIsRunning, 0)==WAIT_TIMEOUT )
		return S_OK;

	// we need an array here
	if ( !(newVal.vt & VT_ARRAY) ) 
		return S_FALSE;

	SAFEARRAY* psa = (newVal.vt & VT_BYREF) ? *newVal.pparray : newVal.parray;

	// check for a one dimensional array
	if ( SafeArrayGetDim(psa)!=1 )
		return S_FALSE;

	long lBound;
	if ( SafeArrayGetLBound(psa, 1, &lBound)!=S_OK )
		return S_FALSE;

	if ( lBound<11 )
		lBound = 11;

	long uBound;
	if ( SafeArrayGetUBound(psa, 1, &uBound)!=S_OK )
		return S_FALSE;

	if ( uBound>118 )
		uBound = 118;

	if ( lBound>uBound)
		return S_OK; // nothing to do...

	VARIANT varValue;
	for (long i=lBound; i<=uBound; i++) {
		SafeArrayGetElement(psa, &i, &varValue);
		// if this fails, return, we currently only support VARIANTs with standard values
		if ( VariantChangeType(&varValue, &varValue, 0, VT_I4)!=S_OK )
			return S_FALSE;
		vp_putSwitch(i, varValue.lVal);
	}

	return S_OK;
}

/******************************************************
 * IController.GameName property: get/set the game name
 ******************************************************/
STDMETHODIMP CController::get_GameName(BSTR *pVal)
{
	CComBSTR Val(m_szROM);
	*pVal = Val.Detach();
	return S_OK;
}

STDMETHODIMP CController::put_GameName(BSTR newVal)
{
	if ( m_hThreadRun!=INVALID_HANDLE_VALUE ) {
		if ( WaitForSingleObject(m_hThreadRun, 0)==WAIT_TIMEOUT )
			return Error(TEXT("Setting the game name is not allowed for a running game!"));
	}

	char szTemp[256];
	WideCharToMultiByte(CP_ACP, 0, newVal, -1, szTemp, sizeof szTemp, NULL, NULL);
    const char* gameName = checkGameAlias(szTemp);
	// don't let the game name set to an invalid value
	int nGameNo = -1;
	if ( gameName[0] && ((nGameNo=GetGameNumFromString(const_cast<char*>(gameName)))<0) )
		return Error(TEXT("Game name not found!"));

	// reset visibility of the controller window to visible
	m_fWindowHidden = !g_fShowWinDMD;

	// reset set use of mechanical samples to false
	m_fMechSamples = false;

	// we allways have a pSettings and a pGame object, so this is save
	m_pGame->Release();
	m_pGameSettings->Release();

	// store the ROM name
	lstrcpy(m_szROM, gameName);
	m_nGameNo = nGameNo;

	// get a pointer to the settings object
	m_pGames->get_Item(&CComVariant(m_szROM), &m_pGame);
	m_pGame->get_Settings((IGameSettings**) &m_pGameSettings);

	return S_OK;
}

/*************************************************************
 * IController.HandleKeyboard property: get/set HandleKeyboard
 *************************************************************/
STDMETHODIMP CController::get_HandleKeyboard(VARIANT_BOOL *pVal)
{
	if (pVal)
		*pVal = m_fHandleKeyboard?VARIANT_TRUE:VARIANT_FALSE;
	return S_OK;
}

STDMETHODIMP CController::put_HandleKeyboard(VARIANT_BOOL newVal)
{
	m_fHandleKeyboard = newVal;

	return S_OK;
}

/***************************************************
 * IController.Machines: get a list of machine names
 ***************************************************/
STDMETHODIMP CController::get_Machines(BSTR sMachine, VARIANT *pVal)
{
	char szMachine[256];
	WideCharToMultiByte(CP_ACP, 0, sMachine, -1, szMachine, sizeof szMachine, NULL, NULL);

	const GameDriver *pMainGameDriver = NULL;

	// looking for compatible games?
	if ( szMachine[0] ) {
		int iNo;
		if ((iNo=GetGameNumFromString(szMachine))<0)  {
			// machine doesn't exist
			pVal->vt = 0;
			return S_OK;
		}

		pMainGameDriver = drivers[iNo]->clone_of;
		if ( !pMainGameDriver || !pMainGameDriver->clone_of )
			pMainGameDriver = drivers[iNo];
	}

	long lCount = pMainGameDriver?1:0;

	long lHelp = 0;
	while ( drivers[lHelp] ) {
		if ( !pMainGameDriver || (drivers[lHelp]->clone_of==pMainGameDriver) )
			lCount++;
		lHelp++;
	}

	SAFEARRAY *psa = SafeArrayCreateVector(VT_VARIANT, 0, lCount);

	CComVariant varMachineName;

	lCount = 0;
	if ( pMainGameDriver ) {
		varMachineName = pMainGameDriver->name;
		SafeArrayPutElement(psa, &lCount, &varMachineName);
		lCount++;
	}

	lHelp = 0;
	while ( drivers[lHelp] ) {
		if ( !pMainGameDriver || (drivers[lHelp]->clone_of==pMainGameDriver) ) {
			varMachineName = drivers[lHelp]->name;
			SafeArrayPutElement(psa, &lCount, &varMachineName);
			lCount++;
		}

		lHelp++;
	}

	pVal->vt = VT_ARRAY|VT_VARIANT;
	pVal->parray = psa;

	return S_OK;
}

/*************************************************************************
 * IController.Running property: is the emulation initialized and running?
 *************************************************************************/
STDMETHODIMP CController::get_Running(VARIANT_BOOL *pVal)
{
	if ( pVal )
		*pVal = (WaitForSingleObject(m_hEmuIsRunning, 0)==WAIT_OBJECT_0)?VARIANT_TRUE:VARIANT_FALSE;

	return S_OK;
}

/***************************************************************
 * IController.ChangedLamps property: returns a list of the 
 * numbers of lamp, which state has changed since the last call
 * number is in the first, state in the second part
 ***************************************************************/
STDMETHODIMP CController::get_ChangedLamps(VARIANT *pVal)
{
  vp_tChgLamps chgLamps;

  if (!pVal) return S_FALSE;

  if (WaitForSingleObject(m_hEmuIsRunning, 0) == WAIT_TIMEOUT)
    { pVal->vt = 0; return S_OK; }

  /*-- if enabled: wait for the worker thread to enter "throttle_speed()" --*/
  if ( (g_hEnterThrottle!=INVALID_HANDLE_VALUE) && g_iSyncFactor ) 
	WaitForSingleObject(g_hEnterThrottle, (synclevel<=20) ? synclevel : 50);
  else if ( synclevel<0 )
	  uSleep(-synclevel*1000);

  /*-- Count changes --*/
  int uCount = vp_getChangedLamps(chgLamps);

  if (uCount == 0)
    { pVal->vt = 0; return S_OK; }

  /*-- Create array --*/
  SAFEARRAYBOUND Bounds[] = {{(ULONG)uCount,0}, {2,0}};
  SAFEARRAY *psa = SafeArrayCreate(VT_VARIANT, 2, Bounds);
  long ix[2];
  VARIANT varValue;

  varValue.vt = VT_I4;

  /*-- add changed lamps to array --*/
  for (ix[0] = 0; ix[0] < uCount; ix[0]++) {
    ix[1] = 0;
    varValue.lVal = chgLamps[ix[0]].lampNo;
    SafeArrayPutElement(psa, ix, &varValue);
    ix[1] = 1; // Lamp value
    varValue.lVal = chgLamps[ix[0]].currStat;
    SafeArrayPutElement(psa, ix, &varValue);
  }

  pVal->vt = VT_ARRAY | VT_VARIANT;
  pVal->parray = psa;

  return S_OK;
}

STDMETHODIMP CController::get_ChangedLEDs(int nHigh, int nLow, int nnHigh, int nnLow, VARIANT *pVal)
{
  UINT64 mask = (((UINT64)nHigh)<<32) | ((UINT32)nLow);
  UINT64 mask2 = (((UINT64)nnHigh)<<32) | ((UINT32)nnLow);
  vp_tChgLED chgLED;

  if (!pVal) return S_FALSE;

  if (WaitForSingleObject(m_hEmuIsRunning, 0) == WAIT_TIMEOUT)
    { pVal->vt = 0; return S_OK; }

  /*-- Count changes --*/
  int uCount = vp_getChangedLEDs(chgLED, mask, mask2);

  if (uCount == 0)
    { pVal->vt = 0; return S_OK; }

  /*-- Create array --*/
  SAFEARRAYBOUND Bounds[] = {{(ULONG)uCount,0}, {3,0}};
  SAFEARRAY *psa = SafeArrayCreate(VT_VARIANT, 2, Bounds);
  long ix[2];
  VARIANT varValue;

  varValue.vt = VT_I4;

  /*-- add changed lamps to array --*/
  for (ix[0] = 0; ix[0] < uCount; ix[0]++) {
    ix[1] = 0;
    varValue.lVal = chgLED[ix[0]].ledNo;
    SafeArrayPutElement(psa, ix, &varValue);
    ix[1] = 1; // Changed segments
    varValue.lVal = chgLED[ix[0]].chgSeg;
    SafeArrayPutElement(psa, ix, &varValue);
    ix[1] = 2; // Current value
    varValue.lVal = chgLED[ix[0]].currStat;
    SafeArrayPutElement(psa, ix, &varValue);
  }

  pVal->vt = VT_ARRAY | VT_VARIANT;
  pVal->parray = psa;

  return S_OK;
}


/*****************************************************************************************
 * get_ChangedLEDsState (read-only): Copy whole Changed LEDS digits/Segments array to a user allocated array
 *****************************************************************************************/
STDMETHODIMP CController::get_ChangedLEDsState(int nHigh, int nLow, int nnHigh, int nnLow, int **buf, int *pVal)
{
	UINT64 mask = (((UINT64)nHigh)<<32) | ((UINT32)nLow);
	UINT64 mask2 = (((UINT64)nnHigh)<<32) | ((UINT32)nnLow);
	vp_tChgLED chgLED;
	
	if (!pVal) return S_FALSE;
	
	if(!buf)
	{
		*pVal = 0;
		return S_FALSE;
	}


  if (WaitForSingleObject(m_hEmuIsRunning, 0) == WAIT_TIMEOUT)
    { pVal = 0; return S_OK; }

  /*-- if enabled: wait for the worker thread to enter "throttle_speed()" --*/
  if ( (g_hEnterThrottle!=INVALID_HANDLE_VALUE) && g_iSyncFactor ) 
	WaitForSingleObject(g_hEnterThrottle, (synclevel<=20) ? synclevel : 50);
  else if ( synclevel<0 )
	  uSleep(-synclevel*1000);

  /*-- Count changes --*/
  int uCount = vp_getChangedLEDs(chgLED, mask, mask2);

  if (uCount == 0)
    { pVal = 0; return S_OK; }

  /*-- add changed LEDs to array --*/
  int *dst = reinterpret_cast<int*>(buf);
  for (int i = 0; i < uCount; i++) {

	  *(dst++) = chgLED[i].ledNo;
	  *(dst++) = chgLED[i].chgSeg;
	  *(dst++) = chgLED[i].currStat;
  }

  *pVal = uCount;

  return S_OK;
}

/******************************************************
 * IController.ShowAboutDialog: shows the About dialog
 ******************************************************/
STDMETHODIMP CController::ShowAboutDialog(long hParentWnd)
{
	switch ( hParentWnd ) {
	case 0:
		break;

	case 1:
		hParentWnd = (long) ::GetActiveWindow();
		if ( !hParentWnd )
			hParentWnd = (long) GetForegroundWindow();
		break;

	default:
		if ( !IsWindow((HWND) hParentWnd) )
			hParentWnd = 0;
	}

	ShowAboutDlg((HWND) hParentWnd);
	
	return S_OK;
}

/***************************************************************
 * IController.HandleMechanics property: get/set HandleMechanics
 ***************************************************************/
STDMETHODIMP CController::get_HandleMechanics(int *pVal)
{
	if (pVal)
		*pVal = m_iHandleMechanics;
	return S_OK;
}

STDMETHODIMP CController::put_HandleMechanics(int newVal)
{
	m_iHandleMechanics = newVal;

	return S_OK;
}

/****************************************************************************
 * IController.GetMech/Mech property: gets/sets the state of playfield 
 * mechanics if vpm is simulating them (also see HandleMech)
 ****************************************************************************/
STDMETHODIMP CController::get_GetMech(int mechNo, int *pVal)
{
	if ( !pVal )
		return S_FALSE;

	*pVal = (WaitForSingleObject(m_hEmuIsRunning, 0)!=WAIT_TIMEOUT) ? vp_getMech(mechNo) : 0;

	return S_OK;
}

STDMETHODIMP CController::put_Mech(int mechNo, int newVal)
{
	vp_setMechData(mechNo, newVal);

	return S_OK;
}

/**************************************************************************
 * IController.GIString property (read-only): returns state of a GI String
 **************************************************************************/
STDMETHODIMP CController::get_GIString(int nString, int *pVal) {
  if (!pVal) return S_FALSE;

  *pVal = (WaitForSingleObject(m_hEmuIsRunning, 0) != WAIT_TIMEOUT) ? vp_getGI(nString) : 0;

  return S_OK;
}

/***************************************************************
 * IController.ChangedGIString property: returns a list of the 
 * numbers of GIStrings which state has changed since the last call
 * number is in the first, state in the second part
 ***************************************************************/
STDMETHODIMP CController::get_ChangedGIStrings(VARIANT *pVal) {
  vp_tChgGIs chgGI;

  if (!pVal) return S_FALSE;

  if (WaitForSingleObject(m_hEmuIsRunning, 0) == WAIT_TIMEOUT)
    { pVal->vt = 0; return S_OK; }

  int uCount = vp_getChangedGI(chgGI);

  if (uCount == 0)
    { pVal->vt = 0; return S_OK; }

  /*-- Create array --*/
  SAFEARRAYBOUND Bounds[] = {{(ULONG)uCount,0}, {2,0}};
  SAFEARRAY *psa = SafeArrayCreate(VT_VARIANT, 2, Bounds);
  long ix[2];
  VARIANT GIState;
  GIState.vt = VT_I4;

  /*-- add changed to array --*/
  for (ix[0] = 0; ix[0] < uCount; ix[0]++) {
    ix[1] = 0; // GI Number
    GIState.lVal = chgGI[ix[0]].giNo;
    SafeArrayPutElement(psa, ix, &GIState);
    ix[1] = 1; // GI status
    GIState.lVal = chgGI[ix[0]].currStat;
    SafeArrayPutElement(psa, ix, &GIState);
  }
  pVal->vt = VT_ARRAY | VT_VARIANT;
  pVal->parray = psa;

  return S_OK;
}

/****************************************************************************
 * IController.ChangedSolenoids property (read-only): gets the state of all 
 * solenoids
 * (also gets the information, if at leats one solenoid has changed after 
 * last call; element 0 state if TRUE if at least one solenoid has changed 
 * state sice last call)
 ****************************************************************************/
STDMETHODIMP CController::get_ChangedSolenoids(VARIANT *pVal)
{
  vp_tChgSols chgSol;

  if (!pVal) return S_FALSE;

  if (WaitForSingleObject(m_hEmuIsRunning, 0) == WAIT_TIMEOUT)
	{ pVal->vt = 0; return S_OK; }

  /*-- Count changed solenoids --*/
  int uCount = vp_getChangedSolenoids(chgSol);

  if (uCount == 0)
	{ pVal->vt = 0; return S_OK; }

  /*-- Create array --*/
  SAFEARRAYBOUND Bounds[] = {{(ULONG)uCount,0}, {2,0}};
  SAFEARRAY *psa = SafeArrayCreate(VT_VARIANT, 2, Bounds);
  long ix[2];
  VARIANT varValue;
  varValue.vt = VT_I4;

  /*-- add changed solenoids to the array --*/
  for (ix[0] = 0; ix[0] < uCount; ix[0]++) {
	ix[1] = 0; // Solenoid number
	varValue.lVal = chgSol[ix[0]].solNo;
	SafeArrayPutElement(psa, ix, &varValue);

	ix[1] = 1; // Solenoid status
	varValue.lVal = chgSol[ix[0]].currStat;
	SafeArrayPutElement(psa, ix, &varValue);
  }
  pVal->vt = VT_ARRAY | VT_VARIANT;
  pVal->parray = psa;

  return S_OK;
}

/****************************************************************************
 * IController.SplashInfoLine property: sets the value of the user setable 
 * text in the splash screen
 ****************************************************************************/
STDMETHODIMP CController::get_SplashInfoLine(BSTR *pVal)
{
	CComBSTR strValue(m_szSplashInfoLine);

	*pVal = strValue.Detach();

	return S_OK;
}

STDMETHODIMP CController::put_SplashInfoLine(BSTR newVal)
{
	WideCharToMultiByte(CP_ACP, 0, newVal, -1, m_szSplashInfoLine, sizeof m_szSplashInfoLine, NULL, NULL);

	return S_OK;
}

/****************************************************************************
 * IController.Solenoids property (read-only): returns the state of all 
 * solenoids at once
 ****************************************************************************/
STDMETHODIMP CController::get_Solenoids(VARIANT *pVal)
{
	if ( !pVal )
		return S_FALSE;

	SAFEARRAY *psa = SafeArrayCreateVector(VT_VARIANT, 0, 65);

	VARIANT SolenoidState;
	SolenoidState.vt = VT_BOOL;

	long ix;

	if ( WaitForSingleObject(m_hEmuIsRunning, 0)==WAIT_TIMEOUT ) {
		SolenoidState.boolVal = VARIANT_FALSE;
		for (ix=0; ix<65; ix++)
			SafeArrayPutElement(psa, &ix, &SolenoidState);
	}
	else {
		for (ix=0; ix<65; ix++) {
			SolenoidState.boolVal = vp_getSolenoid(ix)?VARIANT_TRUE:VARIANT_FALSE;
			SafeArrayPutElement(psa, &ix, &SolenoidState);
		};
	}

	pVal->vt = VT_ARRAY|VT_VARIANT;
	pVal->parray = psa;
	return S_OK;
}

/****************************************************************************
 * IController.Dip property: gets/sets the value of a dip bank
 ****************************************************************************/
STDMETHODIMP CController::get_Dip(int nNo, int *pVal)
{
	// TODO: Add your implementation code here. DONE
	if (!pVal) return S_FALSE;

	*pVal = vp_getDIP(nNo);
	return S_OK;
}

STDMETHODIMP CController::put_Dip(int nNo, int newVal)
{
	// TODO: Add your implementation code here. DONE
        vp_setDIP(nNo, newVal);
	return S_OK;
}

/****************************************************************************
 * IController.GIStrings property (read-only): gets the value of all GI 
 * strings at once
 ****************************************************************************/
STDMETHODIMP CController::get_GIStrings(VARIANT *pVal)
{
	if ( !pVal )
		return S_FALSE;

	SAFEARRAY *psa = SafeArrayCreateVector(VT_VARIANT, 0, 5);

	VARIANT GIStringState;
	GIStringState.vt = VT_I4;

	long ix;

	if ( WaitForSingleObject(m_hEmuIsRunning, 0)==WAIT_TIMEOUT ) {
		GIStringState.lVal = 0;
		for (ix=0; ix<5; ix++) 
			SafeArrayPutElement(psa, &ix, &GIStringState);
	}
	else {
		for (ix=0; ix<5; ix++) {
			GIStringState.lVal = vp_getGI(ix);
			SafeArrayPutElement(psa, &ix, &GIStringState);
		};
	}
	pVal->vt = VT_ARRAY|VT_VARIANT;
	pVal->parray = psa;
	return S_OK;
}

/****************************************************************************
 * IController.LockDisplay property: If the display is locked, anytime the
 * video window will receive the focus the focus will immediatly be returned
 * to the window which had it before
 ****************************************************************************/
STDMETHODIMP CController::get_LockDisplay(VARIANT_BOOL *pVal)
{
	if (pVal)
		*pVal = m_fDisplayLocked?VARIANT_TRUE:VARIANT_FALSE;

	return S_OK;
}

STDMETHODIMP CController::put_LockDisplay(VARIANT_BOOL newVal)
{
	m_fDisplayLocked = newVal;

	return S_OK;
}

/****************************************************************************
 * IController.SolMask property: gets/sets a mask for the solenois, i.e. 
 * which solenoid state should be reported by ChangedSolenoids
 ****************************************************************************/
STDMETHODIMP CController::get_SolMask(int nLow, long *pVal)
{
	if ( !pVal || (nLow<0) || (nLow>1) )
		return S_FALSE;

	*pVal = vp_getSolMask(nLow);

	return S_OK;
}

STDMETHODIMP CController::put_SolMask(int nLow, long newVal)
{
	if ( (nLow<0) || (nLow>2) ) //TODO B2S hack, see vp_setSolMask()
		return S_FALSE;

	vp_setSolMask(nLow, newVal);

	return S_OK;
}

/****************************************************************************
 * IController.Version (read-only): gets the program version of VPM
 ****************************************************************************/
STDMETHODIMP CController::get_Version(BSTR *pVal)
{
	if ( !pVal )
		return S_FALSE;

	int nVersionNo0, nVersionNo1, nVersionNo2, nVersionNo3;
	GetProductVersion(&nVersionNo0, &nVersionNo1, &nVersionNo2, &nVersionNo3);

	char szVersion[9];
	wsprintf(szVersion, "%02i%02i%02i%02i", nVersionNo0, nVersionNo1, nVersionNo2, nVersionNo3);

	CComBSTR bstrVersion(szVersion);

	*pVal = bstrVersion.Detach();

	return S_OK;
}

/****************************************************************************
 * IController.Games (read-only): hands out a pointer to a games-objects
 ****************************************************************************/
STDMETHODIMP CController::get_Games(IGames* *pVal)
{
	if ( !pVal )
		return S_FALSE;

	return m_pGames->QueryInterface(IID_IGames, (void**) pVal);
}


/******************************************************************************
 * IController.Games (read-only): hands out a pointer to a ControllerSettings-
 * object
 ******************************************************************************/
STDMETHODIMP CController::get_Settings(IControllerSettings **pVal)
{
	if ( !pVal )
		return S_FALSE;

	return m_pControllerSettings->QueryInterface(IID_IControllerSettings, (void**) pVal);
}

/* ---------------------------------------------------------------------------------------------*/
/* ------------------ deprecated properties and methods ----------------------------------------*/
/* ---------------------------------------------------------------------------------------------*/

/**************************************************************************
 * IController.BorderSizeX: gets/sets the x value of the space between the
 * window border and the display inself
 *
 * Deprecated:
 * Will be deleted in the next version, actually it does nothing anymore
 **************************************************************************/
STDMETHODIMP CController::get_BorderSizeX(int *pVal)
{
	if (pVal)
		*pVal = 0;
	return S_OK;
}

STDMETHODIMP CController::put_BorderSizeX(int newVal)
{
	if ( newVal<0 )
		return S_FALSE;

	return S_OK;
}

/**************************************************************************
 * IController.BorderSizeY: gets/sets the y value of the space between the
 * window border and the display inself
 *
 * Deprecated:
 * Will be deleted in the next version, actually it does nothing anymore
 **************************************************************************/
STDMETHODIMP CController::get_BorderSizeY(int *pVal)
{
	if (pVal)
		*pVal = 0;
	return S_OK;
}

STDMETHODIMP CController::put_BorderSizeY(int newVal)
{
	if ( newVal<0 )
		return S_FALSE;
	
	return S_OK;
}

/********************************************************************
 * IController.WindowPosX property: gets/sets the x position of the 
 * video window
 *
 * Deprecated:
 * use Controller.Games("name").Settings.DisplayPosX(hwnd) instead
 ********************************************************************/
STDMETHODIMP CController::get_WindowPosX(int *pVal)
{
	if ( !pVal )
		return E_POINTER;

	VARIANT vValue;
	VariantInit(&vValue);

	HRESULT hr = m_pGameSettings->get_Value(CComBSTR("dmd_pos_x"), &vValue);
	*pVal = vValue.lVal;

	return hr;
}

STDMETHODIMP CController::put_WindowPosX(int newVal)
{
	return m_pGameSettings->put_Value(CComBSTR("dmd_pos_x"), CComVariant(newVal));
}

/********************************************************************
 * IController.WindowPosY property: gets/sets the y position of the 
 * video window
 *
 * Deprecated:
 * use Controller.Games("name").Settings.DisplayPosY(hwnd) instead
 ********************************************************************/
STDMETHODIMP CController::get_WindowPosY(int *pVal)
{
	if ( !pVal )
		return E_POINTER;

	VARIANT vValue;
	VariantInit(&vValue);
	HRESULT hr = m_pGameSettings->get_Value(CComBSTR("dmd_pos_y"), &vValue);

	*pVal = vValue.lVal;

	return hr;
}

STDMETHODIMP CController::put_WindowPosY(int newVal)
{
	return m_pGameSettings->put_Value(CComBSTR("dmd_pos_y"), CComVariant(newVal));
}

/*************************************************************************
 * IController.NewSoundCommands property (read-only): returns a list of 
 * latest sound commands for the sound board
 *
 * Deprecated:
 * will be deleted in the next version
 *************************************************************************/
STDMETHODIMP CController::get_NewSoundCommands(VARIANT *pVal)
{
  vp_tChgSound chgSound;

  if (!pVal) return S_FALSE;

  if (WaitForSingleObject(m_hEmuIsRunning, 0) == WAIT_TIMEOUT)
    { pVal->vt = 0; return S_OK; }

  int uCount = vp_getNewSoundCommands(chgSound);

  if (uCount == 0)
    { pVal->vt = 0; return S_OK; }

  /*-- Create array --*/
  SAFEARRAYBOUND Bounds[] = {{(ULONG)uCount,0}, {2,0}};
  SAFEARRAY *psa = SafeArrayCreate(VT_VARIANT, 2, Bounds);
  long ix[2];
  VARIANT SoundsState;
  SoundsState.vt = VT_I4;

  /*-- add changed to array --*/
  for (ix[0] = 0; ix[0] < uCount; ix[0]++) {
    ix[1] = 0; // Sound Command Number
    SoundsState.lVal = chgSound[ix[0]].sndNo;
    SafeArrayPutElement(psa, ix, &SoundsState);
    ix[1] = 1; // Not used for the moment!
    SoundsState.lVal = 0;
    SafeArrayPutElement(psa, ix, &SoundsState);
  }
  pVal->vt = VT_ARRAY | VT_VARIANT;
  pVal->parray = psa;

  return S_OK;
}

/*******************************************************
 * IController.InstallDir property: gets the install dir
 *
 * Deprecated:
 * use Controller.Settings.InstallDir instead
 *******************************************************/
STDMETHODIMP CController::get_InstallDir(BSTR *pVal)
{
	return m_pControllerSettings->get_InstallDir(pVal);
}

/****************************************************************
 * IController:SetDisplayPosition: The set position of the video
 * window relative to the client area of the parent window
 *
 * Deprecated:
 * use 
 *   Controller.Games("name").Settings.SetDisplayPosition(x,y,hwnd)
 * instead
 ***************************************************************/
STDMETHODIMP CController::SetDisplayPosition(int x, int y, long hParentWindow)
{
	if ( IsWindow((HWND) hParentWindow)) {
		RECT rect;
		::GetClientRect((HWND) hParentWindow, &rect);
		::ClientToScreen((HWND) hParentWindow, (LPPOINT) &rect.left);
		x += rect.left;
		y += rect.top;
	}

	HRESULT hr = m_pGameSettings->put_Value(CComBSTR("dmd_pos_x"), CComVariant(x));
	if ( SUCCEEDED(hr) )
		hr = m_pGameSettings->put_Value(CComBSTR("dmd_pos_y"), CComVariant(y));

	return hr;
}

/************************************************
 * IController.RomDirs property: get/set ROM dirs
 *
 * Deprecated:
 * use Controller.Settings.RomPath instead
 ************************************************/
STDMETHODIMP CController::get_RomDirs(BSTR *pVal)
{
	if ( !pVal ) return E_POINTER;

	CComVariant vValue;
	HRESULT hr = m_pGameSettings->get_Value(CComBSTR("rompath"), &vValue);
	*pVal = SysAllocString(vValue.bstrVal);

	return hr;
}

STDMETHODIMP CController::put_RomDirs(BSTR newVal)
{
	return m_pControllerSettings->put_Value(CComBSTR("rompath"), CComVariant(newVal));
}

/*********************************************
 * IController.CfgDir property: get/set CfgDir
 *
 * Deprecated:
 * use Controller.Settings.CfgPath instead
 *********************************************/
STDMETHODIMP CController::get_CfgDir(BSTR *pVal)
{
	if ( !pVal ) return E_POINTER;

	CComVariant vValue;
	HRESULT hr = m_pGameSettings->get_Value(CComBSTR("cfg_directory"), &vValue);
	*pVal = SysAllocString(vValue.bstrVal);

	return hr;
}

STDMETHODIMP CController::put_CfgDir(BSTR newVal)
{
	return m_pControllerSettings->put_Value(CComBSTR("cfg_directory"), CComVariant(newVal));
}

/****************************************************
 * IController.NVRamDirs property: get/set NVRAM dirs
 *
 * Deprecated:
 * use Controller.Settings.NVRamPath instead
 ****************************************************/
STDMETHODIMP CController::get_NVRamDir(BSTR *pVal)
{
	if ( !pVal ) return E_POINTER;

	CComVariant vValue;
	HRESULT hr = m_pGameSettings->get_Value(CComBSTR("nvram_directory"), &vValue);
	*pVal = SysAllocString(vValue.bstrVal);

	return hr;
}

STDMETHODIMP CController::put_NVRamDir(BSTR newVal)
{
	return m_pControllerSettings->put_Value(CComBSTR("nvram_directory"), CComVariant(newVal));
}

/*****************************************************
 * IController.SamplesDir property: get/set SamplesDir
 *
 * Deprecated:
 * use Controller.Settings.SamplesPath instead
 *****************************************************/
STDMETHODIMP CController::get_SamplesDir(BSTR *pVal)
{
	if ( !pVal ) return E_POINTER;

	CComVariant vValue;
	HRESULT hr = m_pGameSettings->get_Value(CComBSTR("samplepath"), &vValue);
	*pVal = SysAllocString(vValue.bstrVal);

	return hr;
}

STDMETHODIMP CController::put_SamplesDir(BSTR newVal)
{
	return m_pControllerSettings->put_Value(CComBSTR("samplepath"), CComVariant(newVal));
}

/*********************************************
 * IController.ImgDir property: get/set ImgDir
 *
 * Deprecated:
 * use Controller.Settings.SnapshotPath instead
 *********************************************/
STDMETHODIMP CController::get_ImgDir(BSTR *pVal)
{
	if ( !pVal ) return E_POINTER;

	CComVariant vValue;
	HRESULT hr = m_pGameSettings->get_Value(CComBSTR("snapshot_directory"), &vValue);
	*pVal = SysAllocString(vValue.bstrVal);

	return hr;
}

STDMETHODIMP CController::put_ImgDir(BSTR newVal)
{
	return m_pControllerSettings->put_Value(CComBSTR("snapshot_directory"), CComVariant(newVal));
}

/*****************************************************************
 * IController.ShowOptsDialog: shows the options dialog
 *
 * Deprecated:
 * use Controller.Games("name").Settings.ShowSettingsDlg instead
 *****************************************************************/
STDMETHODIMP CController::ShowOptsDialog(long hParentWnd)
{
	return m_pGameSettings->ShowSettingsDlg(hParentWnd);
}

/************************************************************
 * IController.ShowDMDOnly property: get/set UseLamps
 *
 * Deprecated:
 * use Controller.Games("name").Settings.DisplayOnly instead
 ************************************************************/
STDMETHODIMP CController::get_ShowDMDOnly(VARIANT_BOOL *pVal)
{
	if ( !pVal )
		return E_POINTER;

	VARIANT vValue;
	VariantInit(&vValue);
	
	HRESULT hr = m_pGameSettings->get_Value(CComBSTR("dmd_only"), &vValue);
	*pVal = vValue.boolVal;

	return hr;
}

STDMETHODIMP CController::put_ShowDMDOnly(VARIANT_BOOL newVal)
{
	CComVariant vValue(newVal);
	return m_pGameSettings->put_Value(CComBSTR("dmd_only"), vValue);
}

/***********************************************************
 * IController.UseSamples property: get/set UseSamples
 *
 * Deprecated:
 * use Controller.Games("name").Settings.UseSamples instead
 ***********************************************************/
STDMETHODIMP CController::get_UseSamples(VARIANT_BOOL *pVal)
{
	if (!pVal)
		return E_POINTER;

	VARIANT vValue;
	VariantInit(&vValue);

	HRESULT hr = m_pGameSettings->get_Value(CComBSTR("samples"), &vValue);
	*pVal = vValue.boolVal;

	return S_OK;
}

STDMETHODIMP CController::put_UseSamples(VARIANT_BOOL newVal)
{
	return m_pGameSettings->put_Value(CComBSTR("samples"), CComVariant(newVal));
}

/******************************************************
 * IController.ShowTitle property: get/set ShowTitle
 *
 * Deprecated: 
 * use Controller.Games("name").Settings.Title instead
 ******************************************************/
STDMETHODIMP CController::get_ShowTitle(VARIANT_BOOL *pVal)
{
	if ( !pVal )
		return E_POINTER;

	VARIANT vValue;
	VariantInit(&vValue);

	HRESULT hr = m_pGameSettings->get_Value(CComBSTR("dmd_title"), &vValue);
	*pVal = vValue.boolVal;

	return hr;
}

STDMETHODIMP CController::put_ShowTitle(VARIANT_BOOL newVal)
{
	return m_pGameSettings->put_Value(CComBSTR("dmd_title"), CComVariant(newVal));
}

/*******************************************************
 * IController.ShowFrame property: get/set the border
 *
 * Deprecated:
 * use Controller.Games("name").Settings.Border instead
 *******************************************************/
STDMETHODIMP CController::get_ShowFrame(VARIANT_BOOL *pVal)
{
	if ( !pVal )
		return E_POINTER;

	VARIANT vValue;
	VariantInit(&vValue);

	HRESULT hr = m_pGameSettings->get_Value(CComBSTR("dmd_border"), &vValue);
	*pVal = vValue.boolVal;

	return hr;
}

STDMETHODIMP CController::put_ShowFrame(VARIANT_BOOL newVal)
{
	return m_pGameSettings->put_Value(CComBSTR("dmd_border"), CComVariant(newVal));
}

/***********************************************************
 * IController.SampleRate property: get/set the sample rate
 *
 * Deprecated:
 * use Controller.Games("name").Settings.SampleRate instead
 ************************************************************/
STDMETHODIMP CController::get_SampleRate(int *pVal)
{
	if ( !pVal )
		return E_POINTER;

	VARIANT vValue;
	VariantInit(&vValue);

	HRESULT hr = m_pGameSettings->get_Value(CComBSTR("samplerate"), &vValue);
	*pVal = vValue.lVal;

	return hr;
}

STDMETHODIMP CController::put_SampleRate(int newVal)
{
	return m_pGameSettings->put_Value(CComBSTR("samplerate"), CComVariant(newVal));
}

/***********************************************************
 * IController.DoubleSize property: display the video window
 * double sized or not
 *
 * Deprecated:
 * use Controller.Games("name").Settings.DoubleSize instead
 ************************************************************/
STDMETHODIMP CController::get_DoubleSize(VARIANT_BOOL *pVal)
{
	if ( !pVal )
		return E_POINTER;

	VARIANT vValue;
	VariantInit(&vValue);

	HRESULT hr = m_pGameSettings->get_Value(CComBSTR("dmd_doublesize"), &vValue);
	*pVal = vValue.boolVal;

	return hr;
}

STDMETHODIMP CController::put_DoubleSize(VARIANT_BOOL newVal)
{
	return m_pGameSettings->put_Value(CComBSTR("dmd_doublesize"), CComVariant(newVal));
}

/***********************************************************
 * IController.DoubleSize property: display the video window
 * in compact size or not
 *
 * Deprecated:
 * use Controller.Games("name").Settings.CompactDisplay instead
 ************************************************************/
STDMETHODIMP CController::get_Antialias(VARIANT_BOOL *pVal)
{
	if ( !pVal )
		return E_POINTER;

	VARIANT vValue;
	VariantInit(&vValue);

	HRESULT hr = m_pGameSettings->get_Value(CComBSTR("dmd_compact"), &vValue);
	*pVal = vValue.boolVal;

	return hr;
}

STDMETHODIMP CController::put_Antialias(VARIANT_BOOL newVal)
{
	return m_pGameSettings->put_Value(CComBSTR("dmd_compact"), CComVariant(newVal));
}

/*********************************************************************
 * IController.CheckROMS: returns TRUE if ROMS are ok
 *
 * Deprecated:
 * use Controller.Games("name").ShowInfoDlg instead
 *********************************************************************/
STDMETHODIMP CController::CheckROMS(/*[in,defaultvalue(0)]*/ int nShowOptions, /*[in,defaultvalue(0)]*/ long hParentWnd, /*[out, retval]*/ VARIANT_BOOL *pVal)
{
	if ( !pVal )
		return S_FALSE;

	int fResult;
	HRESULT hr = m_pGame->ShowInfoDlg(nShowOptions, hParentWnd, &fResult);

	*pVal = (fResult==IDOK)?VARIANT_TRUE:VARIANT_FALSE;

	return hr;
}

/****************************************************************************
 * IController:ShowPathesDialog: Display a dialog to set up the paths
 *
 * Deprecated:
 * use Controller.Settings.ShowSettingsDlg instead
 ****************************************************************************/
STDMETHODIMP CController::ShowPathesDialog(long hParentWnd)
{
	switch ( hParentWnd ) {
	case 0:
		break;

	case 1:
		hParentWnd = (long) ::GetActiveWindow();
		if ( !hParentWnd )
			hParentWnd = (long) GetForegroundWindow();
		break;

	default:
		if ( !IsWindow((HWND) hParentWnd) )
			hParentWnd = 0;
	}

	m_pControllerSettings->ShowSettingsDlg(hParentWnd);

	return S_OK;
}

/****************************************************************************
 * IController.Hidden property: Hides/Shows the display to the user
 ****************************************************************************/
STDMETHODIMP CController::get_Hidden(VARIANT_BOOL *pVal)
{
	if ( !pVal )
		return E_POINTER;

	if ( m_fWindowHidden || !g_fShowWinDMD) 
		*pVal = VARIANT_TRUE;

	return S_OK;
}

STDMETHODIMP CController::put_Hidden(VARIANT_BOOL newVal)
{
	m_fWindowHidden = newVal;

	if ( IsWindow(win_video_window) ) 
		ShowWindow(win_video_window, newVal || !g_fShowWinDMD ?SW_HIDE:SW_SHOW);

	return S_OK;
}

/****************************************************************************
 * IController.Game (read-only): hands out a pointer to the current game
 ****************************************************************************/
STDMETHODIMP CController::get_Game(IGame* *pVal)
{
	if ( !pVal )
		return S_FALSE;

	return m_pGame->QueryInterface(IID_IGame, (void**) pVal);
}

/****************************************************************************
 * IController.MechSamples property: Enabled/Disabled mechnical samples; only
 * has an effect before the machine was started
 ****************************************************************************/
STDMETHODIMP CController::get_MechSamples(VARIANT_BOOL *pVal)
{
	if ( !pVal )
		return E_POINTER;

	*pVal = m_fMechSamples?VARIANT_TRUE:VARIANT_FALSE;

	return S_OK;
}

STDMETHODIMP CController::put_MechSamples(VARIANT_BOOL newVal)
{
	m_fMechSamples = newVal;

	return S_OK;
}

/****************************************************************************
 * IController.GetWindowRect: A simle proxy function to the windows API
 * function GetWindowRect. Returns the rctangle in the from of a safearray
 * with for entries: left, top, right, bottom
 ****************************************************************************/
STDMETHODIMP CController::GetWindowRect(long hWnd, VARIANT *pVal)
{
	RECT rect;
	if ( IsWindow((HWND) hWnd)) 
		::GetWindowRect((HWND) hWnd, &rect);
	else {
		rect.left = 0;
		rect.top = 0;
		rect.right = GetSystemMetrics(SM_CXSCREEN);
		rect.bottom = GetSystemMetrics(SM_CYSCREEN);
	}

	SAFEARRAY *psa = SafeArrayCreateVector(VT_VARIANT, 0, 4);

	VARIANT Value;
	Value.vt = VT_I4;

	long *pValue = &rect.left;
	for(long ix=0; ix<=3; ix++)
	{
		Value.lVal = *pValue++;
		SafeArrayPutElement(psa, &ix, &Value);
	}

	pVal->vt = VT_ARRAY|VT_VARIANT;
	pVal->parray = psa;

	return S_OK;
}

/****************************************************************************
 * IController.GetClientRect: Nearly a proxy function to the windows API
 * function GetClientClient. The client rect will be delivered in screen, not in client
 * coordinates. Returns the rctangle in the from of a safearray
 * with for entries: left, top, right, bottom
 ****************************************************************************/
STDMETHODIMP CController::GetClientRect(long hWnd, VARIANT *pVal)
{
	RECT rect;
	if ( IsWindow((HWND) hWnd)) {
		::GetClientRect((HWND) hWnd, &rect);
		::ClientToScreen((HWND) hWnd, (LPPOINT) &rect.left);
		::ClientToScreen((HWND) hWnd, (LPPOINT) &rect.right);
	}
	else {
		rect.left = 0;
		rect.top = 0;
		rect.right = GetSystemMetrics(SM_CXSCREEN);
		rect.bottom = GetSystemMetrics(SM_CYSCREEN);
	}

	SAFEARRAY *psa = SafeArrayCreateVector(VT_VARIANT, 0, 4);

	VARIANT Value;
	Value.vt = VT_I4;

	long *pValue = &rect.left;
	for(long ix=0; ix<=3; ix++)
	{
		Value.lVal = *pValue++;
		SafeArrayPutElement(psa, &ix, &Value);
	}

	pVal->vt = VT_ARRAY|VT_VARIANT;
	pVal->parray = psa;

	return S_OK;
}


/*********************************************************
 * IController.MasterVolume property: get/set MasterVolume
 *********************************************************/
STDMETHODIMP CController::get_MasterVolume(int *pVal)
{
	if (pVal)
		*pVal = osd_get_mastervolume();
	return S_OK;
}

STDMETHODIMP CController::put_MasterVolume(int newVal)
{
	osd_set_mastervolume(newVal);

	return S_OK;
}

/***********************************************************************************
 IController.EnumAudioDevices property (read only):
    Enumerate audio devices using DirectSound and return the number of found devices
************************************************************************************/
STDMETHODIMP CController::get_EnumAudioDevices(int *pVal)
{
	if (pVal)
		*pVal = osd_enum_audio_devices();
	return S_OK;
}

/*************************************************************************
 IController.AudioDevicesCount property (read only):
    Return the number of found devices (by previous call EnumAudioDevices)
**************************************************************************/
STDMETHODIMP CController::get_AudioDevicesCount(int *pVal)
{
	if (pVal)
		*pVal = osd_get_audio_devices_count();
	return S_OK;
}

/**********************************************************************************
 IController.AudioDeviceDescription property (read only):
   Return the audio device description (null char ended string) of the "num" device
***********************************************************************************/
STDMETHODIMP CController::get_AudioDeviceDescription(int num, BSTR *pVal)
{
	if ( !pVal )
		return S_FALSE;

	CComBSTR bstrDescription(osd_get_audio_device_description(num));

	*pVal = bstrDescription.Detach();

	return S_OK;
}

/***************************************************************************
 IController.AudioDeviceModule property (read only):
 Return the audio device module (null char ended string) of the "num" device
****************************************************************************/
STDMETHODIMP CController::get_AudioDeviceModule(int num, BSTR *pVal)
{
	if ( !pVal )
		return S_FALSE;

	CComBSTR bstrDescription(osd_get_audio_device_module(num));

	*pVal = bstrDescription.Detach();

	return S_OK;
}

/******************************************
 IController.CurrentAudioDevice property):
    Get/Set the current audio device number
*******************************************/
STDMETHODIMP CController::get_CurrentAudioDevice(int *pVal)
{
	if (pVal)
		*pVal = osd_get_current_audio_device();
	return S_OK;
}

STDMETHODIMP CController::put_CurrentAudioDevice(int newVal)
{
	if(osd_set_audio_device(newVal)!=newVal)
		return S_FALSE;
	else
		return S_OK;
}

/***************************************************************
 * IController.FastFrames property: get/set FastFrames
 ***************************************************************/

STDMETHODIMP CController::get_FastFrames(int *pVal)
{
	if ( !pVal )
		return E_POINTER;

	VARIANT vValue;
	VariantInit(&vValue);

	HRESULT hr = m_pGameSettings->get_Value(CComBSTR("fastframes"), &vValue);
	*pVal = vValue.lVal;

	return hr;
}

STDMETHODIMP CController::put_FastFrames(int newVal)
{
	return m_pGameSettings->put_Value(CComBSTR("fastframes"), CComVariant(newVal));
}

/*************************************************************** 
 * IController.IgnoreRomCrc property: get/set IgnoreRomCrc
 ***************************************************************/

STDMETHODIMP CController::get_IgnoreRomCrc(VARIANT_BOOL *pVal)
{
	if ( !pVal )
		return E_POINTER;

	VARIANT vValue;
	VariantInit(&vValue);

	HRESULT hr = m_pGameSettings->get_Value(CComBSTR("ignore_rom_crc"), &vValue);
	*pVal = vValue.boolVal;

	return hr;
}

STDMETHODIMP CController::put_IgnoreRomCrc(VARIANT_BOOL newVal)
{
	return m_pGameSettings->put_Value(CComBSTR("ignore_rom_crc"), CComVariant(newVal));
}

/*************************************************************** 
 * IController.CabinetMode property: get/set CabinetMode
 ***************************************************************/

STDMETHODIMP CController::get_CabinetMode(VARIANT_BOOL *pVal)
{
	if ( !pVal )
		return E_POINTER;

	VARIANT vValue;
	VariantInit(&vValue);

	HRESULT hr = m_pGameSettings->get_Value(CComBSTR("cabinet_mode"), &vValue);
	*pVal = vValue.boolVal;

	return hr;
}

STDMETHODIMP CController::put_CabinetMode(VARIANT_BOOL newVal)
{
	return m_pGameSettings->put_Value(CComBSTR("cabinet_mode"), CComVariant(newVal));
}


/****************************************************************************
 * IController.ShowPinDMD property: activate/deactivate pinDMD board
 ****************************************************************************/
STDMETHODIMP CController::get_ShowPinDMD(VARIANT_BOOL *pVal)
{
	if ( !pVal )
		return E_POINTER;

	VARIANT vValue;
	VariantInit(&vValue);

	HRESULT hr = m_pGameSettings->get_Value(CComBSTR("showpindmd"), &vValue);
	*pVal = vValue.boolVal;

	return hr;
}

STDMETHODIMP CController::put_ShowPinDMD(VARIANT_BOOL newVal)
{
	return m_pGameSettings->put_Value(CComBSTR("showpindmd"), CComVariant(newVal));
}

/****************************************************************************
 * IController.ShowWinDMD property: activate/deactivate windows DMD
 ****************************************************************************/
STDMETHODIMP CController::get_ShowWinDMD(VARIANT_BOOL *pVal)
{
	if ( !pVal )
		return E_POINTER;

	VARIANT vValue;
	VariantInit(&vValue);

	HRESULT hr = m_pGameSettings->get_Value(CComBSTR("showwindmd"), &vValue);
	*pVal = vValue.boolVal;

	return hr;
}

STDMETHODIMP CController::put_ShowWinDMD(VARIANT_BOOL newVal)
{
	return m_pGameSettings->put_Value(CComBSTR("showwindmd"), CComVariant(newVal));
}
