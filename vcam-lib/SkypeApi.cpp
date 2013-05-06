// NO LONGER NEEDED??
// adapted from code by Skype:TheUberOverLord, pulled from  http://tinyurl.com/3dkdklw
// Need to link to "Rpcrt4.lib"
// Alterations are Copyright John MacCormick 2012. Modified BSD license. NO WARRANTY.


#include <stdio.h>
#include <string.h>

#include <windows.h>
#include <rpcdce.h>
#include <streams.h>

#include <string>
#include <iostream>
#include <map>

using namespace std;

#include "jmac-vcam-guids.h"
#include "LogHelpers.h"
#include "WindowHelper.h"
#include "KeystrokeMessages.h"
#include "SkypeApi.h"

//CCritSec SkypeApi::s_mapLock;
//map<HWND, SkypeApi*>  SkypeApi::s_skypeApiMap;


static string PONG_string("PONG");
static string CREATE_APPLICATION_string("CREATE APPLICATION multicam");
static string CONNECT_APPLICATION_string("ALTER APPLICATION multicam CONNECT ");
static string DISCONNECT_APPLICATION_string("ALTER APPLICATION multicam DISCONNECT ");
static string REMOTE_USER_string("dickinsoncomputerscience");
static string LOCAL_USER_string("arawatabill");
static string GET_USER_request("GET CURRENTUSERHANDLE");
static string GET_USER_response("CURRENTUSERHANDLE ");
static string STREAM_NAME_string("APPLICATION multicam STREAMS ");
static string WRITE_APPLICATION_string("ALTER APPLICATION multicam WRITE ");
static string APPLICATION_RECEIVED_string("APPLICATION multicam RECEIVED ");
static string READ_APPLICATION_string("ALTER APPLICATION multicam READ ");

static string CALL_string("CALL ");
static string STATUS_string("STATUS ");
static string INPROGRESS_string("INPROGRESS");
static string ROUTING_string("ROUTING");
static string DURATION_string(" DURATION ");
static string PARTNER_string("PARTNER_HANDLE");
static string FINISHED_string("FINISHED");
static string GET_string("GET ");


PSKYPE_API_PTR SkypeApi::AllocateThisPointer()
{
	PSKYPE_API_PTR skypeApiPtr = (PSKYPE_API_PTR) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SKYPE_API_PTR));
	VCAM_ASSERT(skypeApiPtr != NULL);
	skypeApiPtr->p = this;
	return skypeApiPtr;
}

SkypeApi::SkypeApi() :
hInit_ProcessHandle(NULL),
	hInit_MainWindowHandle(NULL),
	uiGlobal_MsgID_SkypeControlAPIAttach(0),
	uiGlobal_MsgID_SkypeControlAPIDiscover(0),
	m_uiDiscoverKeystrokeMessageId(0),
	m_uiAttachMessageId(0),
	m_uiPressedKeystrokeMessageId(0),
	hGlobal_SkypeAPIWindowHandle(NULL),
	m_hKeystrokeMessageWindow(NULL),
	m_evSkypeAccepted(NULL),
	m_Skype_control_status(SKYPECONTROLAPI_ATTACH_NOT_ATTEMPTED),
	m_hMsgLoopThread(NULL),
	m_strCurrentUser(""),
	m_strStreamName(""),
	m_strPartner(""),
	m_iCallId(-1),
	m_fInitiatedCall(false)
{
	vcamOpenLog(5, "SkypeApi");	

	m_evSkypeAccepted = CreateEvent(NULL, TRUE, FALSE, NULL);
	VCAM_ASSERT(m_evSkypeAccepted != NULL);

	m_evMsgLoopCompleted = CreateEvent(NULL, TRUE, FALSE, NULL);
	VCAM_ASSERT(m_evMsgLoopCompleted != NULL);

	m_evExitNow = CreateEvent(NULL, TRUE, FALSE, NULL);
	VCAM_ASSERT(m_evExitNow != NULL);

	m_evPong = CreateEvent(NULL, FALSE, FALSE, NULL);
	VCAM_ASSERT(m_evPong != NULL);

	m_evAp2ApCompleted = CreateEvent(NULL, FALSE, FALSE, NULL);
	VCAM_ASSERT(m_evAp2ApCompleted != NULL);

	m_evSkypeCreatedApplication = CreateEvent(NULL, FALSE, FALSE, NULL);
	VCAM_ASSERT(m_evSkypeCreatedApplication != NULL);

	m_evGotUser = CreateEvent(NULL, FALSE, FALSE, NULL);
	VCAM_ASSERT(m_evGotUser != NULL);

	

	m_hMsgLoopThread = CreateThread(NULL, 64 * 1024, &SkypeApi::doMsgLoopInThread, 
		AllocateThisPointer(), CREATE_SUSPENDED, &m_dwMsgLoopThreadID);
	if (m_hMsgLoopThread == NULL) {
		vcamLogError(10, "CreateThread (m_hMsgLoopThread) in doSkypeApi");
	}	

	m_hAp2ApThread = CreateThread(NULL, 64 * 1024, &SkypeApi::doAp2ApInThread, 
		AllocateThisPointer(), CREATE_SUSPENDED, &m_dwAp2ApThreadID);
	if (m_hAp2ApThread == NULL) {
		vcamLogError(10, "CreateThread (m_hAp2ApThread) in doSkypeApi");
	}

}

void SkypeApi::Start()
{
	m_uiAttachMessageId = RegisterWindowMessage(MULTICAM_ATTACH_MSG);
	VCAM_ASSERT(m_uiAttachMessageId != 0);

	m_uiDiscoverKeystrokeMessageId = RegisterWindowMessage(MULTICAM_DISCOVER_MSG);
	VCAM_ASSERT(m_uiDiscoverKeystrokeMessageId != 0);

	m_uiPressedKeystrokeMessageId = RegisterWindowMessage(MULTICAM_ADVANCE_MSG);
	VCAM_ASSERT(m_uiPressedKeystrokeMessageId != 0);


	DWORD res = ResumeThread(m_hMsgLoopThread);
	VCAM_ASSERT(res != (DWORD)-1);

	res = ResumeThread(m_hAp2ApThread);
	VCAM_ASSERT(res != (DWORD)-1);

}

SkypeApi::~SkypeApi() 
{
	// Kill off worker threads
	SetEvent(m_evExitNow);
	// End the message loop
	BOOL ok = PostThreadMessage(m_dwMsgLoopThreadID, WM_QUIT, 0, NULL);
	if(!ok) {
		vcamLogError(10, "SkypeApi::~SkypeApi: PostThreadMessage ERROR");
	}

	vcamLog(10, "SkypeApi::~SkypeApi: waiting for thread to exit...");

	HANDLE events[2];
	events[0] = m_evMsgLoopCompleted;
	events[1] = m_evAp2ApCompleted;
	const DWORD timeout = 1000;
	DWORD res = WaitForMultipleObjects(sizeof(events) / sizeof(events[0]), events, TRUE, timeout);
	VCAM_ASSERT(res == WAIT_OBJECT_0 || res == (WAIT_OBJECT_0 + 1) || res == WAIT_TIMEOUT);
	if (res == WAIT_TIMEOUT) {
			vcamLog(10, "         ... timed out while waiting for threats to exit -- indicates a serious problem");
	} else {
		vcamLog(10, "     ...thread has exited");		
	}

	DeInitialize_DestroyMainWindow();
	DeInitialize_DestroyWindowClass();
	vcamCloseLog(5, "SkypeApi");	
}

void SkypeApi::DisconnectStream()
{
	if (m_strStreamName.compare("") != 0) {
		SendMessageToSkype(DISCONNECT_APPLICATION_string + m_strStreamName);
		m_strStreamName = "";
	}	
}

void SkypeApi::HandleCallMessage(string &message)
{
	int id_start_loc = CALL_string.length();
	int id_end_loc = message.find(' ', id_start_loc);
	string remainder(message.substr(id_end_loc + 1));
	//vcamLog(10, "    HandleCallMessage: remainder is '%s'", remainder.c_str());
	int id_len = id_end_loc - id_start_loc;
	string callIdStr(message.substr(id_start_loc, id_len));
	int callId = atoi(callIdStr.c_str());
	if (callId != m_iCallId) 
	{
		m_iCallId = callId;
		m_fInitiatedCall = false;
		vcamLog(10, "    HandleCallMessage: new call %d", callId);
	}
	if (remainder.find(STATUS_string) == 0) {
		string status(remainder.substr(STATUS_string.length()));
		if (status.compare(ROUTING_string) == 0) {
			m_fInitiatedCall = true;
			vcamLog(10, "    HandleCallMessage: Recorded call as initiated");
		}
		else if (status.compare(INPROGRESS_string) == 0) {
			if (m_fInitiatedCall)
			{
				string request(GET_string + CALL_string + callIdStr + " " + PARTNER_string);
				vcamLog(10, "    HandleCallMessage: sending Skype request '%s'", request.c_str());
				SendMessageToSkype(request);
			}
		} 
		else if (status.compare(FINISHED_string) == 0) {
			DisconnectStream();
			m_strPartner = "";
		} 
	}
	else if (remainder.find(PARTNER_string) == 0)
	{
		int partner_start_loc = PARTNER_string.length() + 1;
		string partner(remainder.substr(partner_start_loc));
		// disconnect any existing stream (There shouldn't be one, really,
		// unless we failed to detect the end of a previous call.)
		DisconnectStream();
		// connect new stream
		SendMessageToSkype(CONNECT_APPLICATION_string + partner);
		m_strPartner = partner;
	}
}


void SkypeApi::HandleSkypeMessage(PCOPYDATASTRUCT &poCopyData)
{
	BOOL ok;

	// The "- 1" prevents a null byte being appended to the string
	string message((char*) poCopyData->lpData, poCopyData->cbData - 1);

	// Log anything except "DURATION" messages, which are too frequent
	if (!(message.find(CALL_string) == 0 && message.find(DURATION_string) != message.npos))
	{
		vcamLog(10, "Message from Skype: '%s'", message.c_str());
	}

	if (message.find(CALL_string) == 0) {
		HandleCallMessage(message);
	} 
	else if (message.compare(PONG_string) == 0) {
		vcamLog(10, "SkypeApi::HandleSkypeMessage: Setting the pong event");
		SetEvent(m_evPong);
	} 
	else if (message.compare(CREATE_APPLICATION_string) == 0) {
		vcamLog(10, "SkypeApi::HandleSkypeMessage: Setting the create application event");
		SetEvent(m_evSkypeCreatedApplication);
	}
	else if (message.find(GET_USER_response) == 0) {
		m_strCurrentUser = message.substr(GET_USER_response.length());
		SetEvent(m_evGotUser);
	}
	else if (message.find(STREAM_NAME_string) == 0) {
		if (message.length() > STREAM_NAME_string.length()) {
			m_strStreamName = message.substr(STREAM_NAME_string.length());
			vcamLog(10, "SkypeApi::HandleSkypeMessage: stream name is '%s'", m_strStreamName.c_str());
		} 
		else {
			vcamLog(10, "SkypeApi::HandleSkypeMessage: received empty stream name, so do nothing");
		}
	}
	else if (message.find(APPLICATION_RECEIVED_string) == 0) {
		int startPos = APPLICATION_RECEIVED_string.length();
		int equalPos = message.find('=');
		if (equalPos == string::npos) {
			vcamLog(10, "SkypeApi::HandleSkypeMessage: Got RECEIVED notification, but no stream name, so do nothing");
		}
		else {
			int length = equalPos - startPos;
			string streamName = message.substr(startPos, length);
			VCAM_ASSERT(streamName.compare(m_strStreamName) == 0);
			vcamLog(10, "SkypeApi::HandleSkypeMessage: Got RECEIVED notification with matching string name, will now attempt to read");
			ReadAp2Ap();
		}
	}
	else if (message.find(READ_APPLICATION_string) == 0) {
		int startPos = READ_APPLICATION_string.length();
		int spacePos = message.find(' ', startPos);
		VCAM_ASSERT (spacePos != string::npos);
		int length = spacePos - startPos;
		string streamName = message.substr(startPos, length);
		VCAM_ASSERT(streamName.compare(m_strStreamName) == 0);
		startPos += (length + 1);
		string dataStr = message.substr(startPos);
		vcamLog(10, "SkypeApi::HandleSkypeMessage: Got READ message; data is '%s'", dataStr.c_str());
		int dataVal = atoi(dataStr.c_str());
		VCAM_ASSERT(dataVal != 0);
		vcamLog(10, "SkypeApi::HandleSkypeMessage: Got READ message; data value is %d", dataVal);
		if (m_hKeystrokeMessageWindow != NULL)
		{
			vcamLog(10, "SkypeApi::HandleSkypeMessage: sending this data value to window 0x%x", m_hKeystrokeMessageWindow);
			ok = SendNotifyMessage(m_hKeystrokeMessageWindow, m_uiPressedKeystrokeMessageId,
				(WPARAM) dataVal, 0);
			VCAM_ASSERT(ok);
		}
	}
}

LRESULT APIENTRY SkypeApi::nonstatic_WindowProc(
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
		hInit_MainWindowHandle=NULL;
		PostQuitMessage(0);
		break;
	case WM_COPYDATA:
		if( hGlobal_SkypeAPIWindowHandle==(HWND)uiParam )
		{
			PCOPYDATASTRUCT poCopyData=(PCOPYDATASTRUCT)ulParam;
			//vcamLog(10,  "Message from Skype: %.*s", poCopyData->cbData, poCopyData->lpData);
			HandleSkypeMessage(poCopyData);
			lReturnCode=1;
		}
		break;
	default:
		if( uiMessage==uiGlobal_MsgID_SkypeControlAPIAttach )
		{
			switch(ulParam)
			{
			case SKYPECONTROLAPI_ATTACH_SUCCESS:
				vcamLog(10, "!!! Connected; to terminate issue #disconnect\n");
				hGlobal_SkypeAPIWindowHandle=(HWND)uiParam;
				m_Skype_control_status = (Skype_control_API) ulParam;
				ok = SetEvent(m_evSkypeAccepted);
				VCAM_ASSERT(ok);
				break;
			case SKYPECONTROLAPI_ATTACH_PENDING_AUTHORIZATION:
				vcamLog(10, "!!! Pending authorization\n");
				break;
			case SKYPECONTROLAPI_ATTACH_REFUSED:
				vcamLog(10, "!!! Connection refused\n");
				break;
			case SKYPECONTROLAPI_ATTACH_NOT_AVAILABLE:
				vcamLog(10, "!!! Skype API not available\n");
				break;
			case SKYPECONTROLAPI_ATTACH_API_AVAILABLE:
				vcamLog(10, "!!! Try connect now (API available); issue #connect\n");
				break;
			}
			lReturnCode=1;
			m_Skype_control_status = (Skype_control_API) ulParam;
			break;
		}
		else if (uiMessage == m_uiDiscoverKeystrokeMessageId)
		{
			vcamLog(10, "SkypeApi windows procedure received discover message from camera DLL: %d", uiParam);			
			m_hKeystrokeMessageWindow = (HWND)uiParam;
			ok = SendNotifyMessage(m_hKeystrokeMessageWindow, m_uiAttachMessageId, 
				(WPARAM) hInit_MainWindowHandle, MULTICAM_ATTACH_MSG_SUCCESS);
			VCAM_ASSERT(ok);
		}
		fIssueDefProc=true;
		break;
	}
	if( fIssueDefProc )
		lReturnCode=DefWindowProc( hWindow, uiMessage, uiParam, ulParam);

	//printf( "WindowProc: hWindow=0x%08X, MainWindow=0x%08X, Message=%5u, WParam=0x%08X, LParam=0x%08X; Return=%ld%s\n",
	//	hWindow, hInit_MainWindowHandle, uiMessage, uiParam, ulParam, lReturnCode, fIssueDefProc? " (default)":"");

	return(lReturnCode);
}


LRESULT APIENTRY SkypeApi::SkypeAPITest_Windows_WindowProc(
	HWND hWindow, UINT uiMessage, WPARAM uiParam, LPARAM ulParam)
{
	//SkypeApi* pSkypeApi = NULL;
	//{
	//	CAutoLock lk(&s_mapLock);
	//	if (s_skypeApiMap.count(hWindow) == 1)
	//	{
	//		pSkypeApi = s_skypeApiMap[hWindow];
	//	}

	//}

	SkypeApi* pSkypeApi = (SkypeApi*) GetWindowLongPtr(hWindow, GWLP_USERDATA);

	//if(pSkypeApi != NULL/* && pSkypeApi2 != NULL*/) {
	//	VCAM_ASSERT(pSkypeApi == pSkypeApi2);
	//}

	if(pSkypeApi != NULL) {
		return pSkypeApi->nonstatic_WindowProc(hWindow, uiMessage, uiParam, ulParam);
	}
	else {
		return DefWindowProc( hWindow, uiMessage, uiParam, ulParam);
	}
}

bool SkypeApi::Initialize_CreateWindowClass(void)
{

	return ::Initialize_CreateWindowClass((WNDPROC)&SkypeAPITest_Windows_WindowProc, 
		"Skype-API-Test-", hInit_ProcessHandle, acInit_WindowClassName);

	//unsigned char *paucUUIDString;
	//RPC_STATUS lUUIDResult;
	//bool fReturnStatus;
	//UUID oUUID;

	//fReturnStatus=false;
	//lUUIDResult=UuidCreate(&oUUID);
	//hInit_ProcessHandle=(HINSTANCE)OpenProcess( PROCESS_DUP_HANDLE, FALSE, GetCurrentProcessId());
	//if( hInit_ProcessHandle!=NULL && (lUUIDResult==RPC_S_OK || lUUIDResult==RPC_S_UUID_LOCAL_ONLY) )
	//{
	//	if( UuidToString( &oUUID, &paucUUIDString)==RPC_S_OK )
	//	{
	//		WNDCLASS oWindowClass;

	//		strcpy_s( acInit_WindowClassName, "Skype-API-Test-");
	//		strcat_s( acInit_WindowClassName, (char *)paucUUIDString);

	//		oWindowClass.style=CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS;
	//		oWindowClass.lpfnWndProc=(WNDPROC)&SkypeAPITest_Windows_WindowProc;
	//		oWindowClass.cbClsExtra=0;
	//		oWindowClass.cbWndExtra=0;
	//		oWindowClass.hInstance=hInit_ProcessHandle;
	//		oWindowClass.hIcon=NULL;
	//		oWindowClass.hCursor=NULL;
	//		oWindowClass.hbrBackground=NULL;
	//		oWindowClass.lpszMenuName=NULL;
	//		oWindowClass.lpszClassName=acInit_WindowClassName;

	//		if( RegisterClass(&oWindowClass)!=0 )
	//			fReturnStatus=true;

	//		RpcStringFree(&paucUUIDString);
	//	}
	//}
	//if( fReturnStatus==false )
	//	CloseHandle(hInit_ProcessHandle),hInit_ProcessHandle=NULL;
	//return(fReturnStatus);
}

void SkypeApi::DeInitialize_DestroyWindowClass(void)
{
	::DeInitialize_DestroyWindowClass(acInit_WindowClassName, hInit_ProcessHandle);

	//UnregisterClass( acInit_WindowClassName, hInit_ProcessHandle);
	//CloseHandle(hInit_ProcessHandle),hInit_ProcessHandle=NULL;
}

bool SkypeApi::Initialize_CreateMainWindow(void)
{
	return ::Initialize_CreateMainWindow(acInit_WindowClassName, hInit_ProcessHandle, this, hInit_MainWindowHandle);

	//hInit_MainWindowHandle=CreateWindowEx(WS_EX_APPWINDOW|WS_EX_WINDOWEDGE,
	//	acInit_WindowClassName, "", WS_BORDER|WS_SYSMENU|WS_MINIMIZEBOX,
	//	CW_USEDEFAULT, CW_USEDEFAULT, 128, 128, NULL, 0, hInit_ProcessHandle, 0);
	////{
	////	CAutoLock lk(&s_mapLock);
	////	s_skypeApiMap[hInit_MainWindowHandle] = this;
	////}

	//SetWindowLongPtr(hInit_MainWindowHandle, GWLP_USERDATA, (LONG) this);

	//return(hInit_MainWindowHandle!=NULL? true:false);
}

void SkypeApi::DeInitialize_DestroyMainWindow(void)
{
	::DeInitialize_DestroyMainWindow(hInit_MainWindowHandle);
	//if( hInit_MainWindowHandle!=NULL ) {
	//	DestroyWindow(hInit_MainWindowHandle);
	//	//{
	//	//	CAutoLock lk(&s_mapLock);
	//	//	s_skypeApiMap.erase(hInit_MainWindowHandle);
	//	//}
	//	hInit_MainWindowHandle=NULL;
	//}
}

void SkypeApi::SendMessageToSkype(string &str)
{
	COPYDATASTRUCT oCopyData;

	// send command to skype
	oCopyData.dwData=0;
	oCopyData.lpData= (PVOID) str.c_str();
	oCopyData.cbData=str.length()+1;
	if( oCopyData.cbData!=1 )
	{
		if( SendMessage( hGlobal_SkypeAPIWindowHandle, WM_COPYDATA, (WPARAM)hInit_MainWindowHandle, (LPARAM)&oCopyData)==FALSE )
		{
			hGlobal_SkypeAPIWindowHandle=NULL;
			printf("!!! Disconnected\n");
		}
	}

}


void SkypeApi::SendAp2ApMessage(string &str)
{

	if (m_strStreamName.length() > 0) {
		string full_message(WRITE_APPLICATION_string + m_strStreamName + " " + str);
		SendMessageToSkype(full_message);
	}
	else {
		vcamLog(10, "SkypeApi::SendAp2ApMessage: Not currently connected, so not sending message '%s'", str.c_str());
	}
}

void SkypeApi::ReadAp2Ap()
{
	SendMessageToSkype(READ_APPLICATION_string + m_strStreamName);
}

DWORD WINAPI SkypeApi::doMsgLoopInThread(LPVOID lpParam)
{
	PSKYPE_API_PTR skypeApiPtr = (PSKYPE_API_PTR) lpParam;
	SkypeApi* pSkypeApi = skypeApiPtr->p;

	BOOL ok = HeapFree(GetProcessHeap(), 0, skypeApiPtr);
	VCAM_ASSERT(ok);

	pSkypeApi->NonStaticDoMsgLoopInThread();

	return 0;

}

DWORD WINAPI SkypeApi::doAp2ApInThread(LPVOID lpParam)
{
	PSKYPE_API_PTR skypeApiPtr = (PSKYPE_API_PTR) lpParam;
	SkypeApi* pSkypeApi = skypeApiPtr->p;

	BOOL ok = HeapFree(GetProcessHeap(), 0, skypeApiPtr);
	VCAM_ASSERT(ok);

	pSkypeApi->NonStaticDoAp2ApInThread();

	return 0;

}


void SkypeApi::SendDiscoverMessage()
{
	LRESULT lr = 0;
	lr = SendMessageTimeout( HWND_BROADCAST, uiGlobal_MsgID_SkypeControlAPIDiscover, (WPARAM)hInit_MainWindowHandle, 
		0, SMTO_ABORTIFHUNG, 1000, NULL);
	VCAM_ASSERT(lr != 0);
}

void SkypeApi::RetryUntilConnected()
{
	DWORD timeout = 10;
	const DWORD max_timeout = 4000;
	HANDLE events[3];
	events[0] = m_evExitNow;
	events[1] = m_evSkypeAccepted;
	events[2] = m_evPong;

	BOOL done = FALSE;
	while (!done)
	{
		DWORD res = WaitForMultipleObjects(sizeof(events) / sizeof(events[0]), events, FALSE, timeout);
		VCAM_ASSERT(  (res >= WAIT_OBJECT_0 && res <= (WAIT_OBJECT_0 + 2)) || res == WAIT_TIMEOUT);
		switch (res) {
		case WAIT_OBJECT_0 + 0: // Need to exit right away
			vcamLog(10, "RetryUntilConnected: Received exit signal");
			done = TRUE;
			break;
		case WAIT_OBJECT_0 + 1: // Skype accepted the connection
			vcamLog(10, "RetryUntilConnected: Skype has accepted the connection");
			m_Skype_control_status = SKYPECONTROLAPI_ATTACH_SUCCESS;
			done = TRUE;
			break;
		case WAIT_OBJECT_0 + 2: // Skype sent a pong
			vcamLog(10, "RetryUntilConnected: Received pong message from Skype, so connection exists already");
			SetEvent(m_evSkypeAccepted);
			m_Skype_control_status = SKYPECONTROLAPI_ATTACH_SUCCESS;
			done = TRUE;
			break;
		case WAIT_TIMEOUT:
			vcamLog(10, "RetryUntilConnected: Timed out waiting for attachment success (timeout = %d)", timeout);
			vcamLog(10, "RetryUntilConnected: Retrying Skype connection");
			SendDiscoverMessage();

			if (timeout >= max_timeout) {
				timeout = max_timeout;
			} else {
				timeout *= 2;
			}
			break;
		default:
			VCAM_ASSERT(false);
		}
	}
}

void SkypeApi::DoPingTest()
{
	const DWORD test_interval = 4000;
	string ping("ping");
	HANDLE events[2];
	events[0] = m_evExitNow;
	events[1] = m_evPong;
	DWORD res = 0;

	BOOL done = FALSE;
	BOOL ok = TRUE;

	// Some experimentation suggests that it is a good idea to "kick" Skype with 
	// a meaningless message before we try to do anything else.
	// In experiments, we sometimes get no response to this message,
	// but subsequent messages get the expected responses.

	vcamLog(10, "SkypeApi::DoPingTest: Sending initial ping message to kick Skype");
	SendMessageToSkype(ping);

	while(!done) {
		// Do nothing for a while unless we need to exit right away
		WaitForSingleObject(m_evExitNow, test_interval);

		// Now start the ping test
		vcamLog(10, "SkypeApi::DoPingTest: Sending some ping messages to Skype");
		BOOL received = FALSE;
		ResetEvent(m_evPong);
		for(DWORD timeout = 20; timeout < test_interval && !received; timeout *= 2){
			SendMessageToSkype(ping);
			res = WaitForMultipleObjects(sizeof(events) / sizeof(events[0]), events, FALSE, timeout);
			VCAM_ASSERT(res == WAIT_OBJECT_0 || res == (WAIT_OBJECT_0 + 1) || res == WAIT_TIMEOUT);
			switch (res) {
			case (WAIT_OBJECT_0 + 0): // Need to exit
				received = TRUE;
				done = TRUE;
				break;		
			case (WAIT_OBJECT_0 + 1): // Received a pong response, so the connection is fine
				received = TRUE;
				vcamLog(10, "SkypeApi::DoPingTest: Received pong event");
				break; 
			case WAIT_TIMEOUT:  // Did not get a pong response.  Reconnect.
				vcamLog(10, "SkypeApi::DoPingTest: Timed out waiting for pong event; will send another ping");						
				break;
			default:
				VCAM_ASSERT(FALSE);
			}
		}

		if (!received) {
			vcamLog(10, "SkypeApi::DoPingTest: No response to several consecutive pings; will try to reconnect");
			m_Skype_control_status = SKYPECONTROLAPI_ATTACH_NOT_ATTEMPTED;
			ok = ResetEvent(m_evSkypeAccepted);
			VCAM_ASSERT(ok);
			RetryUntilConnected();
		}

	}
}


DWORD WINAPI SkypeApi::NonStaticDoMsgLoopInThread()
{
	vcamLog(10, "------------------------ starting NonStaticDoMsgLoopInThread -------------------------");

	uiGlobal_MsgID_SkypeControlAPIAttach=RegisterWindowMessage("SkypeControlAPIAttach");
	VCAM_ASSERT(uiGlobal_MsgID_SkypeControlAPIAttach != 0);

	uiGlobal_MsgID_SkypeControlAPIDiscover=RegisterWindowMessage("SkypeControlAPIDiscover");
	VCAM_ASSERT(uiGlobal_MsgID_SkypeControlAPIDiscover != 0);

	BOOL ok = true;
	ok = Initialize_CreateWindowClass();
	VCAM_ASSERT(ok);

	ok = Initialize_CreateMainWindow();
	VCAM_ASSERT(ok);


	vcamLog(10, "NonStaticDoMsgLoopInThread: Broadcasting discover request for Skype");
	SendDiscoverMessage();



	// Start the message loop. 

	MSG msg;
	BOOL bRet;
	while( (bRet = GetMessage( &msg, NULL, 0, 0 )) != 0)
	{ 
		if (bRet == -1)
		{
			vcamLog(10, "NonStaticDoMsgLoopInThread: Unexpected error in message loop; terminating message loop");
			break;
		}
		else
		{
			TranslateMessage(&msg); 
			DispatchMessage(&msg); 
		}
	} 

	vcamLog(10, "------------------------ ending NonStaticDoMsgLoopInThread -------------------------");

		ok = SetEvent(m_evMsgLoopCompleted);
	VCAM_ASSERT(ok);
	return 0;
}

Polite_wait SkypeApi::PoliteWait(HANDLE hEvent, DWORD timeout)
{
	HANDLE events[2];
	events[0] = m_evExitNow;
	events[1] = hEvent;
	DWORD res = WaitForMultipleObjects(sizeof(events) / sizeof(events[0]), events, FALSE, timeout);
	switch (res) {
	case WAIT_OBJECT_0 + 0:
		return POLITE_WAIT_EXIT;
	case WAIT_OBJECT_0 + 1: 
		return POLITE_WAIT_SIGNALED;
	case WAIT_TIMEOUT:
		return POLITE_WAIT_TIMEDOUT;
	default:
		VCAM_ASSERT(FALSE);
	}
	return POLITE_WAIT_EXIT;
}

DWORD WINAPI SkypeApi::NonStaticDoAp2ApInThread()
{
	BOOL ok = true;
	DWORD res = 0;
	Polite_wait pw = POLITE_WAIT_EXIT;

	vcamLog(10, "------------------------ starting NonStaticDoAp2ApInThread -------------------------");

	// Wait until Skype accepts our connection
	pw = PoliteWait(m_evSkypeAccepted, INFINITE);
	if (pw == POLITE_WAIT_EXIT) goto done;

	vcamLog(10, "NonStaticDoAp2ApInThread: Skype has accepted our connection");

	SendMessageToSkype(CREATE_APPLICATION_string);

	// Wait until Skype creates the application
	pw = PoliteWait(m_evSkypeCreatedApplication, INFINITE);
	if (pw == POLITE_WAIT_EXIT) goto done;
	vcamLog(10, "NonStaticDoAp2ApInThread: Skype has created the application");

	// Get current user, to work out who we are -- only used for debugging, I think
	ResetEvent(m_evGotUser);
	SendMessageToSkype(GET_USER_request);
	pw = PoliteWait(m_evGotUser, INFINITE);
	if (pw == POLITE_WAIT_EXIT) goto done;

	//// connect to a hardwired user
	//if (m_strCurrentUser.compare(LOCAL_USER_string) == 0) {
	//	// We are on the desktop computer
	//	SendMessageToSkype(CONNECT_APPLICATION_string + REMOTE_USER_string);
	//}

done:
	vcamLog(10, "------------------------ ending NonStaticDoAp2ApInThread -------------------------");

	ok = SetEvent(m_evAp2ApCompleted);
	VCAM_ASSERT(ok);

	return 0;
}