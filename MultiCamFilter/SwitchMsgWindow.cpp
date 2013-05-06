// Copyright John MacCormick 2012. Modified BSD license. NO WARRANTY.
#include <streams.h>

#include <iostream>
#include <fstream>
#include <vector>

using namespace std;

#include "jmac-vcam-guids.h"
#include "MultiCamFilter.h"
#include "LogHelpers.h"
#include "SwitchMsgWindow.h"

P_SWITCH_LISTENER_PTR CSwitchMsgWindow::AllocateThisPointer()
{
	P_SWITCH_LISTENER_PTR pSwitchListener = (P_SWITCH_LISTENER_PTR) HeapAlloc(GetProcessHeap(), 
		HEAP_ZERO_MEMORY, sizeof(SWITCH_LISTENER_PTR));
	VCAM_ASSERT(pSwitchListener != NULL);
	pSwitchListener->p = this;
	return pSwitchListener;
}

CSwitchMsgWindow::CSwitchMsgWindow(MultiCamFilter *parent) :
m_uiDiscoverMessageId(0),
	m_uiAttachMessageId(0),
	m_uiAdvanceMessageId(0),
	m_Multicam_keystroke_status(MULTICAM_ATTACH_MSG_NOT_ATTEMPTED),
	m_pParent(parent)
{
	vcamOpenLog(5, "CSwitchMsgWindow");	

	m_evSentDiscoverMessage = CreateEvent(NULL, TRUE, FALSE, NULL);
	m_evMulticamAccepted = CreateEvent(NULL, TRUE, FALSE, NULL);

	m_hMsgLoopThread = CreateThread(NULL, 64 * 1024, &CSwitchMsgWindow::doMsgLoopInThread, 
		AllocateThisPointer(), 0, &m_dwMsgLoopThreadID);
	if (m_hMsgLoopThread == NULL) {
		vcamLogError(10, "CreateThread (m_hMsgLoopThread) in CSwitchMsgWindow::CSwitchMsgWindow");
	}	

}


CSwitchMsgWindow::~CSwitchMsgWindow(void)
{
	vcamLog(10, "CSwitchMsgWindow::~CSwitchMsgWindow: starting -- waiting for m_evSentDiscoverMessage");
	WaitForSingleObject(m_evSentDiscoverMessage, 2000);
	vcamLog(10, "CSwitchMsgWindow::~CSwitchMsgWindow: starting -- m_evSentDiscoverMessage was set");

	CloseHandle(m_evSentDiscoverMessage);
	CloseHandle(m_evMulticamAccepted);

	vcamLog(10, "CSwitchMsgWindow::~CSwitchMsgWindow: DeInitialize_DestroyMainWindow");
	DeInitialize_DestroyMainWindow(m_hMainWindow);
	vcamLog(10, "CSwitchMsgWindow::~CSwitchMsgWindow: DeInitialize_DestroyWindowClass");
	DeInitialize_DestroyWindowClass(m_sWindowClassName, m_hProcess);
	vcamLog(10, "CSwitchMsgWindow::~CSwitchMsgWindow: vcamCloseLog");
	vcamCloseLog(5, "CSwitchMsgWindow");	
}

DWORD WINAPI CSwitchMsgWindow::doMsgLoopInThread(LPVOID lpParam)
{
	P_SWITCH_LISTENER_PTR switchListenerPtr = (P_SWITCH_LISTENER_PTR) lpParam;
	CSwitchMsgWindow* pSwitchMsgWindow = switchListenerPtr->p;

	BOOL ok = HeapFree(GetProcessHeap(), 0, switchListenerPtr);
	VCAM_ASSERT(ok);

	pSwitchMsgWindow->NonStaticDoMsgLoopInThread();

	return 0;
}

DWORD WINAPI CSwitchMsgWindow::NonStaticDoMsgLoopInThread()
{
	//vcamLog(10, "------------------------ starting CSwitchMsgWindow::NonStaticDoMsgLoopInThread -------------------------");

	m_uiAttachMessageId=RegisterWindowMessage(MULTICAM_ATTACH_MSG);
	VCAM_ASSERT(m_uiAttachMessageId != 0);

	m_uiDiscoverMessageId=RegisterWindowMessage(MULTICAM_DISCOVER_MSG);
	VCAM_ASSERT(m_uiDiscoverMessageId != 0);

	m_uiAdvanceMessageId=RegisterWindowMessage(MULTICAM_ADVANCE_MSG);
	VCAM_ASSERT(m_uiAdvanceMessageId != 0);

	m_uiKickMessageId=RegisterWindowMessage(MULTICAM_KICK_MSG);
	VCAM_ASSERT(m_uiKickMessageId != 0);

	m_uiPingMessageId=RegisterWindowMessage(MULTICAM_PING_MSG);
	VCAM_ASSERT(m_uiPingMessageId != 0);

	m_uiPongMessageId=RegisterWindowMessage(MULTICAM_PONG_MSG);
	VCAM_ASSERT(m_uiPongMessageId != 0);

	m_uiResetMessageId=RegisterWindowMessage(MULTICAM_RESET_MSG);
	VCAM_ASSERT(m_uiResetMessageId != 0);

	BOOL ok = true;
	ok = Initialize_CreateWindowClass((WNDPROC)&WndProc, "Switch-Message-Window-", 
		/*out*/ m_hProcess, /*out*/ m_sWindowClassName);
	VCAM_ASSERT(ok);

	ok = Initialize_CreateMainWindow(m_sWindowClassName, m_hProcess, this, /*out*/ m_hMainWindow);
	VCAM_ASSERT(ok);

	//vcamLog(10, "CSwitchMsgWindow::NonStaticDoMsgLoopInThread: Broadcasting discover request for MultiCam");
	SendDiscoverMessage();
	ok = SetEvent(m_evSentDiscoverMessage);

	//if (!ok) {
	//	vcamLogError(10, "CSwitchMsgWindow::NonStaticDoMsgLoopInThread: SetEvent failed");
	//}
	//VCAM_ASSERT(ok);

	// Start the message loop. 
	MSG msg;
	BOOL bRet;
	while( (bRet = GetMessage( &msg, NULL, 0, 0 )) != 0)
	{ 
		if (bRet == -1)
		{
			//vcamLog(10, "CSwitchMsgWindow::NonStaticDoMsgLoopInThread: Unexpected error in message loop; terminating message loop");
			break;
		}
		else
		{
			TranslateMessage(&msg); 
			DispatchMessage(&msg); 
		}
	} 

	//vcamLog(10, "------------------------ ending CSwitchMsgWindow::NonStaticDoMsgLoopInThread -------------------------");

	//ok = SetEvent(m_evMsgLoopCompleted);
	//VCAM_ASSERT(ok);
	return 0;
}

LRESULT APIENTRY CSwitchMsgWindow::WndProc(
	HWND hWindow, UINT uiMessage, WPARAM uiParam, LPARAM ulParam)
{
	CSwitchMsgWindow* pSwitchListener = (CSwitchMsgWindow*) GetWindowLongPtr(hWindow, GWLP_USERDATA);
	if(pSwitchListener != NULL) {
		return pSwitchListener->nonstatic_WndProc(hWindow, uiMessage, uiParam, ulParam);
	}
	else {
		return DefWindowProc( hWindow, uiMessage, uiParam, ulParam);
	}
}

LRESULT APIENTRY CSwitchMsgWindow::nonstatic_WndProc(
	HWND hWindow, UINT uiMessage, WPARAM uiParam, LPARAM ulParam)
{

	LRESULT lReturnCode;
	bool fIssueDefProc;
	BOOL ok = FALSE;

	lReturnCode=0;
	fIssueDefProc=false;
	switch(uiMessage)
	{
	case WM_DESTROY:
		m_hMainWindow=NULL;
		PostQuitMessage(0);
		break;
	default:
		if( uiMessage==m_uiAttachMessageId )
		{
			switch(ulParam)
			{
			case MULTICAM_ATTACH_MSG_SUCCESS:
				m_hMulticamWindow=(HWND)uiParam;
				vcamLog(10, "SwitchMsgWindow connected to SkypeApi object; SwitchMsgWindow window 0x%x, SkypeApi window 0x%x",
					m_hMainWindow, m_hMulticamWindow);
				m_Multicam_keystroke_status = (Multicam_keystroke_API) ulParam;
				ok = SetEvent(m_evMulticamAccepted);
				if (!ok) {
					vcamLog(10, "CSwitchMsgWindow::nonstatic_WndProc: SetEvent(m_evMulticamAccepted) failed");
				}
				break;
			default:
				vcamLog(10, "SwitchMsgWindow received unexpected value from SkypeApi object; almost certainly an error");
			}
			lReturnCode=1;
			m_Multicam_keystroke_status = (Multicam_keystroke_API) ulParam;
			break;
		}
		else if( uiMessage==m_uiAdvanceMessageId )
		{
			vcamLog(10, "SwitchMsgWindow received 'pressed' message from SkypeApi object; window = 0x%x, uiParam = 0x%x, ulParam = 0x%x", 
				hWindow, uiParam, ulParam);
			//HRESULT hrSwitch = m_pParent->CallWorker( (DWORD) 0);
			HRESULT hrSwitch = m_pParent->AdvanceInput();
			VCAM_ASSERT(SUCCEEDED(hrSwitch) || hrSwitch == E_INVALIDARG);
		}
		else if( uiMessage==m_uiKickMessageId )
		{
			vcamLog(1, "SwitchMsgWindow received 'kick' message from SkypeApi object.");
			SendDiscoverMessage();
			lReturnCode=1;
		}
		else if( uiMessage==m_uiPingMessageId )
		{
			vcamLog(1, "SwitchMsgWindow received 'ping' message from SkypeApi object.");
			SendPongMessage();
			lReturnCode=1;
		}
		//else if( uiMessage==m_uiResetMessageId )
		//{
		//	vcamLog(10, "SwitchMsgWindow received 'reset' message from SkypeApi object; window = 0x%x, uiParam = 0x%x, ulParam = 0x%x", 
		//		hWindow, uiParam, ulParam);
		//	HRESULT hrReset = m_pParent->Reset();
		//	VCAM_ASSERT(SUCCEEDED(hrReset)/* || hrReset == E_INVALIDARG*/);
		//}
		fIssueDefProc=true;
		break;
	}
	if( fIssueDefProc )
		lReturnCode=DefWindowProc( hWindow, uiMessage, uiParam, ulParam);

	//printf( "WindowProc: hWindow=0x%08X, MainWindow=0x%08X, Message=%5u, WParam=0x%08X, LParam=0x%08X; Return=%ld%s\n",
	//	hWindow, hInit_MainWindowHandle, uiMessage, uiParam, ulParam, lReturnCode, fIssueDefProc? " (default)":"");

	return(lReturnCode);
}

void CSwitchMsgWindow::SendDiscoverMessage()
{
	LRESULT lr = 0;
	int numCams = m_pParent->m_iNumValidFilters;
	vcamLog(1, "CSwitchMsgWindow::SendDiscoverMessage: m_uiDiscoverMessageId = %ud, m_hMainWindow = 0x%x, numCams = %d", 
		m_uiDiscoverMessageId, m_hMainWindow, numCams);
	lr = SendMessageTimeout( HWND_BROADCAST, m_uiDiscoverMessageId, (WPARAM)m_hMainWindow, 
		(LPARAM) numCams, SMTO_ABORTIFHUNG, 1000, NULL);
	VCAM_ASSERT(lr != 0);
}

void CSwitchMsgWindow::SendPongMessage()
{
	LRESULT lr = 0;
	if (m_hMulticamWindow != NULL) 
	{
		vcamLog(1, "CSwitchMsgWindow::SendPongMessage: m_hMulticamWindow = 0x%x", 
			m_hMulticamWindow);
		lr = SendMessage( m_hMulticamWindow, m_uiPongMessageId, 0, 0);
		if (lr == 0)
		{
			vcamLog(1, "CSwitchMsgWindow::SendPongMessage: SendMessage failed");
			vcamLogError(1, "CSwitchMsgWindow::SendPongMessage");
			//m_hMulticamWindow = NULL;
			//m_Multicam_keystroke_status = MULTICAM_ATTACH_MSG_NOT_ATTEMPTED;
		}
		//VCAM_ASSERT(lr != 0);
	}
}

