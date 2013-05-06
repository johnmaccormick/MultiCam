// Copyright John MacCormick 2012. Modified BSD license. NO WARRANTY.
// Helper functions for enumerating and selecting video devices
using namespace std;
#include <streams.h>
#include <string>
#include "MessageBox.h"
#include "DeviceHelpers.h"


// from http://msdn.microsoft.com/en-us/library/dd940435(v=VS.85).aspx
template <class T> void SafeRelease(T **ppT)
{
	if (*ppT)
	{
		(*ppT)->Release();
		*ppT = NULL;
	}
}

// Copied and altered from DirectShow PlayCap Sample
HRESULT CreateVideoClassEnumerator(IEnumMoniker **ppClassEnum) {
	HRESULT hr;

	// create system device enumerator
	ICreateDevEnum *pDevEnum =NULL;
	hr = CoCreateInstance (CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC,
		IID_ICreateDevEnum, (void **) &pDevEnum);
	if (FAILED(hr)) goto done;

	// Create an enumerator for the video capture devices
	hr = pDevEnum->CreateClassEnumerator (CLSID_VideoInputDeviceCategory, ppClassEnum, 0);
	if (FAILED(hr)) goto done;

	// If there are no enumerators for the requested type, then 
	// CreateVideoClassEnumerator will succeed, but *ppClassEnum will be NULL.
	if (*ppClassEnum == NULL)
	{
		hr = E_FAIL;
		goto done;
	}

done:
	SafeRelease(&pDevEnum);
	return hr;
}

// Copied and altered from DirectShow PlayCap Sample
HRESULT getMonikerFriendlyName(IMoniker *pMoniker, wstring &friendlyName) {
	IPropertyBag *pPropBag;
	HRESULT hr;
	hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, 
		(void **)&pPropBag);
	if (FAILED(hr)) goto done;
		// retrieve and print filter's friendly name
		VARIANT varName;
		VariantInit(&varName);
		hr = pPropBag->Read(L"FriendlyName", &varName, 0);
		if (SUCCEEDED(hr))
		{
			if (varName.vt == VT_BSTR) {
				BSTR name = varName.bstrVal;
				friendlyName.clear();
				friendlyName.append(name);
			} else {
				// ERROR -- friendly name was not a BSTR
				hr = E_FAIL;
			}
		}
		VariantClear(&varName);

done:
	SafeRelease(&pPropBag);
	return hr;
}

HRESULT CreateClassEnumerator(GUID deviceCategory, IEnumMoniker **ppClassEnum) {
	HRESULT hr;

	// create system device enumerator
	ICreateDevEnum *pDevEnum =NULL;
	hr = CoCreateInstance (CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC,
                           IID_ICreateDevEnum, (void **) &pDevEnum);
    if (FAILED(hr))
    {
        Msg(TEXT("Couldn't create system enumerator!  hr=0x%x"), hr);
    }

	// Create an enumerator for the video capture devices
	if (SUCCEEDED(hr))
	{
	    hr = pDevEnum->CreateClassEnumerator (deviceCategory, ppClassEnum, 0);
		if (FAILED(hr))
		{
			Msg(TEXT("Couldn't create class enumerator!  hr=0x%x"), hr);
	    }
	}

	if (SUCCEEDED(hr))
	{
		// If there are no enumerators for the requested type, then 
		// CreateVideoClassEnumerator will succeed, but *ppClassEnum will be NULL.
		if (*ppClassEnum == NULL)
		{
			MessageBox(NULL, TEXT("No video capture device was detected.\r\n\r\n")
				TEXT("This sample requires a video capture device, such as a USB WebCam,\r\n")
				TEXT("to be installed and working properly.  The sample will now close."),
				TEXT("No Video Capture Hardware"), MB_OK | MB_ICONINFORMATION);
			hr = E_FAIL;
		}
	}

    SafeRelease(&pDevEnum);

	return hr;
}

HRESULT FindVideoOrDShowDevice(LPWSTR vcam_friendly_name, IBaseFilter ** ppSrcFilter)
{
	HRESULT hr = S_OK;
	hr = FindDevice(CLSID_VideoInputDeviceCategory, vcam_friendly_name, ppSrcFilter);
	if (FAILED(hr)) {
		hr = FindDevice(CLSID_LegacyAmFilterCategory, vcam_friendly_name, ppSrcFilter);
	}
	if (FAILED(hr)) {	
		TCHAR* friendly_name[MSG_STRING_MAXLEN];
		size_t num_chars_converted;
		wcstombs_s(&num_chars_converted, (TCHAR*) friendly_name, MSG_STRING_MAXLEN, vcam_friendly_name, _TRUNCATE);

		Msg("Couldn't create filter %s", friendly_name);
		goto done;
	}

done:
	return hr;
}

HRESULT FindDevice(GUID deviceCategory, LPWSTR vcam_friendly_name, IBaseFilter ** ppSrcFilter)
{
    HRESULT hr = S_OK;
    IBaseFilter * pSrc = NULL;
    IMoniker* pMoniker =NULL;
    IEnumMoniker *pClassEnum = NULL;

    if (!ppSrcFilter)
	{
        return E_POINTER;
	}
   
	hr = CreateClassEnumerator(deviceCategory, &pClassEnum);

	BOOL found = false;
	if (SUCCEEDED(hr))
	{
		BOOL done = false;
		while (!done && !found) {
			HRESULT hr2 = pClassEnum->Next(1, &pMoniker, NULL);
			if (hr2 == S_OK)
			{
				wstring friendly_name(L"");
				hr2 = getMonikerFriendlyName(pMoniker, friendly_name);
				if (friendly_name.compare(vcam_friendly_name)==0) {
					found=true;
				}
			} else {
				done=true;
			}
		}
	}

	// Use the first video capture device on the device list.
	// Note that if the Next() call succeeds but there are no monikers,
	// it will return S_FALSE (which is not a failure).  Therefore, we
	// check that the return code is S_OK instead of using SUCCEEDED() macro.

	if (!found)
	{
		//Msg(TEXT("Unable to find video capture device \"%s\""), vcam_friendly_name);   
		hr = E_FAIL;
	}

	if (SUCCEEDED(hr))
    {
        // Bind Moniker to a filter object
        hr = pMoniker->BindToObject(0,0,IID_IBaseFilter, (void**)&pSrc);
        if (FAILED(hr))
        {
            Msg(TEXT("Couldn't bind moniker to filter object!  hr=0x%x"), hr);
        }
    }

    // Copy the found filter pointer to the output parameter.
	if (SUCCEEDED(hr))
	{
	    *ppSrcFilter = pSrc;
		(*ppSrcFilter)->AddRef();
	}

	SafeRelease(&pSrc);
    SafeRelease(&pMoniker);
    SafeRelease(&pClassEnum);

    return hr;
}


// Copied and altered from DirectShow PlayCap Sample
HRESULT FindCaptureDevice(LPWSTR vcam_friendly_name, IBaseFilter ** ppSrcFilter)
{
    HRESULT hr = S_OK;
    IBaseFilter * pSrc = NULL;
    IMoniker* pMoniker =NULL;
    IEnumMoniker *pClassEnum = NULL;

    if (!ppSrcFilter)
	{
        hr = E_POINTER;
		goto donelabel;
	}
   
	hr = CreateVideoClassEnumerator(&pClassEnum);
	if (FAILED(hr)) goto donelabel;

	BOOL found = false;
	BOOL done = false;
	while (!done && !found) {
		HRESULT hr2 = pClassEnum->Next(1, &pMoniker, NULL);
		if (hr2 == S_OK)
		{
			wstring friendly_name(L"");
			hr2 = getMonikerFriendlyName(pMoniker, friendly_name);
			if (friendly_name.compare(vcam_friendly_name)==0) {
				found=true;
			}
		} else {
			done=true;
		}
	}

	// Use the first video capture device on the device list.
	// Note that if the Next() call succeeds but there are no monikers,
		// it will return S_FALSE (which is not a failure).  Therefore, we
		// check that the return code is S_OK instead of using SUCCEEDED() macro.

		if (!found)
		{
			//Msg(TEXT("Unable to find video capture device \"%s\""), vcam_friendly_name);   
			hr = E_FAIL;
			goto donelabel;
		}

		// Bind Moniker to a filter object
		hr = pMoniker->BindToObject(0,0,IID_IBaseFilter, (void**)&pSrc);
		if (FAILED(hr))
		{
			// Msg(TEXT("Couldn't bind moniker to filter object!  hr=0x%x"), hr);
			goto donelabel;
		}

		// Copy the found filter pointer to the output parameter.
		*ppSrcFilter = pSrc;
		(*ppSrcFilter)->AddRef();

donelabel:
	SafeRelease(&pSrc);
    SafeRelease(&pMoniker);
    SafeRelease(&pClassEnum);

    return hr;
}
