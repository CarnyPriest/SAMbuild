// ControllerSettings.h : Declaration of the CControllerSettings

#ifndef __CONTROLLERSETTINGS_H_
#define __CONTROLLERSETTINGS_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CControllerSettings
class ATL_NO_VTABLE CControllerSettings : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CControllerSettings, &CLSID_ControllerSettings>,
	public ISupportErrorInfo,
	public IDispatchImpl<IControllerSettings, &IID_IControllerSettings, &LIBID_VPinMAMELib>
{
public:
	CControllerSettings();

DECLARE_NO_REGISTRY()
DECLARE_NOT_AGGREGATABLE(CControllerSettings)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CControllerSettings)
	COM_INTERFACE_ENTRY(IControllerSettings)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IControllerSettings
public:
	STDMETHOD(get_InstallDir)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(ShowSettingsDlg)(long hParentWnd);
	STDMETHOD(get_SnapshotPath)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_SnapshotPath)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_CfgPath)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_CfgPath)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_SamplesPath)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_SamplesPath)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_NVRamPath)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_NVRamPath)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_RomPath)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_RomPath)(/*[in]*/ BSTR newVal);
};

#endif //__CONTROLLERSETTINGS_H_
