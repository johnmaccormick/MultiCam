// Copyright John MacCormick 2012. Modified BSD license. NO WARRANTY.

#include <streams.h>

#include <iostream>
#include <fstream>
#include <vector>

using namespace std;

#include "MultiCamFilter.h"
#include "OverlayInputPin.h"
#include "jmac-vcam-guids.h"
#include "ConnectHelpers.h"
#include "LogHelpers.h"



COverlayInputPin::COverlayInputPin(
        __in_opt LPCTSTR pObjectName,
        __inout MultiCamFilter *pTransformFilter,
        __inout HRESULT * phr,
        __in_opt LPCWSTR pName,
		int ID) : CTransformInputPin(pObjectName, pTransformFilter, phr, pName),
		m_iID(ID)
{
		 
}

COverlayInputPin::~COverlayInputPin(void)
{
}

HRESULT COverlayInputPin::Receive(IMediaSample * pSample)
{
    HRESULT hr;
    CAutoLock lck(GetFilter()->GetcsReceive());
    VCAM_ASSERT(pSample);

    // check all is well with the base class
    hr = CBaseInputPin::Receive(pSample);
    if (S_OK == hr) {
        hr = GetFilter()->Receive(pSample, m_iID);
    }
    return hr;
}