// Copyright John MacCormick 2012. Modified BSD license. NO WARRANTY.
//------------------------------------------------------------------------------
// Portions of this file are derived from the DirectShow sample code file EZRGB24.cpp.
// Those portions are copyright (c) Microsoft Corporation.
//------------------------------------------------------------------------------

#include <streams.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
using namespace std;

#include <strsafe.h>
#include <vector>
#include <Dvdmedia.h>




#include "MultiCamFilter.h"
#include "jmac-vcam-guids.h"
#include "LogHelpers.h"
#include "SkypeApi.h"
#include "SwitchMsgWindow.h"
#include "ConnectHelpers.h"
#include "OverlayInputPin.h"
#include "DeviceHelpers.h"
#include "MessageBox.h"
#include "MultiCamOutputPin.h"
#include "DeviceList.h"

#define hrOK VCAM_ASSERT(SUCCEEDED(hr))  if (!SUCCEEDED(hr)) {return hr;}
#define hrOKnoBox VCAM_ASSERT_NOBOX(SUCCEEDED(hr))  if (!SUCCEEDED(hr)) {return hr;}
#define hrOKnoRet VCAM_ASSERT(SUCCEEDED(hr))
#define VCAM_BAIL(_x_) if (!(_x_)) {return MULTICAM_FAIL;}

char* MultiCamVersion = "1.0.2.19";

//WCHAR *UPSTREAM_DEVICE_FRIENDLY_NAME[] = {L"PushSource Bitmap Filter"};
//WCHAR *UPSTREAM_DEVICE_FRIENDLY_NAME[] = {L"Logitech Webcam 250"};
//WCHAR *UPSTREAM_DEVICE_FRIENDLY_NAME[] = {L"Bouncing Ball"};
//WCHAR *UPSTREAM_DEVICE_FRIENDLY_NAME[] = {L"Virtual Cam"};


//WCHAR *UPSTREAM_DEVICE_FRIENDLY_NAME[] = {L"Bouncing Ball", L"Virtual Cam"};
//WCHAR *UPSTREAM_DEVICE_FRIENDLY_NAME[] = {L"Virtual Cam", L"PushSource Bitmap Filter"};
//WCHAR *UPSTREAM_DEVICE_FRIENDLY_NAME[] = {L"PushSource Bitmap Filter", L"Virtual Cam"};
//WCHAR *UPSTREAM_DEVICE_FRIENDLY_NAME[] = {L"Logitech Webcam 250", L"PushSource Bitmap Filter"};
//WCHAR *UPSTREAM_DEVICE_FRIENDLY_NAME[] = {L"Logitech Webcam 250", L"Logitech QuickCam Chat"};
//WCHAR *UPSTREAM_DEVICE_FRIENDLY_NAME[] = {L"Logitech QuickCam Chat", L"Logitech Webcam 250"};
//WCHAR *UPSTREAM_DEVICE_FRIENDLY_NAME[] = {L"Logitech Webcam 250", L"Virtual Cam"};
//WCHAR *UPSTREAM_DEVICE_FRIENDLY_NAME[] = {L"Virtual Cam", L"Logitech Webcam 250"};
//WCHAR *UPSTREAM_DEVICE_FRIENDLY_NAME[] = {L"Virtual Cam Clone", L"Virtual Cam"};

//WCHAR *UPSTREAM_DEVICE_FRIENDLY_NAME[] = {L"Bouncing Ball", L"Logitech Webcam 250", L"Logitech QuickCam Chat"};

//WCHAR *UPSTREAM_DEVICE_FRIENDLY_NAME[] = {L"Logitech Webcam 250", L"Logitech QuickCam Chat", L"PushSource Bitmap Filter",
//											L"Virtual Cam"};

//WCHAR *UPSTREAM_DEVICE_FRIENDLY_NAME[] = {
//											L"Logitech QuickCam Chat", 
//											L"Virtual Cam",
//											L"Bouncing Ball", 
//											L"Logitech Webcam 250", 
//											L"PushSource Bitmap Filter",
//};

GUID UNACCEPTABLE_CAMERA_GUIDS[] = {CLSID_Multicam, CLSID_UpstreamCam, CLSID_HybridCam 
									, CLSID_VirtualCamDoubler, CLSID_VirtualCamClone
									, CLSID_VirtualCam3, CLSID_VirtualCam2, CLSID_VirtualCam
									, CLSID_GoogleAdapter0 
									, CLSID_GoogleAdapter1
									, CLSID_VHScrCap
									, CLSID_Softcam
									, CLSID_FakeWebcamSource
									, CLSID_SplitCam
									, CLSID_UScreenCapture
									, CLSID_VHMultiCam
};

string UNACCEPTABLE_CAMERA_NAMES[] = {
	  "ManyCam Video Source" // NEVER allow this one; it causes a deadlock when switching cameras
	, "ManyCam Virtual Webcam"
	, "CyberLink Webcam Splitter"
	, "TrackerCam Capture"
	, "DVdriver"
	, "WebcamMax Capture"
	//, "Logitech QuickCam Easy/Cool"
	//, "Microsoft LifeCam HD-3000"
};

const LONG MultiCamFilter::s_IDEAL_CAMERA_HEIGHT = 480;//480;
const LONG MultiCamFilter::s_MAX_CAMERA_HEIGHT = 768;//768;

// from http://msdn.microsoft.com/en-us/library/dd940435(v=VS.85).aspx
template <class T> void SafeRelease(T **ppT)
{
	if (*ppT)
	{
		(*ppT)->Release();
		*ppT = NULL;
	}
}

//
// Constructor
//
MultiCamFilter::MultiCamFilter(TCHAR *tszName,
                   LPUNKNOWN punk,
                   HRESULT *phr) :
    CTransformFilter(tszName, punk, CLSID_Multicam),
    m_lBufferRequest(1),
	m_ppUpstreamFilter(NULL),
	m_ppInputs(NULL),
	m_iMainInputID(0),
	m_ppOverlays(NULL),
	m_fViewAllCameras(TRUE),
	m_fOverlayCameras(FALSE),
	m_Width(-1),
	m_Height(-1),
	m_iNumUpstreamFilters(0),
	m_iNumValidFilters(0),
	m_frameCounter(0)
{
	vcamOpenLog(5, "MultiCamFilter");

	vcamLog(10, "**********************MultiCamFilter::MultiCamFilter***********************");
	
	vcamLog(0, "MultiCam filter version %s", MultiCamVersion);

	VCAM_ASSERT(SUCCEEDED(*phr));

	*phr = StartUp(TRUE);
	//*phr = InitializeUpstreamInterfaces(); 	VCAM_ASSERT(SUCCEEDED(*phr));
	//*phr = LogCapabilities();					VCAM_ASSERT(SUCCEEDED(*phr));
	//*phr = SetUpstreamFormats();				VCAM_ASSERT(SUCCEEDED(*phr));

	//VCAM_ASSERT(m_iNumUpstreamFilters >= 0 && m_iNumUpstreamFilters <= MULTICAM_NUM_INPUT_PINS);

	//m_pSwitchMsgWindow = new CSwitchMsgWindow(this);

	//m_ppOverlays = new IMediaSample*[m_iNumUpstreamFilters];
	//for(int i=0; i<m_iNumUpstreamFilters; i++){
	//	m_ppOverlays[i] = NULL;
	//}


	

	vcamLog(10, "MultiCamFilter::MultiCamFilter: constructor complete");
} // (Constructor)


HRESULT MultiCamFilter::StartUp(BOOL inConstructor)
{
	HRESULT hr = S_OK;


	hr = InitializeUpstreamInterfaces(); 	hrOKnoRet;
	hr = LogCapabilities();					hrOKnoRet;
	hr = SetUpstreamFormats();				hrOKnoRet;

	// This wasn't here before. Why not?
	hr = InitializeOutputPin();				hrOKnoRet;

	VCAM_ASSERT(m_iNumUpstreamFilters >= 0 && m_iNumUpstreamFilters <= MULTICAM_NUM_INPUT_PINS);
	VCAM_ASSERT(m_iNumValidFilters >= 0 && m_iNumValidFilters <= MULTICAM_NUM_INPUT_PINS);

	m_pSwitchMsgWindow = new CSwitchMsgWindow(this);

	m_ppOverlays = new IMediaSample*[m_iNumUpstreamFilters];
	for(int i=0; i<m_iNumUpstreamFilters; i++){
		m_ppOverlays[i] = NULL;
	}

	for(int i=0; i<NUM_FRAME_RATES; i++){
		m_frameRates[i] = 0.0;
	}
	return hr;
}

HRESULT MultiCamFilter::Shutdown(BOOL inDestructor)
{
	HRESULT hr = S_OK;

	delete m_pSwitchMsgWindow;

	// See the comment in StoreSampleForOverlay() -- no need to release m_ppOverlays[i]
	delete [] m_ppOverlays;

	if(m_ppUpstreamFilter != NULL) {
		for(int i=0; i<m_iNumUpstreamFilters; i++){
			SafeRelease(&(m_ppUpstreamFilter[i]));
		}

		delete [] m_ppUpstreamFilter;
	}

	if(m_ppInputs != NULL) {
		for(int i=0; i<MULTICAM_NUM_INPUT_PINS; i++){
			// if we are in the destructor don't delete m_pInput, since a superclass will delete it later
			if( !(inDestructor && (m_ppInputs[i] == (COverlayInputPin*) m_pInput) ) ) {
				delete m_ppInputs[i];
				m_ppInputs[i] = NULL;
			}
		}

		delete [] m_ppInputs;
		m_ppInputs = NULL;
	}

	return hr;
}

// This method doesn't work. Commented out for now.
//HRESULT MultiCamFilter::Reset()
//{
//	HRESULT hr = S_OK;
//	IMediaControl * pMC = NULL;
//
//    hr = m_pGraph->QueryInterface(IID_IMediaControl,(LPVOID *) &pMC);
//	hrOK;
//
//	hr = pMC->Stop();
//	if (!SUCCEEDED(hr)) goto done;
//
//	for(int i=0; i<m_iNumUpstreamFilters; i++){
//		hr = m_pGraph->Disconnect(m_ppInputs[i]);
//		hrOKnoRet;
//		if (FAILED(hr)) { goto done;}
//	}
//	hr = m_pGraph->Disconnect(m_pOutput);
//	hrOKnoRet;
//	if (FAILED(hr)) { goto done;}
//
//
//	hr = Shutdown(FALSE);
//	if (!SUCCEEDED(hr)) goto done;
//
//	hr = StartUp(FALSE);
//	if (!SUCCEEDED(hr)) goto done;
//
//done:
//	SafeRelease(&pMC);
//
//	return hr;
//}

//
// CreateInstance
//
// Provide the way for COM to create a MultiCamFilter object
//
CUnknown *MultiCamFilter::CreateInstance(LPUNKNOWN punk, HRESULT *phr)
{
    VCAM_ASSERT(phr);
    
    MultiCamFilter *pNewObject = new MultiCamFilter(NAME("MultiCam Filter"), punk, phr);

    if (pNewObject == NULL) {
        if (phr)
            *phr = E_OUTOFMEMORY;
    }
    return pNewObject;

} // CreateInstance


//
// NonDelegatingQueryInterface
//
STDMETHODIMP MultiCamFilter::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
	HRESULT hr = S_OK;
    CheckPointer(ppv,E_POINTER);

    hr =  CTransformFilter::NonDelegatingQueryInterface(riid, ppv);

	char guidString[GUID_STRING_MAXLEN];
	Riid2String(riid, guidString);
	vcamLog(10, "MultiCamFilter::NonDelegatingQueryInterface, riid = %s", guidString);
	if (hr==S_OK) {
		vcamLog(10, "       MultiCamFilter::NonDelegatingQueryInterface returning S_OK");
	}
	else if (hr==E_NOINTERFACE ) {
		vcamLog(10, "       MultiCamFilter::NonDelegatingQueryInterface returning E_NOINTERFACE ");
	}
	else if (hr==E_POINTER) {
		vcamLog(10, "       MultiCamFilter::NonDelegatingQueryInterface returning E_POINTER");
	} else {
		vcamLog(10, "       MultiCamFilter::NonDelegatingQueryInterface returning 0x%x", hr);
	}

	return hr;
} // NonDelegatingQueryInterface

double MultiCamFilter::getAverageFrameRate()
{
	double sum = 0.0;
	for(int i=0; i<NUM_FRAME_RATES; i++){
		sum += m_frameRates[i];
	}
	return sum / NUM_FRAME_RATES;
}

void MultiCamFilter::insertFrameRate(double newFrameRate)
{
	for(int i=1; i<NUM_FRAME_RATES; i++){
		m_frameRates[i-1] = m_frameRates[i];
	}
	m_frameRates[NUM_FRAME_RATES-1] = newFrameRate;
}

void MultiCamFilter::logFrameRate()
{
	const int interval = 50;
	if (m_frameCounter==0)
	{
		m_lastFrameTime = clock();
	}
	else if (m_frameCounter % interval == 0)
	{
		clock_t latestFrameTime = clock();
		double duration = (double(latestFrameTime)-double(m_lastFrameTime))/CLOCKS_PER_SEC;
		double frameRate = (double) interval / duration;
		insertFrameRate(frameRate);
		m_lastFrameTime = latestFrameTime;
		double averageFrameRate = getAverageFrameRate();
		vcamLog(0, "Frame rate for %d frames: %g; for %d: %g", interval, frameRate, NUM_FRAME_RATES*interval, averageFrameRate);
	}
	m_frameCounter++;
}

//
// Transform
//
// Copy the input sample into the output sample
HRESULT MultiCamFilter::Transform(IMediaSample *pIn, IMediaSample *pOut)
{
    CheckPointer(pIn,E_POINTER);   
    CheckPointer(pOut,E_POINTER);   

	/////////////////////// frame rate stuff //////////////////////////
	logFrameRate();
	/////////////////////// end frame rate stuff //////////////////////////


	////////////////// Debugging //////////////////////
#ifdef DEBUG
	//long ds = pOut->GetSize();
	//long da = pOut->GetActualDataLength();
	//long ss = pIn->GetSize();
	//long sa = pIn->GetActualDataLength();
	VCAM_ASSERT(pOut->GetSize() >= pOut->GetActualDataLength());
	VCAM_ASSERT(pIn->GetSize() >= pIn->GetActualDataLength());
	VCAM_ASSERT(pIn->GetActualDataLength() <= pOut->GetActualDataLength());
	VCAM_ASSERT(pOut->GetActualDataLength() == GetOutputPin()->CurrentMediaType().GetSampleSize());
	VCAM_ASSERT(pIn->GetActualDataLength() == GetInputPin(m_iMainInputID)->CurrentMediaType().lSampleSize);
#endif
	////////////////// end Debugging //////////////////////

    // Copy the properties across
    HRESULT hr = Copy(pIn, pOut);
    if (FAILED(hr)) {
        return hr;
    }

	hr = FillBitmap(pIn, pOut);
    return hr;
} // Transform


//
// Copy
//
// Make destination an identical copy of source
//
HRESULT MultiCamFilter::Copy(IMediaSample *pSource, IMediaSample *pDest) 
{
    CheckPointer(pSource,E_POINTER);   
    CheckPointer(pDest,E_POINTER);   

    ////////////////// Debugging //////////////////////
	VCAM_ASSERT(pDest->GetActualDataLength() == GetOutputPin()->CurrentMediaType().GetSampleSize());
	VCAM_ASSERT(pSource->GetActualDataLength() == GetInputPin(m_iMainInputID)->CurrentMediaType().lSampleSize);
	////////////////// end Debugging //////////////////////

    // Copy the sample data
    BYTE *pSourceBuffer, *pDestBuffer;
    long lSourceSize = pSource->GetActualDataLength();

#ifdef _DEBUG
    long lDestSize = pDest->GetSize();
    VCAM_ASSERT(lDestSize >= lSourceSize);

	long ds = pDest->GetSize();
	long da = pDest->GetActualDataLength();
	long ss = pSource->GetSize();
	long sa = pSource->GetActualDataLength();
	VCAM_ASSERT(pDest->GetSize() >= pDest->GetActualDataLength());
	VCAM_ASSERT(pSource->GetSize() >= pSource->GetActualDataLength());
	VCAM_ASSERT(pSource->GetActualDataLength() <= pDest->GetActualDataLength());
#endif

    pSource->GetPointer(&pSourceBuffer);
    pDest->GetPointer(&pDestBuffer);

	ZeroMemory(pDestBuffer, pDest->GetActualDataLength());

    REFERENCE_TIME TimeStart, TimeEnd;
    if (NOERROR == pSource->GetTime(&TimeStart, &TimeEnd)) {
        pDest->SetTime(&TimeStart, &TimeEnd);
    }

    LONGLONG MediaStart, MediaEnd;
    if (pSource->GetMediaTime(&MediaStart,&MediaEnd) == NOERROR) {
        pDest->SetMediaTime(&MediaStart,&MediaEnd);
    }

    // Copy the Sync point property
    HRESULT hr = pSource->IsSyncPoint();
    if (hr == S_OK) {
        pDest->SetSyncPoint(TRUE);
    }
    else if (hr == S_FALSE) {
        pDest->SetSyncPoint(FALSE);
    }
    else {  // an unexpected error has occured...
        return E_UNEXPECTED;
    }

	// Copy known output media type to destination media type
	AM_MEDIA_TYPE *pmtOut = &GetOutputPin()->CurrentMediaType();
	pDest->SetMediaType(pmtOut);

    // Copy the preroll property
    hr = pSource->IsPreroll();
    if (hr == S_OK) {
        pDest->SetPreroll(TRUE);
    }
    else if (hr == S_FALSE) {
        pDest->SetPreroll(FALSE);
    }
    else {  // an unexpected error has occured...
        return E_UNEXPECTED;
    }

    // Copy the discontinuity property
    hr = pSource->IsDiscontinuity();
    if (hr == S_OK) {
    pDest->SetDiscontinuity(TRUE);
    }
    else if (hr == S_FALSE) {
        pDest->SetDiscontinuity(FALSE);
    }
    else {  // an unexpected error has occured...
        return E_UNEXPECTED;
    }

	// Do a sanity check on the actual data length
	VCAM_ASSERT(pDest->GetActualDataLength() == GetOutputPin()->CurrentMediaType().GetSampleSize());

    return NOERROR;

} // Copy

HRESULT MultiCamFilter::CopyBitmap(IMediaSample *pIn, IMediaSample *pOut) 
{
    ////////////////// Debugging //////////////////////
#ifdef DEBUG
	VCAM_ASSERT(pOut->GetActualDataLength() == GetOutputPin()->CurrentMediaType().GetSampleSize());
	VCAM_ASSERT(pIn->GetActualDataLength() == GetInputPin(m_iMainInputID)->CurrentMediaType().lSampleSize);
#endif
	////////////////// end Debugging //////////////////////

	HRESULT hr = S_OK;
    BYTE *pDataIn, *pDataOut;                // Pointer to the actual image buffer
    long lDataLenIn, lDataLenOut;              // Holds length of any given sample

    AM_MEDIA_TYPE mediaTypeIn = m_pInput->CurrentMediaType();
    VIDEOINFOHEADER *pviIn = (VIDEOINFOHEADER *) mediaTypeIn.pbFormat;
    VCAM_ASSERT(pviIn);

    CheckPointer(pIn,E_POINTER);
    pIn->GetPointer(&pDataIn);
    lDataLenIn = pIn->GetSize();

    int cxImageIn    = pviIn->bmiHeader.biWidth;
    int cyImageIn    = pviIn->bmiHeader.biHeight;
	int bitCountIn   = pviIn->bmiHeader.biBitCount;
	// see "Calculating Surface Stride" in http://msdn.microsoft.com/en-us/library/dd318229(v=VS.85).aspx
	int strideIn     = ((((cxImageIn * bitCountIn) + 31) & ~31) >> 3);
	// check that it is a bottom-up image
	VCAM_ASSERT(pviIn->bmiHeader.biHeight > 0);

	VCAM_ASSERT(lDataLenIn == pviIn->bmiHeader.biSizeImage);

	AM_MEDIA_TYPE* pTypeOut = &m_pOutput->CurrentMediaType();
    VIDEOINFOHEADER *pviOut = (VIDEOINFOHEADER *) pTypeOut->pbFormat;
    VCAM_ASSERT(pviOut);

    CheckPointer(pOut,E_POINTER);
    pOut->GetPointer(&pDataOut);
    lDataLenOut = pOut->GetActualDataLength();

    int cxImageOut    = pviOut->bmiHeader.biWidth;
    int cyImageOut    = pviOut->bmiHeader.biHeight;
	int bitCountOut   = pviOut->bmiHeader.biBitCount;
	// see "Calculating Surface Stride" in http://msdn.microsoft.com/en-us/library/dd318229(v=VS.85).aspx
	int strideOut     = ((((cxImageOut * bitCountOut) + 31) & ~31) >> 3);
	// check that it is a bottom-up image
	VCAM_ASSERT(pviOut->bmiHeader.biHeight > 0);

	VCAM_ASSERT(lDataLenOut == pviOut->bmiHeader.biSizeImage);

	// Check that source is not too big for destination
	VCAM_ASSERT(cxImageIn <= cxImageOut);
	VCAM_ASSERT(cyImageIn <= cyImageOut);

	
	BYTE *pRowIn, *pRowOut;
	pRowIn = pDataIn;
	// if the heights are not the same, leave blank space at the bottom
	pRowOut = pDataOut + (strideOut * (cyImageOut - cyImageIn));
	const int rowBytesIn = cxImageIn * sizeof(RGBTRIPLE);

	for (int yIn = 0; yIn < cyImageIn; yIn++, pRowIn+=strideIn, pRowOut+=strideOut)
	{
		CopyMemory(pRowOut, pRowIn, rowBytesIn);
	}

	return hr;
}

HRESULT MultiCamFilter::FillBitmap(IMediaSample *pIn, IMediaSample *pOut) 
{
	HRESULT hr = S_OK;
	if (!m_fViewAllCameras || (m_iNumValidFilters==1))	
	{
		hr = CopyBitmap(pIn, pOut);			hrOK;
	}
	else
	{
		//for(int i=0; i<5; i++) {
			hr = MakeAllCamerasBitmap(pOut);	hrOKnoBox;
	}

	if (m_fOverlayCameras)
	{
		hr = OverlayCameras(pOut);			hrOK;		
	}
	return hr;
}

// Return the index value of the next valid (i.e. non-null) filter in the array m_ppUpstreamFilter,
// or -1 if no such valid filter exists.
int MultiCamFilter::NextValidFilter(int index)
{
	VCAM_ASSERT(0 <= index && index < m_iNumUpstreamFilters);
	index++;
	if (index == m_iNumUpstreamFilters) {
		return -1;
	}
	return NextOrCurrentValidFilter(index);
}

// Return the index value of the next valid (i.e. non-null) filter in the array m_ppUpstreamFilter,
// including the given index, or -1 if no such valid filter exists.
int MultiCamFilter::NextOrCurrentValidFilter(int index)
{
	VCAM_ASSERT(0 <= index && index < m_iNumUpstreamFilters);
	if (m_iNumValidFilters == 0)
	{
		return -1;
	}
	if (m_ppUpstreamFilter[index] != NULL)
	{
		return index;
	}
	while(m_ppUpstreamFilter[index] == NULL && index < m_iNumUpstreamFilters)
	{
		index++;
	}
	if (index < m_iNumUpstreamFilters) 
	{
		return index;
	}
	else 
	{
		return -1;
	}
}

HRESULT MultiCamFilter::MakeAllCamerasBitmap(IMediaSample *pOut) 
{
	///////// debugging
	//long size = pOut->GetSize();
	//int wid = m_Width;
	//int hei = m_Height;
	//int bytes = sizeof(RGBTRIPLE);
	//long size_needed = wid * hei * bytes;
	//VCAM_ASSERT(size_needed <= size);
	///////// end debugging

	HRESULT hr = S_OK;
	int xStart = 0;
	int yStart = 0;
	int xEnd = 0;
	int yEnd = 0;
	int xBase = 0;
	int yBase = 0;

	int filterIndex = NextOrCurrentValidFilter(0);

	switch (m_iNumValidFilters) {
	case 0:
		break;
	case 1:
		//xStart = 0;
		//yStart = 0;
		//hr = Overlay(0, 1, pOut, /*inout*/xStart, /*inout*/yStart);  hrOKnoBox;
		vcamLog(0, "MultiCamFilter::MakeAllCamerasBitmap: ERROR: it's pointless to make bitmap with only one camera");
		VCAM_ASSERT(FALSE);
		break;
	case 2:
		// left
		xStart = 0;
		yStart = 0;
		xEnd = m_Width / 2;
		yEnd = m_Height;
		hr = OverlayB(filterIndex, pOut, xStart, yStart, xEnd, yEnd);  hrOKnoBox;
		// right
		filterIndex = NextValidFilter(filterIndex);
		VCAM_ASSERT(filterIndex >= 0);
		xStart = m_Width / 2;
		xEnd = m_Width;
		yStart = 0;
		yEnd = m_Height;
		hr = OverlayB(filterIndex, pOut, xStart, yStart, xEnd, yEnd);  hrOKnoBox;
		break;
	case 3:
	case 4:
		// bottom left
		xStart = 0;
		yStart = 0;
		xEnd = m_Width / 2;
		yEnd = m_Height / 2;
		hr = OverlayB(filterIndex, pOut, xStart, yStart, xEnd, yEnd);  hrOKnoBox;
		yBase = yStart;
		xBase = xStart;
		// top left
		filterIndex = NextValidFilter(filterIndex);
		VCAM_ASSERT(filterIndex >= 0);
		xStart = 0;
		yStart = m_Height / 2;
		xEnd = m_Width / 2;
		yEnd = m_Height;
		hr = OverlayB(filterIndex, pOut, xStart, yStart, xEnd, yEnd);  hrOKnoBox;
		// top right
		filterIndex = NextValidFilter(filterIndex);
		VCAM_ASSERT(filterIndex >= 0);
		xStart = m_Width / 2;
		yStart = m_Height / 2;
		xEnd = m_Width;
		yEnd = m_Height;
		hr = OverlayB(filterIndex, pOut, xStart, yStart, xEnd, yEnd);  hrOKnoBox;
		if (m_iNumValidFilters == 4)
		{
			// bottom right
			filterIndex = NextValidFilter(filterIndex);
			VCAM_ASSERT(filterIndex >= 0);
			xStart = m_Width / 2;
			yStart = 0;
			xEnd = m_Width;
			yEnd = m_Height / 2;
			hr = OverlayB(filterIndex, pOut, xStart, yStart, xEnd, yEnd);  hrOKnoBox;
		}
		break;
	case 5:
	case 6:
		// bottom left
		xStart = 0;
		yStart = 0;
		xEnd = m_Width / 3;
		yEnd = m_Height / 2;
		hr = OverlayB(filterIndex, pOut, xStart, yStart, xEnd, yEnd);  hrOKnoBox;
		// top left
		filterIndex = NextValidFilter(filterIndex);
		VCAM_ASSERT(filterIndex >= 0);
		xStart = 0;
		yStart = m_Height / 2;
		xEnd = m_Width / 3;
		yEnd = m_Height;
		hr = OverlayB(filterIndex, pOut, xStart, yStart, xEnd, yEnd); hrOKnoBox;
		// bottom middle
		filterIndex = NextValidFilter(filterIndex);
		VCAM_ASSERT(filterIndex >= 0);
		xStart = m_Width / 3;
		yStart = 0;
		xEnd = 2 * m_Width / 3;
		yEnd = m_Height / 2;
		hr = OverlayB(filterIndex, pOut, xStart, yStart, xEnd, yEnd);  hrOKnoBox;
		// top middle
		filterIndex = NextValidFilter(filterIndex);
		VCAM_ASSERT(filterIndex >= 0);
		xStart = m_Width / 3;
		yStart = m_Height / 2;
		xEnd = 2 * m_Width / 3;
		yEnd = m_Height;
		hr = OverlayB(filterIndex, pOut, xStart, yStart, xEnd, yEnd);  hrOKnoBox;
		// top right
		filterIndex = NextValidFilter(filterIndex);
		VCAM_ASSERT(filterIndex >= 0);
		xStart = 2 * m_Width / 3;
		yStart = m_Height / 2;
		xEnd = m_Width;
		yEnd = m_Height;		
		hr = OverlayB(filterIndex, pOut, xStart, yStart, xEnd, yEnd); hrOKnoBox;
		if (m_iNumValidFilters == 6)
		{
			// bottom right
			filterIndex = NextValidFilter(filterIndex);
			VCAM_ASSERT(filterIndex >= 0);
			xStart = 2 * m_Width / 3;
			yStart = 0;
			xEnd = m_Width;
			yEnd = m_Height / 2;	
			hr = OverlayB(filterIndex, pOut, xStart, yStart, xEnd, yEnd);  hrOKnoBox;
		}
		break;
	default:
		VCAM_ASSERT(FALSE);
	}

	return hr;
}

HRESULT MultiCamFilter::OverlayCameras(IMediaSample *pOut) 
{
	HRESULT hr = S_OK;
	int xStart = 0;
	for(int i=0; i< m_iNumUpstreamFilters; i++){
		if ( (m_ppUpstreamFilter[i] != NULL) 
				&& (m_ppOverlays[i] != NULL) 
				&& (i != m_iMainInputID) ) {
			hr = Overlay(i, pOut, xStart);
		}
		hrOK;
	}
	return hr;
}



// Check the input type is OK - return an error otherwise
HRESULT MultiCamFilter::CheckInputType(const CMediaType *mtIn)
{
    CheckPointer(mtIn,E_POINTER);

    // check this is a VIDEOINFOHEADER type
    if (!(*mtIn->FormatType() == FORMAT_VideoInfo || *mtIn->FormatType() == FORMAT_VideoInfo2)) {
        return E_INVALIDARG;
    }

    // Can we transform this type
    if (IsRgb24(mtIn)) {
        return NOERROR;
    }

	//vcamLog(10, "MultiCamFilter::CheckInputType, returning E_FAIL");
    return E_FAIL;
}


//
// Checktransform
//
// Check a transform can be done between these formats
//
HRESULT MultiCamFilter::CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut)
{
    CheckPointer(mtIn,E_POINTER);
    CheckPointer(mtOut,E_POINTER);

    if (IsRgb24(mtIn)) 
    {
        if (OnlySizesDiffer(mtIn, mtOut) )
		{
			if (MatchesOutputSize(mtOut) )
			{
				return NOERROR;
			}
		}
	}

	//vcamLog(10, "MultiCamFilter::CheckTransform, returning E_FAIL; requested media types were as follows");
	//vcamLog(10, "mtIn:");
	//vcamLogFormat(12, mtIn);
	//vcamLog(10, "mtOut:");
	//vcamLogFormat(12, mtOut);
	
    return E_FAIL;

} // CheckTransform


//
// DecideBufferSize
//
// Tell the output pin's allocator what size buffers we
// require.
//
HRESULT MultiCamFilter::DecideBufferSize(IMemAllocator *pAlloc,ALLOCATOR_PROPERTIES *pProperties)
{
    CheckPointer(pAlloc,E_POINTER);
    CheckPointer(pProperties,E_POINTER);
    HRESULT hr = NOERROR;

    pProperties->cBuffers = 1;
    pProperties->cbBuffer = m_pOutput->CurrentMediaType().GetSampleSize();
    VCAM_ASSERT(pProperties->cbBuffer);

    // Ask the allocator to reserve us some sample memory, NOTE the function
    // can succeed (that is return NOERROR) but still not have allocated the
    // memory that we requested, so we must check we got whatever we wanted

    ALLOCATOR_PROPERTIES Actual;
    hr = pAlloc->SetProperties(pProperties,&Actual);
    if (FAILED(hr)) {
        return hr;
    }

    VCAM_ASSERT( Actual.cBuffers == 1 );

    if (pProperties->cBuffers > Actual.cBuffers ||
            pProperties->cbBuffer > Actual.cbBuffer) {
                return E_FAIL;
    }
    return NOERROR;

} // DecideBufferSize


//
// GetMediaType
//
// I support one type, namely the type of the output pin
HRESULT MultiCamFilter::GetMediaType(int iPosition, CMediaType *pMediaType)
{
	vcamLog(50, "MultiCamFilter::GetMediaType, iPosition = %d", iPosition);
    if (iPosition < 0) {
		vcamLog(50, "    GetMediaType returning E_INVALIDARG (0x%x)", E_INVALIDARG);
        return E_INVALIDARG;
    }

    if (iPosition > 0) {
		vcamLog(50, "    GetMediaType returning VFW_S_NO_MORE_ITEMS (0x%x)", VFW_S_NO_MORE_ITEMS);
        return VFW_S_NO_MORE_ITEMS;
    }

    CheckPointer(pMediaType,E_POINTER);
    *pMediaType = m_pOutput->CurrentMediaType();

	vcamLog(50, "    GetMediaType returning S_OK (0x%x)", S_OK);
    return NOERROR;

} // GetMediaType


//
// IsRgb24
//
// Check if this is a RGB24 true colour format
//
BOOL MultiCamFilter::IsRgb24(const CMediaType *pMediaType) const
{
    CheckPointer(pMediaType,FALSE);

    if (IsEqualGUID(*pMediaType->Type(), MEDIATYPE_Video)) 
    {
        if (IsEqualGUID(*pMediaType->Subtype(), MEDIASUBTYPE_RGB24)) 
        {
            VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *) pMediaType->Format();
            return (pvi->bmiHeader.biBitCount == 24);
        }
    }

    return FALSE;

} // IsRgb24

/////////////////////////////////////////////////////////////////////////////
/////////////////   jmac changes 
/////////////////////////////////////////////////////////////////////////////

MultiCamFilter::~MultiCamFilter(void)
{
	vcamLog(10, "~~~~~~~~~~~~~~~~~~~~~~~MultiCamFilter::~MultiCamFilter~~~~~~~~~~~~~~~~~~~~~~~~~");

	Shutdown(TRUE);

	//delete m_pSwitchMsgWindow;

	//// See the comment in StoreSampleForOverlay() -- no need to release m_ppOverlays[i]
	//delete [] m_ppOverlays;

	//if(m_ppUpstreamFilter != NULL) {
	//	for(int i=0; i<m_iNumUpstreamFilters; i++){
	//		SafeRelease(&(m_ppUpstreamFilter[i]));
	//	}

	//	delete [] m_ppUpstreamFilter;
	//}

	//if(m_ppInputs != NULL) {
	//	for(int i=0; i<MULTICAM_NUM_INPUT_PINS; i++){
	//		// don't delete m_pInput, since a superclass will delete it later
	//		if(m_ppInputs[i] != (COverlayInputPin*) m_pInput) {
	//			delete m_ppInputs[i];
	//			m_ppInputs[i] = NULL;
	//		}
	//	}

	//	delete [] m_ppInputs;
	//	m_ppInputs = NULL;
	//}

	vcamCloseLog(5, "MultiCamFilter");
}

BOOL MultiCamFilter::IsAcceptableCameraGUID(GUID guid)
{
	int num_unacceptable_guids = sizeof(UNACCEPTABLE_CAMERA_GUIDS) / sizeof(UNACCEPTABLE_CAMERA_GUIDS[0]);
	for(int i=0; i<num_unacceptable_guids; i++){
		if (IsEqualGUID(guid, UNACCEPTABLE_CAMERA_GUIDS[i])) {
			return FALSE;
		}
	}
	return TRUE;
}

BOOL MultiCamFilter::IsAcceptableCameraName(string& friendlyName)
{
	int num_unacceptable_names = sizeof(UNACCEPTABLE_CAMERA_NAMES) / sizeof(UNACCEPTABLE_CAMERA_NAMES[0]);
	for(int i=0; i<num_unacceptable_names; i++){
		string currentName = UNACCEPTABLE_CAMERA_NAMES[i];
		if (friendlyName.find(currentName) != string::npos) {
			return FALSE;
		}
	}
	return TRUE;
}

void MultiCamFilter::ExtendDeviceName(string& friendlyName, int deviceCount)
{
	char deviceCountString[256];

	if (deviceCount > 1)
	{
		_itoa_s(deviceCount, deviceCountString, 256, 10);

		friendlyName += " (";
		friendlyName += deviceCountString;
		friendlyName += ")";
	}
}

BOOL MultiCamFilter::IsAcceptableDevice(DeviceList& deviceList, map<string, int>& friendlyNames, 
	string& friendlyName, GUID classID)
{
	if (deviceList.DeviceListExists())
	{
		string extendedName = friendlyName;
		int deviceCount = friendlyNames[friendlyName];
		ExtendDeviceName(extendedName, deviceCount);
		return deviceList.ContainsDevice(extendedName);
	}
	else
	{
		return IsAcceptableCameraGUID(classID) 
						&& IsAcceptableCameraName(friendlyName);
	}
}

HRESULT MultiCamFilter::InitializeUpstreamInterfaces()
{
	//vcamLog(10, "MultiCamFilter::InitializeUpstreamInterfaces");
	HRESULT hr = S_OK;
    IEnumMoniker *pClassEnum = NULL;
	IMoniker* pMoniker =NULL;
    IBaseFilter * pSrc = NULL;
	CLSID classID;
	int filterCount = 0;
	map<string, int> friendlyNames;

	DeviceList deviceList;
	deviceList.ReadList();


	m_cameraFriendlyNames.clear();

	m_ppUpstreamFilter = new IBaseFilter*[MULTICAM_NUM_INPUT_PINS];
	VCAM_ASSERT(m_ppUpstreamFilter != NULL);
	for(int i=0; i<MULTICAM_NUM_INPUT_PINS; i++){
		m_ppUpstreamFilter[i] = NULL;
	}

	hr = CreateVideoClassEnumerator(&pClassEnum);
	if (FAILED(hr)) goto donelabel;

	BOOL done = false;
	filterCount = 0;
	while (filterCount < MULTICAM_NUM_INPUT_PINS && !done)
	{
		hr = pClassEnum->Next(1, &pMoniker, NULL);
		if (hr == S_OK)
		{
			string friendlyName = "NoFriendlyNameFound";
			IPropertyBag *pPropBag;
			hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, 
				(void **)&pPropBag);
			if (SUCCEEDED(hr))
			{
				// Retrieve the filter's friendly name
				VARIANT varName;
				VariantInit(&varName);
				hr = pPropBag->Read(L"FriendlyName", &varName, 0);
				if (SUCCEEDED(hr))
				{
					BSTR bstr = varName.bstrVal;
					CHAR str[GUID_STRING_MAXLEN];
					size_t num_chars_converted;
					wcstombs_s(&num_chars_converted, str, GUID_STRING_MAXLEN, bstr, _TRUNCATE);
					vcamLog(1, "InitializeCameraGUIDs: FriendlyName %s", str);
					//m_cameraFriendlyNames.push_back(str);
					friendlyName = str;
					friendlyNames[friendlyName]++;
				}
				VariantClear(&varName);
				pPropBag->Release();
			}

			hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, 
				(void **)&pPropBag);
			if (SUCCEEDED(hr))
			{
				// Retrieve the filter's class id
				VARIANT varName;
				VariantInit(&varName);
				hr = pPropBag->Read(L"CLSID", &varName, 0);
				if (SUCCEEDED(hr))
				{
					BSTR bstr = varName.bstrVal;
					hr = CLSIDFromString(bstr, &classID);
					hrOK;
					CHAR str[GUID_STRING_MAXLEN];
					size_t num_chars_converted;
					wcstombs_s(&num_chars_converted, str, GUID_STRING_MAXLEN, bstr, _TRUNCATE);
					vcamLog(10, "InitializeCameraGUIDs: CLSID %s", str);
					if ( /*IsAcceptableCameraGUID(classID) 
						&& IsAcceptableCameraName(friendlyName)*/ 
						/*(friendlyName.find("Logitech") != string::npos)
						&&*/
						IsAcceptableDevice(deviceList, friendlyNames, friendlyName, classID)
						) 
					{
						vcamLog(10, "     ... is acceptable");
						HRESULT hr2 = pMoniker->BindToObject(0,0,IID_IBaseFilter, 
							(void**)&m_ppUpstreamFilter[filterCount]);
						if (SUCCEEDED(hr2)) {
							m_cameraFriendlyNames.push_back(friendlyName);
							filterCount++;
							vcamLog(10, "                 ... so we are using it");
						}
						else {
							vcamLog(10, "     ... but BindToObject failed, so we ignore it");
						}

					}
					else {
						vcamLog(10, "     ... is unacceptable");
					}
				}
				VariantClear(&varName);
				pPropBag->Release();
			}
			SafeRelease(&pMoniker);
		} else {
			done=true;
		}
	}


donelabel:
	m_iNumUpstreamFilters = filterCount;
	SafeRelease(&pSrc);
    SafeRelease(&pMoniker);
    SafeRelease(&pClassEnum);

    return hr;
}




STDMETHODIMP MultiCamFilter::JoinFilterGraph(
	__inout_opt IFilterGraph * pGraph,
	__in_opt LPCWSTR pName)
{
	HRESULT hr = S_OK;
	//vcamLog(10, "MultiCamFilter::JoinFilterGraph");
	IGraphBuilder *pGraphBuilder = NULL;

	hr = CTransformFilter::JoinFilterGraph(pGraph, pName);
	if (FAILED(hr)) { goto done;}

	// m_pGraph == NULL means we are leaving the graph and there is nothing more to do here
	if(m_pGraph == NULL) { goto done;}

	hr = m_pGraph->QueryInterface(IID_IGraphBuilder,(void **)&pGraphBuilder);
	if (FAILED(hr)) { goto done;}

	for(int i=0; i<m_iNumUpstreamFilters; i++){
		if (m_ppUpstreamFilter[i] != NULL) {
			WCHAR *filterBaseName = L"MulticamAutoadded%d";
			WCHAR filterName[MAX_PATH];
			StringCbPrintfW(filterName, MAX_PATH - 1, filterBaseName, i);
			hr = pGraph->AddFilter(m_ppUpstreamFilter[i], filterName);
			hrOK;
			if (FAILED(hr)) { goto done;}
		}
	}

	hr = ConnectUpstream();
	if (!SUCCEEDED(hr)) {
		vcamLog(0, "ERROR: MultiCamFilter::JoinFilterGraph: ConnectUpstream() failed, hr = 0x%x", hr);
		goto done;
	}

done:
	SafeRelease(&pGraphBuilder);

	//vcamLog(10, "       MultiCamFilter::JoinFilterGraph returning 0x%x",  hr);

	return hr;
}

HRESULT MultiCamFilter::JoinTemporaryGraph()
{
	HRESULT hr = S_OK;
	VCAM_ASSERT (m_pGraph == NULL);
	IFilterGraph *tempGraph;

	hr = CoCreateInstance (CLSID_FilterGraph, NULL, CLSCTX_INPROC,
                           IID_IGraphBuilder, (void **) &tempGraph);
    hrOK;

	WCHAR* temp_filter_name = L"MultiCam Temporary";
	tempGraph->AddFilter(this, temp_filter_name);

	for(int i=0; i<m_iNumUpstreamFilters; i++){
		if (m_ppUpstreamFilter[i] != NULL) {
			VCAM_ASSERT(m_ppInputs[i]->IsConnected());
			if (!m_ppInputs[i]->IsConnected()) {
				vcamLog(0, "ERROR: MultiCamFilter::JoinTemporaryGraph: m_ppInputs[i]->IsConnected() is false, i = %d", i);
				hr = MULTICAM_FAIL;
				break;
			}			
		}
	}
	return hr;
}

// mostly copied from http://msdn.microsoft.com/en-us/library/dd377520(VS.85).aspx
HRESULT MultiCamFilter::DestroyTemporaryGraph()
{
	HRESULT hr = S_OK;	
	VCAM_ASSERT (m_pGraph != NULL);

	IFilterGraph *pGraph = m_pGraph;
	IEnumFilters *pEnum = NULL;
	hr = pGraph->EnumFilters(&pEnum);
	if (SUCCEEDED(hr))
	{
		IBaseFilter *pFilter = NULL;
		while (S_OK == pEnum->Next(1, &pFilter, NULL))
		{
			// Remove the filter.
			pGraph->RemoveFilter(pFilter);
			// Reset the enumerator.
			pEnum->Reset();
			pFilter->Release();
		}
		pEnum->Release();
	}

	return hr;
}

HRESULT MultiCamFilter::LogCapabilities() 
{
	HRESULT hr = S_OK;
	vcamLog(10, "MultiCamFilter::LogCapabilities:");
	for(int i=0; i<m_iNumUpstreamFilters; i++){
		LogCapabilities(i);
	}
	return hr;
}

HRESULT MultiCamFilter::LogCapabilities(int index) 
{
	HRESULT hr = S_OK;
	IBaseFilter *pFilter = m_ppUpstreamFilter[index];
	IAMStreamConfig *pConfig = NULL;
	IPin *pPin = NULL;

	if (pFilter == NULL) {
		vcamLog(10, "%s: pFilter is null", m_cameraFriendlyNames[index].c_str());
		return hr;
	}

	hr = FindPinByCategory(pFilter, PINDIR_OUTPUT, PIN_CATEGORY_CAPTURE, &pPin);
	if (!SUCCEEDED(hr)) {
		vcamLog(10, "%s: Couldn't find output pin", m_cameraFriendlyNames[index].c_str());
		return hr;
	}

	hr = pPin->QueryInterface(IID_IAMStreamConfig,(void **)&pConfig);
	if (!SUCCEEDED(hr)) {
		vcamLog(10, "%s: Couldn't get IAMStreamConfig interface", m_cameraFriendlyNames[index].c_str());
		return hr;
	}

	int count = 0;
	int size = 0;
	pConfig->GetNumberOfCapabilities(&count, &size);

	//vcamLog(10, "All caps for %s: %d caps (size %d)", m_cameraFriendlyNames[index].c_str(), count, size);
	//for(int i=0; i<count; i++){
	//	LogCapability(pConfig, i, size);
	//}
	return hr;
}

HRESULT MultiCamFilter::LogCapability(IAMStreamConfig *pConfig, int index, int size) 
{
	HRESULT hr = S_OK;
	AM_MEDIA_TYPE* pmt;
	BYTE* pSCC = (BYTE*) CoTaskMemAlloc(size);
	pConfig->GetStreamCaps(index, &pmt, pSCC);

	VCAM_ASSERT(pmt->formattype == FORMAT_VideoInfo || pmt->formattype == FORMAT_VideoInfo2);

	VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *) pmt->pbFormat;
	BITMAPINFOHEADER *pbmi = GetBITMAPINFOHEADER(pmt);
	REFERENCE_TIME AvgTimePerFrame = pvi->AvgTimePerFrame;

	LONG this_width = pbmi->biWidth;
	LONG this_height = pbmi->biHeight;

	vcamLog(10, "        %d:  %ldx%ld, %ld   %s", index, this_height, this_width, AvgTimePerFrame, GuidNames[pmt->subtype]);

	DeleteMediaType(pmt);
	CoTaskMemFree(pSCC);
	return hr;
}

HRESULT MultiCamFilter::SetUpstreamFormats() 
{
	HRESULT hr = S_OK;
	vcamLog(10, "MultiCamFilter::SetUpstreamFormats:");
	m_iNumValidFilters = 0;
	for(int i=0; i<m_iNumUpstreamFilters; i++){
		if (m_ppUpstreamFilter[i] != NULL) {
			hr = SetUpstreamFormat(i);
			if (SUCCEEDED(hr)) {
				m_iNumValidFilters++;
			} else {
				// Couldn't find a suitable format for the filter, so kill it off
				SafeRelease(&(m_ppUpstreamFilter[i]));
			}
		}
	}
	return S_OK;
}


HRESULT MultiCamFilter::ChooseBestHeight(IAMStreamConfig *pConfig, 
	int filterIndex, int capCount,	int capSize,
	BOOL requireRGB24,
	/*out*/ int *bestIndex, /*out*/ LONG *bestHeight)
{
	HRESULT hr = S_OK;

	*bestIndex = -1;
	*bestHeight = -1;
	LONG width = -1;
	LONG height = -1;
	LONG best_height_under_threshold = -1;
	int best_index_under_threshold = -1;
	LONG smallest_height = LONG_MAX;
	int index_of_smallest = -1;

	vcamLog(10, "MultiCamFilter::ChooseBestHeight: %s: %d caps (size %d)", 
		m_cameraFriendlyNames[filterIndex].c_str(), capCount, capSize);
	for(int i=0; i<capCount; i++){
		hr = GetCapabilityImageSize(pConfig, i, capSize, requireRGB24, /*out*/ &width, /*out*/ &height);
		hrOK;
		vcamLog(90, "MultiCamFilter::ChooseBestHeight: i=%d, RGB24=%d, width=%ld, height=%ld", 
			i, requireRGB24, width, height);
		if (height > best_height_under_threshold && height <= s_IDEAL_CAMERA_HEIGHT)
		{
			best_height_under_threshold = height;
			best_index_under_threshold = i;
		}
		if (height < smallest_height)
		{
			smallest_height = height;
			index_of_smallest = i;
		}
	}
	if (best_index_under_threshold != -1) {
		*bestIndex = best_index_under_threshold;
		*bestHeight = best_height_under_threshold;
	}
	else if (index_of_smallest != -1 && smallest_height <= s_MAX_CAMERA_HEIGHT) {
		*bestIndex = index_of_smallest;
		*bestHeight = smallest_height;
	}
	else {
		// do nothing -- bestIndex and bestHeight remain at -1,
		// and the calling function should decide what to do about this
	}
	return hr;		
}

HRESULT MultiCamFilter::SetUpstreamFormat(int index) 
{
	HRESULT hr = S_OK;
	IBaseFilter *pFilter = m_ppUpstreamFilter[index];
	IAMStreamConfig *pConfig = NULL;
	IPin *pPin = NULL;

	vcamLog(10, "MultiCamFilter::SetUpstreamFormat: index = %d, name = %s", 
		index, m_cameraFriendlyNames[index].c_str());

	if (pFilter == NULL) {
		vcamLog(10, "MultiCamFilter::SetUpstreamFormat: pFilter == NULL");
		return MULTICAM_FAIL;
	}

	hr = FindPinByCategory(pFilter, PINDIR_OUTPUT, PIN_CATEGORY_CAPTURE, &pPin);
	if (!SUCCEEDED(hr)) {
		vcamLog(10, "MultiCamFilter::SetUpstreamFormat: Couldn't find output pin");
		return hr;
	}
	
	hr = pPin->QueryInterface(IID_IAMStreamConfig,(void **)&pConfig); 
	if (!SUCCEEDED(hr)) {
		vcamLog(10, "MultiCamFilter::SetUpstreamFormat: Couldn't get IAMStreamConfig interface");
		return hr;
	}

	int count = 0;
	int size = 0;
	hr = pConfig->GetNumberOfCapabilities(&count, &size); hrOK;

	// look first for a suitable height with the RGB24 media type
	LONG best_height = -1;
	int best_index = -1;
	hr = ChooseBestHeight(pConfig, index, count, size, TRUE,
	/*out*/ &best_index, /*out*/ &best_height); 
	hrOK;
	
	// If we found nothing suitable, settle for any other media type
	if (best_height == -1)
	{
		vcamLog(10, "MultiCamFilter::SetUpstreamFormat: No RGB24 media type found, will try alternative media type");
		hr = ChooseBestHeight(pConfig, index, count, size, FALSE,
			/*out*/ &best_index, /*out*/ &best_height); 
		hrOK;
	}

	if (best_height == -1)
	{
		vcamLog(0, "MultiCamFilter::SetUpstreamFormat: ERROR: best_height == -1");
		return MULTICAM_FAIL;
	}

	AM_MEDIA_TYPE* pmt;
	hr = GetCapabilityFormat(pConfig, best_index, size, &pmt);			hrOK;
	hr = pConfig->SetFormat(pmt);										hrOK;

	vcamLog(10, "MultiCamFilter::SetUpstreamFormat: chose to use capability %d, with height %ld; details:", best_index, best_height);
	LogCapability(pConfig, best_index, size);

	DeleteMediaType(pmt);

	return hr;
}

HRESULT MultiCamFilter::GetCapabilityImageSize(IAMStreamConfig *pConfig, 
	int index, int size, 
	BOOL requireRGB24,
	/*out*/ LONG *width, /*out*/ LONG *height) 
{
	HRESULT hr = S_OK;
	AM_MEDIA_TYPE* pmt;
	BYTE* pSCC = (BYTE*) CoTaskMemAlloc(size);
	hr = pConfig->GetStreamCaps(index, &pmt, pSCC); hrOK;

	VCAM_ASSERT(pmt->formattype == FORMAT_VideoInfo || pmt->formattype == FORMAT_VideoInfo2);

	BITMAPINFOHEADER *pbmi = GetBITMAPINFOHEADER(pmt);

	LONG this_width = pbmi->biWidth;
	LONG this_height = pbmi->biHeight;
	
	if (!requireRGB24 || IsEqualGUID(pmt->subtype, MEDIASUBTYPE_RGB24))
	//if (!requireRGB24 || IsEqualGUID(pmt->subtype, MEDIASUBTYPE_RGB565))
	{
		*width = this_width;
		*height = this_height;
	}
	else
	{
		*width = -1;
		*height = -1;	
	}

#ifdef DEBUG
	char guidString[GUID_STRING_MAXLEN];
	Riid2String(pmt->subtype, guidString);
	vcamLog(90, "MultiCamFilter::GetCapabilityImageSize: index %d:  %ldx%ld  %s(%s)", 
		index, this_height, this_width, GuidNames[pmt->subtype], guidString);
#endif

	DeleteMediaType(pmt);
	CoTaskMemFree(pSCC);
	return hr;
}

HRESULT MultiCamFilter::GetCapabilityFormat(IAMStreamConfig *pConfig, int index, int size, 
	/*out*/ AM_MEDIA_TYPE** ppmt) 
{
	HRESULT hr = S_OK;
	BYTE* pSCC = (BYTE*) CoTaskMemAlloc(size);
	pConfig->GetStreamCaps(index, ppmt, pSCC);
	CoTaskMemFree(pSCC);
	return hr;
}

HRESULT MultiCamFilter::ConnectUpstream() 
{
	vcamLog(10, "MultiCamFilter::ConnectUpstream");
	VCAM_ASSERT (m_pGraph!=NULL);
	HRESULT hr = S_OK;
	IGraphBuilder *pGraphBuilder = NULL;

	if(m_pInput == NULL)
	{
		hr = InitializeInputPins();
		if (FAILED(hr)) { goto done;}
	}
	VCAM_ASSERT (m_pInput!=NULL);

	if(m_pOutput == NULL)
	{
		hr = InitializeOutputPin();
		if (FAILED(hr)) { goto done;}
	}
	VCAM_ASSERT (m_pOutput!=NULL);

	if(!m_pInput->IsConnected())
	{
		vcamLog(10, "    MultiCamFilter::ConnectUpstream: m_pInput is not connected; will attempt to connect upstream filters");
		for(int i=0; i<m_iNumUpstreamFilters; i++){
			if (m_ppUpstreamFilter[i] != NULL) {
				VCAM_ASSERT(!m_ppInputs[i]->IsConnected());
			}
		}

		hr = m_pGraph->QueryInterface(IID_IGraphBuilder,(void **)&pGraphBuilder);
		if (FAILED(hr)) { goto done;}

		for(int i=0; i<m_iNumUpstreamFilters; i++){
			if (m_ppUpstreamFilter[i] != NULL) {
				hr = ConnectFilters(pGraphBuilder, m_ppUpstreamFilter[i], m_ppInputs[i]);
				if (FAILED(hr)) {
					vcamLog(0, "ERROR: MultiCamFilter::ConnectUpstream: ConnectFilters() failed, hr = 0x%x", hr);
					goto done;
				}		
				vcamLog(10, "    MultiCamFilter::ConnectUpstream: successfully connected upstream filter %d", i);
			}
		}
	} else {
		vcamLog(10, "    MultiCamFilter::ConnectUpstream: m_pInput is already connected");
	}

done:
	SafeRelease(&pGraphBuilder);

	vcamLog(10, "       MultiCamFilter::ConnectUpstream returning 0x%x",  hr);
	return hr;
}

HRESULT MultiCamFilter::DisconnectUpstream() 
{
	vcamLog(10, "MultiCamFilter::DisconnectUpstream");
	VCAM_ASSERT (m_pGraph!=NULL);
	HRESULT hr = S_OK;

	vcamLog(10, "       MultiCamFilter::ConnectUpstream returning 0x%x",  hr);
	return hr;
}

STDMETHODIMP MultiCamFilter::QueryInterface(REFIID riid, __deref_out void **ppv) 
{      
	char guidString[GUID_STRING_MAXLEN];
	Riid2String(riid, guidString);
	vcamLog(10, "MultiCamFilter::QueryInterface, riid = %s", guidString);

	HRESULT hr = S_OK;

	//Forward request for IAMStreamConfig & IKsPropertySet to the pin
	if(riid == _uuidof(IAMStreamConfig) || riid == _uuidof(IKsPropertySet))
	{
		vcamLog(50, "MultiCamFilter::QueryInterface forwarding to pin");
		hr = m_pOutput->QueryInterface(riid, ppv);
	} else {
		// or should this be CTransformFilter::QueryInterface() ?? -- That would be more closely analogous to
		// the vcam code
		hr = GetOwner()->QueryInterface(riid,ppv);
	}

	if (hr==S_OK) {
		vcamLog(10, "       MultiCamFilter::QueryInterface returning S_OK");
	}
	else if (hr==E_NOINTERFACE ) {
		vcamLog(10, "       MultiCamFilter::QueryInterface returning E_NOINTERFACE ");
	}
	else if (hr==E_POINTER) {
		vcamLog(10, "       MultiCamFilter::QueryInterface returning E_POINTER");
	} else {
		vcamLog(10, "       MultiCamFilter::QueryInterface returning 0x%x", hr);
	}

	return hr;            
};



HRESULT MultiCamFilter::InitializeInputPins()
{
	vcamLog(50, "MultiCamFilter::InitializeInputPins");
	HRESULT hr = S_OK;
	VCAM_ASSERT(m_ppInputs==NULL);
	m_ppInputs = new COverlayInputPin*[MULTICAM_NUM_INPUT_PINS];
	VCAM_ASSERT(m_ppInputs!=NULL);
	for(int i=0; i<MULTICAM_NUM_INPUT_PINS; i++){
		WCHAR *pinBaseName = L"MultiCam input pin %d";
		WCHAR pinName[MAX_PATH];
		StringCbPrintfW(pinName, MAX_PATH - 1, pinBaseName, i);
		m_ppInputs[i] = new COverlayInputPin(NAME("MultiCam input pin"),
			this,              // Owner filter
			&hr,               // Result code
			pinName,			// Pin name
			i);      
		hrOK;
	}
	m_pInput = m_ppInputs[0];
	return hr;
}

HRESULT MultiCamFilter::InitializeOutputPin()
{
	vcamLog(50, "MultiCamFilter::InitializeOutputPin");
	HRESULT hr = S_OK;
	VCAM_ASSERT(m_pOutput==NULL);
	m_pOutput = new MultiCamOutputPin( NAME("MultiCam output pin")
		, this       // Owner filter
		, &hr        // Result code
		, L"MultiCamOutput"  // Pin name
		);
	hrOK;
	
	// Set a provisional media type in the output pin
	// TODO
	///////////////

	return hr;
}

CBasePin* MultiCamFilter::GetPin(int n)
{
    HRESULT hr = S_OK;
	CBasePin* retVal = NULL;
	vcamLog(50, "MultiCamFilter::GetPin, n = %d", n);

    // Create the input pins if necessary
    if (m_pInput == NULL) {
        hr = InitializeInputPins();
        hrOKnoRet;
        if (m_pInput == NULL) {
            retVal = NULL;
			goto done;
        }
		hrOKnoRet;
    }

	// Create the output pin if necessary
    if (m_pOutput == NULL) {
		hr = InitializeOutputPin();        
		hrOKnoRet;
	}

    // Return the appropriate pin
	if (n >= 0 && n < MULTICAM_NUM_INPUT_PINS) {
        retVal =  m_ppInputs[n];
    } else if (n == MULTICAM_NUM_INPUT_PINS) {
        retVal =   m_pOutput;
    } else {
        retVal =   NULL;
    }
done:
	if (retVal == NULL) {
		vcamLog(50, "  MultiCamFilter::GetPin: returning NULL");
	} else if(retVal->IsConnected()) {
		vcamLog(50, "  MultiCamFilter::GetPin: returning; pin is connected");
	} else {
		vcamLog(50, "  MultiCamFilter::GetPin: returning; pin is not connected");
	}

	return retVal;
}

HRESULT MultiCamFilter::Overlay(int inID, IMediaSample *pOut, /*inout*/ int &xStart)
{
	HRESULT hr = S_OK;
	const int out_scale = 6;
    BYTE *pDataIn, *pDataOut;                // Pointer to the actual image buffer
    long lDataLenIn, lDataLenOut;              // Holds length of any given sample

    AM_MEDIA_TYPE* pTypeIn = &m_ppInputs[inID]->CurrentMediaType();
    VIDEOINFOHEADER *pviIn = (VIDEOINFOHEADER *) pTypeIn->pbFormat;
    VCAM_ASSERT(pviIn);
	
	IMediaSample *pIn = m_ppOverlays[inID];

	// if pIn==NULL, camera inID hasn't started up yet, or perhaps isn't working properly.
	// In any case, we'll just ignore this camera
	if (pIn == NULL) 
	{
		return hr;
	}

    CheckPointer(pIn,E_POINTER);
    pIn->GetPointer(&pDataIn);
    lDataLenIn = pIn->GetSize();

    int cxImageIn    = pviIn->bmiHeader.biWidth;
    int cyImageIn    = pviIn->bmiHeader.biHeight;
	int bitCountIn   = pviIn->bmiHeader.biBitCount;
	// see "Calculating Surface Stride" in http://msdn.microsoft.com/en-us/library/dd318229(v=VS.85).aspx
	int strideIn     = ((((cxImageIn * bitCountIn) + 31) & ~31) >> 3);
	// check that it is a bottom-up image
	VCAM_ASSERT(pviIn->bmiHeader.biHeight > 0);
	VCAM_BAIL(pviIn->bmiHeader.biHeight > 0);

	VCAM_ASSERT(lDataLenIn == pviIn->bmiHeader.biSizeImage);

	AM_MEDIA_TYPE* pTypeOut = &m_pOutput->CurrentMediaType();
    VIDEOINFOHEADER *pviOut = (VIDEOINFOHEADER *) pTypeOut->pbFormat;
    VCAM_ASSERT(pviOut);

    CheckPointer(pOut,E_POINTER);
    pOut->GetPointer(&pDataOut);
    lDataLenOut = pOut->GetSize();

    int cxImageOut    = pviOut->bmiHeader.biWidth;
    int cyImageOut    = pviOut->bmiHeader.biHeight;
	int bitCountOut   = pviOut->bmiHeader.biBitCount;
	// see "Calculating Surface Stride" in http://msdn.microsoft.com/en-us/library/dd318229(v=VS.85).aspx
	int strideOut     = ((((cxImageOut * bitCountOut) + 31) & ~31) >> 3);
	// check that it is a bottom-up image
	VCAM_ASSERT(pviOut->bmiHeader.biHeight > 0);

	VCAM_ASSERT(lDataLenOut == pviOut->bmiHeader.biSizeImage);

	const int in_scale = out_scale * cxImageIn / cxImageOut;

	// Check that overlay is not too big for destination
	VCAM_ASSERT(cxImageIn <= (cxImageOut - xStart) * in_scale);
	VCAM_ASSERT(cyImageIn <= cyImageOut * in_scale);

	int xIn, yIn, xOut, yOut;
	RGBTRIPLE *prgbIn, *prgbOut;
	prgbIn = (RGBTRIPLE*) pDataIn;
	prgbOut = (RGBTRIPLE*) pDataOut;
	BYTE *pRowIn, *pRowOut;
	pRowIn = pDataIn;
	pRowOut = pDataOut;
	yIn = 0; yOut = 0;
	for (; yIn < cyImageIn; yIn+=in_scale, yOut++, pRowIn+=(strideIn*in_scale), pRowOut+=strideOut)
	{
		prgbIn = (RGBTRIPLE*) pRowIn;
		prgbOut = ((RGBTRIPLE*) pRowOut) + xStart;
		xIn = 0; xOut = xStart;
		for (; xIn < cxImageIn; xIn+=in_scale, xOut++, prgbIn+=in_scale, prgbOut++)
		{
			*prgbOut = *prgbIn;
		}
	}

	xStart = xOut;
	return hr;
}


//HRESULT MultiCamFilter::Overlay(int inID, int out_scale, IMediaSample *pOut, 
//	/*inout*/ int &xStart, /*inout*/ int &yStart)
//{
//	HRESULT hr = S_OK;
//    BYTE *pDataIn, *pDataOut;                // Pointer to the actual image buffer
//    long lDataLenIn, lDataLenOut;              // Holds length of any given sample
//
//    AM_MEDIA_TYPE* pTypeIn = &m_ppInputs[inID]->CurrentMediaType();
//    VIDEOINFOHEADER *pviIn = (VIDEOINFOHEADER *) pTypeIn->pbFormat;
//    VCAM_ASSERT(pviIn);
//
//	IMediaSample *pIn = m_ppOverlays[inID];
//
//	// if pIn==NULL, camera inID hasn't started up yet, or perhaps isn't working properly.
//	// In any case, we'll just ignore this camera
//	if (pIn == NULL) 
//	{
//		return hr;
//	}
//
//    CheckPointer(pIn,E_POINTER);
//    pIn->GetPointer(&pDataIn);
//    lDataLenIn = pIn->GetSize();
//
//    int cxImageIn    = pviIn->bmiHeader.biWidth;
//    int cyImageIn    = pviIn->bmiHeader.biHeight;
//	int bitCountIn   = pviIn->bmiHeader.biBitCount;
//	// see "Calculating Surface Stride" in http://msdn.microsoft.com/en-us/library/dd318229(v=VS.85).aspx
//	int strideIn     = ((((cxImageIn * bitCountIn) + 31) & ~31) >> 3);
//	// check that it is a bottom-up image
//	VCAM_ASSERT(pviIn->bmiHeader.biHeight > 0);
//
//	VCAM_ASSERT(lDataLenIn == pviIn->bmiHeader.biSizeImage);
//
//	AM_MEDIA_TYPE* pTypeOut = &m_pOutput->CurrentMediaType();
//    VIDEOINFOHEADER *pviOut = (VIDEOINFOHEADER *) pTypeOut->pbFormat;
//    VCAM_ASSERT(pviOut);
//
//    CheckPointer(pOut,E_POINTER);
//    pOut->GetPointer(&pDataOut);
//    lDataLenOut = pOut->GetSize();
//
//    int cxImageOut    = pviOut->bmiHeader.biWidth;
//    int cyImageOut    = pviOut->bmiHeader.biHeight;
//	int bitCountOut   = pviOut->bmiHeader.biBitCount;
//	// see "Calculating Surface Stride" in http://msdn.microsoft.com/en-us/library/dd318229(v=VS.85).aspx
//	int strideOut     = ((((cxImageOut * bitCountOut) + 31) & ~31) >> 3);
//	// check that it is a bottom-up image
//	if (!(pviOut->bmiHeader.biHeight > 0))
//	{
//		vcamLog(0, "MultiCamFilter::Overlay: ERROR: !(pviOut->bmiHeader.biHeight > 0) [val=%ld]", 
//			pviOut->bmiHeader.biHeight);
//		return MULTICAM_FAIL;
//	}
//
//	if (!(lDataLenOut == pviOut->bmiHeader.biSizeImage))
//	{
//		vcamLog(0, "MultiCamFilter::Overlay: ERROR: !(lDataLenOut == pviOut->bmiHeader.biSizeImage) [%d,%d]", 
//			lDataLenOut, pviOut->bmiHeader.biSizeImage);
//		return MULTICAM_FAIL;
//	}
//
//	const int in_scale = out_scale * cxImageIn / cxImageOut;
//
//
//	vcamLog(90, "MultiCamFilter::Overlay: (cxImageIn, cyImageIn) = (%d, %d), (cxImageOut, cyImageOut) = (%d, %d), xStart = %d, in_scale = %d",
//		cxImageIn, cyImageIn, cxImageOut, cyImageOut, xStart, in_scale);
//
//	if (!(cxImageIn <= (cxImageOut - xStart) * in_scale))
//	{
//			vcamLog(0, "MultiCamFilter::Overlay: ERROR: Overlay too big");
//	}
//
//	// Check that overlay is not too big for destination
//	VCAM_ASSERT(cxImageIn <= (cxImageOut - xStart) * in_scale);
//	VCAM_ASSERT(cyImageIn <= (cyImageOut - yStart) * in_scale);
//
//	int xIn, yIn, xOut, yOut;
//	RGBTRIPLE *prgbIn, *prgbOut;
//	prgbIn = (RGBTRIPLE*) pDataIn;
//	prgbOut = (RGBTRIPLE*) pDataOut;
//	BYTE *pRowIn, *pRowOut;
//	pRowIn = pDataIn;
//	pRowOut = pDataOut + (strideOut * yStart);
//	yIn = 0; yOut = yStart;
//	for (; yIn < cyImageIn; yIn+=in_scale, yOut++, pRowIn+=(strideIn*in_scale), pRowOut+=strideOut)
//	{
//		prgbIn = (RGBTRIPLE*) pRowIn;
//		prgbOut = ((RGBTRIPLE*) pRowOut) + xStart;
//		xIn = 0; xOut = xStart;
//		for (; xIn < cxImageIn; xIn+=in_scale, xOut++, prgbIn+=in_scale, prgbOut++)
//		{
//			*prgbOut = *prgbIn;
//		}
//	}
//
//	xStart = xOut;
//	yStart = yOut;
//	return hr;
//}

//HRESULT MultiCamFilter::Overlay(int inID, int out_scale, IMediaSample *pOut, 
//	/*inout*/ int &xStart, /*inout*/ int &yStart)
//{
//	HRESULT hr = S_OK;
//    BYTE *pDataIn, *pDataOut;                // Pointer to the actual image buffer
//    long lDataLenIn, lDataLenOut;              // Holds length of any given sample
//
//    AM_MEDIA_TYPE* pTypeIn = &m_ppInputs[inID]->CurrentMediaType();
//    VIDEOINFOHEADER *pviIn = (VIDEOINFOHEADER *) pTypeIn->pbFormat;
//    VCAM_ASSERT(pviIn);
//
//	IMediaSample *pIn = m_ppOverlays[inID];
//
//	// if pIn==NULL, camera inID hasn't started up yet, or perhaps isn't working properly.
//	// In any case, we'll just ignore this camera
//	if (pIn == NULL) 
//	{
//		return hr;
//	}
//
//    CheckPointer(pIn,E_POINTER);
//    pIn->GetPointer(&pDataIn);
//    lDataLenIn = pIn->GetSize();
//
//    int cxImageIn    = pviIn->bmiHeader.biWidth;
//    int cyImageIn    = pviIn->bmiHeader.biHeight;
//	int bitCountIn   = pviIn->bmiHeader.biBitCount;
//	// see "Calculating Surface Stride" in http://msdn.microsoft.com/en-us/library/dd318229(v=VS.85).aspx
//	int strideIn     = ((((cxImageIn * bitCountIn) + 31) & ~31) >> 3);
//	// check that it is a bottom-up image
//	VCAM_ASSERT(pviIn->bmiHeader.biHeight > 0);
//
//	VCAM_ASSERT(lDataLenIn == pviIn->bmiHeader.biSizeImage);
//
//	AM_MEDIA_TYPE* pTypeOut = &m_pOutput->CurrentMediaType();
//    VIDEOINFOHEADER *pviOut = (VIDEOINFOHEADER *) pTypeOut->pbFormat;
//    VCAM_ASSERT(pviOut);
//
//    CheckPointer(pOut,E_POINTER);
//    pOut->GetPointer(&pDataOut);
//    lDataLenOut = pOut->GetSize();
//
//    int cxImageOut    = pviOut->bmiHeader.biWidth;
//    int cyImageOut    = pviOut->bmiHeader.biHeight;
//	int bitCountOut   = pviOut->bmiHeader.biBitCount;
//	// see "Calculating Surface Stride" in http://msdn.microsoft.com/en-us/library/dd318229(v=VS.85).aspx
//	int strideOut     = ((((cxImageOut * bitCountOut) + 31) & ~31) >> 3);
//	// check that it is a bottom-up image
//	if (!(pviOut->bmiHeader.biHeight > 0))
//	{
//		vcamLog(0, "MultiCamFilter::Overlay: ERROR: !(pviOut->bmiHeader.biHeight > 0) [val=%ld]", 
//			pviOut->bmiHeader.biHeight);
//		return MULTICAM_FAIL;
//	}
//
//	if (!(lDataLenOut == pviOut->bmiHeader.biSizeImage))
//	{
//		vcamLog(0, "MultiCamFilter::Overlay: ERROR: !(lDataLenOut == pviOut->bmiHeader.biSizeImage) [%d,%d]", 
//			lDataLenOut, pviOut->bmiHeader.biSizeImage);
//		return MULTICAM_FAIL;
//	}
//
//	const double x_zoom = (double) cxImageIn / (cxImageOut - xStart);
//	const double y_zoom = (double) cyImageIn / (cyImageOut - yStart);
//	const double in_scale = (double) out_scale * min(x_zoom, y_zoom);
//
//
//	vcamLog(90, "MultiCamFilter::Overlay: (cxImageIn, cyImageIn) = (%d, %d), (cxImageOut, cyImageOut) = (%d, %d), xStart = %d, in_scale = %g, out_scale = %d",
//		cxImageIn, cyImageIn, cxImageOut, cyImageOut, xStart, in_scale, out_scale);
//
//	if (!(cxImageIn <= (cxImageOut - xStart) * in_scale))
//	{
//			vcamLog(0, "MultiCamFilter::Overlay: ERROR: Overlay too big");
//	}
//
//	// Check that overlay is not too big for destination
//	VCAM_ASSERT(cxImageIn <= (cxImageOut - xStart) * in_scale);
//	VCAM_ASSERT(cyImageIn <= (cyImageOut - yStart) * in_scale);
//
//	int yCount, xCount, yCountMax, xCountMax;
//	RGBTRIPLE *prgbIn, *prgbOut;
//	prgbIn = (RGBTRIPLE*) pDataIn;
//	prgbOut = (RGBTRIPLE*) pDataOut;
//	BYTE *pRowIn, *pRowOut;
//	pRowIn = pDataIn;
//	pRowOut = pDataOut + (strideOut * yStart);
//	
//	yCountMax = (int) ((double) cyImageIn / in_scale);
//	xCountMax = (int) ((double) cxImageIn / in_scale);
//	for (yCount = 0; yCount < yCountMax; pRowOut+=strideOut, yCount++)
//	{
//		pRowIn = pDataIn + strideIn * ((int) ((double) yCount * in_scale));
//		prgbIn = (RGBTRIPLE*) pRowIn;
//		prgbOut = ((RGBTRIPLE*) pRowOut) + xStart;
//		
//		for (xCount = 0; xCount < xCountMax; prgbOut++, xCount++)
//		{
//			prgbIn =  ((RGBTRIPLE*) pRowIn) + ((int) ((double) xCount * in_scale));
//			*prgbOut = *prgbIn;
//		}
//	}
//
//	//xStart += (int) ((double) xCountMax * in_scale);
//	//yStart += (int) ((double) yCountMax * in_scale);
//	xStart += xCountMax;
//	yStart += yCountMax;
//	vcamLog(0, "Overlay: returning, xStart = %d, yStart = %d", xStart, yStart);
//	return hr;
//}


HRESULT MultiCamFilter::OverlayB(int inID, IMediaSample *pOut, 
	int xStart, int yStart, int xEnd, int yEnd)
{
	HRESULT hr = S_OK;
    BYTE *pDataIn, *pDataOut;                // Pointer to the actual image buffer
    long lDataLenIn, lDataLenOut;              // Holds length of any given sample

    AM_MEDIA_TYPE* pTypeIn = &m_ppInputs[inID]->CurrentMediaType();
    VIDEOINFOHEADER *pviIn = (VIDEOINFOHEADER *) pTypeIn->pbFormat;
    VCAM_ASSERT(pviIn);

	IMediaSample *pIn = m_ppOverlays[inID];

	// if pIn==NULL, camera inID hasn't started up yet, or perhaps isn't working properly.
	// In any case, we'll just ignore this camera
	if (pIn == NULL) 
	{
		return hr;
	}

    CheckPointer(pIn,E_POINTER);
    pIn->GetPointer(&pDataIn);
    lDataLenIn = pIn->GetSize();

    int cxImageIn    = pviIn->bmiHeader.biWidth;
    int cyImageIn    = pviIn->bmiHeader.biHeight;
	int bitCountIn   = pviIn->bmiHeader.biBitCount;
	// see "Calculating Surface Stride" in http://msdn.microsoft.com/en-us/library/dd318229(v=VS.85).aspx
	int strideIn     = ((((cxImageIn * bitCountIn) + 31) & ~31) >> 3);
	// check that it is a bottom-up image
	VCAM_ASSERT(pviIn->bmiHeader.biHeight > 0);

	VCAM_ASSERT(lDataLenIn == pviIn->bmiHeader.biSizeImage);

	AM_MEDIA_TYPE* pTypeOut = &m_pOutput->CurrentMediaType();
    VIDEOINFOHEADER *pviOut = (VIDEOINFOHEADER *) pTypeOut->pbFormat;
    VCAM_ASSERT(pviOut);

    CheckPointer(pOut,E_POINTER);
    pOut->GetPointer(&pDataOut);
    lDataLenOut = pOut->GetSize();

    int cxImageOut    = pviOut->bmiHeader.biWidth;
    int cyImageOut    = pviOut->bmiHeader.biHeight;
	int bitCountOut   = pviOut->bmiHeader.biBitCount;
	// see "Calculating Surface Stride" in http://msdn.microsoft.com/en-us/library/dd318229(v=VS.85).aspx
	int strideOut     = ((((cxImageOut * bitCountOut) + 31) & ~31) >> 3);
	// check that it is a bottom-up image
	if (!(pviOut->bmiHeader.biHeight > 0))
	{
		vcamLog(0, "MultiCamFilter::Overlay: ERROR: !(pviOut->bmiHeader.biHeight > 0) [val=%ld]", 
			pviOut->bmiHeader.biHeight);
		return MULTICAM_FAIL;
	}

	if (!(lDataLenOut == pviOut->bmiHeader.biSizeImage))
	{
		vcamLog(0, "MultiCamFilter::Overlay: ERROR: !(lDataLenOut == pviOut->bmiHeader.biSizeImage) [%d,%d]", 
			lDataLenOut, pviOut->bmiHeader.biSizeImage);
		return MULTICAM_FAIL;
	}

	const int overlayWidth = xEnd - xStart;
	const int overlayHeight = yEnd - yStart;
	const double x_zoom = (double) cxImageIn / overlayWidth;
	const double y_zoom = (double) cyImageIn / overlayHeight;
	double in_scale = 0.0;
	if (x_zoom >= y_zoom) {
		in_scale = x_zoom;
		// center image in y direction
		int zoomedHeight = (int) ((double) cyImageIn / x_zoom);
		int yGap = overlayHeight - zoomedHeight;
		VCAM_ASSERT(yGap >= 0);
		yStart += yGap / 2;
	} else {
		in_scale = y_zoom;
		// center image in x direction
		int zoomedWidth = (int) ((double) cxImageIn / y_zoom);
		int xGap = overlayWidth - zoomedWidth;
		VCAM_ASSERT(xGap >= 0);
		xStart += xGap / 2;
	}

	vcamLog(90, "MultiCamFilter::Overlay: (cxImageIn, cyImageIn) = (%d, %d), (cxImageOut, cyImageOut) = (%d, %d), xStart = %d, in_scale = %g",
		cxImageIn, cyImageIn, cxImageOut, cyImageOut, xStart, in_scale);

	if (!(cxImageIn <= (cxImageOut - xStart) * in_scale))
	{
			vcamLog(0, "MultiCamFilter::Overlay: ERROR: Overlay too big");
	}

	// Check that overlay is not too big for destination
	VCAM_ASSERT(cxImageIn <= (cxImageOut - xStart) * in_scale);
	VCAM_ASSERT(cyImageIn <= (cyImageOut - yStart) * in_scale);

	int yCount, xCount, yCountMax, xCountMax;
	RGBTRIPLE *prgbIn, *prgbOut;
	prgbIn = (RGBTRIPLE*) pDataIn;
	prgbOut = (RGBTRIPLE*) pDataOut;
	BYTE *pRowIn, *pRowOut;
	pRowIn = pDataIn;
	pRowOut = pDataOut + (strideOut * yStart);
	
	yCountMax = (int) ((double) cyImageIn / in_scale);
	xCountMax = (int) ((double) cxImageIn / in_scale);
	for (yCount = 0; yCount < yCountMax; pRowOut+=strideOut, yCount++)
	{
		pRowIn = pDataIn + strideIn * ((int) ((double) yCount * in_scale));
		prgbIn = (RGBTRIPLE*) pRowIn;
		prgbOut = ((RGBTRIPLE*) pRowOut) + xStart;
		
		for (xCount = 0; xCount < xCountMax; prgbOut++, xCount++)
		{
			prgbIn =  ((RGBTRIPLE*) pRowIn) + ((int) ((double) xCount * in_scale));
			*prgbOut = *prgbIn;
		}
	}

	//xStart += (int) ((double) xCountMax * in_scale);
	//yStart += (int) ((double) yCountMax * in_scale);
	//xStart += xCountMax;
	//yStart += yCountMax;
	//vcamLog(0, "Overlay: returning, xStart = %d, yStart = %d", xStart, yStart);
	return hr;
}


// return the number of pins we provide
int MultiCamFilter::GetPinCount()
{
	vcamLog(95, "MultiCamFilter::GetPinCount");
	return MULTICAM_NUM_PINS;
}

HRESULT MultiCamFilter::Receive(IMediaSample *pSample, int ID)
{
    ////////////////// Debugging //////////////////////
	VCAM_ASSERT(pSample->GetActualDataLength() == GetInputPin(ID)->CurrentMediaType().lSampleSize);
	////////////////// end Debugging //////////////////////


	HRESULT hr = S_OK;

	if(ID == m_iMainInputID) {
		/*  Check for other streams and pass them on */
		AM_SAMPLE2_PROPERTIES * const pProps = m_pInput->SampleProps();
		if (pProps->dwStreamId != AM_STREAM_MEDIA) {
			return GetOutputPin()->GetIMemInput()->Receive(pSample);
		}
		VCAM_ASSERT(pSample);
		IMediaSample * pOutSample;

		// If no output to deliver to then no point sending us data

		VCAM_ASSERT (m_pOutput != NULL) ;

		// Set up the output sample
		hr = InitializeOutputSample(pSample, &pOutSample);

		if (FAILED(hr)) {
			return hr;
		}

		////////////////// Debugging //////////////////////
		//long ds = pOutSample->GetSize();
		//long da = pOutSample->GetActualDataLength();
		//long ss = pSample->GetSize();
		//long sa = pSample->GetActualDataLength();
		VCAM_ASSERT(pOutSample->GetSize() >= pOutSample->GetActualDataLength());
		VCAM_ASSERT(pSample->GetSize() >= pSample->GetActualDataLength());
		VCAM_ASSERT(pSample->GetActualDataLength() <= pOutSample->GetActualDataLength());
		VCAM_ASSERT(pOutSample->GetActualDataLength() == GetOutputPin()->CurrentMediaType().GetSampleSize());
		////////////////// end Debugging //////////////////////


		// Start timing the transform (if PERF is defined)
		MSR_START(m_idTransform);

		// have the derived class transform the data
		hr = Transform(pSample, pOutSample);

		// Stop the clock and log it (if PERF is defined)
		MSR_STOP(m_idTransform);

		if (FAILED(hr)) {
			DbgLog((LOG_TRACE,1,TEXT("Error from transform")));
		} else {
			// the Transform() function can return S_FALSE to indicate that the
			// sample should not be delivered; we only deliver the sample if it's
			// really S_OK (same as NOERROR, of course.)
			if (hr == NOERROR) {
				hr = GetOutputPin()->GetIMemInput()->Receive(pOutSample);
				m_bSampleSkipped = FALSE;	// last thing no longer dropped
			} else {
				// S_FALSE returned from Transform is a PRIVATE agreement
				// We should return NOERROR from Receive() in this cause because returning S_FALSE
				// from Receive() means that this is the end of the stream and no more data should
				// be sent.
				if (S_FALSE == hr) {

					//  Release the sample before calling notify to avoid
					//  deadlocks if the sample holds a lock on the system
					//  such as DirectDraw buffers do
					pOutSample->Release();
					m_bSampleSkipped = TRUE;
					if (!m_bQualityChanged) {
						NotifyEvent(EC_QUALITY_CHANGE,0,0);
						m_bQualityChanged = TRUE;
					}
					return NOERROR;
				}
			}
		}

		// release the output buffer. If the connected pin still needs it,
		// it will have addrefed it itself.
		pOutSample->Release();
	} 

	hr = StoreSampleForOverlay(pSample, ID);

	return hr;
}


HRESULT MultiCamFilter::StoreSampleForOverlay(IMediaSample *pSample, int ID)
{
	HRESULT hr = S_OK;

	// We play deliberately fast and loose here. Choosing to AddRef() the IMediaSample here
	// has bad consequences with some filters, since they get stuck waiting for
	// a buffer to become available. Therefore, we store the pointer without AddRefing,
	// and hope for the best. Experiments did not reveal any problems with this approach, but
	// it's possible the media sample could go away before we use it.

	//if(m_ppOverlays[ID] != NULL) {
	//	m_ppOverlays[ID]->Release();
	//}
	m_ppOverlays[ID] = pSample;
	//int numRefs = m_ppOverlays[ID]->AddRef();

	return hr;
}

HRESULT MultiCamFilter::AdvanceInput()
{
	vcamLog(10, "MultiCamFilter::AdvanceInput...");

	// If currently viewing all cameras, switch to viewing camera first valid camera
	if (m_fViewAllCameras)
	{
		m_fViewAllCameras = FALSE;
		m_fOverlayCameras = TRUE;
		int firstValid = NextOrCurrentValidFilter(0);
		if (firstValid < 0)
		{
			// No valid cameras, so do nothing
			return S_OK;
		}
		else {
			return SwitchInput(firstValid);
		}
	}
	// Okay, we are currently viewing a single primary camera, but which one is it?
	else {
		// If currently viewing last valid camera, switch to viewing all cameras
		int nextValid = NextValidFilter(m_iMainInputID);
		if (nextValid < 0) 
		{
			m_fViewAllCameras = TRUE;
			m_fOverlayCameras = FALSE;
			return S_OK;
		}
		// Otherwise we must be viewing a camera other than the last one,
		// so switch to viewing the next camera in the sequence
		else
		{
			return SwitchInput(nextValid);
		}
	}
}


HRESULT MultiCamFilter::SwitchInput(int ID)
{
	HRESULT hr = S_OK;
	IMediaControl* pmc = NULL;

	vcamLog(10, "MultiCamFilter::SwitchInput: ID = %d", ID);

	VCAM_ASSERT( (ID >= 0 && ID < m_iNumUpstreamFilters) );

	m_fViewAllCameras = FALSE;

	if (m_iMainInputID == ID)
	{
		vcamLog(10, "MultiCamFilter::SwitchInput: new ID is the same as old ID; returning immediately");
		return S_OK;
	}

	CMediaType new_mt(m_ppInputs[ID]->CurrentMediaType());

	hr = m_pGraph->QueryInterface(IID_IMediaControl, (void**) &pmc);
	if (FAILED(hr))
	{
		vcamLog(10, "       MultiCamFilter::SwitchInput couldn't get media control");
		goto done;
	}
	VCAM_ASSERT(pmc != NULL);

	//hr = pmc->Stop();
	//hrOK;

	hr = m_pOutput->GetConnected()->QueryAccept(&new_mt);
	VCAM_ASSERT(hr == S_OK);

	// We really *should* hold some sort of lock when switching the input, but
	// the following cavalier approach did not seem to cause any problems in practice.
	m_iMainInputID = ID;
	m_pInput = m_ppInputs[ID];

	//////hr = GetOutputPin()->SetFormat(&new_mt);
	//////hrOK;
	//////hr = ReconnectPin( m_pOutput, &new_mt );
	//////hrOK;
	//hr = pmc->Run();
	//hrOK;

done:
	SafeRelease(&pmc);
	vcamLog(10, "       MultiCamFilter::SwitchInput: returning 0x%x", hr);
	return hr;
}

HRESULT MultiCamFilter::GetLargestInputDetails(/*out*/ LONG& max_width, /*out*/ LONG& max_height)
{
	HRESULT hr = S_OK;
	AM_MEDIA_TYPE mt;
	max_width = 0;
	max_height = 0;

	for(int i=0; i<m_iNumUpstreamFilters; i++){
		if (m_ppUpstreamFilter[i] != NULL) {
			m_ppInputs[i]->ConnectionMediaType(&mt);
			VCAM_ASSERT(mt.formattype == FORMAT_VideoInfo || mt.formattype == FORMAT_VideoInfo2);

			BITMAPINFOHEADER *pbmi = GetBITMAPINFOHEADER(&mt);

			LONG this_width = pbmi->biWidth;
			LONG this_height = pbmi->biHeight;
			VCAM_ASSERT(this_height > 0);
			if (this_width > max_width)
				max_width = this_width;
			if (this_height > max_height)
				max_height = this_height;
			FreeMediaType(mt);
		}
	}

	m_Width = max_width;
	m_Height = max_height;
	return hr;
}

BOOL MultiCamFilter::IsEqualRect(const RECT r1, const RECT r2) 
{
	return r1.left==r2.left && r1.right==r2.right && r1.top==r2.top && r1.bottom==r2.bottom;
}



BOOL MultiCamFilter::OnlySizesDiffer(const CMediaType *pmt1, const CMediaType *pmt2) 
{
    CheckPointer(pmt1,FALSE);
    CheckPointer(pmt2,FALSE);

    if (!IsEqualGUID(*pmt1->Type(), *pmt2->Type())) return FALSE;
    if (!IsEqualGUID(*pmt1->Subtype(), *pmt2->Subtype())) return FALSE;
    if (!IsEqualGUID(*pmt1->FormatType(), *pmt2->FormatType())) return FALSE;
    if (pmt1->IsFixedSize() != pmt2->IsFixedSize() ) return FALSE;
    if (pmt1->IsTemporalCompressed() != pmt2->IsTemporalCompressed() ) return FALSE;
	// Note: by design, we do not compare GetSampleSize()
    if (pmt1->FormatLength() != pmt2->FormatLength() ) return FALSE;

	VCAM_ASSERT(IsEqualGUID(*pmt1->Subtype(), MEDIASUBTYPE_RGB24));
	VCAM_ASSERT(IsEqualGUID(*pmt1->FormatType(), FORMAT_VideoInfo) || IsEqualGUID(*pmt1->FormatType(), FORMAT_VideoInfo2));

	VIDEOINFOHEADER *pvi1 = (VIDEOINFOHEADER *) pmt1->Format();
	VIDEOINFOHEADER *pvi2 = (VIDEOINFOHEADER *) pmt2->Format();

    if (!IsEqualRect(pvi1->rcSource, pvi2->rcSource)) return FALSE;
    if (!IsEqualRect(pvi1->rcTarget, pvi2->rcTarget)) return FALSE;
    if ( pvi1->dwBitRate != pvi2->dwBitRate) return FALSE;
    if ( pvi1->dwBitErrorRate != pvi2->dwBitErrorRate) return FALSE;
	// The following caused problems in release builds, so
	// turn a blind eye to discrepancies in AvgTimePerFrame.
    //if ( pvi1->AvgTimePerFrame != pvi2->AvgTimePerFrame) return FALSE;

	BITMAPINFOHEADER *pbmi1 = GetBITMAPINFOHEADER(pmt1);
	BITMAPINFOHEADER *pbmi2 = GetBITMAPINFOHEADER(pmt2);

	// Note: by design, we do not compare the biWidth, biHeight, or biSizeImage fields

    if ( pbmi1->biSize != pbmi2->biSize) return FALSE;
    if ( pbmi1->biPlanes != pbmi2->biPlanes) return FALSE;
    if ( pbmi1->biBitCount != pbmi2->biBitCount) return FALSE;
    if ( pbmi1->biCompression != pbmi2->biCompression) return FALSE;
    if ( pbmi1->biXPelsPerMeter != pbmi2->biXPelsPerMeter) return FALSE;
    if ( pbmi1->biYPelsPerMeter != pbmi2->biYPelsPerMeter) return FALSE;
    if ( pbmi1->biClrUsed != pbmi2->biClrUsed) return FALSE;
    if ( pbmi1->biClrImportant != pbmi2->biClrImportant) return FALSE;

    return TRUE;

} // OnlySizesDiffer

BOOL MultiCamFilter::MatchesOutputSize(const CMediaType *pmt1) 
{
	CMediaType *pmt2 = &GetOutputPin()->CurrentMediaType();

	if (pmt1->GetSampleSize() != pmt2->GetSampleSize() ) return FALSE;

	VCAM_ASSERT(IsEqualGUID(*pmt1->Subtype(), MEDIASUBTYPE_RGB24));
	VCAM_ASSERT(IsEqualGUID(*pmt1->FormatType(), FORMAT_VideoInfo) || IsEqualGUID(*pmt1->FormatType(), FORMAT_VideoInfo2));
	BITMAPINFOHEADER *pbmi1 = GetBITMAPINFOHEADER(pmt1);

	VCAM_ASSERT(IsEqualGUID(*pmt2->Subtype(), MEDIASUBTYPE_RGB24));
	VCAM_ASSERT(IsEqualGUID(*pmt2->FormatType(), FORMAT_VideoInfo) || IsEqualGUID(*pmt2->FormatType(), FORMAT_VideoInfo2));
	BITMAPINFOHEADER *pbmi2 = GetBITMAPINFOHEADER(pmt2);

	if ( pbmi1->biWidth != pbmi2->biWidth) return FALSE;
	if ( pbmi1->biHeight != pbmi2->biHeight) return FALSE;
	if ( pbmi1->biSizeImage != pbmi2->biSizeImage) return FALSE;

    return TRUE;
} // MatchesOutputSize

HRESULT MultiCamFilter::ConnectionMediaType(/*out*/ AM_MEDIA_TYPE *pmt)
{
	if (m_iNumUpstreamFilters > 0 && m_pInput != NULL)
	{
		return m_pInput->ConnectionMediaType(pmt);
	}
	else // Special case -- no physical cameras are connected, so create a mock media type
	{
		return CreateMockMediaType(pmt);
	}
}

HRESULT MultiCamFilter::CreateMockMediaType(/*out*/ AM_MEDIA_TYPE *pmt)
{
	ZeroMemory(pmt, sizeof(AM_MEDIA_TYPE));
	CMediaType* pcmt = (CMediaType*) pmt;
	VIDEOINFOHEADER* pvi = (VIDEOINFOHEADER*) pcmt->AllocFormatBuffer(sizeof(VIDEOINFOHEADER));
	ZeroMemory(pvi, sizeof(VIDEOINFOHEADER));

    pvi->bmiHeader.biCompression = BI_RGB;
    pvi->bmiHeader.biBitCount    = 24;
    pvi->bmiHeader.biSize       = sizeof(BITMAPINFOHEADER);
    pvi->bmiHeader.biWidth      = 0;
    pvi->bmiHeader.biHeight     = 0;
    pvi->bmiHeader.biPlanes     = 1;
    pvi->bmiHeader.biSizeImage  = GetBitmapSize(&pvi->bmiHeader);
    pvi->bmiHeader.biClrImportant = 0;

    SetRectEmpty(&(pvi->rcSource)); // we want the whole image area rendered.
    SetRectEmpty(&(pvi->rcTarget)); // no particular destination rectangle

    pmt->majortype = MEDIATYPE_Video;
    pmt->subtype = MEDIASUBTYPE_RGB24;
    pmt->formattype = FORMAT_VideoInfo;
    pmt->bTemporalCompression = FALSE;
    pmt->bFixedSizeSamples= TRUE;
    pmt->lSampleSize = pvi->bmiHeader.biSizeImage;
    pmt->cbFormat = sizeof(VIDEOINFOHEADER);

	return S_OK;
}

//
// GetClassID
//
// This is the only method of IPersist
//
STDMETHODIMP MultiCamFilter::GetClassID(CLSID *pClsid)
{
    vcamLog(95, "MultiCamFilter::GetClassID");
    return CTransformFilter::GetClassID(pClsid);

} // GetClassID

HRESULT MultiCamFilter::StreamTime(CRefTime& rtStream)
{
	vcamLog(95, "MultiCamFilter::StreamTime");
	return CTransformFilter::StreamTime(rtStream);
}

LONG MultiCamFilter::GetPinVersion()
{
	vcamLog(95, "MultiCamFilter::GetPinVersion");
	LONG val = CTransformFilter::GetPinVersion();
	vcamLog(95, "          %ld", val);
	return val;
}

__out_opt LPAMOVIESETUP_FILTER MultiCamFilter::GetSetupData()
{
	vcamLog(95, "MultiCamFilter::GetSetupData");
	return CTransformFilter::GetSetupData();
}

HRESULT STDMETHODCALLTYPE MultiCamFilter::EnumPins(__out  IEnumPins **ppEnum)
{
	vcamLog(95, "MultiCamFilter::EnumPins");
	return CTransformFilter::EnumPins(ppEnum);
}

HRESULT STDMETHODCALLTYPE MultiCamFilter::FindPin(LPCWSTR Id, __out  IPin **ppPin)
{
	vcamLog(95, "MultiCamFilter::FindPin");
	return CTransformFilter::FindPin(Id, ppPin);
}

HRESULT STDMETHODCALLTYPE MultiCamFilter::QueryFilterInfo(__out  FILTER_INFO *pInfo)
{
	vcamLog(95, "MultiCamFilter::QueryFilterInfo");
	return CTransformFilter::QueryFilterInfo(pInfo);
}

HRESULT STDMETHODCALLTYPE MultiCamFilter::QueryVendorInfo(__out  LPWSTR *pVendorInfo)
{
	vcamLog(95, "MultiCamFilter::QueryVendorInfo");
	return CTransformFilter::QueryVendorInfo(pVendorInfo);
}

HRESULT STDMETHODCALLTYPE MultiCamFilter::Stop( void)
{
	vcamLog(20, "MultiCamFilter::Stop");
	HRESULT hr = CTransformFilter::Stop();
	hrOKnoRet;

	vcamLog(20, "       MultiCamFilter::Stop: returning 0x%x", hr);
	return hr;
}

HRESULT STDMETHODCALLTYPE MultiCamFilter::Pause( void)
{
	vcamLog(95, "MultiCamFilter::Pause");
	return CTransformFilter::Pause();
}

HRESULT STDMETHODCALLTYPE MultiCamFilter::Run( 
		REFERENCE_TIME tStart)
{
	vcamLog(95, "MultiCamFilter::Run");
	return CTransformFilter::Run(tStart);
}

HRESULT STDMETHODCALLTYPE MultiCamFilter::GetState( 
		/* [in] */ DWORD dwMilliSecsTimeout,
		/* [annotation][out] */ 
		__out  FILTER_STATE *State)
{
	vcamLog(95, "MultiCamFilter::GetState");
	return CTransformFilter::GetState(dwMilliSecsTimeout, State);
}

HRESULT STDMETHODCALLTYPE MultiCamFilter::SetSyncSource( 
		/* [annotation][in] */ 
		__in_opt  IReferenceClock *pClock)
{
	vcamLog(95, "MultiCamFilter::SetSyncSource");
	return CTransformFilter::SetSyncSource(pClock);
}

HRESULT STDMETHODCALLTYPE MultiCamFilter::GetSyncSource( 
		/* [annotation][out] */ 
		__deref_out_opt  IReferenceClock **pClock)
{
	vcamLog(95, "MultiCamFilter::GetSyncSource");
	return CTransformFilter::GetSyncSource(pClock);
}

HRESULT STDMETHODCALLTYPE MultiCamFilter::Register( void)
{
	vcamLog(95, "MultiCamFilter::Register");
	return CTransformFilter::Register();
}

HRESULT STDMETHODCALLTYPE MultiCamFilter::Unregister( void)
{
	vcamLog(95, "MultiCamFilter::Unregister");
	return CTransformFilter::Unregister();
}

