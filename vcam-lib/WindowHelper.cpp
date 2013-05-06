// Copyright John MacCormick 2012. Modified BSD license. NO WARRANTY.
#include <stdio.h>
#include <string.h>

#include <windows.h>
#include <rpcdce.h>

#include "WindowHelper.h"


bool Initialize_CreateWindowClass(WNDPROC wndProc, 
		char* windowClassNamePrefix,
		/*out*/ HINSTANCE &hInit_ProcessHandle,  
		/*out*/ char acInit_WindowClassName[WINDOW_CLASS_NAME_LENGTH])
{
	unsigned char *paucUUIDString;
	RPC_STATUS lUUIDResult;
	bool fReturnStatus;
	UUID oUUID;

	fReturnStatus=false;
	lUUIDResult=UuidCreate(&oUUID);
	hInit_ProcessHandle=(HINSTANCE)OpenProcess( PROCESS_DUP_HANDLE, FALSE, GetCurrentProcessId());
	if( hInit_ProcessHandle!=NULL && (lUUIDResult==RPC_S_OK || lUUIDResult==RPC_S_UUID_LOCAL_ONLY) )
	{
		if( UuidToString( &oUUID, &paucUUIDString)==RPC_S_OK )
		{
			WNDCLASS oWindowClass;

			strcpy_s( acInit_WindowClassName, WINDOW_CLASS_NAME_LENGTH, windowClassNamePrefix);
			strcat_s( acInit_WindowClassName, WINDOW_CLASS_NAME_LENGTH, (char *)paucUUIDString);

			oWindowClass.style=CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS;
			oWindowClass.lpfnWndProc=wndProc;
			oWindowClass.cbClsExtra=0;
			oWindowClass.cbWndExtra=0;
			oWindowClass.hInstance=hInit_ProcessHandle;
			oWindowClass.hIcon=NULL;
			oWindowClass.hCursor=NULL;
			oWindowClass.hbrBackground=NULL;
			oWindowClass.lpszMenuName=NULL;
			oWindowClass.lpszClassName=acInit_WindowClassName;

			if( RegisterClass(&oWindowClass)!=0 )
				fReturnStatus=true;

			RpcStringFree(&paucUUIDString);
		}
	}
	if( fReturnStatus==false )
		CloseHandle(hInit_ProcessHandle),hInit_ProcessHandle=NULL;
	return(fReturnStatus);
}

void DeInitialize_DestroyWindowClass(char* acInit_WindowClassName, /*inout*/ HINSTANCE &hInit_ProcessHandle)
{
	UnregisterClass( acInit_WindowClassName, hInit_ProcessHandle);
	CloseHandle(hInit_ProcessHandle);
	hInit_ProcessHandle=NULL;
}

bool Initialize_CreateMainWindow(char* acInit_WindowClassName, HINSTANCE hInit_ProcessHandle,
			void* pThis, /*out*/ HWND &hInit_MainWindowHandle)
{
	hInit_MainWindowHandle=CreateWindowEx(WS_EX_APPWINDOW|WS_EX_WINDOWEDGE,
		acInit_WindowClassName, "", WS_BORDER|WS_SYSMENU|WS_MINIMIZEBOX,
		CW_USEDEFAULT, CW_USEDEFAULT, 128, 128, NULL, 0, hInit_ProcessHandle, 0);
	//{
	//	CAutoLock lk(&s_mapLock);
	//	s_skypeApiMap[hInit_MainWindowHandle] = this;
	//}

	SetWindowLongPtr(hInit_MainWindowHandle, GWLP_USERDATA, (LONG) pThis);

	return(hInit_MainWindowHandle!=NULL? true:false);
}

void DeInitialize_DestroyMainWindow(/*inout*/ HWND &hInit_MainWindowHandle)
{
	if( hInit_MainWindowHandle!=NULL ) {
		DestroyWindow(hInit_MainWindowHandle);
		hInit_MainWindowHandle=NULL;
	}
}
