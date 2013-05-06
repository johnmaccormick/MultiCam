// Copyright John MacCormick 2012. Modified BSD license. NO WARRANTY.
// Helper functions for connecting pins and filters
#include <streams.h>
#include <Dvdmedia.h>

#include "jmac-vcam-guids.h"
#include "LogHelpers.h"
#include "ConnectHelpers.h"

// from http://msdn.microsoft.com/en-us/library/dd940435(v=VS.85).aspx
template <class T> void SafeRelease(T **ppT)
{
	if (*ppT)
	{
		(*ppT)->Release();
		*ppT = NULL;
	}
}

void Riid2String(REFIID riid, char *guidString) {
	OLECHAR guidWString[GUID_STRING_MAXLEN]; 
	StringFromGUID2(riid, guidWString, GUID_STRING_MAXLEN); 
	size_t num_chars_converted;
	wcstombs_s(&num_chars_converted, guidString, GUID_STRING_MAXLEN, guidWString, _TRUNCATE);
}

// Create a filter by CLSID and add it to the graph.
// copied from http://msdn.microsoft.com/en-us/library/dd373416(v=VS.85).aspx
HRESULT AddFilterByCLSID(
	IGraphBuilder *pGraph,      // Pointer to the Filter Graph Manager.
	REFGUID clsid,              // CLSID of the filter to create.
	IBaseFilter **ppF,          // Receives a pointer to the filter.
	LPCWSTR wszName             // A name for the filter (can be NULL).
	)
{
	*ppF = 0;

	IBaseFilter *pFilter = NULL;

	HRESULT hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, 
		IID_PPV_ARGS(&pFilter));
	if (FAILED(hr))
	{
		goto done;
	}

	hr = pGraph->AddFilter(pFilter, wszName);
	if (FAILED(hr))
	{
		goto done;
	}

	*ppF = pFilter;
	(*ppF)->AddRef();

done:
	SafeRelease(&pFilter);
	return hr;
}



// Query whether a pin is connected to another pin.
//
// Note: This function does not return a pointer to the connected pin.
// from http://msdn.microsoft.com/en-us/library/dd375792(v=VS.85).aspx
HRESULT IsPinConnected(IPin *pPin, BOOL *pResult)
{
	IPin *pTmp = NULL;
	HRESULT hr = pPin->ConnectedTo(&pTmp);
	if (SUCCEEDED(hr))
	{
		*pResult = TRUE;
	}
	else if (hr == VFW_E_NOT_CONNECTED)
	{
		// The pin is not connected. This is not an error for our purposes.
		*pResult = FALSE;
		hr = S_OK;
	}

	SafeRelease(&pTmp);
	return hr;
}


// Query whether a pin has a specified direction (input / output)
// from http://msdn.microsoft.com/en-us/library/dd375792(v=VS.85).aspx
HRESULT IsPinDirection(IPin *pPin, PIN_DIRECTION dir, BOOL *pResult)
{
	PIN_DIRECTION pinDir;
	HRESULT hr = pPin->QueryDirection(&pinDir);
	if (SUCCEEDED(hr))
	{
		*pResult = (pinDir == dir);
	}
	return hr;
}


// Match a pin by pin direction and connection state.
// from http://msdn.microsoft.com/en-us/library/dd375792(v=VS.85).aspx
HRESULT MatchPin(IPin *pPin, PIN_DIRECTION direction, BOOL bShouldBeConnected, BOOL *pResult)
{
	VCAM_ASSERT(pResult != NULL);

	BOOL bMatch = FALSE;
	BOOL bIsConnected = FALSE;

	HRESULT hr = IsPinConnected(pPin, &bIsConnected);
	if (SUCCEEDED(hr))
	{
		if (bIsConnected == bShouldBeConnected)
		{
			hr = IsPinDirection(pPin, direction, &bMatch);
		}
	}

	if (SUCCEEDED(hr))
	{
		*pResult = bMatch;
	}
	return hr;
}

// Return the first unconnected input pin or output pin.
// from http://msdn.microsoft.com/en-us/library/dd375792(v=VS.85).aspx
HRESULT FindUnconnectedPin(IBaseFilter *pFilter, PIN_DIRECTION PinDir, IPin **ppPin)
{
	IEnumPins *pEnum = NULL;
	IPin *pPin = NULL;
	BOOL bFound = FALSE;

	HRESULT hr = pFilter->EnumPins(&pEnum);
	if (FAILED(hr))
	{
		goto done;
	}

	while (S_OK == pEnum->Next(1, &pPin, NULL))
	{
		hr = MatchPin(pPin, PinDir, FALSE, &bFound);
		if (FAILED(hr))
		{
			goto done;
		}
		if (bFound)
		{
			*ppPin = pPin;
			(*ppPin)->AddRef();
			break;
		}
		SafeRelease(&pPin);
	}

	if (!bFound)
	{
		hr = VFW_E_NOT_FOUND;
	}

done:
	SafeRelease(&pPin);
	SafeRelease(&pEnum);
	return hr;
}

// Return the first input pin or output pin with desired connectivity.
// jmac edited FindUnconnectedPin(),
// from http://msdn.microsoft.com/en-us/library/dd375792(v=VS.85).aspx
HRESULT FindAPin(IBaseFilter *pFilter, PIN_DIRECTION PinDir, BOOL bShouldBeConnected, IPin **ppPin)
{
	IEnumPins *pEnum = NULL;
	IPin *pPin = NULL;
	BOOL bFound = FALSE;

	HRESULT hr = pFilter->EnumPins(&pEnum);
	if (FAILED(hr))
	{
		goto done;
	}

	while (S_OK == pEnum->Next(1, &pPin, NULL))
	{
		hr = MatchPin(pPin, PinDir, bShouldBeConnected, &bFound);
		if (FAILED(hr))
		{
			goto done;
		}
		if (bFound)
		{
			*ppPin = pPin;
			(*ppPin)->AddRef();
			break;
		}
		SafeRelease(&pPin);
	}

	if (!bFound)
	{
		hr = VFW_E_NOT_FOUND;
	}

done:
	SafeRelease(&pPin);
	SafeRelease(&pEnum);
	return hr;
}

// Return the first input pin or output pin.
// jmac edited FindUnconnectedPin(),
// from http://msdn.microsoft.com/en-us/library/dd375792(v=VS.85).aspx
HRESULT FindAPin(IBaseFilter *pFilter, PIN_DIRECTION PinDir, IPin **ppPin)
{
	IEnumPins *pEnum = NULL;
	IPin *pPin = NULL;
	BOOL bFound = FALSE;

	HRESULT hr = pFilter->EnumPins(&pEnum);
	if (FAILED(hr))	{goto done;}

	while (S_OK == pEnum->Next(1, &pPin, NULL))
	{
		hr = IsPinDirection(pPin, PinDir, &bFound);
		if (FAILED(hr))	{goto done;}
		if (bFound)
		{
			*ppPin = pPin;
			(*ppPin)->AddRef();
			break;
		}
		SafeRelease(&pPin);
	}

	if (!bFound)
	{
		hr = VFW_E_NOT_FOUND;
	}

done:
	SafeRelease(&pPin);
	SafeRelease(&pEnum);
	return hr;
}


// Connect output pin to filter.
// from http://msdn.microsoft.com/en-us/library/dd387915(v=VS.85).aspx
HRESULT ConnectFilters(
	IGraphBuilder *pGraph, // Filter Graph Manager.
	IPin *pOut,            // Output pin on the upstream filter.
	IBaseFilter *pDest)    // Downstream filter.
{
	IPin *pIn = NULL;

	// Find an input pin on the downstream filter.
	HRESULT hr = FindUnconnectedPin(pDest, PINDIR_INPUT, &pIn);
	if (SUCCEEDED(hr))
	{
		// Try to connect them.
		hr = pGraph->Connect(pOut, pIn);
		pIn->Release();
	}
	return hr;
}


// Connect filter to input pin.
// from http://msdn.microsoft.com/en-us/library/dd387915(v=VS.85).aspx
HRESULT ConnectFilters(IGraphBuilder *pGraph, IBaseFilter *pSrc, IPin *pIn)
{
	IPin *pOut = NULL;

	// Find an output pin on the upstream filter.
	HRESULT hr = FindUnconnectedPin(pSrc, PINDIR_OUTPUT, &pOut);
	if (SUCCEEDED(hr))
	{
		// Try to connect them.
		hr = pGraph->Connect(pOut, pIn);
		pOut->Release();
	}
	return hr;
}


// Connect filter to filter
// from http://msdn.microsoft.com/en-us/library/dd387915(v=VS.85).aspx
HRESULT ConnectFilters(IGraphBuilder *pGraph, IBaseFilter *pSrc, IBaseFilter *pDest)
{
	IPin *pOut = NULL;

	// Find an output pin on the first filter.
	HRESULT hr = FindUnconnectedPin(pSrc, PINDIR_OUTPUT, &pOut);
	if (SUCCEEDED(hr))
	{
		hr = ConnectFilters(pGraph, pOut, pDest);
		pOut->Release();
	}
	return hr;
}

// Given a pin, find a preferred media type 
// from http://msdn.microsoft.com/en-us/library/dd375618(v=VS.85).aspx
//
// pPin         Pointer to the pin.
// majorType    Preferred major type (GUID_NULL = don't care).
// subType      Preferred subtype (GUID_NULL = don't care).
// formatType   Preferred format type (GUID_NULL = don't care).
// ppmt         Receives a pointer to the media type. Can be NULL.
//
// Note: If you want to check whether a pin supports a desired media type,
//       but do not need the format details, set ppmt to NULL.
//
//       If ppmt is not NULL and the method succeeds, the caller must
//       delete the media type, including the format block. 

HRESULT GetPinMediaType(
	IPin *pPin,             // pointer to the pin
	REFGUID majorType,      // desired major type, or GUID_NULL = don't care
	REFGUID subType,        // desired subtype, or GUID_NULL = don't care
	REFGUID formatType,     // desired format type, of GUID_NULL = don't care
	AM_MEDIA_TYPE **ppmt    // Receives a pointer to the media type. (Can be NULL)
	)
{
	*ppmt = NULL;

	IEnumMediaTypes *pEnum = NULL;
	AM_MEDIA_TYPE *pmt = NULL;
	BOOL bFound = FALSE;

	HRESULT hr = pPin->EnumMediaTypes(&pEnum);
	if (FAILED(hr))
	{
		return hr;
	}

	while (hr = pEnum->Next(1, &pmt, NULL), hr == S_OK)
	{
		if ((majorType == GUID_NULL) || (majorType == pmt->majortype))
		{
			if ((subType == GUID_NULL) || (subType == pmt->subtype))
			{
				if ((formatType == GUID_NULL) || 
					(formatType == pmt->formattype))
				{
					// Found a match. 
					if (ppmt)
					{
						*ppmt = pmt;  // Return it to the caller
					}
					else
					{
						DeleteMediaType(pmt);
					}
					bFound = TRUE;
					break;
				}
			}
		}
		DeleteMediaType(pmt);
	}

	SafeRelease(&pEnum);
	if (SUCCEEDED(hr))
	{
		if (!bFound)
		{
			hr = VFW_E_NOT_FOUND;
		}
	}
	return hr;
}

// from http://msdn.microsoft.com/en-us/library/dd391001(VS.85).aspx
// Returns TRUE if a pin matches the specified pin category.
BOOL PinMatchesCategory(IPin *pPin, REFGUID Category)
{
    BOOL bFound = FALSE;

    IKsPropertySet *pKs = NULL;
    HRESULT hr = pPin->QueryInterface(IID_PPV_ARGS(&pKs));
    if (SUCCEEDED(hr))
    {
        GUID PinCategory;
        DWORD cbReturned;
        hr = pKs->Get(AMPROPSETID_Pin, AMPROPERTY_PIN_CATEGORY, NULL, 0, 
            &PinCategory, sizeof(GUID), &cbReturned);
        if (SUCCEEDED(hr) && (cbReturned == sizeof(GUID)))
        {
            bFound = (PinCategory == Category);
        }
        pKs->Release();
    }
    return bFound;
}


// from http://msdn.microsoft.com/en-us/library/dd391001(VS.85).aspx
// Finds the first pin that matches a specified pin category and direction.
HRESULT FindPinByCategory(
    IBaseFilter *pFilter, // Pointer to the filter to search.
    PIN_DIRECTION PinDir, // Direction of the pin.
    REFGUID Category,     // Pin category.
    IPin **ppPin)         // Receives a pointer to the pin.
{
    *ppPin = 0;

    HRESULT hr = S_OK;
    BOOL bFound = FALSE;

    IEnumPins *pEnum = 0;
    IPin *pPin = 0;

    hr = pFilter->EnumPins(&pEnum);
    if (FAILED(hr))
    {
        goto done;
    }

    while (hr = pEnum->Next(1, &pPin, 0), hr == S_OK)
    {
        PIN_DIRECTION ThisPinDir;
        hr = pPin->QueryDirection(&ThisPinDir);
        if (FAILED(hr))
        {
            goto done;
        }
        if ((ThisPinDir == PinDir) && 
            PinMatchesCategory(pPin, Category))
        {
            *ppPin = pPin;
            (*ppPin)->AddRef();   // The caller must release the interface.
            bFound = TRUE;
            break;
        }
        SafeRelease(&pPin);
    }

done:
    SafeRelease(&pPin);
    SafeRelease(&pEnum);
    if (SUCCEEDED(hr) && !bFound)
    {
        hr = E_FAIL;
    }
    return hr;
}

BITMAPINFOHEADER* GetBITMAPINFOHEADER(const CMediaType *pmt)
{
			BITMAPINFOHEADER *pbmi = NULL;
			if (IsEqualGUID(*pmt->FormatType(), FORMAT_VideoInfo))
			{
				VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *) pmt->pbFormat;
				pbmi = &(pvi->bmiHeader);
			}
			else if(IsEqualGUID(*pmt->FormatType(), FORMAT_VideoInfo2))
			{
				VIDEOINFOHEADER2 *pvi2 = (VIDEOINFOHEADER2 *) pmt->pbFormat;
				pbmi = &(pvi2->bmiHeader);	
			}
			else 
			{
				vcamLog(0, "MultiCamFilter::GetBITMAPINFOHEADER: ERROR: unexpected video format");
			}
			return pbmi;
}

BITMAPINFOHEADER* GetBITMAPINFOHEADER(const AM_MEDIA_TYPE *pmt)
{
			BITMAPINFOHEADER *pbmi = NULL;
			if (IsEqualGUID(pmt->formattype, FORMAT_VideoInfo))
			{
				VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *) pmt->pbFormat;
				pbmi = &(pvi->bmiHeader);
			}
			else if(IsEqualGUID(pmt->formattype, FORMAT_VideoInfo2))
			{
				VIDEOINFOHEADER2 *pvi2 = (VIDEOINFOHEADER2 *) pmt->pbFormat;
				pbmi = &(pvi2->bmiHeader);	
			}
			else 
			{
				vcamLog(0, "MultiCamFilter::GetBITMAPINFOHEADER: ERROR: unexpected video format");
			}
			return pbmi;
}

