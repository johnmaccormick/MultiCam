// Downloaded from http://tmhare.mvps.org/downloads.htm
// Altered by John MacCormick, 2012.
// Alterations are released under modified BSD license. NO WARRANTY.
#pragma warning(disable:4244)
#pragma warning(disable:4711)

#include <streams.h>
#include <stdio.h>
#include <olectl.h>
#include <dvdmedia.h>
#include "HybridCam.h"

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
//  CHybridCam is the source filter which masquerades as a capture device
//////////////////////////////////////////////////////////////////////////
CUnknown * WINAPI CHybridCam::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
    ASSERT(phr);
    CUnknown *punk = new CHybridCam(lpunk, phr);
    return punk;
}

CHybridCam::CHybridCam(LPUNKNOWN lpunk, HRESULT *phr) : 
    //CSource(NAME("HybridCam"), lpunk, CLSID_HybridCam)
		CTransformFilter(NAME("HybridCam"), lpunk, CLSID_HybridCam)
{
	//m_debugOut.exceptions(ios::failbit);
	//m_debugOut.open(VCAM1_DEBUG_FNAME, ios_base::out | ios_base::app );
	vcamOpenLog(10, "CHybridCam::CHybridCam");
	vcamLog(10, "***********************CHybridCam::CHybridCam***********************");
    ASSERT(phr);
    CAutoLock cAutoLock(&m_csFilter);
    // Create the one and only output pin
    m_pOutput = new CHybridCamStream(phr, this, L"HybridCam output pin");
    ASSERT(phr);


}

CHybridCam::~CHybridCam()
{
	vcamLog(10, "~~~~~~~~~~~~~~~~~~~~~~~CHybridCamStream::~CHybridCamStream~~~~~~~~~~~~~~~~~~~~~~~~~");
	//m_debugOut.close();
	vcamCloseLog(10, "CHybridCam::~CHybridCam");

}


HRESULT CHybridCam::QueryInterface(REFIID riid, void **ppv)
{
	HRESULT hr = S_OK;

	char guidString[GUID_STRING_MAXLEN];
	Riid2String(riid, guidString);

    //Forward request for IAMStreamConfig & IKsPropertySet to the pin
    if(riid == _uuidof(IAMStreamConfig) || riid == _uuidof(IKsPropertySet))
	{
		hr = m_pOutput->QueryInterface(riid, ppv);
		vcamLog(50, "CHybridCam::QueryInterface, riid = %s", guidString);
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
		hr = CTransformFilter::QueryInterface(riid, ppv);
		vcamLog(50, "CHybridCam::QueryInterface (to CTransformFilter), riid = %s", guidString);
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


STDMETHODIMP CHybridCam::JoinFilterGraph(
		__inout_opt IFilterGraph * pGraph,
		__in_opt LPCWSTR pName)
{
	vcamLog(10, "CHybridCam::JoinFilterGraph");
	HRESULT hr = CBaseFilter::JoinFilterGraph(pGraph, pName);

	//string msg("");
	//if (CBaseFilter::m_pGraph==NULL) {
	//	msg.append("after JoinFilterGraph, CBaseFilter::m_pGraph==NULL");
	//} else {
	//	msg.append("after JoinFilterGraph, CBaseFilter::m_pGraph!=NULL");
	//}
	//Msg((TCHAR*) msg.c_str());


	vcamLog(10, "       CHybridCam::JoinFilterGraph returning %d", (int) hr);
	return hr;
}

//void CHybridCam::vcamLog(10, const char* message) 
//{
//	return;
//	m_debugOut << LOG_PREFIX << message << endl;
//}
//
//void CHybridCam::vcamLog(10, TCHAR *szFormat, ...)
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
CBasePin *CHybridCam::GetPin(int n)
{
	CBasePin *pin = NULL;
 	vcamLog(50, "CHybridCam::GetPin, n = %d", n);
	if (n==0) {
		pin = m_pOutput;
	}
	//else if (n==1) {
 //       return m_pOutput;
 //   }
	else {
        return NULL;
    }
	//CBasePin *pin = CTransformFilter::GetPin(n);
	ASSERT(pin!=NULL);
	if(pin->IsConnected()) {
		vcamLog(50, "  CHybridCam::GetPin: pin is connected");
	} else {
		vcamLog(50, "  CHybridCam::GetPin: pin is not connected");
	}
	return pin;
}





// 2. from CTransformFilter
int CHybridCam::GetPinCount()
{
	vcamLog(50, "CHybridCam::GetPinCount");
	return 1;
	//return CTransformFilter::GetPinCount();
}




// 3. from CBaseFilter
HRESULT CHybridCam::StreamTime(CRefTime& rtStream)
{
	vcamLog(50, "CHybridCam::StreamTime");
	return CTransformFilter::StreamTime(rtStream);
}

LONG CHybridCam::GetPinVersion()
{
	vcamLog(50, "CHybridCam::GetPinVersion");
	return CTransformFilter::GetPinVersion();
}

__out_opt LPAMOVIESETUP_FILTER CHybridCam::GetSetupData()
{
	vcamLog(50, "CHybridCam::GetSetupData");
	return CTransformFilter::GetSetupData();
}

HRESULT STDMETHODCALLTYPE CHybridCam::EnumPins(__out  IEnumPins **ppEnum)
{
	vcamLog(50, "CHybridCam::EnumPins");
	return CTransformFilter::EnumPins(ppEnum);
}

HRESULT STDMETHODCALLTYPE CHybridCam::FindPin(LPCWSTR Id, __out  IPin **ppPin)
{
	vcamLog(50, "CHybridCam::FindPin");
	return CTransformFilter::FindPin(Id, ppPin);
}

HRESULT STDMETHODCALLTYPE CHybridCam::QueryFilterInfo(__out  FILTER_INFO *pInfo)
{
	vcamLog(50, "CHybridCam::QueryFilterInfo");
	return CTransformFilter::QueryFilterInfo(pInfo);
}

HRESULT STDMETHODCALLTYPE CHybridCam::QueryVendorInfo(__out  LPWSTR *pVendorInfo)
{
	vcamLog(50, "CHybridCam::QueryVendorInfo");
	return CTransformFilter::QueryVendorInfo(pVendorInfo);
}


HRESULT STDMETHODCALLTYPE CHybridCam::Stop( void)
{
	vcamLog(50, "CHybridCam::Stop");
	//return CTransformFilter::Stop( );
	return CBaseFilter::Stop( );

}

HRESULT STDMETHODCALLTYPE CHybridCam::Pause( void)
{
	vcamLog(50, "CHybridCam::Pause");
	//return CTransformFilter::Pause( );

	return CBaseFilter::Pause( );


}

HRESULT STDMETHODCALLTYPE CHybridCam::Run(REFERENCE_TIME tStart)
{
	vcamLog(50, "CHybridCam::Run");
	//HRESULT hr = CTransformFilter::Run(tStart);
	HRESULT hr = CBaseFilter::Run( tStart);
	vcamLog(50, "       CHybridCam::Run returning %d", (int) hr);
	return hr;
}

HRESULT STDMETHODCALLTYPE CHybridCam::GetState(DWORD dwMilliSecsTimeout,   __out  FILTER_STATE *State)
{
	vcamLog(50, "CHybridCam::GetState");
	//return CTransformFilter::GetState(dwMilliSecsTimeout,   State);
	return CBaseFilter::GetState(dwMilliSecsTimeout,   State);

}

HRESULT STDMETHODCALLTYPE CHybridCam::SetSyncSource(__in_opt  IReferenceClock *pClock)
{
	vcamLog(50, "CHybridCam::SetSyncSource");
	return CTransformFilter::SetSyncSource(pClock);
}

HRESULT STDMETHODCALLTYPE CHybridCam::GetSyncSource(__deref_out_opt  IReferenceClock **pClock)
{
	vcamLog(50, "CHybridCam::GetSyncSource");
	return CTransformFilter::GetSyncSource(pClock);

}

STDMETHODIMP CHybridCam::GetClassID(__out CLSID *pClsID)
{
	vcamLog(50, "CHybridCam::GetClassID");
	return CTransformFilter::GetClassID(pClsID);
}

ULONG STDMETHODCALLTYPE CHybridCam::AddRef( void)
{
	//vcamLog(50, "CHybridCam::AddRef");
	return CTransformFilter::AddRef( );
}

ULONG STDMETHODCALLTYPE CHybridCam::Release( void)
{
	//vcamLog(50, "CHybridCam::Release");
	return CTransformFilter::Release( );
}

HRESULT STDMETHODCALLTYPE CHybridCam::Register( void)
{
	vcamLog(50, "CHybridCam::Register");
	return CTransformFilter::Register( );
}

HRESULT STDMETHODCALLTYPE CHybridCam::Unregister( void)
{
	vcamLog(50, "CHybridCam::Unregister");
	return CTransformFilter::Unregister( );
}

// Check the input type is OK - return an error otherwise
HRESULT CHybridCam::CheckInputType(const CMediaType *mtIn)
{
    return NOERROR;
 //   CheckPointer(mtIn,E_POINTER);

 //   // check this is a VIDEOINFOHEADER type
 //   if (!(*mtIn->FormatType() == FORMAT_VideoInfo || *mtIn->FormatType() == FORMAT_VideoInfo2)) {
 //       return E_INVALIDARG;
 //   }

 //   // Can we transform this type
 //   if (IsRgb24(mtIn)) {
 //       return NOERROR;
 //   }

	////vcamLog(10, "MultiCamFilter::CheckInputType, returning E_FAIL");
 //   return E_FAIL;
}

//
// Checktransform
//
// Check a transform can be done between these formats
//
HRESULT CHybridCam::CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut)
{
				return NOERROR;
 //   CheckPointer(mtIn,E_POINTER);
 //   CheckPointer(mtOut,E_POINTER);

 //   if (IsRgb24(mtIn)) 
 //   {
 //       if (OnlySizesDiffer(mtIn, mtOut) )
	//	{
	//		if (MatchesOutputSize(mtOut) )
	//		{
	//			return NOERROR;
	//		}
	//	}
	//}

	////vcamLog(10, "MultiCamFilter::CheckTransform, returning E_FAIL; requested media types were as follows");
	////vcamLog(10, "mtIn:");
	////vcamLogFormat(12, mtIn);
	////vcamLog(10, "mtOut:");
	////vcamLogFormat(12, mtOut);
	//
 //   return E_FAIL;

} // CheckTransform

//
// DecideBufferSize
//
// Tell the output pin's allocator what size buffers we
// require.
//
HRESULT CHybridCam::DecideBufferSize(IMemAllocator *pAlloc,ALLOCATOR_PROPERTIES *pProperties)
{
	return m_pOutput->DecideBufferSize(pAlloc, pProperties);
    //CheckPointer(pAlloc,E_POINTER);
    //CheckPointer(pProperties,E_POINTER);
    //HRESULT hr = NOERROR;

    //pProperties->cBuffers = 1;
    //pProperties->cbBuffer = m_pOutput->CurrentMediaType().GetSampleSize();
    //VCAM_ASSERT(pProperties->cbBuffer);

    //// Ask the allocator to reserve us some sample memory, NOTE the function
    //// can succeed (that is return NOERROR) but still not have allocated the
    //// memory that we requested, so we must check we got whatever we wanted

    //ALLOCATOR_PROPERTIES Actual;
    //hr = pAlloc->SetProperties(pProperties,&Actual);
    //if (FAILED(hr)) {
    //    return hr;
    //}

    //VCAM_ASSERT( Actual.cBuffers == 1 );

    //if (pProperties->cBuffers > Actual.cBuffers ||
    //        pProperties->cbBuffer > Actual.cbBuffer) {
    //            return E_FAIL;
    //}
    //return NOERROR;

} // DecideBufferSize

//
// GetMediaType
//
// I support one type, namely the type of the output pin
HRESULT CHybridCam::GetMediaType(int iPosition, CMediaType *pMediaType)
{
    if (iPosition < 0) {
        return E_INVALIDARG;
    }

    if (iPosition > 0) {
        return VFW_S_NO_MORE_ITEMS;
    }

    CheckPointer(pMediaType,E_POINTER);
    *pMediaType = m_pOutput->CurrentMediaType();

    return NOERROR;

} // GetMediaType









//////////////////////////////////////////////////////////////////////////
// CHybridCamStream is the one and only output pin of CHybridCam which handles 
// all the stuff.
//////////////////////////////////////////////////////////////////////////
CHybridCamStream::CHybridCamStream(HRESULT *phr, CHybridCam *pParent, LPCWSTR pPinName) :
    //CSourceStream(NAME("HybridCam"),phr, pParent, pPinName), 
		CTransformOutputPin(NAME("HybridCamStream"), (CTransformFilter *) pParent, phr, pPinName),
		m_pParent(pParent)
{
	vcamOpenLog(10, "CHybridCamStream::CHybridCamStream");

	//m_debugOut.exceptions(ios::failbit);
	//m_debugOut.open(VCAM1PIN_DEBUG_FNAME, ios_base::out | ios_base::app );
	vcamLog(10, "***********************CHybridCamStream::CHybridCamStream***********************");

    // Set the default media type as 320x240x24@15
    GetMediaType(4, &m_mt);

 //   string msg("");
	//if (this->m_pParent->GetGraph()==NULL) {
	//	msg.append("in CHybridCamStream::CHybridCamStream, cbasefilter::m_pgraph==null");
	//} else {
	//	msg.append("in CHybridCamStream::CHybridCamStream, cbasefilter::m_pgraph!=null");
	//}
	//Msg((TCHAR*) msg.c_str());
	
}

CHybridCamStream::~CHybridCamStream()
{
	vcamLog(10, "~~~~~~~~~~~~~~~~~~~~~~~CHybridCamStream::~CHybridCamStream~~~~~~~~~~~~~~~~~~~~~~~~~");
	vcamCloseLog(10, "CHybridCamStream::~CHybridCamStream");
	//m_debugOut.close();
} 

HRESULT CHybridCamStream::QueryInterface(REFIID riid, void **ppv)
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
		hr =  CTransformOutputPin::QueryInterface(riid, ppv);
		vcamLog(50, "CHybridCamStream::QueryInterface, riid = %s", guidString);
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

	vcamLog(50, "CHybridCamStream::QueryInterface, riid = %s", guidString);
	vcamLog(50, "       S_OK");

    return S_OK;
}


//////////////////////////////////////////////////////////////////////////
//  This is the routine where we create the data being output by the Virtual
//  Camera device.
//////////////////////////////////////////////////////////////////////////

HRESULT CHybridCamStream::FillBuffer(IMediaSample *pms)
{
	vcamLog(90, "CHybridCamStream::FillBuffer");
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
STDMETHODIMP CHybridCamStream::Notify(IBaseFilter * pSender, Quality q)
{
	vcamLog(50, "CHybridCamStream::Notify");
    return E_NOTIMPL;
} // Notify

//////////////////////////////////////////////////////////////////////////
// This is called when the output format has been negotiated
//////////////////////////////////////////////////////////////////////////
HRESULT CHybridCamStream::SetMediaType(const CMediaType *pmt)
{
	vcamLog(50, "CHybridCamStream::SetMediaType");
    //DECLARE_PTR(VIDEOINFOHEADER, pvi, pmt->Format());
    //HRESULT hr = CTransformOutputPin::SetMediaType(pmt);
    HRESULT hr = CBasePin::SetMediaType(pmt);
    return hr;
}

// See Directshow help topic for IAMStreamConfig for details on this method
HRESULT CHybridCamStream::GetMediaType(int iPosition, CMediaType *pmt)
{
	vcamLog(50, "CHybridCamStream::GetMediaType, iPosition = %d", iPosition);

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
    
	vcamLog(50, "CHybridCamStream::GetMediaType: got media type:");
	vcamLogFormat(50, pmt);

	vcamLog(50, "    GetMediaType returning NOERROR (0x%x)", NOERROR);
    return NOERROR;

} // GetMediaType

// This method is called to see if a given output format is supported
HRESULT CHybridCamStream::CheckMediaType(const CMediaType *pMediaType)
{
	vcamLog(50, "CHybridCamStream::CheckMediaType");
    //VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *)(pMediaType->Format());

	if(*pMediaType != m_mt) 
        return E_INVALIDARG;


    return S_OK;
} // CheckMediaType

// This method is called after the pins are connected to allocate buffers to stream data
HRESULT CHybridCamStream::DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProperties)
{
	vcamLog(50, "CHybridCamStream::DecideBufferSize");
    CAutoLock cAutoLock(m_pFilter->pStateLock());
    HRESULT hr = NOERROR;

    VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *) m_mt.Format();
    pProperties->cBuffers = 1;
    pProperties->cbBuffer = pvi->bmiHeader.biSizeImage;

    ALLOCATOR_PROPERTIES Actual;
    hr = pAlloc->SetProperties(pProperties,&Actual);

    if(FAILED(hr)) 
	{
		vcamLog(50, "   CHybridCamStream::DecideBufferSize returning FAILED");
		return hr;
	}
    if(Actual.cbBuffer < pProperties->cbBuffer) 
	{
		vcamLog(50, "   CHybridCamStream::DecideBufferSize returning E_FAIL");
		return E_FAIL;
	}

	vcamLog(50, "   CHybridCamStream::DecideBufferSize returning NOERROR");
    return NOERROR;
} // DecideBufferSize

// Called when graph is run
HRESULT CHybridCamStream::OnThreadCreate()
{
	vcamLog(50, "CHybridCamStream::OnThreadCreate");
    m_rtLastTime = 0;
    return NOERROR;
} // OnThreadCreate


//////////////////////////////////////////////////////////////////////////
//  IAMStreamConfig
//////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE CHybridCamStream::SetFormat(AM_MEDIA_TYPE *pmt)
{
	vcamLog(50, "CHybridCamStream::SetFormat");
    //DECLARE_PTR(VIDEOINFOHEADER, pvi, m_mt.pbFormat);
    m_mt = *pmt;
    IPin* pin; 
    ConnectedTo(&pin);
    if(pin)
    {
        IFilterGraph *pGraph = m_pParent->GetGraph();
        pGraph->Reconnect(this);
		vcamLog(50, "   CHybridCamStream::SetFormat returning S_OK (reconnected to graph)");
    } else {
		vcamLog(50, "   CHybridCamStream::SetFormat returning S_OK (pin not connected)");
	}
	SafeRelease(&pin);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CHybridCamStream::GetFormat(AM_MEDIA_TYPE **ppmt)
{
	vcamLog(50, "CHybridCamStream::GetFormat");
    *ppmt = CreateMediaType(&m_mt);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CHybridCamStream::GetNumberOfCapabilities(int *piCount, int *piSize)
{
	*piCount = 8;
    *piSize = sizeof(VIDEO_STREAM_CONFIG_CAPS);

	vcamLog(50, "CHybridCamStream::GetNumberOfCapabilities, Count = %d, Size = %d", *piCount, *piSize);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CHybridCamStream::GetStreamCaps(int iIndex, AM_MEDIA_TYPE **pmt, BYTE *pSCC)
{
	vcamLog(50, "CHybridCamStream::GetStreamCaps, iIndex = %d", iIndex);
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


HRESULT CHybridCamStream::Set(REFGUID guidPropSet, DWORD dwID, void *pInstanceData, 
                        DWORD cbInstanceData, void *pPropData, DWORD cbPropData)
{// Set: Cannot set any properties.
	vcamLog(50, "CHybridCamStream::Set");
    return E_NOTIMPL;
}

// Get: Return the pin category (our only property). 
HRESULT CHybridCamStream::Get(
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
	vcamLog(50, "CHybridCamStream::Get, guidPropSet = %s, dwPropID = %d", guidString, dwPropID);

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
HRESULT CHybridCamStream::QuerySupported(REFGUID guidPropSet, DWORD dwPropID, DWORD *pTypeSupport)
{
	vcamLog(50, "CHybridCamStream::QuerySupported");
    if (guidPropSet != AMPROPSETID_Pin) return E_PROP_SET_UNSUPPORTED;
    if (dwPropID != AMPROPERTY_PIN_CATEGORY) return E_PROP_ID_UNSUPPORTED;
    // We support getting this property, but not setting it.
    if (pTypeSupport) *pTypeSupport = KSPROPERTY_SUPPORT_GET; 
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CHybridCamStream::QueryPinInfo( 
            /* [annotation][out] */ 
            __out  PIN_INFO *pInfo)
{
  vcamLog(50, "CHybridCamStream::QueryPinInfo");
  HRESULT hr = S_OK;
  hr = CTransformOutputPin::QueryPinInfo(pInfo);

  if(pInfo->dir==PINDIR_INPUT) {
	  vcamLog(50, "   PINDIR_INPUT");
  }
  if(pInfo->dir==PINDIR_OUTPUT) {
	  vcamLog(50, "   PINDIR_OUTPUT");
  }
  return hr;
}



STDMETHODIMP  CHybridCamStream::NonDelegatingQueryInterface(REFIID riid, __deref_out void **ppv)
{
	vcamLog(50, "CHybridCamStream::NonDelegatingQueryInterface");
	HRESULT hr = S_OK;
	hr = CBasePin::NonDelegatingQueryInterface(riid, ppv);
	//hr = CTransformOutputPin::NonDelegatingQueryInterface(riid, ppv);
	
	char guidString[GUID_STRING_MAXLEN];
	Riid2String(riid, guidString);
	vcamLog(50, "CHybridCamStream::NonDelegatingQueryInterface, riid = %s", guidString);
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

//void CHybridCamStream::vcamLog(50, const char* message) 
//{
//	return;
//	m_debugOut << LOG_PREFIX << message << endl;
//}
//
//void CHybridCamStream::vcamLog(50, TCHAR *szFormat, ...)
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

HRESULT STDMETHODCALLTYPE CHybridCamStream::ConnectedTo(/* [annotation][out] */ 
	__out  IPin **pPin)
{
	vcamLog(50, "CHybridCamStream::ConnectedTo");
	HRESULT hr = CTransformOutputPin::ConnectedTo(pPin);
	if (hr == S_OK) {
		vcamLog(50, "   S_OK");
	} else {
		vcamLog(50, "   not S_OK");
	}
	return hr;
}

HRESULT STDMETHODCALLTYPE CHybridCamStream::EnumMediaTypes( 
	/* [annotation][out] */ 
	__out  IEnumMediaTypes **ppEnum)
{
	vcamLog(50, "CHybridCamStream::EnumMediaTypes");
	HRESULT hr = CTransformOutputPin::EnumMediaTypes(ppEnum);
	if (hr == S_OK) {
		vcamLog(50, "   S_OK");
	} else if (hr == VFW_E_NOT_CONNECTED) {
		vcamLog(50, "   VFW_E_NOT_CONNECTED");
	} else {
		vcamLog(50, "   error");
	}
	return hr;
}

LONG  CHybridCamStream::GetMediaTypeVersion()
{
	vcamLog(50, "CHybridCamStream::GetMediaTypeVersion");
	return CTransformOutputPin::GetMediaTypeVersion();
}

HRESULT  CHybridCamStream::CompleteConnect(IPin *pReceivePin)
{
	vcamLog(50, "CHybridCamStream::CompleteConnect");
	HRESULT hr = CTransformOutputPin::CompleteConnect(pReceivePin);
	vcamLog(50, "     CompleteConnect returning %d", (int) hr);
	return hr;
}

HRESULT  CHybridCamStream::CheckConnect(IPin *pPin)
{
	vcamLog(50, "CHybridCamStream::CheckConnect");
	HRESULT hr = m_pTransformFilter->CheckConnect(PINDIR_OUTPUT,pPin);
    if (FAILED(hr)) {
		vcamLog(50, "     CheckConnect returning %d", (int) hr);
	    return hr;
    }
    hr = CBaseOutputPin::CheckConnect(pPin);
	//HRESULT hr = CTransformOutputPin::CheckConnect(pPin);
	vcamLog(50, "     CheckConnect returning %d", (int) hr);
	return hr;
}

HRESULT  CHybridCamStream::BreakConnect()
{
	vcamLog(50, "CHybridCamStream::BreakConnect");
	HRESULT hr = CTransformOutputPin::BreakConnect();
	vcamLog(50, "     BreakConnect returning %d", (int) hr);
	return hr;
}

HRESULT STDMETHODCALLTYPE CHybridCamStream::Connect( 
	/* [in] */ IPin *pReceivePin,
	/* [annotation][in] */ 
	__in_opt  const AM_MEDIA_TYPE *pmt)
{
	HRESULT hr = S_OK;
	vcamLog(50, "!!!!!!!!!!!!!!!CHybridCamStream::Connect");

	hr = CTransformOutputPin::Connect( pReceivePin, pmt);

	//vcamLog(50, "         CHybridCamStream::Connect returning %d", (int) hr);
	if(hr==S_OK) {
		vcamLog(50, "         CHybridCamStream::Connect returning S_OK");
	}
	if(hr==VFW_E_ALREADY_CONNECTED) {
		vcamLog(50, "         CHybridCamStream::Connect returning VFW_E_ALREADY_CONNECTED");
	}
	if(hr==VFW_E_NO_ACCEPTABLE_TYPES) {
		vcamLog(50, "         CHybridCamStream::Connect returning VFW_E_NO_ACCEPTABLE_TYPES");
	}
	if(hr==VFW_E_NO_TRANSPORT) {
		vcamLog(50, "         CHybridCamStream::Connect returning VFW_E_NO_TRANSPORT");
	}
	if(hr==VFW_E_NOT_STOPPED) {
		vcamLog(50, "         CHybridCamStream::Connect returning VFW_E_NOT_STOPPED");
	}
	if(hr==VFW_E_TYPE_NOT_ACCEPTED) {
		vcamLog(50, "         CHybridCamStream::Connect returning VFW_E_TYPE_NOT_ACCEPTED");
	}
	return hr;
}

HRESULT STDMETHODCALLTYPE CHybridCamStream::ReceiveConnection( 
	/* [in] */ IPin *pConnector,
	/* [in] */ const AM_MEDIA_TYPE *pmt)
{
	vcamLog(50, "CHybridCamStream::ReceiveConnection");
	return CTransformOutputPin::ReceiveConnection( pConnector,pmt);
}

HRESULT STDMETHODCALLTYPE CHybridCamStream::Disconnect( void)
{
	vcamLog(50, "CHybridCamStream::Disconnect");
	return CTransformOutputPin::Disconnect( );
}


HRESULT STDMETHODCALLTYPE CHybridCamStream::ConnectionMediaType( 
	/* [annotation][out] */ 
	__out  AM_MEDIA_TYPE *pmt)
{
	vcamLog(50, "CHybridCamStream::ConnectionMediaType");
	return CTransformOutputPin::ConnectionMediaType(pmt);
}


HRESULT STDMETHODCALLTYPE CHybridCamStream::QueryDirection( 
	/* [annotation][out] */ 
	__out  PIN_DIRECTION *pPinDir)
{
	vcamLog(50, "CHybridCamStream::QueryDirection");
	return CTransformOutputPin::QueryDirection(pPinDir);
}

HRESULT STDMETHODCALLTYPE CHybridCamStream::QueryId( 
	/* [annotation][out] */ 
	__out  LPWSTR *Id)
{
	vcamLog(50, "CHybridCamStream::QueryId");
	return CTransformOutputPin::QueryId(Id);
}

HRESULT STDMETHODCALLTYPE CHybridCamStream::QueryAccept( 
	/* [in] */ const AM_MEDIA_TYPE *pmt)
{
	vcamLog(50, "CHybridCamStream::QueryAccept");
	return CTransformOutputPin::QueryAccept(pmt);
}


HRESULT STDMETHODCALLTYPE CHybridCamStream::QueryInternalConnections( 
	/* [annotation][out] */ 
	__out_ecount_part_opt(*nPin, *nPin)  IPin **apPin,
	/* [out][in] */ ULONG *nPin)
{
	vcamLog(50, "CHybridCamStream::QueryInternalConnections");
	return CTransformOutputPin::QueryInternalConnections(apPin,nPin);
}

HRESULT STDMETHODCALLTYPE CHybridCamStream::EndOfStream( void)
{
	vcamLog(50, "CHybridCamStream::EndOfStream");
	return CTransformOutputPin::EndOfStream( );
}

HRESULT STDMETHODCALLTYPE CHybridCamStream::BeginFlush( void)
{
	vcamLog(50, "CHybridCamStream::BeginFlush");
	return CTransformOutputPin::BeginFlush( );
}

HRESULT STDMETHODCALLTYPE CHybridCamStream::EndFlush( void)
{
	vcamLog(50, "CHybridCamStream::EndFlush");
	return CTransformOutputPin::EndFlush( );
}

HRESULT STDMETHODCALLTYPE CHybridCamStream::NewSegment( 
	/* [in] */ REFERENCE_TIME tStart,
	/* [in] */ REFERENCE_TIME tStop,
	/* [in] */ double dRate)
{
	vcamLog(50, "CHybridCamStream::NewSegment");
	return CTransformOutputPin::NewSegment(tStart,tStop, dRate);
}

HRESULT CHybridCamStream::Active(void)  {
	vcamLog(50, "CHybridCamStream::Active");

	HRESULT hr = CTransformOutputPin::Active();
	vcamLog(50, "         CHybridCamStream::Active returning %d", (int) hr);
	return hr;
}

//STDMETHODIMP CHybridCamStream::QueryInterface(REFIID riid, __deref_out void **ppv) 
//{      
//	HRESULT hr = S_OK;
//
//	char guidString[GUID_STRING_MAXLEN];
//	Riid2String(riid, guidString);
//
//	hr = GetOwner()->QueryInterface(riid,ppv);
//
//	vcamLog(10, "MultiCamOutputPin::QueryInterface, riid = %s", guidString);
//	if (hr==S_OK) {
//		vcamLog(10, "       MultiCamOutputPin::QueryInterface returning S_OK");
//	}
//	else if (hr==E_NOINTERFACE ) {
//		vcamLog(10, "       MultiCamOutputPin::QueryInterface returning E_NOINTERFACE ");
//	}
//	else if (hr==E_POINTER) {
//		vcamLog(10, "       MultiCamOutputPin::QueryInterface returning E_POINTER");
//	} else {
//		vcamLog(10, "       MultiCamOutputPin::QueryInterface returning 0x%x", hr);
//	}
//
//	return hr;            
//};
//
//STDMETHODIMP CHybridCamStream::NonDelegatingQueryInterface(REFIID riid, void **ppv)
//{
//	HRESULT hr = S_OK;
//	CheckPointer(ppv,E_POINTER);
//
//	if (riid == IID_IKsPropertySet) {
//        hr = GetInterface((IKsPropertySet *) this, ppv);
//		goto done;
//    } else if (riid == IID_IAMStreamConfig) {
//        hr =  GetInterface((IAMStreamConfig *) this, ppv);
//		goto done;
//    } else {
//        hr =  CTransformOutputPin::NonDelegatingQueryInterface(riid, ppv);
//		goto done;
//    }
//
//done:
//	char guidString[GUID_STRING_MAXLEN];
//	Riid2String(riid, guidString);
//	vcamLog(10, "MultiCamOutputPin::NonDelegatingQueryInterface, riid = %s", guidString);
//	if (hr==NOERROR) {
//		vcamLog(10, "       MultiCamOutputPin::NonDelegatingQueryInterface returning NOERROR");
//	}
//	else if (hr==E_NOINTERFACE ) {
//		vcamLog(10, "       MultiCamOutputPin::NonDelegatingQueryInterface returning E_NOINTERFACE ");
//	}
//	else if (hr==E_POINTER) {
//		vcamLog(10, "       MultiCamOutputPin::NonDelegatingQueryInterface returning E_POINTER");
//	} else {
//		vcamLog(10, "       MultiCamOutputPin::NonDelegatingQueryInterface returning 0x%x", hr);
//	}
//
//	return hr;
//} // NonDelegatingQueryInterface
