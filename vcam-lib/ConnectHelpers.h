// Copyright John MacCormick 2012. Modified BSD license. NO WARRANTY.
#pragma once

const int GUID_STRING_MAXLEN = 1024;
void Riid2String(REFIID riid, char *guidString);
HRESULT AddFilterByCLSID(
	IGraphBuilder *pGraph,      // Pointer to the Filter Graph Manager.
	REFGUID clsid,              // CLSID of the filter to create.
	IBaseFilter **ppF,          // Receives a pointer to the filter.
	LPCWSTR wszName             // A name for the filter (can be NULL).
	);
HRESULT IsPinConnected(IPin *pPin, BOOL *pResult);
HRESULT IsPinDirection(IPin *pPin, PIN_DIRECTION dir, BOOL *pResult);
HRESULT MatchPin(IPin *pPin, PIN_DIRECTION direction, BOOL bShouldBeConnected, BOOL *pResult);
HRESULT FindUnconnectedPin(IBaseFilter *pFilter, PIN_DIRECTION PinDir, IPin **ppPin);
HRESULT FindAPin(IBaseFilter *pFilter, PIN_DIRECTION PinDir, BOOL bShouldBeConnected, IPin **ppPin);
HRESULT FindAPin(IBaseFilter *pFilter, PIN_DIRECTION PinDir, IPin **ppPin);
HRESULT ConnectFilters(
	IGraphBuilder *pGraph, // Filter Graph Manager.
	IPin *pOut,            // Output pin on the upstream filter.
	IBaseFilter *pDest);    // Downstream filter.
HRESULT ConnectFilters(IGraphBuilder *pGraph, IBaseFilter *pSrc, IBaseFilter *pDest);
HRESULT ConnectFilters(IGraphBuilder *pGraph, IBaseFilter *pSrc, IPin *pIn);
HRESULT GetPinMediaType(
	IPin *pPin,             // pointer to the pin
	REFGUID majorType,      // desired major type, or GUID_NULL = don't care
	REFGUID subType,        // desired subtype, or GUID_NULL = don't care
	REFGUID formatType,     // desired format type, of GUID_NULL = don't care
	AM_MEDIA_TYPE **ppmt    // Receives a pointer to the media type. (Can be NULL)
	);
BOOL PinMatchesCategory(IPin *pPin, REFGUID Category);
HRESULT FindPinByCategory(
    IBaseFilter *pFilter, // Pointer to the filter to search.
    PIN_DIRECTION PinDir, // Direction of the pin.
    REFGUID Category,     // Pin category.
    IPin **ppPin);         // Receives a pointer to the pin.
BITMAPINFOHEADER* GetBITMAPINFOHEADER(const CMediaType *pmt);
BITMAPINFOHEADER* GetBITMAPINFOHEADER(const AM_MEDIA_TYPE *pmt);







