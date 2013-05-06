// Copyright John MacCormick 2012. Modified BSD license. NO WARRANTY.
#pragma once
// needs:
//#include "transfrm.h"
//#include <iostream>
//#include <fstream>
//using namespace std;

class MultiCamFilter;

class COverlayInputPin :
	public CTransformInputPin
{
	friend class MultiCamFilter;

public:
	COverlayInputPin(
        __in_opt LPCTSTR pObjectName,
        __inout MultiCamFilter *pTransformFilter,
        __inout HRESULT * phr,
        __in_opt LPCWSTR pName,
		int ID);

	virtual ~COverlayInputPin(void);

	virtual STDMETHODIMP Receive(IMediaSample * pSample);

protected:
	MultiCamFilter* GetFilter() {return (MultiCamFilter *) m_pTransformFilter;};

	int m_iID;
};

