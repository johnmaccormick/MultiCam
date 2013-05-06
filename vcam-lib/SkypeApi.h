// Copyright John MacCormick 2012. Modified BSD license. NO WARRANTY.
// ENTIRE FILE NO LONGER NEEDED??
#include <map>

class SkypeApi;

enum Skype_control_API {
	SKYPECONTROLAPI_ATTACH_SUCCESS=0,								// Client is successfully attached and API window handle can be found in wParam parameter
	SKYPECONTROLAPI_ATTACH_PENDING_AUTHORIZATION=1,	// Skype has acknowledged connection request and is waiting for confirmation from the user.
	// The client is not yet attached and should wait for SKYPECONTROLAPI_ATTACH_SUCCESS message
	SKYPECONTROLAPI_ATTACH_REFUSED=2,								// User has explicitly denied access to client
	SKYPECONTROLAPI_ATTACH_NOT_AVAILABLE=3,					// API is not available at the moment. For example, this happens when no user is currently logged in.
	// Client should wait for SKYPECONTROLAPI_ATTACH_API_AVAILABLE broadcast before making any further
	// connection attempts.
	SKYPECONTROLAPI_ATTACH_API_AVAILABLE=0x8001,
	SKYPECONTROLAPI_ATTACH_NOT_ATTEMPTED=4
};

enum Polite_wait {
	POLITE_WAIT_EXIT = 0,	
	POLITE_WAIT_SIGNALED = 1,	
	POLITE_WAIT_TIMEDOUT = 2	
};

typedef struct SkypeApiPtr {
	SkypeApi* p;
} SKYPE_API_PTR, *PSKYPE_API_PTR;


class SkypeApi
{
public:
	SkypeApi();
	virtual ~SkypeApi();
	//void __cdecl startSkypeApi(void);
	//void __cdecl stopSkypeApi(void);
	void SendMessageToSkype(string &str);
	void Start();
	void SendAp2ApMessage(string &str);

protected:
	//static CCritSec s_mapLock;
	//static map<HWND, SkypeApi*>  SkypeApi::s_skypeApiMap;
	HWND hInit_MainWindowHandle;
	HINSTANCE hInit_ProcessHandle;
	char acInit_WindowClassName[128];
	UINT uiGlobal_MsgID_SkypeControlAPIAttach;
	UINT uiGlobal_MsgID_SkypeControlAPIDiscover;
	HWND hGlobal_SkypeAPIWindowHandle;
	HWND m_hKeystrokeMessageWindow;
	HANDLE m_evSkypeAccepted;
	UINT m_uiDiscoverKeystrokeMessageId;
	UINT m_uiAttachMessageId;
	UINT m_uiPressedKeystrokeMessageId;

	HANDLE m_hMsgLoopThread;
	DWORD m_dwMsgLoopThreadID;
	HANDLE m_evMsgLoopCompleted;
	HANDLE m_hAp2ApThread;
	DWORD m_dwAp2ApThreadID;
	HANDLE m_evAp2ApCompleted;
	HANDLE m_evExitNow;
	HANDLE m_evPong;
	HANDLE m_evGotUser;
	HANDLE m_evSkypeCreatedApplication;
	Skype_control_API m_Skype_control_status;

	string m_strCurrentUser;
	string m_strStreamName;
	int m_iCallId;
	bool m_fInitiatedCall;
	string m_strPartner;


	static LRESULT APIENTRY SkypeAPITest_Windows_WindowProc(
		HWND hWindow, UINT uiMessage, 
		WPARAM uiParam, LPARAM ulParam);
	LRESULT APIENTRY nonstatic_WindowProc(HWND hWindow, UINT uiMessage, WPARAM uiParam, LPARAM ulParam);

	bool Initialize_CreateWindowClass(void);
	void DeInitialize_DestroyWindowClass(void);
	bool Initialize_CreateMainWindow(void);
	void DeInitialize_DestroyMainWindow(void);
	static DWORD WINAPI doMsgLoopInThread(LPVOID lpParam);
	DWORD WINAPI NonStaticDoMsgLoopInThread();
	static DWORD WINAPI doAp2ApInThread(LPVOID lpParam);
	DWORD WINAPI NonStaticDoAp2ApInThread();

	void SendDiscoverMessage();
	void RetryUntilConnected();
	void DoPingTest();
	void HandleSkypeMessage(PCOPYDATASTRUCT &poCopyData);
	PSKYPE_API_PTR AllocateThisPointer();
	Polite_wait PoliteWait(HANDLE event, DWORD timeout);
	void SkypeApi::ReadAp2Ap();
	void SkypeApi::HandleCallMessage(string &message);
	void SkypeApi::DisconnectStream();



};


