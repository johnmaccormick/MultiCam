// Downloaded from http://tmhare.mvps.org/downloads.htm
// Altered by John MacCormick, 2012.
// Alterations are released under modified BSD license. NO WARRANTY.
#pragma warning(disable:4244)
#pragma warning(disable:4711)

#include <streams.h>
#include <stdio.h>
#include <olectl.h>
#include <dvdmedia.h>
#include "filters.h"

#include <Strsafe.h>
#include <string>

using namespace std;

#include "jmac-vcam-guids.h"
#include "ConnectHelpers.h"
#include "LogHelpers.h"
#include "MessageBox.h"
#include "win32_exception.h"


//const char *VCAM1_DEBUG_FNAME = "C:\\Users\\jmac\\temp\\vcam-debug.txt";
//const char *VCAM1PIN_DEBUG_FNAME = "C:\\Users\\jmac\\temp\\vcam-debug.txt";
//const int GUID_STRING_MAXLEN = 1024;
//const char *LOG_PREFIX = "                                          ";

// from http://msdn.microsoft.com/en-us/library/dd940435(v=VS.85).aspx
template <class T> void SafeRelease(T **ppT)
{
	if (*ppT)
	{
		(*ppT)->Release();
		*ppT = NULL;
	}
}

//void Riid2String(REFIID riid, char *guidString) {
//	OLECHAR guidWString[GUID_STRING_MAXLEN]; 
//	StringFromGUID2(riid, guidWString, GUID_STRING_MAXLEN); 
//	size_t num_chars_converted;
//	wcstombs_s(&num_chars_converted, guidString, GUID_STRING_MAXLEN, guidWString, _TRUNCATE);
//}
//
//void Msg(TCHAR *szFormat, ...)
//{
//    TCHAR szBuffer[1024];  // Large buffer for long filenames or URLs
//    const size_t NUMCHARS = sizeof(szBuffer) / sizeof(szBuffer[0]);
//    const int LASTCHAR = NUMCHARS - 1;
//
//    // Format the input string
//    va_list pArgs;
//    va_start(pArgs, szFormat);
//
//    // Use a bounded buffer size to prevent buffer overruns.  Limit count to
//    // character size minus one to allow for a NULL terminating character.
//    (void)StringCchVPrintf(szBuffer, NUMCHARS - 1, szFormat, pArgs);
//    va_end(pArgs);
//
//    // Ensure that the formatted string is NULL-terminated
//    szBuffer[LASTCHAR] = TEXT('\0');
//
//    MessageBox(NULL, szBuffer, TEXT("PlayCap Message"), MB_OK | MB_ICONERROR);
//}

//////////////////////////////////////////////////////////////////////////
//  CVCam is the source filter which masquerades as a capture device
//////////////////////////////////////////////////////////////////////////
CUnknown * WINAPI CVCam::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
    ASSERT(phr);
    CUnknown *punk = new CVCam(lpunk, phr);
    return punk;
}

CVCam::CVCam(LPUNKNOWN lpunk, HRESULT *phr) : 
    CSource(NAME("Virtual Cam"), lpunk, CLSID_VirtualCam)
{
	//m_debugOut.exceptions(ios::failbit);
	//m_debugOut.open(VCAM1_DEBUG_FNAME, ios_base::out | ios_base::app );
	vcamOpenLog(10, "CVCam::CVCam");
	vcamLog(10, "***********************CVCam::CVCam***********************");
    ASSERT(phr);
    CAutoLock cAutoLock(&m_cStateLock);
    // Create the one and only output pin
    m_paStreams = (CSourceStream **) new CVCamStream*[1];
    m_paStreams[0] = new CVCamStream(phr, this, L"Virtual Cam");

}

CVCam::~CVCam()
{
	vcamLog(10, "~~~~~~~~~~~~~~~~~~~~~~~CVCamStream::~CVCamStream~~~~~~~~~~~~~~~~~~~~~~~~~");
	//m_debugOut.close();
	vcamCloseLog(10, "CVCam::~CVCam");

}


HRESULT CVCam::QueryInterface(REFIID riid, void **ppv)
{
	HRESULT hr = S_OK;

	char guidString[GUID_STRING_MAXLEN];
	Riid2String(riid, guidString);

    //Forward request for IAMStreamConfig & IKsPropertySet to the pin
    if(riid == _uuidof(IAMStreamConfig) || riid == _uuidof(IKsPropertySet))
	{
		hr = m_paStreams[0]->QueryInterface(riid, ppv);
		vcamLog(50, "CVCam::QueryInterface, riid = %s", guidString);
		if (hr==S_OK) {
			vcamLog(50, "       S_OK");
		}
		if (hr==E_NOINTERFACE ) {
			vcamLog(50, "       E_NOINTERFACE ");
		}
		if (hr==E_POINTER) {
			vcamLog(50, "       E_POINTER");
		}
        return hr;
	}
    else
	{
		hr = CSource::QueryInterface(riid, ppv);
		vcamLog(50, "CVCam::QueryInterface (to CSource), riid = %s", guidString);
		if (hr==S_OK) {
			vcamLog(50, "       S_OK");
		}
		if (hr==E_NOINTERFACE ) {
			vcamLog(50, "       E_NOINTERFACE ");
		}
		if (hr==E_POINTER) {
			vcamLog(50, "       E_POINTER");
		}
        return hr;
	}
}


STDMETHODIMP CVCam::JoinFilterGraph(
		__inout_opt IFilterGraph * pGraph,
		__in_opt LPCWSTR pName)
{
	vcamLog(10, "CVCam::JoinFilterGraph");
	HRESULT hr = CBaseFilter::JoinFilterGraph(pGraph, pName);

	//string msg("");
	//if (CBaseFilter::m_pGraph==NULL) {
	//	msg.append("after JoinFilterGraph, CBaseFilter::m_pGraph==NULL");
	//} else {
	//	msg.append("after JoinFilterGraph, CBaseFilter::m_pGraph!=NULL");
	//}
	//Msg((TCHAR*) msg.c_str());


	vcamLog(10, "       CVCam::JoinFilterGraph returning %d", (int) hr);
	return hr;
}

//void CVCam::vcamLog(10, const char* message) 
//{
//	return;
//	m_debugOut << LOG_PREFIX << message << endl;
//}
//
//void CVCam::vcamLog(10, TCHAR *szFormat, ...)
//{
//	return;
//	TCHAR szBuffer[1024];  // Large buffer for long filenames or URLs
//	const size_t NUMCHARS = sizeof(szBuffer) / sizeof(szBuffer[0]);
//	const int LASTCHAR = NUMCHARS - 1;
//
//	// Format the input string
//	va_list pArgs;
//	va_start(pArgs, szFormat);
//
//	// Use a bounded buffer size to prevent buffer overruns.  Limit count to
//	// character size minus one to allow for a NULL terminating character.
//	(void)StringCchVPrintf(szBuffer, NUMCHARS - 1, szFormat, pArgs);
//	va_end(pArgs);
//
//	// Ensure that the formatted string is NULL-terminated
//	szBuffer[LASTCHAR] = TEXT('\0');
//
//	m_debugOut << LOG_PREFIX << szBuffer << endl;
//}


///////////////////////////////////////////////////////////
// all inherited virtual functions
///////////////////////////////////////////////////////////
// 1. from CSource
CBasePin *CVCam::GetPin(int n)
{
	vcamLog(50, "CVCam::GetPin, n = %d", n);
	CBasePin *pin = CSource::GetPin(n);
	ASSERT(pin!=NULL);
	if(pin->IsConnected()) {
		vcamLog(50, "  CVCam::GetPin: pin is connected");
	} else {
		vcamLog(50, "  CVCam::GetPin: pin is not connected");
	}
	return pin;
}





// 2. from CTransformFilter
int CVCam::GetPinCount()
{
	vcamLog(50, "CVCam::GetPinCount");
	return CSource::GetPinCount();
}




// 3. from CBaseFilter
HRESULT CVCam::StreamTime(CRefTime& rtStream)
{
	vcamLog(50, "CVCam::StreamTime");
	return CSource::StreamTime(rtStream);
}

LONG CVCam::GetPinVersion()
{
	vcamLog(50, "CVCam::GetPinVersion");
	return CSource::GetPinVersion();
}

__out_opt LPAMOVIESETUP_FILTER CVCam::GetSetupData()
{
	vcamLog(50, "CVCam::GetSetupData");
	return CSource::GetSetupData();
}

HRESULT STDMETHODCALLTYPE CVCam::EnumPins(__out  IEnumPins **ppEnum)
{
	vcamLog(50, "CVCam::EnumPins");
	return CSource::EnumPins(ppEnum);
}

HRESULT STDMETHODCALLTYPE CVCam::FindPin(LPCWSTR Id, __out  IPin **ppPin)
{
	vcamLog(50, "CVCam::FindPin");
	return CSource::FindPin(Id, ppPin);
}

HRESULT STDMETHODCALLTYPE CVCam::QueryFilterInfo(__out  FILTER_INFO *pInfo)
{
	vcamLog(50, "CVCam::QueryFilterInfo");
	return CSource::QueryFilterInfo(pInfo);
}

HRESULT STDMETHODCALLTYPE CVCam::QueryVendorInfo(__out  LPWSTR *pVendorInfo)
{
	vcamLog(50, "CVCam::QueryVendorInfo");
	return CSource::QueryVendorInfo(pVendorInfo);
}


HRESULT STDMETHODCALLTYPE CVCam::Stop( void)
{
	vcamLog(50, "CVCam::Stop");
	return CSource::Stop( );
}

HRESULT STDMETHODCALLTYPE CVCam::Pause( void)
{
	vcamLog(50, "CVCam::Pause");
	return CSource::Pause( );
}

HRESULT STDMETHODCALLTYPE CVCam::Run(REFERENCE_TIME tStart)
{
	vcamLog(50, "CVCam::Run");
	HRESULT hr = CSource::Run(tStart);
	vcamLog(50, "       CVCam::Run returning %d", (int) hr);
	return hr;
}

HRESULT STDMETHODCALLTYPE CVCam::GetState(DWORD dwMilliSecsTimeout,   __out  FILTER_STATE *State)
{
	vcamLog(50, "CVCam::GetState");
	return CSource::GetState(dwMilliSecsTimeout,   State);

}

HRESULT STDMETHODCALLTYPE CVCam::SetSyncSource(__in_opt  IReferenceClock *pClock)
{
	vcamLog(50, "CVCam::SetSyncSource");
	return CSource::SetSyncSource(pClock);
}

HRESULT STDMETHODCALLTYPE CVCam::GetSyncSource(__deref_out_opt  IReferenceClock **pClock)
{
	vcamLog(50, "CVCam::GetSyncSource");
	return CSource::GetSyncSource(pClock);

}

STDMETHODIMP CVCam::GetClassID(__out CLSID *pClsID)
{
	vcamLog(50, "CVCam::GetClassID");
	return CSource::GetClassID(pClsID);
}

ULONG STDMETHODCALLTYPE CVCam::AddRef( void)
{
	//vcamLog(50, "CVCam::AddRef");
	return CSource::AddRef( );
}

ULONG STDMETHODCALLTYPE CVCam::Release( void)
{
	//vcamLog(50, "CVCam::Release");
	return CSource::Release( );
}

HRESULT STDMETHODCALLTYPE CVCam::Register( void)
{
	vcamLog(50, "CVCam::Register");
	return CSource::Register( );
}

HRESULT STDMETHODCALLTYPE CVCam::Unregister( void)
{
	vcamLog(50, "CVCam::Unregister");
	return CSource::Unregister( );
}












//////////////////////////////////////////////////////////////////////////
// CVCamStream is the one and only output pin of CVCam which handles 
// all the stuff.
//////////////////////////////////////////////////////////////////////////
CVCamStream::CVCamStream(HRESULT *phr, CVCam *pParent, LPCWSTR pPinName) :
    CSourceStream(NAME("Virtual Cam"),phr, pParent, pPinName), m_pParent(pParent)
{
	vcamOpenLog(10, "CVCamStream::CVCamStream");

	//m_debugOut.exceptions(ios::failbit);
	//m_debugOut.open(VCAM1PIN_DEBUG_FNAME, ios_base::out | ios_base::app );
	vcamLog(10, "***********************CVCamStream::CVCamStream***********************");

    // Set the default media type as 320x240x24@15
    GetMediaType(4, &m_mt);

 //   string msg("");
	//if (this->m_pParent->GetGraph()==NULL) {
	//	msg.append("in CVCamStream::CVCamStream, cbasefilter::m_pgraph==null");
	//} else {
	//	msg.append("in CVCamStream::CVCamStream, cbasefilter::m_pgraph!=null");
	//}
	//Msg((TCHAR*) msg.c_str());
	
}

CVCamStream::~CVCamStream()
{
	vcamLog(10, "~~~~~~~~~~~~~~~~~~~~~~~CVCamStream::~CVCamStream~~~~~~~~~~~~~~~~~~~~~~~~~");
	vcamCloseLog(10, "CVCamStream::~CVCamStream");
	//m_debugOut.close();
} 

HRESULT CVCamStream::QueryInterface(REFIID riid, void **ppv)
{   
	HRESULT hr = S_OK;

	char guidString[GUID_STRING_MAXLEN];
	Riid2String(riid, guidString);

    // Standard OLE stuff
    if(riid == _uuidof(IAMStreamConfig))
        *ppv = (IAMStreamConfig*)this;
    else if(riid == _uuidof(IKsPropertySet))
        *ppv = (IKsPropertySet*)this;
	else {
		hr =  CSourceStream::QueryInterface(riid, ppv);
		vcamLog(50, "CVCamStream::QueryInterface, riid = %s", guidString);
		if (hr==S_OK) {
			vcamLog(50, "       S_OK");
		}
		if (hr==E_NOINTERFACE ) {
			vcamLog(50, "       E_NOINTERFACE ");
		}
		if (hr==E_POINTER) {
			vcamLog(50, "       E_POINTER");
		}
		return hr;
	}
    AddRef();

	vcamLog(50, "CVCamStream::QueryInterface, riid = %s", guidString);
	vcamLog(50, "       S_OK");

    return S_OK;
}


//////////////////////////////////////////////////////////////////////////
//  This is the routine where we create the data being output by the Virtual
//  Camera device.
//////////////////////////////////////////////////////////////////////////

HRESULT CVCamStream::FillBuffer(IMediaSample *pms)
{
	vcamLog(90, "CVCamStream::FillBuffer");
    REFERENCE_TIME rtNow;
    
    REFERENCE_TIME avgFrameTime = ((VIDEOINFOHEADER*)m_mt.pbFormat)->AvgTimePerFrame;

    rtNow = m_rtLastTime;
    m_rtLastTime += avgFrameTime;
    pms->SetTime(&rtNow, &m_rtLastTime);
    pms->SetSyncPoint(TRUE);

    BYTE *pData;
    long lDataLen;
    pms->GetPointer(&pData);
    lDataLen = pms->GetSize();
    for(int i = 0; i < lDataLen; ++i)
        pData[i] = rand();


	//string msg("");
	//if (this->m_pParent->GetGraph()==NULL) {
	//	msg.append("in fillbuffer, cbasefilter::m_pgraph==null");
	//} else {
	//	msg.append("in fillbuffer, cbasefilter::m_pgraph!=null");
	//}
	//Msg((TCHAR*) msg.c_str());


    return NOERROR;
} // FillBuffer


//
// Notify
// Ignore quality management messages sent from the downstream filter
STDMETHODIMP CVCamStream::Notify(IBaseFilter * pSender, Quality q)
{
	vcamLog(50, "CVCamStream::Notify");
    return E_NOTIMPL;
} // Notify

//////////////////////////////////////////////////////////////////////////
// This is called when the output format has been negotiated
//////////////////////////////////////////////////////////////////////////
HRESULT CVCamStream::SetMediaType(const CMediaType *pmt)
{
	vcamLog(50, "CVCamStream::SetMediaType");
    //DECLARE_PTR(VIDEOINFOHEADER, pvi, pmt->Format());
    HRESULT hr = CSourceStream::SetMediaType(pmt);
    return hr;
}

// See Directshow help topic for IAMStreamConfig for details on this method
HRESULT CVCamStream::GetMediaType(int iPosition, CMediaType *pmt)
{
	vcamLog(50, "CVCamStream::GetMediaType, iPosition = %d", iPosition);

    if(iPosition < 0) {
		vcamLog(50, "    GetMediaType returning E_INVALIDARG (0x%x)", E_INVALIDARG);
		return E_INVALIDARG;
	}
    if(iPosition > 8) {
		vcamLog(50, "    GetMediaType returning VFW_S_NO_MORE_ITEMS (0x%x)", VFW_S_NO_MORE_ITEMS);
		return VFW_S_NO_MORE_ITEMS;
	}
	
    if(iPosition == 0) 
    {
        *pmt = m_mt;
		vcamLog(50, "    GetMediaType returning S_OK (0x%x)", S_OK);
        return S_OK;
    }

    DECLARE_PTR(VIDEOINFOHEADER, pvi, pmt->AllocFormatBuffer(sizeof(VIDEOINFOHEADER)));
    ZeroMemory(pvi, sizeof(VIDEOINFOHEADER));

    pvi->bmiHeader.biCompression = BI_RGB;
    pvi->bmiHeader.biBitCount    = 24;
    pvi->bmiHeader.biSize       = sizeof(BITMAPINFOHEADER);
    pvi->bmiHeader.biWidth      = 80 * iPosition;
    pvi->bmiHeader.biHeight     = 60 * iPosition;
    pvi->bmiHeader.biPlanes     = 1;
    pvi->bmiHeader.biSizeImage  = GetBitmapSize(&pvi->bmiHeader);
    pvi->bmiHeader.biClrImportant = 0;

    pvi->AvgTimePerFrame = 1000000;

    SetRectEmpty(&(pvi->rcSource)); // we want the whole image area rendered.
    SetRectEmpty(&(pvi->rcTarget)); // no particular destination rectangle

    pmt->SetType(&MEDIATYPE_Video);
    pmt->SetFormatType(&FORMAT_VideoInfo);
    pmt->SetTemporalCompression(FALSE);

    // Work out the GUID for the subtype from the header info.
    const GUID SubTypeGUID = GetBitmapSubtype(&pvi->bmiHeader);
    pmt->SetSubtype(&SubTypeGUID);
    pmt->SetSampleSize(pvi->bmiHeader.biSizeImage);
    
	vcamLog(50, "CVCamStream::GetMediaType: got media type:");
	vcamLogFormat(50, pmt);

	vcamLog(50, "    GetMediaType returning NOERROR (0x%x)", NOERROR);
    return NOERROR;

} // GetMediaType

// This method is called to see if a given output format is supported
HRESULT CVCamStream::CheckMediaType(const CMediaType *pMediaType)
{
	vcamLog(50, "CVCamStream::CheckMediaType");
    //VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *)(pMediaType->Format());

	if(*pMediaType != m_mt) 
        return E_INVALIDARG;


    return S_OK;
} // CheckMediaType

// This method is called after the pins are connected to allocate buffers to stream data
HRESULT CVCamStream::DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProperties)
{
	vcamLog(50, "CVCamStream::DecideBufferSize");
    CAutoLock cAutoLock(m_pFilter->pStateLock());
    HRESULT hr = NOERROR;

    VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *) m_mt.Format();
    pProperties->cBuffers = 1;
    pProperties->cbBuffer = pvi->bmiHeader.biSizeImage;

    ALLOCATOR_PROPERTIES Actual;
    hr = pAlloc->SetProperties(pProperties,&Actual);

    if(FAILED(hr)) 
	{
		vcamLog(50, "   CVCamStream::DecideBufferSize returning FAILED");
		return hr;
	}
    if(Actual.cbBuffer < pProperties->cbBuffer) 
	{
		vcamLog(50, "   CVCamStream::DecideBufferSize returning E_FAIL");
		return E_FAIL;
	}

	vcamLog(50, "   CVCamStream::DecideBufferSize returning NOERROR");
    return NOERROR;
} // DecideBufferSize

// Called when graph is run
HRESULT CVCamStream::OnThreadCreate()
{
	vcamLog(50, "CVCamStream::OnThreadCreate");
    m_rtLastTime = 0;
    return NOERROR;
} // OnThreadCreate


//////////////////////////////////////////////////////////////////////////
//  IAMStreamConfig
//////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE CVCamStream::SetFormat(AM_MEDIA_TYPE *pmt)
{
	vcamLog(50, "CVCamStream::SetFormat");
    //DECLARE_PTR(VIDEOINFOHEADER, pvi, m_mt.pbFormat);
    m_mt = *pmt;
    IPin* pin; 
    ConnectedTo(&pin);
    if(pin)
    {
        IFilterGraph *pGraph = m_pParent->GetGraph();
        pGraph->Reconnect(this);
		vcamLog(50, "   CVCamStream::SetFormat returning S_OK (reconnected to graph)");
    } else {
		vcamLog(50, "   CVCamStream::SetFormat returning S_OK (pin not connected)");
	}
	SafeRelease(&pin);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CVCamStream::GetFormat(AM_MEDIA_TYPE **ppmt)
{
	vcamLog(50, "CVCamStream::GetFormat");
    *ppmt = CreateMediaType(&m_mt);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CVCamStream::GetNumberOfCapabilities(int *piCount, int *piSize)
{
	*piCount = 8;
    *piSize = sizeof(VIDEO_STREAM_CONFIG_CAPS);

	vcamLog(50, "CVCamStream::GetNumberOfCapabilities, Count = %d, Size = %d", *piCount, *piSize);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CVCamStream::GetStreamCaps(int iIndex, AM_MEDIA_TYPE **pmt, BYTE *pSCC)
{
	vcamLog(50, "CVCamStream::GetStreamCaps, iIndex = %d", iIndex);
    *pmt = CreateMediaType(&m_mt);
    DECLARE_PTR(VIDEOINFOHEADER, pvi, (*pmt)->pbFormat);

    if (iIndex == 0) iIndex = 4;

    pvi->bmiHeader.biCompression = BI_RGB;
    pvi->bmiHeader.biBitCount    = 24;
    pvi->bmiHeader.biSize       = sizeof(BITMAPINFOHEADER);
    pvi->bmiHeader.biWidth      = 80 * iIndex;
    pvi->bmiHeader.biHeight     = 60 * iIndex;
    pvi->bmiHeader.biPlanes     = 1;
    pvi->bmiHeader.biSizeImage  = GetBitmapSize(&pvi->bmiHeader);
    pvi->bmiHeader.biClrImportant = 0;

    SetRectEmpty(&(pvi->rcSource)); // we want the whole image area rendered.
    SetRectEmpty(&(pvi->rcTarget)); // no particular destination rectangle

    (*pmt)->majortype = MEDIATYPE_Video;
    (*pmt)->subtype = MEDIASUBTYPE_RGB24;
    (*pmt)->formattype = FORMAT_VideoInfo;
    (*pmt)->bTemporalCompression = FALSE;
//    (*pmt)->bFixedSizeSamples= FALSE;
    (*pmt)->bFixedSizeSamples= TRUE;
    (*pmt)->lSampleSize = pvi->bmiHeader.biSizeImage;
    (*pmt)->cbFormat = sizeof(VIDEOINFOHEADER);
    
    DECLARE_PTR(VIDEO_STREAM_CONFIG_CAPS, pvscc, pSCC);
    
    pvscc->guid = FORMAT_VideoInfo;
    pvscc->VideoStandard = AnalogVideo_None;
    pvscc->InputSize.cx = 640;
    pvscc->InputSize.cy = 480;
    pvscc->MinCroppingSize.cx = 80;
    pvscc->MinCroppingSize.cy = 60;
    pvscc->MaxCroppingSize.cx = 640;
    pvscc->MaxCroppingSize.cy = 480;
    pvscc->CropGranularityX = 80;
    pvscc->CropGranularityY = 60;
    pvscc->CropAlignX = 0;
    pvscc->CropAlignY = 0;

    pvscc->MinOutputSize.cx = 80;
    pvscc->MinOutputSize.cy = 60;
    pvscc->MaxOutputSize.cx = 640;
    pvscc->MaxOutputSize.cy = 480;
    pvscc->OutputGranularityX = 0;
    pvscc->OutputGranularityY = 0;
    pvscc->StretchTapsX = 0;
    pvscc->StretchTapsY = 0;
    pvscc->ShrinkTapsX = 0;
    pvscc->ShrinkTapsY = 0;
    pvscc->MinFrameInterval = 200000;   //50 fps
    pvscc->MaxFrameInterval = 50000000; // 0.2 fps
    pvscc->MinBitsPerSecond = (80 * 60 * 3 * 8) / 5;
    pvscc->MaxBitsPerSecond = 640 * 480 * 3 * 8 * 50;

	vcamLog(50, "        S_OK");
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////
// IKsPropertySet
//////////////////////////////////////////////////////////////////////////


HRESULT CVCamStream::Set(REFGUID guidPropSet, DWORD dwID, void *pInstanceData, 
                        DWORD cbInstanceData, void *pPropData, DWORD cbPropData)
{// Set: Cannot set any properties.
	vcamLog(50, "CVCamStream::Set");
    return E_NOTIMPL;
}

// Get: Return the pin category (our only property). 
HRESULT CVCamStream::Get(
    REFGUID guidPropSet,   // Which property set.
    DWORD dwPropID,        // Which property in that set.
    void *pInstanceData,   // Instance data (ignore).
    DWORD cbInstanceData,  // Size of the instance data (ignore).
    void *pPropData,       // Buffer to receive the property data.
    DWORD cbPropData,      // Size of the buffer.
    DWORD *pcbReturned     // Return the size of the property.
)
{
	char guidString[GUID_STRING_MAXLEN];
	Riid2String(guidPropSet, guidString);
	vcamLog(50, "CVCamStream::Get, guidPropSet = %s, dwPropID = %d", guidString, dwPropID);

    if (guidPropSet != AMPROPSETID_Pin)             return E_PROP_SET_UNSUPPORTED;
    if (dwPropID != AMPROPERTY_PIN_CATEGORY)        return E_PROP_ID_UNSUPPORTED;
    if (pPropData == NULL && pcbReturned == NULL)   return E_POINTER;
    
    if (pcbReturned) *pcbReturned = sizeof(GUID);
    if (pPropData == NULL)          return S_OK; // Caller just wants to know the size. 
    if (cbPropData < sizeof(GUID))  return E_UNEXPECTED;// The buffer is too small.
        
    *(GUID *)pPropData = PIN_CATEGORY_CAPTURE;
    return S_OK;
}

// QuerySupported: Query whether the pin supports the specified property.
HRESULT CVCamStream::QuerySupported(REFGUID guidPropSet, DWORD dwPropID, DWORD *pTypeSupport)
{
	vcamLog(50, "CVCamStream::QuerySupported");
    if (guidPropSet != AMPROPSETID_Pin) return E_PROP_SET_UNSUPPORTED;
    if (dwPropID != AMPROPERTY_PIN_CATEGORY) return E_PROP_ID_UNSUPPORTED;
    // We support getting this property, but not setting it.
    if (pTypeSupport) *pTypeSupport = KSPROPERTY_SUPPORT_GET; 
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CVCamStream::QueryPinInfo( 
            /* [annotation][out] */ 
            __out  PIN_INFO *pInfo)
{
  vcamLog(50, "CVCamStream::QueryPinInfo");
  HRESULT hr = S_OK;
  hr = CSourceStream::QueryPinInfo(pInfo);

  if(pInfo->dir==PINDIR_INPUT) {
	  vcamLog(50, "   PINDIR_INPUT");
  }
  if(pInfo->dir==PINDIR_OUTPUT) {
	  vcamLog(50, "   PINDIR_OUTPUT");
  }
  return hr;
}



STDMETHODIMP  CVCamStream::NonDelegatingQueryInterface(REFIID riid, __deref_out void **ppv)
{
	vcamLog(50, "CVCamStream::NonDelegatingQueryInterface");
	HRESULT hr = S_OK;
	hr = CSourceStream::NonDelegatingQueryInterface(riid, ppv);
	
	char guidString[GUID_STRING_MAXLEN];
	Riid2String(riid, guidString);
	vcamLog(50, "CVCamStream::NonDelegatingQueryInterface, riid = %s", guidString);
	if (hr==NOERROR) {
		vcamLog(50, "       NOERROR");
	}
	if (hr==E_NOINTERFACE ) {
		vcamLog(50, "       E_NOINTERFACE ");
	}
	if (hr==E_POINTER) {
		vcamLog(50, "       E_POINTER");
	}
	return hr;
}

//void CVCamStream::vcamLog(50, const char* message) 
//{
//	return;
//	m_debugOut << LOG_PREFIX << message << endl;
//}
//
//void CVCamStream::vcamLog(50, TCHAR *szFormat, ...)
//{
//	return;
//	TCHAR szBuffer[1024];  // Large buffer for long filenames or URLs
//	const size_t NUMCHARS = sizeof(szBuffer) / sizeof(szBuffer[0]);
//	const int LASTCHAR = NUMCHARS - 1;
//
//	// Format the input string
//	va_list pArgs;
//	va_start(pArgs, szFormat);
//
//	// Use a bounded buffer size to prevent buffer overruns.  Limit count to
//	// character size minus one to allow for a NULL terminating character.
//	(void)StringCchVPrintf(szBuffer, NUMCHARS - 1, szFormat, pArgs);
//	va_end(pArgs);
//
//	// Ensure that the formatted string is NULL-terminated
//	szBuffer[LASTCHAR] = TEXT('\0');
//
//	m_debugOut << LOG_PREFIX << szBuffer << endl;
//}

HRESULT STDMETHODCALLTYPE CVCamStream::ConnectedTo(/* [annotation][out] */ 
	__out  IPin **pPin)
{
	vcamLog(50, "CVCamStream::ConnectedTo");
	HRESULT hr = CSourceStream::ConnectedTo(pPin);
	if (hr == S_OK) {
		vcamLog(50, "   S_OK");
	} else {
		vcamLog(50, "   not S_OK");
	}
	return hr;
}

HRESULT STDMETHODCALLTYPE CVCamStream::EnumMediaTypes( 
	/* [annotation][out] */ 
	__out  IEnumMediaTypes **ppEnum)
{
	vcamLog(50, "CVCamStream::EnumMediaTypes");
	HRESULT hr = CSourceStream::EnumMediaTypes(ppEnum);
	if (hr == S_OK) {
		vcamLog(50, "   S_OK");
	} else if (hr == VFW_E_NOT_CONNECTED) {
		vcamLog(50, "   VFW_E_NOT_CONNECTED");
	} else {
		vcamLog(50, "   error");
	}
	return hr;
}

LONG  CVCamStream::GetMediaTypeVersion()
{
	vcamLog(50, "CVCamStream::GetMediaTypeVersion");
	return CSourceStream::GetMediaTypeVersion();
}

HRESULT  CVCamStream::CompleteConnect(IPin *pReceivePin)
{
	vcamLog(50, "CVCamStream::CompleteConnect");
	HRESULT hr = CSourceStream::CompleteConnect(pReceivePin);
	vcamLog(50, "     CompleteConnect returning %d", (int) hr);
	return hr;
}

HRESULT  CVCamStream::CheckConnect(IPin *pPin)
{
	vcamLog(50, "CVCamStream::CheckConnect");
	HRESULT hr = CSourceStream::CheckConnect(pPin);
	vcamLog(50, "     CheckConnect returning %d", (int) hr);
	return hr;
}

HRESULT  CVCamStream::BreakConnect()
{
	vcamLog(50, "CVCamStream::BreakConnect");
	HRESULT hr = CSourceStream::BreakConnect();
	vcamLog(50, "     BreakConnect returning %d", (int) hr);
	return hr;
}

HRESULT STDMETHODCALLTYPE CVCamStream::Connect( 
	/* [in] */ IPin *pReceivePin,
	/* [annotation][in] */ 
	__in_opt  const AM_MEDIA_TYPE *pmt)
{
	HRESULT hr = S_OK;
	vcamLog(50, "CVCamStream::Connect");

	hr = CSourceStream::Connect( pReceivePin, pmt);

	//vcamLog(50, "         CVCamStream::Connect returning %d", (int) hr);
	if(hr==S_OK) {
		vcamLog(50, "         CVCamStream::Connect returning S_OK");
	}
	if(hr==VFW_E_ALREADY_CONNECTED) {
		vcamLog(50, "         CVCamStream::Connect returning VFW_E_ALREADY_CONNECTED");
	}
	if(hr==VFW_E_NO_ACCEPTABLE_TYPES) {
		vcamLog(50, "         CVCamStream::Connect returning VFW_E_NO_ACCEPTABLE_TYPES");
	}
	if(hr==VFW_E_NO_TRANSPORT) {
		vcamLog(50, "         CVCamStream::Connect returning VFW_E_NO_TRANSPORT");
	}
	if(hr==VFW_E_NOT_STOPPED) {
		vcamLog(50, "         CVCamStream::Connect returning VFW_E_NOT_STOPPED");
	}
	if(hr==VFW_E_TYPE_NOT_ACCEPTED) {
		vcamLog(50, "         CVCamStream::Connect returning VFW_E_TYPE_NOT_ACCEPTED");
	}
	return hr;
}

HRESULT STDMETHODCALLTYPE CVCamStream::ReceiveConnection( 
	/* [in] */ IPin *pConnector,
	/* [in] */ const AM_MEDIA_TYPE *pmt)
{
	vcamLog(50, "CVCamStream::ReceiveConnection");
	return CSourceStream::ReceiveConnection( pConnector,pmt);
}

HRESULT STDMETHODCALLTYPE CVCamStream::Disconnect( void)
{
	vcamLog(50, "CVCamStream::Disconnect");
	return CSourceStream::Disconnect( );
}


HRESULT STDMETHODCALLTYPE CVCamStream::ConnectionMediaType( 
	/* [annotation][out] */ 
	__out  AM_MEDIA_TYPE *pmt)
{
	vcamLog(50, "CVCamStream::ConnectionMediaType");
	return CSourceStream::ConnectionMediaType(pmt);
}


HRESULT STDMETHODCALLTYPE CVCamStream::QueryDirection( 
	/* [annotation][out] */ 
	__out  PIN_DIRECTION *pPinDir)
{
	vcamLog(50, "CVCamStream::QueryDirection");
	return CSourceStream::QueryDirection(pPinDir);
}

HRESULT STDMETHODCALLTYPE CVCamStream::QueryId( 
	/* [annotation][out] */ 
	__out  LPWSTR *Id)
{
	vcamLog(50, "CVCamStream::QueryId");
	return CSourceStream::QueryId(Id);
}

HRESULT STDMETHODCALLTYPE CVCamStream::QueryAccept( 
	/* [in] */ const AM_MEDIA_TYPE *pmt)
{
	vcamLog(50, "CVCamStream::QueryAccept");
	return CSourceStream::QueryAccept(pmt);
}


HRESULT STDMETHODCALLTYPE CVCamStream::QueryInternalConnections( 
	/* [annotation][out] */ 
	__out_ecount_part_opt(*nPin, *nPin)  IPin **apPin,
	/* [out][in] */ ULONG *nPin)
{
	vcamLog(50, "CVCamStream::QueryInternalConnections");
	return CSourceStream::QueryInternalConnections(apPin,nPin);
}

HRESULT STDMETHODCALLTYPE CVCamStream::EndOfStream( void)
{
	vcamLog(50, "CVCamStream::EndOfStream");
	return CSourceStream::EndOfStream( );
}

HRESULT STDMETHODCALLTYPE CVCamStream::BeginFlush( void)
{
	vcamLog(50, "CVCamStream::BeginFlush");
	return CSourceStream::BeginFlush( );
}

HRESULT STDMETHODCALLTYPE CVCamStream::EndFlush( void)
{
	vcamLog(50, "CVCamStream::EndFlush");
	return CSourceStream::EndFlush( );
}

HRESULT STDMETHODCALLTYPE CVCamStream::NewSegment( 
	/* [in] */ REFERENCE_TIME tStart,
	/* [in] */ REFERENCE_TIME tStop,
	/* [in] */ double dRate)
{
	vcamLog(50, "CVCamStream::NewSegment");
	return CSourceStream::NewSegment(tStart,tStop, dRate);
}

HRESULT CVCamStream::Active(void)  {
	vcamLog(50, "CVCamStream::Active");

	HRESULT hr = CSourceStream::Active();
	vcamLog(50, "         CVCamStream::Active returning %d", (int) hr);
	return hr;
}
