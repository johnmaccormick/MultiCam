// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.

//------------------------------------------------------------------------------
// File: PlayCap.cpp
//
// Desc: DirectShow sample code - a very basic application using video capture
//       devices.  It creates a window and uses the first available capture
//       device to render and preview video capture data.
//------------------------------------------------------------------------------

// Altered by John MacCormick 2012


#define _WIN32_WINNT 0x0500

#include <windows.h>
#include <dshow.h>

#include <string>

using namespace std;


#include <streams.h>

#include "PlayCap.h"
#include "MessageBox.h"
#include "DeviceHelpers.h"
#include "ConnectHelpers.h"

#include "LogHelpers.h"
#include "SkypeApi.h"

#include <iostream>
#include <fstream>


// from http://msdn.microsoft.com/en-us/library/dd940435(v=VS.85).aspx
template <class T> void SafeRelease(T **ppT)
{
	if (*ppT)
	{
		(*ppT)->Release();
		*ppT = NULL;
	}
}

HRESULT FindAPin(IBaseFilter *pFilter, PIN_DIRECTION PinDir, IPin **ppPin);
HRESULT ConnectFilters(IGraphBuilder *pGraph, IBaseFilter *pSrc, IBaseFilter *pDest);


// An application can advertise the existence of its filter graph
// by registering the graph with a global Running Object Table (ROT).
// The GraphEdit application can detect and remotely view the running
// filter graph, allowing you to 'spy' on the graph with GraphEdit.
//
// To enable registration in this sample, define REGISTER_FILTERGRAPH.
//
#define REGISTER_FILTERGRAPH

//
// Global data
//
HWND ghApp=0;
DWORD g_dwGraphRegister=0;

IVideoWindow  * g_pVW = NULL;
IMediaControl * g_pMC = NULL;
IMediaEventEx * g_pME = NULL;
IGraphBuilder * g_pGraph = NULL;
ICaptureGraphBuilder2 * g_pCapture = NULL;
PLAYSTATE g_psCurrent = Stopped;

int g_iVideoWidth = 0;
int g_iVideoHeight = 0;





HRESULT AddFiltersToGraph(int num_filters, LPWSTR friendly_names[], /*out*/ IBaseFilter **ppLastFilter)
{
	HRESULT hr = S_OK;
	IBaseFilter **pSrcFilter = new IBaseFilter*[num_filters];
	for(int i=0; i<num_filters; i++){
		pSrcFilter[i] = NULL;
	}
	for(int i=0; i < num_filters; i++){
		hr = FindVideoOrDShowDevice(friendly_names[i], &(pSrcFilter[i]));
		if (FAILED(hr)) goto done;

		TestFilterCapabilities(pSrcFilter[i]);

		hr = g_pGraph->AddFilter(pSrcFilter[i], L"Video Capture");
		if (FAILED(hr)) goto done;

		if (i>0) {
			ConnectFilters(g_pGraph, pSrcFilter[i-1], pSrcFilter[i]);
		}

		if (i==num_filters-1) {
			(*ppLastFilter) = pSrcFilter[i];
			(*ppLastFilter)->AddRef();
		}
	}

done:
	for(int i=0; i<num_filters; i++){
		SafeRelease(&(pSrcFilter[i]));
	}
	delete [] pSrcFilter;
	return hr;
}


HRESULT CaptureVideo(int num_filters, LPWSTR* friendly_names)
{
	HRESULT hr;
	IBaseFilter *pSrcFilter=NULL;

	// Get DirectShow interfaces
	hr = GetInterfaces();
	if (FAILED(hr))
	{
		Msg(TEXT("Failed to get video interfaces!  hr=0x%x"), hr);
		return hr;
	}

	// Attach the filter graph to the capture graph
	hr = g_pCapture->SetFiltergraph(g_pGraph);
	if (FAILED(hr))
	{
		Msg(TEXT("Failed to set capture filter graph!  hr=0x%x"), hr);
		return hr;
	}

	wstring devices(L"");
	hr = ListCaptureDevices(devices);
	if (FAILED(hr))
	{
		// Don't display a message because FindCaptureDevice will handle it
		return hr;
	} else {
		//wstring msg(L"video devices found: ");
		//msg.append(devices);
		//Msg((TCHAR*) msg.c_str());
	}

#ifdef REGISTER_FILTERGRAPH
	// Add our graph to the running object table, which will allow
	// the GraphEdit application to "spy" on our graph
	hr = AddGraphToRot(g_pGraph, &g_dwGraphRegister);
	if (FAILED(hr))
	{
		Msg(TEXT("Failed to register filter graph with ROT!  hr=0x%x"), hr);
		g_dwGraphRegister = 0;
	}
#endif

	hr = AddFiltersToGraph(num_filters, friendly_names, /*out*/ &pSrcFilter);
	ASSERT(SUCCEEDED(hr));
	ASSERT(pSrcFilter!=NULL);

	TestFilterCapabilities(pSrcFilter);

	// Render the preview pin on the video capture filter
	// Use this instead of g_pGraph->RenderFile
	//hr = g_pCapture->RenderStream (&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Video,
	//                               pSrcFilter, NULL, NULL);
	hr = g_pCapture->RenderStream (NULL, &MEDIATYPE_Video,
		pSrcFilter, NULL, NULL);

	if (FAILED(hr))
	{
		Msg(TEXT("Couldn't render the video capture stream.  hr=0x%x\r\n")
			TEXT("The capture device may already be in use by another application.\r\n\r\n")
			TEXT("The sample will now close."), hr);
		goto done;
	}

	// Set video window style and position
	hr = SetupVideoWindow();
	if (FAILED(hr))
	{
		Msg(TEXT("Couldn't initialize video window!  hr=0x%x"), hr);
		goto done;
	}


	// Start previewing video data
	hr = g_pMC->Run();
	if (FAILED(hr))
	{
		Msg(TEXT("Couldn't run the graph!  hr=0x%x"), hr);
		goto done;
	}

	// Remember current state
	g_psCurrent = Running;
	GetVideoSize(pSrcFilter, /*out*/ g_iVideoWidth, /*out*/ g_iVideoHeight);


done:
	SafeRelease(&pSrcFilter);
	return hr;
}


HRESULT GetVideoSize(IBaseFilter *pFilter, /*out*/ int &width, /*out*/ int &height)
{
	HRESULT hr = S_OK;
	IPin *pPin = NULL;
	IAMStreamConfig *pConfig = NULL;
	AM_MEDIA_TYPE *pmt = NULL;

	ASSERT(pFilter != NULL);
	hr = FindAPin(pFilter, PINDIR_OUTPUT, &pPin);
	if (FAILED(hr))
	{
		vcamLog(10, "       GetVideoSize couldn't get output pin");
		goto done;
	}

	ASSERT(pPin != NULL);
	hr = pPin->QueryInterface(IID_IAMStreamConfig, (void**) &pConfig);
	if (FAILED(hr))
	{
		vcamLog(10, "       GetVideoSize couldn't get stream Config");
		goto done;
	}

	ASSERT(pConfig != NULL);

	hr = pConfig->GetFormat(&pmt);
	if (FAILED(hr))
	{
		vcamLog(10, "       GetVideoSize couldn't get format");
		goto done;
	}
	ASSERT(pmt->formattype == FORMAT_VideoInfo || pmt->formattype == FORMAT_VideoInfo2);
	BITMAPINFOHEADER *bmiHeader = GetBITMAPINFOHEADER(pmt);
	width = bmiHeader->biWidth;
	height = bmiHeader->biHeight;

done:
	SafeRelease(&pPin);
	SafeRelease(&pConfig);
	if(pmt != NULL) {
		DeleteMediaType(pmt);
	}

	vcamLog(10, "       GetVideoSize returning 0x%x", hr);

	return hr;

}

void TestFilterCapabilities(IBaseFilter *pFilter)
{
	HRESULT hr = S_OK;
	IAMStreamConfig *pConfig = NULL;
	IPin* pPin = NULL;
	hr = FindAPin(pFilter, PINDIR_OUTPUT, &pPin);
	ASSERT(SUCCEEDED(hr));
	ASSERT(pPin!=NULL);
	pPin->QueryInterface(IID_IAMStreamConfig, (void**) &pConfig);
	if (hr!=S_OK) {
		Msg(TEXT("pFilter->QueryInterface failed"));
	}
	if (pConfig==NULL) {
		Msg(TEXT("pConfig==NULL"));
	}


	int iCount, iSize;
	BYTE *pSCC = NULL;
	AM_MEDIA_TYPE *pmt;

	hr = pConfig->GetNumberOfCapabilities(&iCount, &iSize);

	pSCC = new BYTE[iSize];
	if (pSCC == NULL)
	{
		Msg(TEXT("pSCC == NULL"));
	}
	if (iCount < 1)
	{
		Msg(TEXT("iCount < 1"));
	}

	hr = pConfig->GetFormat(&pmt);
	ASSERT(SUCCEEDED(hr));
	DeleteMediaType(pmt);

	// Get the first format.
	hr = pConfig->GetStreamCaps(0, &pmt, pSCC);
	if (hr == S_OK)
	{

		// TODO: Examine the format. If it's not suitable for some
		// reason, call GetStreamCaps with the next index value (up
		// to iCount). Otherwise, set the format:

		hr = pConfig->SetFormat(pmt);
		if (FAILED(hr))
		{
			Msg(TEXT("pConfig->SetFormat FAILED"));
		}
		DeleteMediaType(pmt);
	}
	else 
	{
		Msg(TEXT("pConfig->GetStreamCaps FAILED"));
	}
	delete [] pSCC;
	SafeRelease(&pConfig);
	SafeRelease(&pPin);
}

void TestGraphPauseAndStop() {
	HRESULT hr = S_OK;
	//Sleep(3000);
	//Msg(TEXT("about to pause graph"));

	//hr = g_pMC->Pause();
	//if (FAILED(hr))
	//{
	//	Msg(TEXT("Couldn't pause the graph!  hr=0x%x"), hr);
	//	goto done;
	//}

	//Sleep(1000);
	//hr = g_pMC->Run();
	//if (FAILED(hr))
	//{
	//	Msg(TEXT("Couldn't run the graph the second time!  hr=0x%x"), hr);
	//	goto done;
	//}

	//Sleep(1000);
	//hr = g_pMC->Pause();
	//if (FAILED(hr))
	//{
	//	Msg(TEXT("Couldn't pause the graph the second time!  hr=0x%x"), hr);
	//	goto done;
	//}

	Sleep(1000);
	hr = g_pMC->Stop();
	if (FAILED(hr))
	{
		Msg(TEXT("Couldn't stop the graph!  hr=0x%x"), hr);
		goto done;
	}
	else {
		Msg(TEXT("Successfully stopped the graph"));
	}
done:
	;
}

//HRESULT CreateClassEnumerator(GUID deviceCategory, IEnumMoniker **ppClassEnum) {
//	HRESULT hr;
//
//	// create system device enumerator
//	ICreateDevEnum *pDevEnum =NULL;
//	hr = CoCreateInstance (CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC,
//                           IID_ICreateDevEnum, (void **) &pDevEnum);
//    if (FAILED(hr))
//    {
//        Msg(TEXT("Couldn't create system enumerator!  hr=0x%x"), hr);
//    }
//
//	// Create an enumerator for the video capture devices
//	if (SUCCEEDED(hr))
//	{
//	    hr = pDevEnum->CreateClassEnumerator (deviceCategory, ppClassEnum, 0);
//		if (FAILED(hr))
//		{
//			Msg(TEXT("Couldn't create class enumerator!  hr=0x%x"), hr);
//	    }
//	}
//
//	if (SUCCEEDED(hr))
//	{
//		// If there are no enumerators for the requested type, then 
//		// CreateVideoClassEnumerator will succeed, but *ppClassEnum will be NULL.
//		if (*ppClassEnum == NULL)
//		{
//			MessageBox(ghApp,TEXT("No video capture device was detected.\r\n\r\n")
//				TEXT("This sample requires a video capture device, such as a USB WebCam,\r\n")
//				TEXT("to be installed and working properly.  The sample will now close."),
//				TEXT("No Video Capture Hardware"), MB_OK | MB_ICONINFORMATION);
//			hr = E_FAIL;
//		}
//	}
//
//    SAFE_RELEASE(pDevEnum);
//
//	return hr;
//}

HRESULT ListCaptureDevices(wstring &devices) {
	HRESULT hr = S_OK;
	IMoniker* pMoniker =NULL;

	IEnumMoniker *pClassEnum = NULL;

	hr = CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pClassEnum);

	if (SUCCEEDED(hr))
	{
		BOOL done = false;
		devices.clear();
		while (!done) {
			HRESULT hr2 = pClassEnum->Next(1, &pMoniker, NULL);
			if (hr2 == S_OK)
			{
				wstring friendly_name(L"");
				hr2 = getMonikerFriendlyName(pMoniker, friendly_name);
				devices.append(friendly_name);
				devices.append(L"|");
			} else {
				done=true;
			}
		}
	}

	SAFE_RELEASE(pMoniker);
	SAFE_RELEASE(pClassEnum);

	return hr;
}


//HRESULT FindDevice(GUID deviceCategory, LPWSTR vcam_friendly_name, IBaseFilter ** ppSrcFilter)
//{
//    HRESULT hr = S_OK;
//    IBaseFilter * pSrc = NULL;
//    IMoniker* pMoniker =NULL;
//    IEnumMoniker *pClassEnum = NULL;
//
//    if (!ppSrcFilter)
//	{
//        return E_POINTER;
//	}
//   
//	hr = CreateClassEnumerator(deviceCategory, &pClassEnum);
//
//	BOOL found = false;
//	if (SUCCEEDED(hr))
//	{
//		BOOL done = false;
//		while (!done && !found) {
//			HRESULT hr2 = pClassEnum->Next(1, &pMoniker, NULL);
//			if (hr2 == S_OK)
//			{
//				wstring friendly_name(L"");
//				hr2 = getMonikerFriendlyName(pMoniker, friendly_name);
//				if (friendly_name.compare(vcam_friendly_name)==0) {
//					found=true;
//				}
//			} else {
//				done=true;
//			}
//		}
//	}
//
//	// Use the first video capture device on the device list.
//	// Note that if the Next() call succeeds but there are no monikers,
//	// it will return S_FALSE (which is not a failure).  Therefore, we
//	// check that the return code is S_OK instead of using SUCCEEDED() macro.
//
//	if (!found)
//	{
//		//Msg(TEXT("Unable to find video capture device \"%s\""), vcam_friendly_name);   
//		hr = E_FAIL;
//	}
//
//	if (SUCCEEDED(hr))
//    {
//        // Bind Moniker to a filter object
//        hr = pMoniker->BindToObject(0,0,IID_IBaseFilter, (void**)&pSrc);
//        if (FAILED(hr))
//        {
//            Msg(TEXT("Couldn't bind moniker to filter object!  hr=0x%x"), hr);
//        }
//    }
//
//    // Copy the found filter pointer to the output parameter.
//	if (SUCCEEDED(hr))
//	{
//	    *ppSrcFilter = pSrc;
//		(*ppSrcFilter)->AddRef();
//	}
//
//	SAFE_RELEASE(pSrc);
//    SAFE_RELEASE(pMoniker);
//    SAFE_RELEASE(pClassEnum);
//
//    return hr;
//}


HRESULT GetInterfaces(void)
{
	HRESULT hr;

	// Create the filter graph
	hr = CoCreateInstance (CLSID_FilterGraph, NULL, CLSCTX_INPROC,
		IID_IGraphBuilder, (void **) &g_pGraph);
	if (FAILED(hr))
		return hr;

	// Create the capture graph builder
	hr = CoCreateInstance (CLSID_CaptureGraphBuilder2 , NULL, CLSCTX_INPROC,
		IID_ICaptureGraphBuilder2, (void **) &g_pCapture);
	if (FAILED(hr))
		return hr;

	// Obtain interfaces for media control and Video Window
	hr = g_pGraph->QueryInterface(IID_IMediaControl,(LPVOID *) &g_pMC);
	if (FAILED(hr))
		return hr;

	hr = g_pGraph->QueryInterface(IID_IVideoWindow, (LPVOID *) &g_pVW);
	if (FAILED(hr))
		return hr;

	hr = g_pGraph->QueryInterface(IID_IMediaEventEx, (LPVOID *) &g_pME);
	if (FAILED(hr))
		return hr;

	// Set the window handle used to process graph events
	hr = g_pME->SetNotifyWindow((OAHWND)ghApp, WM_GRAPHNOTIFY, 0);

	return hr;
}


void CloseInterfaces(void)
{
	// Stop previewing data
	if (g_pMC)
		g_pMC->StopWhenReady();

	g_psCurrent = Stopped;

	// Stop receiving events
	if (g_pME)
		g_pME->SetNotifyWindow(NULL, WM_GRAPHNOTIFY, 0);

	// Relinquish ownership (IMPORTANT!) of the video window.
	// Failing to call put_Owner can lead to assert failures within
	// the video renderer, as it still assumes that it has a valid
	// parent window.
	if(g_pVW)
	{
		g_pVW->put_Visible(OAFALSE);
		g_pVW->put_Owner(NULL);
	}

#ifdef REGISTER_FILTERGRAPH
	// Remove filter graph from the running object table   
	if (g_dwGraphRegister)
		RemoveGraphFromRot(g_dwGraphRegister);
#endif

	// Release DirectShow interfaces
	SAFE_RELEASE(g_pMC);
	SAFE_RELEASE(g_pME);
	SAFE_RELEASE(g_pVW);
	SAFE_RELEASE(g_pGraph);
	//SAFE_RELEASE(g_pCapture);
	if (g_pCapture) {
		g_pCapture->Release(); 
		g_pCapture = NULL;
	}
}


HRESULT SetupVideoWindow(void)
{
	vcamLog(10, "SetupVideoWindow");
	HRESULT hr;

	// Set the video window to be a child of the main window
	hr = g_pVW->put_Owner((OAHWND)ghApp);
	if (FAILED(hr))
		goto done;

	// Set video window style
	hr = g_pVW->put_WindowStyle(WS_CHILD | WS_CLIPCHILDREN);
	if (FAILED(hr))
		goto done;

	// Use helper function to position video window in client rect 
	// of main application window
	ResizeVideoWindow();

	// Make the video window visible, now that it is properly positioned
	hr = g_pVW->put_Visible(OATRUE);
	if (FAILED(hr))
		goto done;

done:
	if(hr==S_OK) {
		vcamLog(10, "         SetupVideoWindow returning S_OK");
	}
	else if(hr==VFW_E_ALREADY_CONNECTED) {
		vcamLog(10, "         SetupVideoWindow returning VFW_E_ALREADY_CONNECTED");
	}
	else if(hr==VFW_E_NOT_CONNECTED) {
		vcamLog(10, "         SetupVideoWindow returning VFW_E_NOT_CONNECTED");
	}
	else if(hr==E_NOINTERFACE) {
		vcamLog(10, "         SetupVideoWindow returning E_NOINTERFACE");
	} else {
		vcamLog(10, "       SetupVideoWindow returning 0x%x", hr);
	}
	return hr;
}


void ResizeVideoWindow(void)
{
	// Resize the video preview window to match owner window size
	if (g_pVW)
	{
		RECT rc;

		// Make the preview video fill our window
		GetClientRect(ghApp, &rc);
		g_pVW->SetWindowPosition(0, 0, rc.right, rc.bottom);
	}
}


HRESULT ChangePreviewState(int nShow)
{
	HRESULT hr=S_OK;

	// If the media control interface isn't ready, don't call it
	if (!g_pMC)
		return S_OK;

	if (nShow)
	{
		if (g_psCurrent != Running)
		{
			// Start previewing video data
			hr = g_pMC->Run();
			g_psCurrent = Running;
		}
	}
	else
	{
		// Stop previewing video data
		hr = g_pMC->StopWhenReady();
		g_psCurrent = Stopped;
	}

	return hr;
}


#ifdef REGISTER_FILTERGRAPH

HRESULT AddGraphToRot(IUnknown *pUnkGraph, DWORD *pdwRegister) 
{
	IMoniker * pMoniker;
	IRunningObjectTable *pROT;
	WCHAR wsz[128];
	HRESULT hr;

	if (!pUnkGraph || !pdwRegister)
		return E_POINTER;

	if (FAILED(GetRunningObjectTable(0, &pROT)))
		return E_FAIL;

	hr = StringCchPrintfW(wsz, NUMELMS(wsz), L"FilterGraph %08x pid %08x\0", (DWORD_PTR)pUnkGraph, 
		GetCurrentProcessId());

	hr = CreateItemMoniker(L"!", wsz, &pMoniker);
	if (SUCCEEDED(hr)) 
	{
		// Use the ROTFLAGS_REGISTRATIONKEEPSALIVE to ensure a strong reference
		// to the object.  Using this flag will cause the object to remain
		// registered until it is explicitly revoked with the Revoke() method.
		//
		// Not using this flag means that if GraphEdit remotely connects
		// to this graph and then GraphEdit exits, this object registration 
		// will be deleted, causing future attempts by GraphEdit to fail until
		// this application is restarted or until the graph is registered again.
		hr = pROT->Register(ROTFLAGS_REGISTRATIONKEEPSALIVE, pUnkGraph, 
			pMoniker, pdwRegister);
		pMoniker->Release();
	}

	pROT->Release();
	return hr;
}


// Removes a filter graph from the Running Object Table
void RemoveGraphFromRot(DWORD pdwRegister)
{
	IRunningObjectTable *pROT;

	if (SUCCEEDED(GetRunningObjectTable(0, &pROT))) 
	{
		pROT->Revoke(pdwRegister);
		pROT->Release();
	}
}

#endif


HRESULT HandleGraphEvent(void)
{
	LONG evCode;
	LONG_PTR evParam1, evParam2;
	HRESULT hr=S_OK;

	if (!g_pME)
		return E_POINTER;

	while(SUCCEEDED(g_pME->GetEvent(&evCode, &evParam1, &evParam2, 0)))
	{
		//
		// Free event parameters to prevent memory leaks associated with
		// event parameter data.  While this application is not interested
		// in the received events, applications should always process them.
		//
		hr = g_pME->FreeEventParams(evCode, evParam1, evParam2);

		// Insert event processing code here, if desired
	}

	return hr;
}


LRESULT CALLBACK WndMainProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_GRAPHNOTIFY:
		HandleGraphEvent();
		break;

	case WM_SIZE:
		ResizeVideoWindow();
		break;

	case WM_WINDOWPOSCHANGED:
		ChangePreviewState(! (IsIconic(hwnd)));
		break;

	case WM_CLOSE:            
		// Hide the main window while the graph is destroyed
		ShowWindow(ghApp, SW_HIDE);
		CloseInterfaces();  // Stop capturing and release interfaces
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	// Pass this message to the video window for notification of system changes
	if (g_pVW)
		g_pVW->NotifyOwnerMessage((LONG_PTR) hwnd, message, wParam, lParam);

	return DefWindowProc (hwnd , message, wParam, lParam);
}


void ResizeMainWindow()
{
	BOOL br = true;
	RECT rect, client_rect;
	WINDOWINFO window_info;
	window_info.cbSize = sizeof(WINDOWINFO);
	br = GetWindowInfo(ghApp, &window_info);
	ASSERT(br);
	client_rect = window_info.rcClient;
	client_rect.right = client_rect.left + g_iVideoWidth;
	client_rect.bottom = client_rect.top + g_iVideoHeight;
	BOOL bHasMenu = (GetMenu(ghApp) != NULL);
	rect = client_rect;
	br = AdjustWindowRect(&rect, window_info.dwStyle, bHasMenu);
	ASSERT(br);
	LONG new_width = rect.right-rect.left;
	LONG new_height = rect.bottom-rect.top;
	br = MoveWindow(ghApp, rect.left, rect.top, new_width, new_height, false);
	ASSERT(br);

	RECT new_rect, new_client_rect;
	br = GetClientRect(ghApp, &new_client_rect);
	ASSERT(br);
	br = GetWindowRect(ghApp, &new_rect);
	ASSERT(br);
	ASSERT(rect.bottom == new_rect.bottom);
	ASSERT(rect.left == new_rect.left);
	ASSERT(rect.top == new_rect.top);
	ASSERT(rect.right == new_rect.right);
	ASSERT(new_client_rect.bottom - new_client_rect.top == g_iVideoHeight);
	ASSERT(new_client_rect.right - new_client_rect.left == g_iVideoWidth);

	ResizeVideoWindow();
}

int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hInstP, LPSTR lpCmdLine, int nCmdShow)
{
	MSG msg={0};
	WNDCLASS wc;

	vcamOpenLog(10, "PlayCap");
	vcamLog(10, "***********************Playcap***********************");

	//startSkypeApi();


	LPWSTR* argv;
	int argc;
	argv = CommandLineToArgvW(GetCommandLineW(), &argc);

	int num_filters = argc-1;
	LPWSTR* friendly_names = new LPWSTR[num_filters];

	if (argc < 2) {
		Msg(TEXT("Usage: playcap device [device] [device] ... \r\n"));   
		exit(1);
	} else {
		for(int i=1; i<argc; i++){
			friendly_names[i-1] = argv[i];
		}
	}

	// Initialize COM
	if(FAILED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED)))
	{
		Msg(TEXT("CoInitialize Failed!\r\n"));   
		exit(1);
	} 

	// Register the window class
	ZeroMemory(&wc, sizeof wc);
	wc.lpfnWndProc   = WndMainProc;
	wc.hInstance     = hInstance;
	wc.lpszClassName = CLASSNAME;
	wc.lpszMenuName  = NULL;
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wc.hIcon         = NULL;
	if(!RegisterClass(&wc))
	{
		Msg(TEXT("RegisterClass Failed! Error=0x%x\r\n"), GetLastError());
		CoUninitialize();
		exit(1);
	}

	// Create the main window.  The WS_CLIPCHILDREN style is required.
	ghApp = CreateWindow(CLASSNAME, APPLICATIONNAME,
		WS_OVERLAPPEDWINDOW | WS_CAPTION | WS_CLIPCHILDREN,
		//0, 0,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		//DEFAULT_VIDEO_WIDTH, DEFAULT_VIDEO_HEIGHT,
		0, 0, hInstance, 0);

	if(ghApp)
	{
		HRESULT hr;

		// Create DirectShow graph and start capturing video
		hr = CaptureVideo(num_filters, friendly_names);
		if (FAILED (hr))
		{
			CloseInterfaces();
			DestroyWindow(ghApp);
		}
		else
		{
			// Resize main window now that we know how big the video window is
			ResizeMainWindow();

			// Don't display the main window until the DirectShow
			// preview graph has been created.  Once video data is
			// being received and processed, the window will appear
			// and immediately have useful video data to display.
			// Otherwise, it will be black until video data arrives.
			ShowWindow(ghApp, nCmdShow);


			TestGraphPauseAndStop();

			hr = CaptureVideo(num_filters, friendly_names);
			if (FAILED (hr))
			{
				CloseInterfaces();
				DestroyWindow(ghApp);
				Msg(TEXT("second video capture failed"));
			}

		}
		// Main message loop
		while(GetMessage(&msg,NULL,0,0))
		{
			//if (msg.message == WM_KEYDOWN) {
			//	vcamLog(10, "key down in PlayCap, Thread ID = %d", GetCurrentThreadId());				
			//}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	delete [] friendly_names;
	vcamCloseLog(10, "PlayCap");

	// Release COM
	CoUninitialize();

	//stopSkypeApi();


	return (int) msg.wParam;
}

HRESULT printMonikerFriendlyName(IMoniker *pMoniker) {
	wstring friendlyName(L"");
	HRESULT hr;
	hr = getMonikerFriendlyName(pMoniker, friendlyName);
	if (SUCCEEDED(hr))
	{
		wstring a(L"Found device ");
		a += friendlyName;
		Msg((TCHAR *) a.c_str());
	} else {
		Msg(TEXT("ERROR retrieving friendly name"));
	}
	return hr;
}

//HRESULT getMonikerFriendlyName(IMoniker *pMoniker, wstring &friendlyName) {
//	IPropertyBag *pPropBag;
//	HRESULT hr;
//	hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, 
//		(void **)&pPropBag);
//	if (SUCCEEDED(hr))
//	{
//		// retrieve and print filter's friendly name
//		VARIANT varName;
//		VariantInit(&varName);
//		hr = pPropBag->Read(L"FriendlyName", &varName, 0);
//		if (SUCCEEDED(hr))
//		{
//			if (varName.vt == VT_BSTR) {
//				BSTR name = varName.bstrVal;
//				friendlyName.clear();
//				friendlyName.append(name);
//			} else {
//				Msg(TEXT("ERROR -- friendly name was not a BSTR"));
//				hr = E_FAIL;
//			}
//		}
//		VariantClear(&varName);
//	}
//	else {
//		Msg(TEXT("couldn't bind moniker to property bag\n"));
//	}
//	pPropBag->Release();
//	return hr;
//}

