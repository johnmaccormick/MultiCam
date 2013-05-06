// Copyright John MacCormick 2012. Modified BSD license. NO WARRANTY.
using namespace std;
#include <windows.h>
#include <tlhelp32.h>
#include <vector>
#include <Dshow.h>
#include <iostream>
#include <fstream>

#include "LogHelpers.h"
#include "KeyHook.h"

BOOL GetPIDFromExeName(TCHAR* szExeFile, /*out*/ DWORD* pPid)
{
	HANDLE hProcessSnap;
	PROCESSENTRY32 pe32;

	hProcessSnap = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
	if( hProcessSnap == INVALID_HANDLE_VALUE )
	{
		vcamLogError(10,  TEXT("CreateToolhelp32Snapshot (of processes)") );
		return( FALSE );
	}

	// Set the size of the structure before using it.
	pe32.dwSize = sizeof( PROCESSENTRY32 );

	// Retrieve information about the first process,
	// and exit if unsuccessful
	if( !Process32First( hProcessSnap, &pe32 ) )
	{
		vcamLogError(10,  TEXT("Process32First") ); // show cause of failure
		CloseHandle( hProcessSnap );          // clean the snapshot object
		return( FALSE );
	}

	BOOL found = FALSE;
	do
	{
		if(lstrcmpi(szExeFile, pe32.szExeFile) == 0)
		{
			found = TRUE;
			*pPid = pe32.th32ProcessID;
		}

	} while(!found && Process32Next( hProcessSnap, &pe32 ) );


	CloseHandle( hProcessSnap );
	return( found );
}

BOOL GetProcessThreadIDs(TCHAR* szExeFile, /*out*/ vector<DWORD> &tids)
{
	DWORD dwOwnerPID = 0;
	BOOL ok = TRUE;
	ok = GetPIDFromExeName(szExeFile, &dwOwnerPID);
	if(!ok) return FALSE;

	return GetProcessThreadIDs(dwOwnerPID, tids);
}

BOOL GetProcessThreadIDs(DWORD dwOwnerPID, /*out*/ vector<DWORD> &tids)
{
	HANDLE hThreadSnap = INVALID_HANDLE_VALUE; 
	THREADENTRY32 te32; 

	// Take a snapshot of all running threads  
	hThreadSnap = CreateToolhelp32Snapshot( TH32CS_SNAPTHREAD, 0 ); 
	if( hThreadSnap == INVALID_HANDLE_VALUE ) 
		return( FALSE ); 

	// Fill in the size of the structure before using it. 
	te32.dwSize = sizeof(THREADENTRY32); 

	// Retrieve information about the first thread,
	// and exit if unsuccessful
	if( !Thread32First( hThreadSnap, &te32 ) ) 
	{
		vcamLogError(10,  TEXT("Thread32First") ); // show cause of failure
		CloseHandle( hThreadSnap );          // clean the snapshot object
		return( FALSE );
	}

	// Now walk the thread list of the system,
	// and display information about each thread
	// associated with the specified process
	do 
	{ 
		if( te32.th32OwnerProcessID == dwOwnerPID )
		{
			tids.push_back(te32.th32ThreadID);
		}
	} while( Thread32Next(hThreadSnap, &te32 ) ); 

	CloseHandle( hThreadSnap );
	return( TRUE );

}



BOOL AddHookToThreadIDs(vector<DWORD> &tids)
{
	// return true if we succeeded at least once
	BOOL ok = FALSE;
	for(vector<DWORD>::iterator it=tids.begin(); it != tids.end() ; it++) {
		ok = AddHookToThreadID(*it) || ok;
		//if (!ok) return FALSE;
	}
	return ok;
}

BOOL AddHookToThreadID(DWORD tid)
{
	HHOOK hh = NULL;

	HOOKPROC hkprcSysMsg;
	static HINSTANCE hinstDLL; 
	static HHOOK hhookSysMsg; 

	hinstDLL = LoadLibrary(TEXT("KeyHookDll.dll")); 
	hkprcSysMsg = (HOOKPROC)GetProcAddress(hinstDLL, /*(LPCSTR) 3*/ "?JmacKeyboardProc@@YGJHIJ@Z" ); 

	if (hkprcSysMsg == NULL)
	{
		vcamLogError(10,  TEXT("GetProcAddress") );
		return FALSE;
	}

	hh = SetWindowsHookEx(WH_KEYBOARD, hkprcSysMsg, hinstDLL, tid);
	if (hh == NULL)
	{
		//vcamLog(10, "SetWindowsHookEx failed on thread ID %d", tid);
		vcamLogError(10,  TEXT("SetWindowsHookEx") );
		return FALSE;
	}

	return TRUE;

}
