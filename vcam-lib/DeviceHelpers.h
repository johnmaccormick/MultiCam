// Copyright John MacCormick 2012. Modified BSD license. NO WARRANTY.
HRESULT FindVideoOrDShowDevice(LPWSTR vcam_friendly_name, IBaseFilter ** ppSrcFilter);
HRESULT FindDevice(GUID deviceCategory, LPWSTR vcam_friendly_name, IBaseFilter ** ppSrcFilter);
HRESULT FindCaptureDevice(LPWSTR vcam_friendly_name, IBaseFilter ** ppSrcFilter);
HRESULT CreateClassEnumerator(GUID deviceCategory, IEnumMoniker **ppClassEnum);
HRESULT CreateVideoClassEnumerator(IEnumMoniker **ppClassEnum);
HRESULT getMonikerFriendlyName(IMoniker *pMoniker, wstring &friendlyName);


