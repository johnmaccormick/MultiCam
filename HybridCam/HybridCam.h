// Downloaded from http://tmhare.mvps.org/downloads.htm
// Altered by John MacCormick, 2012.
// Alterations are released under modified BSD license. NO WARRANTY.
#pragma once

#include <iostream>
#include <fstream>
using namespace std;


#define DECLARE_PTR(type, ptr, expr) type* ptr = (type*)(expr);

EXTERN_C const GUID CLSID_HybridCam;

class CHybridCamStream;
class CHybridCam : public CSource
{
public:
    //////////////////////////////////////////////////////////////////////////
    //  IUnknown
    //////////////////////////////////////////////////////////////////////////
    static CUnknown * WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);

    IFilterGraph *GetGraph() {return m_pGraph;}

	STDMETHODIMP JoinFilterGraph(
		__inout_opt IFilterGraph * pGraph,
		__in_opt LPCWSTR pName);

	///////// jmac ////////
	//void log(const char* message) ;
	//void log2(TCHAR *szFormat, ...);
	virtual ~CHybridCam();
	private:
	//ofstream m_debugOut;
	////////////////////////////// end jmac /////////////

private:
    CHybridCam(LPUNKNOWN lpunk, HRESULT *phr);







///////////////////////////////////////////////////////////
// all inherited virtual functions
///////////////////////////////////////////////////////////

public:
// 1. from ??
	virtual CBasePin *GetPin(int n);

#ifdef PERF
    virtual void RegisterPerfId()
         {m_idTransInPlace = MSR_REGISTER(TEXT("Vcam"));}
#endif // PERF

// 2. from ??
	virtual int GetPinCount();
// 3. from CBaseFilter
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
	virtual STDMETHODIMP GetClassID(__out CLSID *pClsID);
	virtual ULONG STDMETHODCALLTYPE AddRef( void);
	virtual ULONG STDMETHODCALLTYPE Release( void);
	virtual HRESULT STDMETHODCALLTYPE Register( void);
    virtual HRESULT STDMETHODCALLTYPE Unregister( void);




};

class CHybridCamStream : public CSourceStream, public IAMStreamConfig, public IKsPropertySet
{
public:

    //////////////////////////////////////////////////////////////////////////
    //  IUnknown
    //////////////////////////////////////////////////////////////////////////
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef() { return GetOwner()->AddRef(); }                                                          \
    STDMETHODIMP_(ULONG) Release() { return GetOwner()->Release(); }

    //////////////////////////////////////////////////////////////////////////
    //  IQualityControl
    //////////////////////////////////////////////////////////////////////////
    STDMETHODIMP Notify(IBaseFilter * pSender, Quality q);

    //////////////////////////////////////////////////////////////////////////
    //  IAMStreamConfig
    //////////////////////////////////////////////////////////////////////////
    HRESULT STDMETHODCALLTYPE SetFormat(AM_MEDIA_TYPE *pmt);
    HRESULT STDMETHODCALLTYPE GetFormat(AM_MEDIA_TYPE **ppmt);
    HRESULT STDMETHODCALLTYPE GetNumberOfCapabilities(int *piCount, int *piSize);
    HRESULT STDMETHODCALLTYPE GetStreamCaps(int iIndex, AM_MEDIA_TYPE **pmt, BYTE *pSCC);

    //////////////////////////////////////////////////////////////////////////
    //  IKsPropertySet
    //////////////////////////////////////////////////////////////////////////
    HRESULT STDMETHODCALLTYPE Set(REFGUID guidPropSet, DWORD dwID, void *pInstanceData, DWORD cbInstanceData, void *pPropData, DWORD cbPropData);
    HRESULT STDMETHODCALLTYPE Get(REFGUID guidPropSet, DWORD dwPropID, void *pInstanceData,DWORD cbInstanceData, void *pPropData, DWORD cbPropData, DWORD *pcbReturned);
    HRESULT STDMETHODCALLTYPE QuerySupported(REFGUID guidPropSet, DWORD dwPropID, DWORD *pTypeSupport);
    
    //////////////////////////////////////////////////////////////////////////
    //  IPin
    //////////////////////////////////////////////////////////////////////////
	virtual HRESULT STDMETHODCALLTYPE QueryPinInfo( 
		/* [annotation][out] */ 
		__out  PIN_INFO *pInfo);

		virtual HRESULT STDMETHODCALLTYPE Connect( 
		/* [in] */ IPin *pReceivePin,
		/* [annotation][in] */ 
		__in_opt  const AM_MEDIA_TYPE *pmt);

	virtual HRESULT STDMETHODCALLTYPE ReceiveConnection( 
		/* [in] */ IPin *pConnector,
		/* [in] */ const AM_MEDIA_TYPE *pmt);

	virtual HRESULT STDMETHODCALLTYPE Disconnect( void);

	virtual HRESULT STDMETHODCALLTYPE ConnectedTo( 
		/* [annotation][out] */ 
		__out  IPin **pPin);

	virtual HRESULT STDMETHODCALLTYPE ConnectionMediaType( 
		/* [annotation][out] */ 
		__out  AM_MEDIA_TYPE *pmt);

	virtual HRESULT STDMETHODCALLTYPE QueryDirection( 
		/* [annotation][out] */ 
		__out  PIN_DIRECTION *pPinDir);

	virtual HRESULT STDMETHODCALLTYPE QueryId( 
		/* [annotation][out] */ 
		__out  LPWSTR *Id);

	virtual HRESULT STDMETHODCALLTYPE QueryAccept( 
		/* [in] */ const AM_MEDIA_TYPE *pmt);

	virtual HRESULT STDMETHODCALLTYPE EnumMediaTypes( 
		/* [annotation][out] */ 
		__out  IEnumMediaTypes **ppEnum);

	virtual HRESULT STDMETHODCALLTYPE QueryInternalConnections( 
		/* [annotation][out] */ 
		__out_ecount_part_opt(*nPin, *nPin)  IPin **apPin,
		/* [out][in] */ ULONG *nPin);

	virtual HRESULT STDMETHODCALLTYPE EndOfStream( void);

	virtual HRESULT STDMETHODCALLTYPE BeginFlush( void);

	virtual HRESULT STDMETHODCALLTYPE EndFlush( void);

	virtual HRESULT STDMETHODCALLTYPE NewSegment( 
		/* [in] */ REFERENCE_TIME tStart,
		/* [in] */ REFERENCE_TIME tStop,
		/* [in] */ double dRate);


    //////////////////////////////////////////////////////////////////////////
    //  CSourceStream
    //////////////////////////////////////////////////////////////////////////
    CHybridCamStream(HRESULT *phr, CHybridCam *pParent, LPCWSTR pPinName);
    virtual ~CHybridCamStream();

    HRESULT FillBuffer(IMediaSample *pms);
    HRESULT DecideBufferSize(IMemAllocator *pIMemAlloc, ALLOCATOR_PROPERTIES *pProperties);
    HRESULT CheckMediaType(const CMediaType *pMediaType);
    HRESULT GetMediaType(int iPosition, CMediaType *pmt);
    HRESULT SetMediaType(const CMediaType *pmt);
    HRESULT OnThreadCreate(void);
  
	///////// jmac ////////
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, __deref_out void **ppv);
	//void log(const char* message) ;
	//void log2(TCHAR *szFormat, ...);
	private:
	//ofstream m_debugOut;
	LONG GetMediaTypeVersion();
	HRESULT CompleteConnect(IPin *pReceivePin);
	HRESULT CheckConnect(IPin *pPin);
	HRESULT BreakConnect();
	protected:
	HRESULT Active(void);

	////////////////////////////// end jmac /////////////


private:
    CHybridCam *m_pParent;
    REFERENCE_TIME m_rtLastTime;
    HBITMAP m_hLogoBmp;
    CCritSec m_cSharedState;
    IReferenceClock *m_pClock;

};


