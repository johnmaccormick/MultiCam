// Copyright John MacCormick 2012. Modified BSD license. NO WARRANTY.
#include "stdafx.h"



using namespace std;

class DeviceEnumeratorImproved 
{
public:
	DeviceEnumeratorImproved();
	void print_devices();


private:
	HMACHINE m_hMachine;
	//SP_CLASSIMAGELIST_DATA m_ImageListData;
	vector<wstring> m_Devices;

	void RetrieveSubNodes(DEVNODE dn);
	wstring GetDeviceName(DEVNODE DevNode);
	HRESULT printMonikerFriendlyName(IMoniker *pMoniker);
	void initializeCom();
	HRESULT getVideoInputDeviceMoniker() ;

};