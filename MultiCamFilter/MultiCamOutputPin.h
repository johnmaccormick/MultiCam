// Copyright John MacCormick 2012. Modified BSD license. NO WARRANTY.
#pragma once
// needs:
//#include "transfrm.h"
//#include <iostream>
//#include <fstream>
//using namespace std;

class MultiCamOutputPin :
	public CTransformOutputPin, public IAMStreamConfig, public IKsPropertySet
{
	friend class MultiCamFilter;

public:
	STDMETHODIMP QueryInterface(REFIID riid, __deref_out void **ppv);                                                          
    STDMETHODIMP_(ULONG) AddRef() {                             
        return GetOwner()->AddRef();                            
    };                                                          
    STDMETHODIMP_(ULONG) Release() {                            
        return GetOwner()->Release();                           
    };

	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

	MultiCamOutputPin(__in_opt LPCTSTR     pObjectName,
		__inout MultiCamFilter       *pFilter,
		__inout HRESULT      *phr,
		__in_opt LPCWSTR     pName);
	virtual ~MultiCamOutputPin(void);

	virtual HRESULT STDMETHODCALLTYPE Connect( 
		/* [in] */ IPin *pReceivePin,
		/* [annotation][in] */ 
		__in_opt  const AM_MEDIA_TYPE *pmt);

	virtual HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);
	virtual HRESULT CheckMediaType(const CMediaType *);

	virtual HRESULT AttemptConnection(
        IPin* pReceivePin,      // connect to this pin
        const CMediaType* pmt   // using this type
    );

	//////////////////////////////////////////////////////////////////////////
	//  IKsPropertySet
	//////////////////////////////////////////////////////////////////////////
	HRESULT STDMETHODCALLTYPE Set(REFGUID guidPropSet, DWORD dwID, void *pInstanceData, DWORD cbInstanceData, void *pPropData, DWORD cbPropData);
	HRESULT STDMETHODCALLTYPE Get(REFGUID guidPropSet, DWORD dwPropID, void *pInstanceData,DWORD cbInstanceData, void *pPropData, DWORD cbPropData, DWORD *pcbReturned);
	HRESULT STDMETHODCALLTYPE QuerySupported(REFGUID guidPropSet, DWORD dwPropID, DWORD *pTypeSupport);

	//////////////////////////////////////////////////////////////////////////
	//  IAMStreamConfig
	//////////////////////////////////////////////////////////////////////////
	HRESULT STDMETHODCALLTYPE SetFormat(AM_MEDIA_TYPE *pmt);
	HRESULT STDMETHODCALLTYPE GetFormat(AM_MEDIA_TYPE **ppmt);
	HRESULT STDMETHODCALLTYPE GetNumberOfCapabilities(int *piCount, int *piSize);
	HRESULT STDMETHODCALLTYPE GetStreamCaps(int iIndex, AM_MEDIA_TYPE **pmt, BYTE *pSCC);

    //////////////////////////////////////////////////////////////////////////
    //  IPin
    //////////////////////////////////////////////////////////////////////////
	virtual HRESULT STDMETHODCALLTYPE QueryPinInfo( 
		/* [annotation][out] */ 
		__out  PIN_INFO *pInfo);

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

protected:
	MultiCamFilter *m_pParent;

	virtual HRESULT InitializeMediaType();
	IMemInputPin *GetIMemInput() {return m_pInputPin;};

};

