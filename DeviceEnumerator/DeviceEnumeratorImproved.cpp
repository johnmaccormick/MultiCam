// Copyright John MacCormick 2012. Modified BSD license. NO WARRANTY.
// DeviceEnumeratorImproved.cpp :
//
// Based closely on http://www.codeproject.com/KB/system/EnumDevices.aspx
#include "stdafx.h"

#include "DeviceEnumeratorImproved.h"
#include <iostream>

#include <dshow.h>

//int _tmainImproved(int argc, _TCHAR* argv[])
int _tmain(int argc, _TCHAR* argv[])
{
	//printf("this is the device enumerator\n");


	DeviceEnumeratorImproved* enumerator = new DeviceEnumeratorImproved();
	enumerator->print_devices();
	delete enumerator;

	return 0;
}

DeviceEnumeratorImproved::DeviceEnumeratorImproved()
{
}


void DeviceEnumeratorImproved::print_devices() {
	//for(vector<wstring>::const_iterator it = m_Devices.begin(); it != m_Devices.end(); ++it)
	//{
	//	wcout << *it << endl;
	//}

	initializeCom();
	cout << "\n\nDirectShow VideoInput devices:\n\n";

	HRESULT hr = getVideoInputDeviceMoniker();

	CoUninitialize();	
}

HRESULT DeviceEnumeratorImproved::printMonikerFriendlyName(IMoniker *pMoniker) {
	IPropertyBag *pPropBag;
	HRESULT hr;
	hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, 
		(void **)&pPropBag);
	if (SUCCEEDED(hr))
	{
		// retrieve and print filter's friendly name
		VARIANT varName;
		VariantInit(&varName);
		hr = pPropBag->Read(L"FriendlyName", &varName, 0);
		if (SUCCEEDED(hr))
		{
			if (varName.vt == VT_BSTR) {
				BSTR name = varName.bstrVal;
				printf("%S\n", name);
			} else {
				printf("ERROR -- friendly name was not a BSTR\n");
			}
		}
		VariantClear(&varName);
	}
	pPropBag->Release();
	return hr;
}


void DeviceEnumeratorImproved::initializeCom() {
	HRESULT hr = CoInitialize(NULL);
	if (FAILED(hr))
	{
		printf("ERROR - Could not initialize COM library");
		return;
	}
}

HRESULT DeviceEnumeratorImproved::getVideoInputDeviceMoniker() {

	IMoniker *pMoniker = NULL;
	IMoniker **ppMoniker = &pMoniker;

	// Create the System Device Enumerator.
	HRESULT hr;
	ICreateDevEnum *pSysDevEnum = NULL;
	hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
		IID_ICreateDevEnum, (void **)&pSysDevEnum);
	if (FAILED(hr))
	{
		printf("ERROR - Could not create system device enumerator.\n");

	} else {
		IEnumMoniker *pEnumCat = NULL;

		// Obtain a class enumerator for the video input device category.
		hr = pSysDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, 
			&pEnumCat, 0);

		if (hr == S_OK) 
		{
			// Enumerate the monikers.
			ULONG cFetched;
			BOOL done = false;
			hr = pEnumCat->Next(1, ppMoniker, &cFetched);
			if (hr == S_OK)
			{
				printMonikerFriendlyName(*ppMoniker);
			} else {
				printf("ERROR - could not find a video input device.\n");
			}

			// just out of curiosity, list the remaining devices even though we won't use them
			//IMoniker **ppMoniker2 = NULL;
			while (!done) {
				//cout << "looking for another moniker" << endl;
				HRESULT hr2 = pEnumCat->Next(1, ppMoniker, &cFetched);
				if (cFetched<1) {
					//cout << "didn't find another moniker" << endl;
				}
				if (hr2 == S_OK)
				{
					//cout << "hr2 == S_OK" << endl;
					printMonikerFriendlyName(*ppMoniker);
				} else {
					done=true;
				}
			}

			pEnumCat->Release();
		} else {
			printf("ERROR - could not create class enumerator.");
		}
		pSysDevEnum->Release();
	}
	return hr;
}


