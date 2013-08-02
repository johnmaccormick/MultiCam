// Copyright John MacCormick 2012. Modified BSD license. NO WARRANTY.
//------------------------------------------------------------------------------
// Portions of this file are derived from the DirectShow sample code file EZRGB24.h.
// Those portions are copyright (c) Microsoft Corporation.
//------------------------------------------------------------------------------

#pragma once

// needs:
//#include <iostream>
//#include <fstream>
//#include <vector>
#include <ctime>
#include <map>
#include <string>
#include "DeviceList.h"
using namespace std;

#define MULTICAM_NUM_INPUT_PINS 6
#define MULTICAM_NUM_PINS (MULTICAM_NUM_INPUT_PINS + 1)
#define MULTICAM_FAIL ((HRESULT) -1L)

class MultiCamFilter : public CTransformFilter
{
friend class MultiCamOutputPin;
friend class COverlayInputPin;
friend class CSwitchMsgWindow;

public:

	//DECLARE_IUNKNOWN;
	STDMETHODIMP QueryInterface(REFIID riid, __deref_out void **ppv);                                                          
    STDMETHODIMP_(ULONG) AddRef() {                             
        return GetOwner()->AddRef();                            
    };                                                          
    STDMETHODIMP_(ULONG) Release() {                            
        return GetOwner()->Release();                           
    };

    static CUnknown * WINAPI CreateInstance(LPUNKNOWN punk, HRESULT *phr);

    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

    // Overrriden from CTransformFilter base class
    HRESULT Transform(IMediaSample *pIn, IMediaSample *pOut);
    HRESULT CheckInputType(const CMediaType *mtIn);
    HRESULT CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut);
    HRESULT DecideBufferSize(IMemAllocator *pAlloc,
                             ALLOCATOR_PROPERTIES *pProperties);
    HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);
	virtual CBasePin * GetPin(int n);

	STDMETHODIMP GetClassID(CLSID *pClsid);

	// A bunch of methods that are used in vcam, so implement them here but pass straight through to CBaseFilter, or whatever is appropriate
	virtual int GetPinCount();
	virtual HRESULT StreamTime(CRefTime& rtStream);
	virtual LONG GetPinVersion();
	virtual __out_opt LPAMOVIESETUP_FILTER GetSetupData();
	virtual HRESULT STDMETHODCALLTYPE EnumPins(__out  IEnumPins **ppEnum);
	virtual HRESULT STDMETHODCALLTYPE FindPin(LPCWSTR Id, __out  IPin **ppPin);
	virtual HRESULT STDMETHODCALLTYPE QueryFilterInfo(__out  FILTER_INFO *pInfo);
	virtual HRESULT STDMETHODCALLTYPE QueryVendorInfo(__out  LPWSTR *pVendorInfo);
	virtual HRESULT STDMETHODCALLTYPE Stop( void);
	virtual HRESULT STDMETHODCALLTYPE Pause( void);
	virtual HRESULT STDMETHODCALLTYPE Run( 
		REFERENCE_TIME tStart);
	virtual HRESULT STDMETHODCALLTYPE GetState( 
		/* [in] */ DWORD dwMilliSecsTimeout,
		/* [annotation][out] */ 
		__out  FILTER_STATE *State);
	virtual HRESULT STDMETHODCALLTYPE SetSyncSource( 
		/* [annotation][in] */ 
		__in_opt  IReferenceClock *pClock);
	virtual HRESULT STDMETHODCALLTYPE GetSyncSource( 
		/* [annotation][out] */ 
		__deref_out_opt  IReferenceClock **pClock);
	virtual HRESULT STDMETHODCALLTYPE Register( void);
    virtual HRESULT STDMETHODCALLTYPE Unregister( void);

protected:

    // Constructor
    MultiCamFilter(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr);

private:
    BOOL IsRgb24(const CMediaType *pMediaType) const;
    HRESULT Copy(IMediaSample *pSource, IMediaSample *pDest);

    const long m_lBufferRequest;        // The number of buffers to use

/////////////////////////////////////////////////////////////////////////////
/////////////////   jmac changes 
/////////////////////////////////////////////////////////////////////////////

public:
	virtual ~MultiCamFilter();
	STDMETHODIMP JoinFilterGraph(__inout_opt IFilterGraph * pGraph, __in_opt LPCWSTR pName);
	virtual HRESULT Receive(IMediaSample *pSample, int ID);

	static LRESULT CALLBACK JmacKeyboardProc2(int code, WPARAM wParam, LPARAM lParam);

	double getAverageFrameRate();


protected:
	int m_iNumUpstreamFilters;
	int m_iNumValidFilters;
	int m_iMainInputID;
	CSwitchMsgWindow* m_pSwitchMsgWindow;
	BOOL m_fViewAllCameras;
	BOOL m_fOverlayCameras;
	
	HRESULT ConnectUpstream();
	HRESULT DisconnectUpstream();
	virtual HRESULT InitializeInputPins();
	virtual HRESULT InitializeOutputPin();
	HRESULT JoinTemporaryGraph();
	HRESULT DestroyTemporaryGraph();
	HRESULT Overlay(int inID, IMediaSample *pOut, /*inout*/ int &xStart);
	CCritSec* GetcsReceive() {return &m_csReceive;};
	MultiCamOutputPin* GetOutputPin() {return (MultiCamOutputPin*) m_pOutput;};
	COverlayInputPin* GetInputPin(int i) const {return m_ppInputs[i];};
	HRESULT StoreSampleForOverlay(IMediaSample *pSample, int ID);
	HRESULT MultiCamFilter::AdvanceInput();
	HRESULT SwitchInput(int ID);
	HRESULT GetLargestInputDetails(/*out*/ LONG& max_width, /*out*/ LONG& max_height);
	static BOOL OnlySizesDiffer(const CMediaType *pmt1, const CMediaType *pmt2) ;
	static BOOL MultiCamFilter::IsEqualRect(const RECT r1, const RECT r2) ;
	BOOL MatchesOutputSize(const CMediaType *pmt) ;
	HRESULT CopyBitmap(IMediaSample *pIn, IMediaSample *pOut);
	HRESULT MultiCamFilter::FillBitmap(IMediaSample *pIn, IMediaSample *pOut) ;
	HRESULT MultiCamFilter::OverlayCameras(IMediaSample *pOut) ;
	HRESULT MultiCamFilter::MakeAllCamerasBitmap(IMediaSample *pOut) ;
	//HRESULT MultiCamFilter::Overlay(int inID, int out_scale, IMediaSample *pOut, 
	///*inout*/ int &xStart, /*inout*/ int &yStart);
	HRESULT MultiCamFilter::OverlayB(int inID, IMediaSample *pOut, 
	int xStart, int yStart, int xEnd, int yEnd);
	BOOL MultiCamFilter::IsAcceptableDevice(DeviceList& deviceList, map<string, int>& friendlyNames, 
		string& friendlyName, GUID classID);
	void MultiCamFilter::ExtendDeviceName(string& friendlyName, int deviceCount);

	BOOL MultiCamFilter::IsAcceptableCameraGUID(GUID guid);
	BOOL MultiCamFilter::IsAcceptableCameraName(string& friendlyName);
	HRESULT MultiCamFilter::InitializeUpstreamInterfaces();
	HRESULT MultiCamFilter::LogCapabilities(int index);
	HRESULT MultiCamFilter::LogCapabilities() ;
	HRESULT MultiCamFilter::LogCapability(IAMStreamConfig *pConfig, int index, int size) ;
	HRESULT MultiCamFilter::SetUpstreamFormats() ;
	HRESULT MultiCamFilter::SetUpstreamFormat(int index) ;
	HRESULT MultiCamFilter::GetCapabilityImageSize(IAMStreamConfig *pConfig, int index, int size, 
			BOOL requireRGB24,
		/*out*/ LONG *width, /*out*/ LONG *height) ;
	HRESULT MultiCamFilter::ChooseBestHeight(IAMStreamConfig *pConfig, 
			int filterIndex, int count,	int size,
			BOOL requireRGB24,
			/*out*/ int *bestIndex, /*out*/ LONG *bestHeight);
	HRESULT MultiCamFilter::GetCapabilityFormat(IAMStreamConfig *pConfig, int index, int size, 
	/*out*/ AM_MEDIA_TYPE** ppmt) ;
    HRESULT ConnectionMediaType(/*out*/ AM_MEDIA_TYPE *pmt);
	HRESULT MultiCamFilter::CreateMockMediaType(/*out*/ AM_MEDIA_TYPE *pmt);
	HRESULT MultiCamFilter::StartUp(BOOL inConstructor);
	HRESULT MultiCamFilter::Shutdown(BOOL inDestructor);
	//HRESULT MultiCamFilter::Reset();
	int MultiCamFilter::NextValidFilter(int index);
	int MultiCamFilter::NextOrCurrentValidFilter(int index);
	void MultiCamFilter::insertFrameRate(double newFrameRate);
	void MultiCamFilter::logFrameRate();



	IBaseFilter **m_ppUpstreamFilter;
	IMediaSample **m_ppOverlays;
	COverlayInputPin **m_ppInputs;

	vector<HHOOK> m_hooks;
	vector<string> m_cameraFriendlyNames;

	static const LONG s_IDEAL_CAMERA_HEIGHT;
	static const LONG s_MAX_CAMERA_HEIGHT;
	int m_Width, m_Height;

	
	long m_frameCounter;
	clock_t m_lastFrameTime;
#define NUM_FRAME_RATES 5
	double m_frameRates[NUM_FRAME_RATES];

}; // MultiCamFilter

